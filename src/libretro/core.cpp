// SPDX-License-Identifier: BSD-3-Clause
// Libretro owns presentation and pacing; the upstream machine owns emulation.
#include "libretro.h"
#include "core_options.h"
#include "crosshair.h"
#include "initial_nvram.h"
#include "input.h"
#include "netpacket.h"
#include "rumble.h"
#include "save_ram.h"
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
#include "gpu.h"
#endif
#include "hw/machine_factory.h"
#include "hw/m2comm.h"
#include "hw/model2.h"
#include "hw/model2_original.h"
#include "hw/model2b.h"
#include "hw/model2c.h"
#include "hw/model2_softrender.h"
#include "hw/sound_board.h"
#include "rom/game_db.h"
#include "rom/rom_loader.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace sm2;
retro_environment_t environment = nullptr;
retro_video_refresh_t video_cb = nullptr;
retro_audio_sample_t audio_cb = nullptr;
retro_audio_sample_batch_t audio_batch_cb = nullptr;
retro_input_poll_t input_poll_cb = nullptr;
retro_input_state_t input_state_cb = nullptr;
retro_log_printf_t log_cb = nullptr;
libretro::InputDevices devices{
    RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD,
    RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD,
};
libretro::SaveRam save_ram{};
constexpr unsigned width = hw::SoftRenderer::kWidth;
constexpr unsigned height = hw::SoftRenderer::kHeight;
enum class RendererApi { Software, Vulkan, OpenGL, OpenGLES };
// RetroArch 1.22 queries this before loading content and caches a zero as
// "unsupported". All four board images fit below 8 MiB in the real-ROM matrix;
// one extra MiB keeps bounded FIFO growth from changing the frontend contract.
constexpr size_t libretro_state_size = 9 * 1024 * 1024;
constexpr u64 ski_super_g_drive_board_test_frame = 900;

struct Content {
    rom::GameSpec game;
    libretro::ControllerConfiguration controllers;
    libretro::InputRuntime input_runtime;
    std::unique_ptr<hw::Model2MachineBase> machine;
    hw::SoftRenderer renderer;
    std::vector<u32> frame = std::vector<u32>(width * height);
    std::vector<s16> audio;
    std::vector<retro_input_descriptor> input_descriptors;
    libretro::GamepadRumble rumble;
    double native_fps = 0;
    double fps = 0;
    u32 rate = 0;
    bool failed = false;
    RendererApi renderer_api = RendererApi::Software;
    bool hardware_frame_valid = false;
    unsigned scale = 1;
    bool save_ram_initialized = false;
    bool native_nvram_available = false;
    bool option_pending = false;
    unsigned option_wait_frames = 0;
    std::string nvram_game;
    std::vector<std::string> nvram_values;
    libretro::AVTimingMode timing = libretro::AVTimingMode::Native;
    double cadence_accumulator = 0;
    u64 audio_frame_remainder = 0;
    size_t audio_frames_due = 0;
    bool can_dupe = false;
    bool timing_overlay = false;
    bool timing_overlay_visible = false;
    bool external_link_active = false;
    float aspect_ratio = 4.0f / 3.0f;
    std::chrono::steady_clock::time_point previous_run_start{};
    bool have_previous_run_start = false;
    double timing_machine_ms = 0;
    double timing_video_ms = 0;
    double timing_audio_ms = 0;
    double timing_run_ms = 0;
    double timing_interval_ms = 0;
    double timing_worst_ms = 0;
    unsigned timing_callbacks = 0;
    unsigned timing_intervals = 0;
    unsigned timing_machine_frames = 0;
#ifdef SM2_LIBRETRO_VULKAN
    std::unique_ptr<libretro::VulkanRenderer> gpu;
#endif
#ifdef SM2_LIBRETRO_OPENGL
    std::unique_ptr<libretro::OpenGlRenderer> gl;
#endif
};
std::unique_ptr<Content> content;

void message(enum retro_log_level level, const char* text);

bool automatic_widescreen(const Content& c)
{
    const auto eeprom = c.machine->settings_eeprom();
    if (eeprom.size() <= 0x17) return false;
    const std::string_view family = c.game.parent.empty() ? c.game.name : c.game.parent;
    if (family == "indy500")
        return eeprom[0x17] == 0x01;
    if (family == "stcc")
        return (eeprom[0x10] & 0x08) != 0;
    return false;
}

float selected_aspect_ratio(const Content& c)
{
    switch (libretro::aspect_ratio_mode()) {
        case libretro::AspectRatioMode::FourThree: return 4.0f / 3.0f;
        case libretro::AspectRatioMode::SixteenNine: return 16.0f / 9.0f;
        case libretro::AspectRatioMode::Automatic:
            return automatic_widescreen(c) ? 16.0f / 9.0f : 4.0f / 3.0f;
    }
    return 4.0f / 3.0f;
}

void update_aspect_ratio(Content& c)
{
    const float aspect_ratio = selected_aspect_ratio(c);
    if (aspect_ratio == c.aspect_ratio) return;
    c.aspect_ratio = aspect_ratio;
    retro_game_geometry geometry{
        width * c.scale, height * c.scale, width * c.scale, height * c.scale,
        c.aspect_ratio};
    if (environment)
        environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
}

double elapsed_ms(std::chrono::steady_clock::time_point begin,
                  std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

void clear_timing_overlay(Content& c)
{
    if (!c.timing_overlay_visible || !environment) return;
    const retro_message_ext clear{"", 1, 0, RETRO_LOG_INFO, RETRO_MESSAGE_TARGET_OSD,
                                  RETRO_MESSAGE_TYPE_STATUS, -1};
    environment(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, const_cast<retro_message_ext*>(&clear));
    c.timing_overlay_visible = false;
}

void reset_timing_measurements(Content& c)
{
    c.timing_machine_ms = c.timing_video_ms = c.timing_audio_ms = 0;
    c.timing_run_ms = c.timing_interval_ms = c.timing_worst_ms = 0;
    c.timing_callbacks = c.timing_intervals = c.timing_machine_frames = 0;
}

void reset_frontend_after_state_load(Content& c)
{
    c.rumble.stop();
    c.machine->sound_board().clear_pending_samples();
    c.machine->sound_board().set_audio_balance_enabled(
        libretro::audio_balance_enabled());
    c.machine->sound_board().set_music_volume_percent(
        libretro::music_volume_percent());
    c.audio.clear();
    c.input_runtime = {};
    c.cadence_accumulator = 60.0 - c.native_fps;
    c.audio_frame_remainder = 0;
    c.audio_frames_due = 0;
    c.have_previous_run_start = false;
    c.hardware_frame_valid = false;
    reset_timing_measurements(c);
    update_aspect_ratio(c);
}

void publish_timing_overlay(Content& c)
{
    if (!c.timing_callbacks) return;
    const double callbacks = static_cast<double>(c.timing_callbacks);
    const double machine_frames = std::max(1u, c.timing_machine_frames);
    const double machine_ms = c.timing_machine_ms / machine_frames;
    const double video_ms = c.timing_video_ms / callbacks;
    const double audio_ms = c.timing_audio_ms / callbacks;
    const double run_ms = c.timing_run_ms / callbacks;
    const double actual_fps = c.timing_intervals && c.timing_interval_ms > 0
        ? 1000.0 * c.timing_intervals / c.timing_interval_ms : 0.0;
    const double engine_ms = machine_ms + video_ms;
    char text[512];
    std::snprintf(text, sizeof(text),
        "SM2 timing (%s)\nMachine: %5.2f ms  Video: %5.2f ms\n"
        "Audio/pacing: %5.2f ms  retro_run: %5.2f ms\n"
        "Worst: %5.2f ms  Actual: %5.1f FPS\n"
        "Engine cap: %5.1f FPS  Callback cap: %5.1f FPS",
        c.timing == libretro::AVTimingMode::Native ? "Native 57.524160 Hz" : "60 Hz Compatibility",
        machine_ms, video_ms, audio_ms, run_ms, c.timing_worst_ms, actual_fps,
        engine_ms > 0 ? 1000.0 / engine_ms : 0.0,
        run_ms > 0 ? 1000.0 / run_ms : 0.0);
    const retro_message_ext status{text, 2000, 0, RETRO_LOG_INFO,
                                   RETRO_MESSAGE_TARGET_OSD,
                                   RETRO_MESSAGE_TYPE_STATUS, -1};
    const bool shown = environment && environment(
        RETRO_ENVIRONMENT_SET_MESSAGE_EXT, const_cast<retro_message_ext*>(&status));
    if (!shown && environment) {
        retro_message fallback{text, 120};
        environment(RETRO_ENVIRONMENT_SET_MESSAGE, &fallback);
    }
    c.timing_overlay_visible = true;
    if (log_cb) log_cb(RETRO_LOG_INFO,
        "[SM2 Timing] %u callbacks, %u machine frames | machine %.2f ms, video %.2f ms, "
        "audio/pacing %.2f ms, retro_run %.2f ms, worst %.2f ms, actual %.1f FPS, "
        "engine-cap %.1f FPS, callback-cap %.1f FPS\n",
        c.timing_callbacks, c.timing_machine_frames, machine_ms, video_ms, audio_ms,
        run_ms, c.timing_worst_ms, actual_fps,
        engine_ms > 0 ? 1000.0 / engine_ms : 0.0,
        run_ms > 0 ? 1000.0 / run_ms : 0.0);
    reset_timing_measurements(c);
}

void export_frontend_save(Content& c)
{
    libretro::export_save_ram(c.game.name, c.machine->backup_ram(),
                              c.machine->settings_eeprom(), save_ram);
}

bool native_nvram_available(const std::filesystem::path& directory,
                            std::string_view game)
{
    const auto valid_size = [](const std::filesystem::path& path, uintmax_t expected) {
        std::error_code error;
        return std::filesystem::is_regular_file(path, error)
            && !error && std::filesystem::file_size(path, error) == expected && !error;
    };
    const std::string base(game);
    return valid_size(directory / (base + ".nv"), libretro::kBackupRamSize)
        || valid_size(directory / (base + ".eeprom"), libretro::kEepromSize);
}

void initialize_frontend_save(Content& c)
{
    const auto result = libretro::import_save_ram(c.game.name, save_ram,
                                                   c.machine->backup_ram(),
                                                   c.machine->settings_eeprom());
    if (result == libretro::SaveImportResult::Loaded) {
        c.machine->reset();
        c.machine->sound_board().clear_pending_samples();
        c.audio.clear();
        message(RETRO_LOG_INFO, "Loaded frontend-managed save RAM");
    } else {
        if (result == libretro::SaveImportResult::Invalid)
            message(RETRO_LOG_WARN, "Ignored invalid or mismatched frontend save RAM");
        bool seeded = false;
        if (!c.native_nvram_available && libretro::initial_nvram_setup_enabled()) {
            std::string_view seed_game;
            if (libretro::initial_nvram::has_template(c.game.name))
                seed_game = c.game.name;
            else if (libretro::initial_nvram::can_use_parent_template(c.game.name))
                seed_game = c.game.parent;
            if (!seed_game.empty()) {
                const auto seeded_result = libretro::initial_nvram::seed(
                    seed_game, c.machine->backup_ram(), c.machine->settings_eeprom());
                if (seeded_result == libretro::initial_nvram::SeedResult::InvalidTemplate)
                    throw std::runtime_error("Invalid embedded initial NVRAM template");
                if (seeded_result == libretro::initial_nvram::SeedResult::Loaded) {
                    if (!c.nvram_game.empty()) {
                        const auto initial = libretro::nvram::initial_values(c.nvram_game);
                        const auto applied = libretro::nvram::apply(
                            c.nvram_game, c.machine->backup_ram(), c.machine->settings_eeprom(), initial);
                        if (applied == libretro::nvram::ApplyResult::Unsupported
                            || applied == libretro::nvram::ApplyResult::LayoutNotReady)
                            throw std::runtime_error("Embedded initial NVRAM template has an unsupported layout");
                    }
                    c.machine->reset();
                    c.machine->sound_board().clear_pending_samples();
                    c.audio.clear();
                    seeded = true;
                    message(RETRO_LOG_INFO, "Applied automatic initial NVRAM setup before the first frame");
                }
            }
        }
        if (!seeded)
            message(RETRO_LOG_INFO, "Initialized frontend save RAM from native NVRAM or machine defaults");
    }
    c.save_ram_initialized = true;
    export_frontend_save(c);
}

void apply_pending_option(Content& c)
{
    if (!c.option_pending) return;
    const auto result = libretro::nvram::apply(c.nvram_game, c.machine->backup_ram(),
                                                c.machine->settings_eeprom(),
                                                c.nvram_values);
    if (result == libretro::nvram::ApplyResult::LayoutNotReady) return;
    c.option_pending = false;
    if (result == libretro::nvram::ApplyResult::Changed) {
        c.machine->reset();
        c.machine->sound_board().clear_pending_samples();
        c.audio.clear();
        c.cadence_accumulator = 60.0 - c.native_fps;
        c.audio_frame_remainder = 0;
        c.audio_frames_due = 0;
        export_frontend_save(c);
        message(RETRO_LOG_INFO, "Applied NVRAM core options");
    }
}

void message(enum retro_log_level level, const char* text)
{
    if (log_cb) log_cb(level, "[SM2] %s\n", text);
    else std::fprintf(stderr, "[SM2] %s\n", text);
    if (level >= RETRO_LOG_WARN && environment) {
        retro_message msg{text, 300};
        environment(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
    }
}
void failure(const char* text)
{
    message(RETRO_LOG_ERROR, text);
    if (content) {
        content->rumble.stop();
        content->failed = true;
    }
    if (environment) environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
}
std::filesystem::path directory(unsigned command)
{
    const char* value = nullptr;
    if (!environment || !environment(command, &value) || !value || !*value)
        throw std::runtime_error(command == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY
            ? "Frontend system directory is missing; supply system/sm2-emu/games.xml."
            : "Frontend save directory is missing.");
    return value;
}
template<class Board> constexpr double board_fps()
{
    return static_cast<double>(Board::kCpuClock) / Board::kCyclesPerFrame;
}
double game_fps(rom::Board board)
{
    switch (board) {
    case rom::Board::Model2: return board_fps<hw::Model2Original>();
    case rom::Board::Model2A: return board_fps<hw::Model2>();
    case rom::Board::Model2B: return board_fps<hw::Model2B>();
    case rom::Board::Model2C: return board_fps<hw::Model2C>();
    }
    throw std::runtime_error("Unknown board timing");
}
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
void context_destroy()
{
    if (content) {
#ifdef SM2_LIBRETRO_OPENGL
        content->gl.reset();
#endif
#ifdef SM2_LIBRETRO_VULKAN
        content->gpu.reset();
#endif
        content->hardware_frame_valid = false;
    }
    message(RETRO_LOG_INFO, "GPU context resources released");
}
void context_reset()
{
    if (!content || content->renderer_api == RendererApi::Software) return;
    try {
#ifdef SM2_LIBRETRO_OPENGL
        // With GL, reset may follow an unannounced context loss. Resource
        // names from that dead context must be forgotten, not deleted.
        if (content->gl) content->gl->abandon_context();
        content->gl.reset();
#endif
#ifdef SM2_LIBRETRO_VULKAN
        content->gpu.reset();
#endif
        if (content->renderer_api == RendererApi::Vulkan) {
#ifdef SM2_LIBRETRO_VULKAN
            auto gpu = std::make_unique<libretro::VulkanRenderer>();
            gpu->init(environment, content->scale, log_cb);
            content->gpu = std::move(gpu);
#else
            throw std::runtime_error("This core was built without Vulkan support");
#endif
        } else {
#ifdef SM2_LIBRETRO_OPENGL
            auto gl = std::make_unique<libretro::OpenGlRenderer>();
            gl->init(content->scale, content->renderer_api == RendererApi::OpenGLES, log_cb);
            content->gl = std::move(gl);
#else
            throw std::runtime_error("This core was built without OpenGL support");
#endif
        }
        // A recreated frontend device has no image to duplicate yet. The next
        // callback must submit one even when 60 Hz pacing repeats the machine frame.
        content->hardware_frame_valid = false;
    } catch (const std::exception& e) { failure(e.what()); }
    catch (...) { failure("Unexpected GPU context failure"); }
}
void select_renderer(Content& c)
{
    retro_variable renderer{"sm2_renderer", nullptr};
    environment(RETRO_ENVIRONMENT_GET_VARIABLE, &renderer);
    const std::string value = renderer.value ? renderer.value : "auto";
    unsigned preferred = RETRO_HW_CONTEXT_NONE;
    environment(RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER, &preferred);
    if (value == "vulkan") {
#ifdef SM2_LIBRETRO_VULKAN
        c.renderer_api = RendererApi::Vulkan;
#else
        throw std::runtime_error("This core was built without Vulkan support");
#endif
    } else if (value == "opengl") {
#ifdef SM2_LIBRETRO_OPENGL
        c.renderer_api = preferred == RETRO_HW_CONTEXT_OPENGLES2 ||
                                 preferred == RETRO_HW_CONTEXT_OPENGLES3 ||
                                 preferred == RETRO_HW_CONTEXT_OPENGLES_VERSION
                             ? RendererApi::OpenGLES
                             : RendererApi::OpenGL;
#else
        throw std::runtime_error("This core was built without OpenGL support");
#endif
    } else if (value == "auto") {
#ifdef SM2_LIBRETRO_VULKAN
        if (preferred == RETRO_HW_CONTEXT_VULKAN) c.renderer_api = RendererApi::Vulkan;
#endif
#ifdef SM2_LIBRETRO_OPENGL
        if (preferred == RETRO_HW_CONTEXT_OPENGL || preferred == RETRO_HW_CONTEXT_OPENGL_CORE)
            c.renderer_api = RendererApi::OpenGL;
        else if (preferred == RETRO_HW_CONTEXT_OPENGLES2 ||
                 preferred == RETRO_HW_CONTEXT_OPENGLES3 ||
                 preferred == RETRO_HW_CONTEXT_OPENGLES_VERSION)
            c.renderer_api = RendererApi::OpenGLES;
#endif
    }
    if (c.renderer_api == RendererApi::Software) return;
    retro_variable resolution{"sm2_internal_resolution", nullptr};
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &resolution) && resolution.value &&
        resolution.value[0] >= '1' && resolution.value[0] <= '4' && resolution.value[1] == '\0')
        c.scale = static_cast<unsigned>(resolution.value[0] - '0');
    bool requested = false;
    if (c.renderer_api == RendererApi::Vulkan) {
#ifdef SM2_LIBRETRO_VULKAN
        requested = libretro::request_vulkan(environment, context_reset, context_destroy, log_cb);
#endif
    } else {
#ifdef SM2_LIBRETRO_OPENGL
        requested = libretro::request_opengl(environment, context_reset, context_destroy,
                                             c.renderer_api == RendererApi::OpenGLES, log_cb);
        // Some frontends report the generic desktop GL preference even when
        // their build can only create GLES contexts (RetroArch 1.18 on Ubuntu
        // arm64 is one example).  A rejected request has created no context, so
        // retry the other GL dialect once before reporting failure.
        if (!requested) {
            c.renderer_api = c.renderer_api == RendererApi::OpenGLES
                ? RendererApi::OpenGL : RendererApi::OpenGLES;
            requested = libretro::request_opengl(environment, context_reset, context_destroy,
                                                 c.renderer_api == RendererApi::OpenGLES,
                                                 log_cb);
        }
#endif
    }
    if (!requested)
        throw std::runtime_error("Hardware renderer context negotiation failed; select Software and reload content.");
}
#endif
void unload()
{
    if (content) {
        clear_timing_overlay(*content);
        content->rumble.stop();
        try {
            if (content->save_ram_initialized) export_frontend_save(*content);
        }
        catch (const std::exception& error) { message(RETRO_LOG_ERROR, error.what()); }
        content.reset();
    }
    libretro::set_option_game({});
    if (environment) {
        static const retro_input_descriptor empty[] = {{}};
        environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(empty));
        static const retro_controller_info no_controllers[] = {{nullptr, 0}};
        environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, const_cast<retro_controller_info*>(no_controllers));
    }
}

void publish_controls()
{
    if (!content || !environment) return;
    const auto gun_mode = libretro::gun_input_mode();
    libretro::configure_controllers(content->game, content->controllers, gun_mode);
    content->input_descriptors = libretro::descriptors(
        content->game, devices, gun_mode, libretro::offscreen_reload_shortcut_enabled());
    environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, content->controllers.ports.data());
    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, content->input_descriptors.data());
}
size_t send_audio(Content& c, size_t requested_frames = static_cast<size_t>(-1))
{
    auto& sound = c.machine->sound_board();
    auto samples = sound.pending_samples();
    if (samples.size() % 2 || sound.sample_rate() != c.rate)
        throw std::runtime_error("Invalid stereo audio or unexpected sample-rate change");
    // Keep unaccepted samples for the next call. A stalled frontend must not
    // cause unbounded allocation or silently corrupt the emulated audio stream.
    if (samples.size() + c.audio.size() > static_cast<size_t>(c.rate) * 2)
        throw std::runtime_error("Frontend audio stalled for more than one second");
    c.audio.insert(c.audio.end(), samples.begin(), samples.end());
    sound.clear_pending_samples();
    size_t accepted = 0;
    const size_t frames = std::min(c.audio.size() / 2, requested_frames);
    if (frames && audio_batch_cb) {
        accepted = audio_batch_cb(c.audio.data(), frames);
        if (accepted > frames) throw std::runtime_error("Invalid audio callback result");
    } else if (audio_cb) {
        for (size_t i = 0; i < frames; ++i) audio_cb(c.audio[2*i], c.audio[2*i+1]);
        accepted = frames;
    } else {
        accepted = frames; // The frontend intentionally supplied no audio sink.
    }
    c.audio.erase(c.audio.begin(), c.audio.begin() + static_cast<std::ptrdiff_t>(accepted * 2));
    return accepted;
}
}

extern "C" {
void retro_set_environment(retro_environment_t cb)
{
    environment = cb;
    if (cb) {
        bool no_game = false;
        cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
        libretro::register_core_options(cb);
    }
}
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }
unsigned retro_api_version() { return RETRO_API_VERSION; }
void retro_init()
{
    log_cb = nullptr;
    retro_log_callback logger{};
    if (environment && environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logger)) log_cb = logger.log;
    if (!libretro::register_netpacket_interface(environment, log_cb))
        message(RETRO_LOG_WARN, "Libretro Netpacket is unavailable; linked cabinets are disabled");
    if (environment) {
        uint64_t quirks = RETRO_SERIALIZATION_QUIRK_ENDIAN_DEPENDENT
                        | RETRO_SERIALIZATION_QUIRK_PLATFORM_DEPENDENT;
        environment(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &quirks);
    }
    devices.fill(RETRO_DEVICE_JOYPAD);
}
void retro_deinit()
{
    unload();
    libretro::shutdown_netpacket_interface();
    log_cb = nullptr;
}
void retro_get_system_info(retro_system_info* info)
{
    if (info) *info = {"SM2-Emu", "0.9.9-libretro-dev", "zip|7z", true, true};
}
void retro_get_system_av_info(retro_system_av_info* info)
{
    const unsigned scale = content ? content->scale : 1;
    const float aspect_ratio = content ? content->aspect_ratio : 4.0f / 3.0f;
    if (info) *info = {{width * scale, height * scale, width * scale, height * scale, aspect_ratio},
                      {content ? content->fps : board_fps<hw::Model2>(),
                       content ? static_cast<double>(content->rate) : 44100.0}};
}
void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port >= devices.size()) return;
    devices[port] = device;
    if (port == 0 && device == RETRO_DEVICE_NONE && content)
        content->rumble.stop();
    publish_controls();
}
bool retro_load_game(const retro_game_info* game)
{
    unload();
    save_ram.fill(0);
    try {
        if (!game || !game->path || !*game->path) throw std::runtime_error("A ROM archive path is required");
        enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
        if (!environment || !environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format))
            throw std::runtime_error("Frontend does not support XRGB8888 video");
        const auto db_path = directory(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY) / "sm2-emu" / "games.xml";
        // RetroArch may already return a core- or content-specific save path.
        // Use it directly, as the frontend owns the .srm location. Optional
        // standalone .nv/.eeprom files are imported from the same directory.
        const auto save_path = directory(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY);
        rom::GameDatabase database;
        if (!database.load(db_path.string()))
            throw std::runtime_error("Cannot load system/sm2-emu/games.xml");
        auto loaded = rom::RomLoader::load(database, game->path);
        if (!loaded) throw std::runtime_error("ROM loading failed; see missing-file/CRC details in log");
        // The database name becomes a native-save filename and save-container ID,
        // never an arbitrary path.
        if (loaded->game.name.empty() || loaded->game.name == "." || loaded->game.name == ".."
            || loaded->game.name.find_first_of("/\\") != std::string::npos)
            throw std::runtime_error("Invalid ROM set name in database");
        auto next = std::make_unique<Content>();
        next->game = loaded->game;
        next->machine = hw::create_machine(next->game, std::move(loaded->roms));
        if (!next->machine) throw std::runtime_error("Machine initialization failed");
        next->machine->sound_board().set_audio_balance_enabled(
            libretro::audio_balance_enabled());
        next->machine->sound_board().set_music_volume_percent(
            libretro::music_volume_percent());
        next->nvram_game = !libretro::nvram::options_for_game(next->game.name).empty()
            ? next->game.name : next->game.parent;
        libretro::set_option_game(next->nvram_game, next->game.name);
        const unsigned linked_cabinets = libretro::linked_cabinets(next->game.name);
        if (linked_cabinets > 1) {
            auto network = std::make_unique<libretro::NetpacketTransport>();
            network->configure(libretro::linked_cabinet_network_family(next->game.name),
                               linked_cabinets);
            next->machine->comm().set_transport(std::move(network));
            next->external_link_active = true;
            if (!libretro::netpacket_interface_supported())
                message(RETRO_LOG_WARN,
                        "Linked Cabinets is enabled but the frontend has no Netpacket support");
            else {
                const std::string link_message = "Linked-cabinet communication enabled for "
                    + next->game.name + " through RetroArch Netplay";
                message(RETRO_LOG_INFO, link_message.c_str());
            }
        }
        next->native_nvram_available = native_nvram_available(save_path, next->game.name);
        next->machine->set_nvram_directory(save_path.string());
        next->machine->load_nvram();
        next->machine->reset();
        if (!next->external_link_active) {
            std::vector<u8> initial_state;
            if (!next->machine->save_state(initial_state) || initial_state.empty())
                throw std::runtime_error("Cannot initialize save-state support");
            if (initial_state.size() > libretro_state_size)
                throw std::runtime_error("Machine state exceeds the Libretro buffer");
        }
        next->rate = next->machine->sound_board().sample_rate();
        if (!next->rate) throw std::runtime_error("Sound board reported zero sample rate");
        next->native_fps = game_fps(next->game.board);
        next->timing = libretro::av_timing_mode();
        next->fps = next->timing == libretro::AVTimingMode::Compatibility60Hz
            ? 60.0 : next->native_fps;
        next->cadence_accumulator = 60.0 - next->native_fps;
        next->timing_overlay = libretro::timing_overlay_enabled();
        environment(RETRO_ENVIRONMENT_GET_CAN_DUPE, &next->can_dupe);
        next->nvram_values = libretro::selected_nvram_values(next->nvram_game);
        next->option_pending = !next->nvram_values.empty() && libretro::nvram_settings_enabled();
        next->audio.reserve(static_cast<size_t>(next->rate) * 2);
        const bool rumble_ready = next->rumble.init(environment);
        content = std::move(next);
        if (libretro::aspect_ratio_mode() == libretro::AspectRatioMode::SixteenNine)
            content->aspect_ratio = 16.0f / 9.0f;
        libretro::set_option_game(content->nvram_game, content->game.name);
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
        select_renderer(*content);
#endif
        devices.fill(RETRO_DEVICE_JOYPAD);
        publish_controls();
        if (content->game.has_steering() && !rumble_ready)
            message(RETRO_LOG_INFO, "Frontend gamepad rumble interface is unavailable");
        if (!libretro::digital_profile(content->game))
            message(RETRO_LOG_WARN, "This cabinet's gameplay controls are not implemented yet; Coin/Start/Test/Service only.");
        else {
            char controls[192];
            std::snprintf(controls, sizeof(controls), "Input profile: %s",
                          libretro::profile_name(libretro::recognize_profile(content->game)));
            message(RETRO_LOG_INFO, controls);
        }
        char report[256];
        std::snprintf(report, sizeof(report), "Loaded %s: %ux%u, %.9f Hz, %u Hz stereo",
                      content->game.name.c_str(), width, height, content->fps, content->rate);
        message(RETRO_LOG_INFO, report);
        const char* renderer = "Renderer: Software";
        if (content->renderer_api == RendererApi::Vulkan)
            renderer = "Renderer: Vulkan (awaiting frontend context)";
        else if (content->renderer_api == RendererApi::OpenGL)
            renderer = "Renderer: OpenGL 4.3 core (awaiting frontend context)";
        else if (content->renderer_api == RendererApi::OpenGLES)
            renderer = "Renderer: OpenGL ES 3.1 (awaiting frontend context)";
        message(RETRO_LOG_INFO, renderer);
        return true;
    } catch (const std::exception& error) { message(RETRO_LOG_ERROR, error.what()); }
    catch (...) { message(RETRO_LOG_ERROR, "Unexpected error while loading content"); }
    content.reset();
    libretro::set_option_game({});
    return false;
}
void retro_unload_game() { unload(); }
void retro_reset()
{
    if (!content) return;
    try {
        content->rumble.stop();
        content->machine->reset();
        content->machine->sound_board().clear_pending_samples();
        content->audio.clear();
        content->cadence_accumulator = 60.0 - content->native_fps;
        content->input_runtime = {};
        content->audio_frame_remainder = 0;
        content->audio_frames_due = 0;
        content->have_previous_run_start = false;
        reset_timing_measurements(*content);
        content->failed = false;
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
        // A machine reset can reuse generation numbers: discard GPU caches.
        if (content->renderer_api != RendererApi::Software) {
            context_destroy();
            context_reset();
        }
#endif
    } catch (const std::exception& error) { failure(error.what()); }
    catch (...) { failure("Unexpected reset failure"); }
}
void retro_run()
{
    if (!content || content->failed) return;
    try {
        const auto run_start = std::chrono::steady_clock::now();
        if (content->have_previous_run_start) {
            content->timing_interval_ms += elapsed_ms(content->previous_run_start, run_start);
            ++content->timing_intervals;
        }
        content->previous_run_start = run_start;
        content->have_previous_run_start = true;
        bool variables_updated = false;
        if (environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &variables_updated)
            && variables_updated) {
            const bool overlay = libretro::timing_overlay_enabled();
            if (!overlay) clear_timing_overlay(*content);
            if (overlay && !content->timing_overlay) reset_timing_measurements(*content);
            content->timing_overlay = overlay;
            if (!libretro::gamepad_rumble_enabled()) content->rumble.stop();
            content->machine->sound_board().set_audio_balance_enabled(
                libretro::audio_balance_enabled());
            content->machine->sound_board().set_music_volume_percent(
                libretro::music_volume_percent());
            publish_controls();
        }
        if (!content->save_ram_initialized) initialize_frontend_save(*content);
        apply_pending_option(*content);
        update_aspect_ratio(*content);
        if (input_poll_cb) input_poll_cb();
        s16 rumble_steering = 0;
        if (input_state_cb && devices[0] != RETRO_DEVICE_NONE)
            rumble_steering = input_state_cb(0, RETRO_DEVICE_ANALOG,
                RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
        bool advance_machine = true;
        if (content->timing == libretro::AVTimingMode::Compatibility60Hz) {
            content->cadence_accumulator += content->native_fps;
            advance_machine = content->cadence_accumulator >= 60.0;
            if (advance_machine) content->cadence_accumulator -= 60.0;
        }
        if (advance_machine) {
            libretro::poll_input(content->machine->inputs(), content->game, devices,
                                 content->input_runtime, libretro::four_speed_h_gate(),
                                 input_state_cb, libretro::gun_input_mode(),
                                 libretro::offscreen_reload_shortcut_enabled(),
                                 libretro::driving_analog_options(),
                                 libretro::desert_elevation_options(),
                                 libretro::automatic_start_gear_enabled(),
                                 libretro::mouse_edge_offscreen_reload_enabled());
            if (content->game.name == "skisuprg"
                && content->machine->frames() == ski_super_g_drive_board_test_frame
                && libretro::ski_super_g_drive_board_bypass_enabled()) {
                content->machine->inputs().in0 &= static_cast<u8>(~0x04);
                message(RETRO_LOG_INFO,
                        "Sega Ski Super G Drive Board error bypass: pressed Test");
            }
            const auto machine_start = std::chrono::steady_clock::now();
            content->machine->run_frame();
            update_aspect_ratio(*content);
            const auto drive_writes = content->machine->take_drive_board_writes();
            content->rumble.update(content->game, drive_writes.view(), rumble_steering,
                                   libretro::gamepad_rumble_enabled());
            const auto machine_end = std::chrono::steady_clock::now();
            content->timing_machine_ms += elapsed_ms(machine_start, machine_end);
            ++content->timing_machine_frames;
            if (content->option_pending) {
                apply_pending_option(*content);
                if (content->option_pending && ++content->option_wait_frames == 600) {
                    content->option_pending = false;
                    message(RETRO_LOG_WARN, "NVRAM layout did not become ready; core options were not applied");
                }
            }
            const auto status = content->machine->main_cpu_status();
            if (status.faulted) throw std::runtime_error(status.fault_message);
        }
        const auto video_start = std::chrono::steady_clock::now();
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
        if (content->renderer_api != RendererApi::Software) {
            if (advance_machine || !content->can_dupe || !content->hardware_frame_valid) {
                if (content->renderer_api == RendererApi::Vulkan) {
#ifdef SM2_LIBRETRO_VULKAN
                    if (!content->gpu) throw std::runtime_error("Frontend Vulkan context is not ready");
                    content->gpu->render(*content->machine, video_cb,
                        libretro::crosshair_state(content->game, content->input_runtime,
                                                  libretro::crosshair_mask(content->game),
                                                  libretro::crosshair_style()),
                        libretro::texture_filter_quality(), libretro::upscale_2d_mode());
#else
                    throw std::runtime_error("This core was built without Vulkan support");
#endif
                } else {
#ifdef SM2_LIBRETRO_OPENGL
                    if (!content->gl) throw std::runtime_error("Frontend OpenGL context is not ready");
                    content->gl->render(*content->machine, video_cb,
                        libretro::crosshair_state(content->game, content->input_runtime,
                                                  libretro::crosshair_mask(content->game),
                                                  libretro::crosshair_style()),
                        libretro::texture_filter_quality(), libretro::upscale_2d_mode());
#else
                    throw std::runtime_error("This core was built without OpenGL support");
#endif
                }
                content->hardware_frame_valid = true;
            }
            else if (video_cb) video_cb(nullptr, width * content->scale, height * content->scale, 0);
        } else
#endif
        {
            if (advance_machine) {
                content->machine->compose_video();
                content->renderer.render(*content->machine, content->machine->render_list(), content->frame);
                // Upstream packs R in bits 0..7. Libretro XRGB8888 packs R in 16..23.
                for (auto& pixel : content->frame)
                    pixel = ((pixel & 0xffu) << 16) | (pixel & 0xff00u) | ((pixel >> 16) & 0xffu);
                libretro::draw_crosshairs(content->frame, width, height,
                    libretro::crosshair_state(content->game, content->input_runtime,
                                              libretro::crosshair_mask(content->game),
                                              libretro::crosshair_style()));
            }
            if (video_cb) {
                if (!advance_machine && content->can_dupe) video_cb(nullptr, width, height, 0);
                else video_cb(content->frame.data(), width, height, width * sizeof(u32));
            }
        }
        const auto video_end = std::chrono::steady_clock::now();
        content->timing_video_ms += elapsed_ms(video_start, video_end);
        const auto audio_start = video_end;
        size_t requested_audio = static_cast<size_t>(-1);
        if (content->timing == libretro::AVTimingMode::Compatibility60Hz) {
            content->audio_frame_remainder += content->rate;
            requested_audio = static_cast<size_t>(content->audio_frame_remainder / 60);
            content->audio_frame_remainder %= 60;
            content->audio_frames_due += requested_audio;
            requested_audio = content->audio_frames_due;
        }
        const size_t accepted_audio = send_audio(*content, requested_audio);
        if (content->timing == libretro::AVTimingMode::Compatibility60Hz)
            content->audio_frames_due -= std::min(content->audio_frames_due, accepted_audio);
        const auto run_end = std::chrono::steady_clock::now();
        content->timing_audio_ms += elapsed_ms(audio_start, run_end);
        const double run_ms = elapsed_ms(run_start, run_end);
        content->timing_run_ms += run_ms;
        content->timing_worst_ms = std::max(content->timing_worst_ms, run_ms);
        if (++content->timing_callbacks >= 61) {
            if (content->timing_overlay) publish_timing_overlay(*content);
            else reset_timing_measurements(*content);
        }
    } catch (const std::exception& error) { failure(error.what()); }
    catch (...) { failure("Unexpected frame execution failure"); }
}
size_t retro_serialize_size()
{
    return libretro_state_size;
}
bool retro_serialize(void* data, size_t size)
{
    if (content && content->external_link_active) {
        message(RETRO_LOG_WARN,
                "Save states are unavailable during a Linked Cabinets session");
        return false;
    }
    if (!content || !data || size < libretro_state_size) {
        return false;
    }
    try {
        std::vector<u8> state;
        if (!content->machine->save_state(state)
            || state.size() > libretro_state_size) {
            message(RETRO_LOG_ERROR, "Save-state buffer capacity exceeded");
            return false;
        }
        std::memcpy(data, state.data(), state.size());
        std::memset(static_cast<u8*>(data) + state.size(), 0,
                    libretro_state_size - state.size());
        return true;
    } catch (const std::exception& error) {
        message(RETRO_LOG_ERROR, error.what());
        return false;
    } catch (...) {
        message(RETRO_LOG_ERROR, "Unexpected save-state serialization failure");
        return false;
    }
}
bool retro_unserialize(const void* data, size_t size)
{
    if (content && content->external_link_active) {
        message(RETRO_LOG_WARN,
                "Save states are unavailable during a Linked Cabinets session");
        return false;
    }
    if (!content || !data || size == 0)
        return false;
    try {
        if (!content->machine->load_state(static_cast<const u8*>(data), size))
            return false;
        reset_frontend_after_state_load(*content);
        return true;
    } catch (const std::exception& error) {
        message(RETRO_LOG_ERROR, error.what());
        return false;
    } catch (...) {
        message(RETRO_LOG_ERROR, "Unexpected save-state load failure");
        return false;
    }
}
void retro_cheat_reset() {}
void retro_cheat_set(unsigned, bool, const char*) {}
bool retro_load_game_special(unsigned, const retro_game_info*, size_t) { return false; }
unsigned retro_get_region() { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned id)
{
    if (id != RETRO_MEMORY_SAVE_RAM) return nullptr;
    try {
        // Before the first frame this buffer must remain untouched: the frontend
        // obtains this pointer and restores its .srm contents into it.
        if (content && content->save_ram_initialized) export_frontend_save(*content);
        return save_ram.data();
    } catch (const std::exception& error) {
        message(RETRO_LOG_ERROR, error.what());
        return nullptr;
    }
}
size_t retro_get_memory_size(unsigned id)
{
    return id == RETRO_MEMORY_SAVE_RAM ? save_ram.size() : 0;
}
}
