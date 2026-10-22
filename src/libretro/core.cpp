// SPDX-License-Identifier: BSD-3-Clause
// Libretro owns presentation and pacing; the upstream machine owns emulation.
#include "libretro.h"
#include "core/archive.h"
#include "core_options.h"
#include "gun_flash_state.h"
#include "crosshair.h"
#include "audio_output.h"
#include "timing_overlay.h"
#include "initial_nvram.h"
#include "manxtt_audio.h"
#include "power_sled_audio.h"
#include "power_sled_standalone.h"
#include "power_sled_commands.h"
#include "input.h"
#include "netpacket.h"
#include "rumble.h"
#include "save_ram.h"
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
#include "gpu.h"
#endif
#include "render/backend.h"
#include "hw/machine_factory.h"
#include "hw/m2comm.h"
#include "hw/model2.h"
#include "hw/model2_original.h"
#include "hw/model2b.h"
#include "hw/model2c.h"
#include "hw/model2_softrender.h"
#include "hw/model2_video.h"
#include "hw/model2_softrender_async.h"
#include "hw/sound_board.h"
#include "hw/model2_sound.h"
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
#include <utility>
#include <vector>

namespace {
using namespace sm2;
constexpr std::array<hw::AudioSource, 5> kAudioSources{{
    hw::AudioSource::Scsp, hw::AudioSource::Dsb,
    hw::AudioSource::MultiPcm1, hw::AudioSource::MultiPcm2,
    hw::AudioSource::Ym3438}};

void configure_audio_sources(hw::SoundBoard& sound)
{
    std::array<bool, kAudioSources.size()> present{};
    for (size_t i = 0; i < kAudioSources.size(); ++i) {
        present[i] = sound.has_audio_source(kAudioSources[i]);
        if (present[i]) sound.set_audio_source_gain(
            kAudioSources[i], libretro::audio_source_gain(i),
            libretro::audio_source_gain_automatic(i));
    }
    if (auto* board = dynamic_cast<hw::Model2Sound*>(&sound))
        if (auto* relay = board->auxiliary_scsp())
            relay->set_audio_source_gain(hw::AudioSource::Scsp,
                libretro::audio_gain("sm2_power_sled_relay_audio_gain"),
                libretro::audio_gain_automatic("sm2_power_sled_relay_audio_gain"));
    libretro::set_audio_sources(present);
}
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
constexpr size_t frontend_state_size = 96;

struct Content {
    rom::GameSpec game;
    libretro::ControllerConfiguration controllers;
    bool published_test_service = true;
    libretro::InputRuntime input_runtime;
    libretro::ManxTtAudio manxtt_audio;  // ROM storage must outlive the machine.
    libretro::PowerSledAudio power_sled_audio;
    bool power_sled_standalone_requested = false;
    bool power_sled_standalone = false;
    bool power_sled_commands_requested = false;
    bool power_sled_ai_finish_speed_fix_requested = false;
    bool power_sled_commands = false;
    std::unique_ptr<hw::Model2MachineBase> machine;
    hw::SoftRenderer renderer;
    std::unique_ptr<hw::AsyncSoftRenderer> async_renderer;
    std::vector<u32> frame = std::vector<u32>(width * height);
    std::vector<u32> async_frame = std::vector<u32>(width * height);
    std::vector<s16> audio;
    libretro::OutputDcBlocker output_dc;
    std::vector<retro_input_descriptor> input_descriptors;
    libretro::GamepadRumble rumble;
    double native_fps = 0;
    double fps = 0;
    u32 rate = 0;
    bool failed = false;
    RendererApi renderer_api = RendererApi::Software;
    bool async_frame_ready = false;
    libretro::CrosshairState async_crosshairs{};
    libretro::CrosshairPresentation crosshair_presentation;
    libretro::TimingOverlayData async_timing_overlay_data{};
    double async_fps = 0;
    unsigned scale = 1;
    unsigned requested_scale = 1;
    bool save_ram_initialized = false;
    bool native_nvram_available = false;
    bool first_frame_complete = false;
    bool option_pending = false;
    bool reset_pending = false;
    bool automation_applied = false;
    bool automation_requested = false;
    std::vector<libretro::network::ManagedValue> managed_requested;
    std::vector<libretro::network::ManagedValue> managed_applied;
    std::array<std::vector<u8>, 2> power_sled_programs;
    unsigned option_wait_frames = 0;
    std::string nvram_game;
    std::vector<std::string> nvram_values;
    std::vector<std::pair<std::string, std::string>> network_option_updates;
    libretro::AVTimingMode timing = libretro::AVTimingMode::Native;
    // Retained in frontend Save States created by the earlier 60 Hz cadence.
    double cadence_accumulator = 0;
    u64 audio_frame_remainder = 0;
    size_t audio_frames_due = 0;
    bool timing_overlay = false;
    unsigned timing_overlay_font_pixels = 0;
    libretro::TimingOverlayData timing_overlay_data{};
    bool external_link_active = false;
    u64 logged_comm_state = ~u64{0};
    unsigned linked_cabinets = 1;
    float aspect_ratio = 4.0f / 3.0f;
    unsigned wide_columns = 0;
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

constexpr std::array<u8, 8> frontend_state_magic{'S', 'M', '2', 'L', 'R', 'T', 'R', '1'};

template <typename T>
void state_write(u8*& cursor, T value)
{
    std::memcpy(cursor, &value, sizeof(value));
    cursor += sizeof(value);
}

template <typename T>
T state_read(const u8*& cursor)
{
    T value{};
    std::memcpy(&value, cursor, sizeof(value));
    cursor += sizeof(value);
    return value;
}

void save_frontend_state(u8* destination, const Content& c)
{
    std::memset(destination, 0, frontend_state_size);
    u8* cursor = destination;
    std::memcpy(cursor, frontend_state_magic.data(), frontend_state_magic.size());
    cursor += frontend_state_magic.size();
    state_write<u32>(cursor, 1);
    state_write<u32>(cursor, c.input_runtime.gear);
    u32 flags = c.input_runtime.gear_initialized ? 1u : 0u;
    flags |= c.input_runtime.shift_up_held ? 1u << 1 : 0u;
    flags |= c.input_runtime.shift_down_held ? 1u << 2 : 0u;
    flags |= (std::min(c.input_runtime.shift_neutral_frames, 29u) & 0x1fu) << 8;
    flags |= c.input_runtime.desert_shift ? 1u << 3 : 0u;
    flags |= c.input_runtime.desert_shift_held ? 1u << 4 : 0u;
    state_write<u32>(cursor, flags);
    state_write<float>(cursor, c.input_runtime.desert_elevation);
    for (int value : c.input_runtime.gun_cursor_x) state_write<s32>(cursor, value);
    for (int value : c.input_runtime.gun_cursor_y) state_write<s32>(cursor, value);
    for (s16 value : c.input_runtime.gun_lightgun_x) state_write<s16>(cursor, value);
    for (s16 value : c.input_runtime.gun_lightgun_y) state_write<s16>(cursor, value);
    flags = 0;
    for (unsigned player = 0; player < 2; ++player) {
        flags |= c.input_runtime.gun_lightgun_initialized[player] ? 1u << player : 0u;
        flags |= c.input_runtime.gun_aim_active[player] ? 1u << (player + 2) : 0u;
        flags |= c.input_runtime.gun_aim_offscreen[player] ? 1u << (player + 4) : 0u;
    }
    state_write<u32>(cursor, flags);
    state_write<double>(cursor, c.cadence_accumulator);
    state_write<u64>(cursor, c.audio_frame_remainder);
    state_write<u64>(cursor, static_cast<u64>(c.audio_frames_due));
    const auto flash = c.machine->video().gun_flash_state();
    state_write<u32>(cursor, libretro::gun_flash_flags(flash) | (1u << 5)
        | (c.power_sled_audio.active() ? 1u << 7 : 0u)
        | (c.power_sled_standalone ? 1u << 8 : 0u)
        | (c.power_sled_commands ? 1u << 9 : 0u));
    for (float value : c.output_dc.previous_input) state_write<float>(cursor, value);
    for (float value : c.output_dc.previous_output) state_write<float>(cursor, value);
}

bool load_frontend_state(const u8* source, libretro::InputRuntime& input_runtime,
                         double& cadence_accumulator, u64& audio_frame_remainder,
                         size_t& audio_frames_due, hw::Model2Video::GunFlashState& flash,
                         libretro::OutputDcBlocker& output_dc)
{
    if (std::memcmp(source, frontend_state_magic.data(), frontend_state_magic.size()) != 0)
        return false;
    const u8* cursor = source + frontend_state_magic.size();
    if (state_read<u32>(cursor) != 1) return false;
    input_runtime = {};
    input_runtime.gear = state_read<u32>(cursor);
    u32 flags = state_read<u32>(cursor);
    input_runtime.gear_initialized = flags & 1u;
    input_runtime.shift_up_held = flags & (1u << 1);
    input_runtime.shift_down_held = flags & (1u << 2);
    input_runtime.shift_neutral_frames = std::min((flags >> 8) & 0x1fu, 29u);
    input_runtime.desert_shift = flags & (1u << 3);
    input_runtime.desert_shift_held = flags & (1u << 4);
    input_runtime.desert_elevation = state_read<float>(cursor);
    for (int& value : input_runtime.gun_cursor_x) value = state_read<s32>(cursor);
    for (int& value : input_runtime.gun_cursor_y) value = state_read<s32>(cursor);
    for (s16& value : input_runtime.gun_lightgun_x) value = state_read<s16>(cursor);
    for (s16& value : input_runtime.gun_lightgun_y) value = state_read<s16>(cursor);
    flags = state_read<u32>(cursor);
    for (unsigned player = 0; player < 2; ++player) {
        input_runtime.gun_lightgun_initialized[player] = flags & (1u << player);
        input_runtime.gun_aim_active[player] = flags & (1u << (player + 2));
        input_runtime.gun_aim_offscreen[player] = flags & (1u << (player + 4));
    }
    cadence_accumulator = state_read<double>(cursor);
    audio_frame_remainder = state_read<u64>(cursor);
    audio_frames_due = static_cast<size_t>(state_read<u64>(cursor));
    const u32 flash_flags = state_read<u32>(cursor);
    if ((flash_flags & 0xffff0000u) == 0x47460000u) {
        flash = libretro::gun_flash_state_from_flags(flash_flags);
        if (flash_flags & (1u << 5)) {
            for (float& value : output_dc.previous_input) value = state_read<float>(cursor);
            for (float& value : output_dc.previous_output) value = state_read<float>(cursor);
            if (!output_dc.valid()) output_dc.reset();
        }
    }
    return true;
}

void message(enum retro_log_level level, const char* text, bool notify = false);

bool automatic_widescreen(const Content& c)
{
    const std::string_view family = c.game.parent.empty() ? c.game.name : c.game.parent;
    if (family == "sgt24h") {
        const auto backup = c.machine->backup_ram();
        return backup.size() > 0x0b && backup[0x0a] == 0x03 && backup[0x0b] == 0x00;
    }
    const auto eeprom = c.machine->settings_eeprom();
    if (eeprom.size() <= 0x17) return false;
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

unsigned displayed_width(const Content& c)
{
    return width + 2 * c.wide_columns;
}

unsigned maximum_width(const Content& c)
{
    return (c.renderer_api == RendererApi::Software ? width : width + 2 * render::kWideColumns)
        * c.requested_scale;
}

void update_aspect_ratio(Content& c)
{
    const unsigned columns = libretro::expand_3d_view_for(
        c.game, c.renderer_api != RendererApi::Software) ? render::kWideColumns : 0;
    const float aspect_ratio = columns ? 16.0f / 9.0f : selected_aspect_ratio(c);
    const bool changed = aspect_ratio != c.aspect_ratio || columns != c.wide_columns;
    c.aspect_ratio = aspect_ratio;
    c.wide_columns = columns;
    // Reapply after reset or state load; the machine owns the geometry changes.
    c.machine->set_widescreen(columns);
    if (!changed) return;
    retro_game_geometry geometry{
        displayed_width(c) * c.scale, height * c.scale, maximum_width(c),
        height * c.requested_scale, c.aspect_ratio};
    if (environment)
        environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
}

unsigned wide_background(const Content& c)
{
    if (!c.wide_columns || c.game.side_fill == rom::SideFill::Pillarbox) return 0;
    const auto& polygons = c.machine->render_list().polygons;
    const bool reaches_side = std::any_of(polygons.begin(), polygons.end(), [&](const auto& polygon) {
        for (unsigned v = 0; v < polygon.num_vertices; ++v)
            if (polygon.v[v].x < c.wide_columns || polygon.v[v].x > c.wide_columns + width)
                return true;
        return false;
    });
    return reaches_side ? 1u + static_cast<unsigned>(c.game.side_fill) : 0;
}

double elapsed_ms(std::chrono::steady_clock::time_point begin,
                  std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

void clear_timing_overlay(Content& c)
{
    c.timing_overlay_data = {};
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
    c.machine->sound_board().set_output_low_pass_enabled(
        libretro::scsp_output_low_pass_enabled());
    c.machine->sound_board().set_music_volume_percent(
        libretro::music_volume_percent());
    c.audio.clear();
    c.output_dc.reset();
    c.have_previous_run_start = false;
    reset_timing_measurements(c);
    update_aspect_ratio(c);
}

void finish_software_frame(Content& c, const libretro::CrosshairState& crosshairs,
                           const libretro::TimingOverlayData& timing, double fps)
{
    // Upstream packs R in bits 0..7. Libretro XRGB8888 packs R in 16..23.
    for (auto& pixel : c.frame)
        pixel = ((pixel & 0xffu) << 16) | (pixel & 0xff00u) | ((pixel >> 16) & 0xffu);
    libretro::draw_crosshairs(c.frame, width, height, crosshairs);
    libretro::draw_timing_overlay_software(c.frame, width, height, timing, fps);
}

void reset_async_software_renderer(Content& c)
{
    if (c.async_renderer) c.async_renderer->forget_machine();
    c.async_frame_ready = false;
    std::fill(c.frame.begin(), c.frame.end(), 0);
    std::fill(c.async_frame.begin(), c.async_frame.end(), 0);
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
    c.timing_overlay_data = {
        true,
        true,
        c.timing == libretro::AVTimingMode::Native,
        c.timing_overlay_font_pixels,
        static_cast<float>(machine_ms),
        static_cast<float>(video_ms),
        static_cast<float>(audio_ms),
        static_cast<float>(run_ms),
        static_cast<float>(c.timing_worst_ms),
        static_cast<float>(actual_fps),
        static_cast<float>(engine_ms > 0 ? 1000.0 / engine_ms : 0.0),
        static_cast<float>(run_ms > 0 ? 1000.0 / run_ms : 0.0),
    };
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
        c.reset_pending = true;
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
                    c.reset_pending = true;
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

#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
void context_destroy();
void context_reset();
#endif

void reset_configured_machine(Content& c)
{
    reset_async_software_renderer(c);
    c.rumble.stop();
    c.machine->reset();
    c.machine->sound_board().clear_pending_samples();
    c.audio.clear();
    c.output_dc.reset();
    c.cadence_accumulator = 60.0 - c.native_fps;
    c.input_runtime = {};
    c.crosshair_presentation.reset_activity();
    c.audio_frame_remainder = 0;
    c.audio_frames_due = 0;
    c.have_previous_run_start = false;
    reset_timing_measurements(c);
    c.failed = false;
    c.reset_pending = false;
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
    if (c.renderer_api != RendererApi::Software) {
        context_destroy();
        context_reset();
    }
#endif
}

void publish_option_corrections()
{
    // Move before calling the frontend: display callbacks can re-enter readers.
    const auto corrections = std::exchange(libretro::option_corrections, {});
    const auto warnings = std::exchange(libretro::option_warnings, {});
    if (environment) for (const auto& [key, value] : corrections) {
        retro_variable variable{key.c_str(), value.c_str()};
        environment(RETRO_ENVIRONMENT_SET_VARIABLE, &variable);
    }
    for (const auto& warning : warnings) message(RETRO_LOG_WARN, warning.c_str(), true);
}

void apply_pending_option(Content& c)
{
    bool changed = false;
    if (c.option_pending) {
        const auto result = libretro::nvram::apply(c.nvram_game, c.machine->backup_ram(),
                                                  c.machine->settings_eeprom(), c.nvram_values);
        if (result == libretro::nvram::ApplyResult::LayoutNotReady) return;
        c.option_pending = false;
        changed = result == libretro::nvram::ApplyResult::Changed;
        if (changed) export_frontend_save(c);
        if (result != libretro::nvram::ApplyResult::Unsupported) {
            c.automation_applied = c.automation_requested;
            if (c.automation_requested) c.managed_applied = c.managed_requested;
        }
        const auto updates = std::exchange(c.network_option_updates, {});
        if (environment) for (const auto& [key, value] : updates) {
            retro_variable variable{key.c_str(), value.c_str()};
            environment(RETRO_ENVIRONMENT_SET_VARIABLE, &variable);
        }
    }
    const bool topology_changed = c.reset_pending;
    if (changed || topology_changed) {
        reset_configured_machine(c);
        if (changed && c.first_frame_complete) message(RETRO_LOG_INFO, "Applied NVRAM Settings; machine reset", true);
        else if (!changed && c.first_frame_complete) message(RETRO_LOG_INFO, "Applied Network Settings; machine reset", true);
    }
    publish_option_corrections();
}

void refresh_nvram_options(Content& c)
{
    const auto manual = libretro::selected_nvram_values(c.nvram_game);
    const bool enabled = libretro::nvram_settings_enabled();
    auto selected = enabled ? manual : std::vector<std::string>(manual.size());
    const auto* network = libretro::linked_cabinet_game(c.game.name);
    const std::string preset = libretro::automatic_network_preset(c.game.name);
    c.automation_requested = network && preset != "-1";
    const auto catalog = c.game.name == "vonr" ? std::string_view{"vonr"} : std::string_view{c.nvram_game};
    c.managed_requested = c.automation_requested
        ? libretro::network::managed_values(catalog, c.linked_cabinets, preset)
        : std::vector<libretro::network::ManagedValue>{};
    const auto& managed = c.automation_requested ? c.managed_requested
        : c.automation_applied ? c.managed_applied : c.managed_requested;
    c.network_option_updates.clear();
    const auto options = libretro::nvram::options_for_game(c.nvram_game);
    for (const auto& target : managed) {
        for (size_t index = 0; index < options.size(); ++index) {
            if (target.suffix != options[index].suffix) continue;
            const auto* begin = options[index].values;
            const auto* end = begin + options[index].value_count;
            if (std::find_if(begin, end, [&](const auto& v) { return target.key == v.key; }) == end)
                throw std::runtime_error("ANRS mapping is absent from the reviewed native catalog");
            if (manual[index] != target.key)
                c.network_option_updates.emplace_back("sm2_nvram_" + c.nvram_game + "_" + std::string(target.suffix), target.key);
            if (enabled || c.automation_requested) selected[index] = target.key;
            break;
        }
    }
    c.nvram_values = std::move(selected);
    c.option_pending = !c.nvram_values.empty() && (enabled || !managed.empty());
    c.option_wait_frames = 0;
}

void message(enum retro_log_level level, const char* text, bool notify)
{
    if (log_cb) log_cb(level, "[SM2] %s\n", text);
    else std::fprintf(stderr, "[SM2] %s\n", text);
    if ((notify || level >= RETRO_LOG_WARN) && environment) {
        unsigned version = 0;
        if (environment(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &version)
            && version >= 1) {
            retro_message_ext msg{text, 5000,
                                  level == RETRO_LOG_ERROR ? 2U : 1U,
                                  level, RETRO_MESSAGE_TARGET_OSD,
                                  RETRO_MESSAGE_TYPE_NOTIFICATION, -1};
            if (environment(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg)) return;
        }
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
    }
    message(RETRO_LOG_INFO, "GPU context resources released");
}
void context_reset()
{
    if (!content || content->renderer_api == RendererApi::Software) return;
    try {
        const unsigned previous_scale = content->scale;
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
            content->scale = gpu->init(environment, content->requested_scale, log_cb);
            content->gpu = std::move(gpu);
#else
            throw std::runtime_error("This core was built without Vulkan support");
#endif
        } else {
#ifdef SM2_LIBRETRO_OPENGL
            auto gl = std::make_unique<libretro::OpenGlRenderer>();
            content->scale = gl->init(content->requested_scale, content->renderer_api == RendererApi::OpenGLES, log_cb);
            content->gl = std::move(gl);
#else
            throw std::runtime_error("This core was built without OpenGL support");
#endif
        }
        if (content->scale != previous_scale && environment) {
            retro_game_geometry geometry{
                displayed_width(*content) * content->scale, height * content->scale,
                maximum_width(*content), height * content->requested_scale,
                content->aspect_ratio};
            environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
        }
        // A recreated frontend device has no image to duplicate yet. The next
        // callback must submit one even when 60 Hz pacing repeats the machine frame.
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
        resolution.value[0] >= '1' && resolution.value[0] <= '8' && resolution.value[1] == '\0')
        c.scale = static_cast<unsigned>(resolution.value[0] - '0');
    c.requested_scale = c.scale;
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
    libretro::set_netpacket_offline();
    libretro::timing_overlay_shutdown();
    libretro::set_option_game({});
    libretro::set_audio_sources({});
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
    const bool test_service = libretro::controller_test_service_enabled();
    content->published_test_service = test_service;
    libretro::configure_controllers(content->game, content->controllers, gun_mode, test_service);
    content->input_descriptors = libretro::descriptors(
        content->game, libretro::profile_devices(devices, test_service), gun_mode,
        libretro::offscreen_reload_shortcut_enabled());
    environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, content->controllers.ports.data());
    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, content->input_descriptors.data());
}

bool update_profile_controls()
{
    if (!content || content->published_test_service == libretro::controller_test_service_enabled())
        return false;
    publish_controls();
    return true;
}

void configure_linked_cabinets(Content& c, unsigned cabinets)
{
    const bool sled_local = (c.game.name == "powsledm" || c.game.name == "powsledr")
        && c.power_sled_standalone_requested && cabinets == 1;
    if (sled_local != c.power_sled_standalone) {
        const auto& program = c.power_sled_programs[sled_local ? 1 : 0];
        auto destination = c.machine->program_rom();
        if (program.size() != destination.size())
            throw std::runtime_error("Power Sled topology program is unavailable");
        std::copy(program.begin(), program.end(), destination.begin());
        c.power_sled_standalone = sled_local;
    }
    // Match the standalone emulator: keep the board fitted, using local
    // loopback when no external cabinet link is configured.
    c.machine->comm().set_present(true);
    if (cabinets > 1) {
        auto network = std::make_unique<libretro::NetpacketTransport>();
        network->configure(libretro::linked_cabinet_network_family(c.game.name), cabinets,
                           libretro::power_sled_program_role(c.game.name));
        c.machine->comm().set_transport(std::move(network));
        c.external_link_active = true;
    } else {
        c.machine->comm().set_transport(nullptr);
        libretro::set_netpacket_offline();
        c.external_link_active = false;
    }
    c.linked_cabinets = cabinets;
    c.logged_comm_state = ~u64{0};
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
    const size_t appended = c.audio.size();
    c.audio.insert(c.audio.end(), samples.begin(), samples.end());
    c.output_dc.process(std::span<s16>(c.audio).subspan(appended), c.rate);
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

void collect_fast_audio(Content& c, size_t output_frames)
{
    auto& sound = c.machine->sound_board();
    const auto input = sound.pending_samples();
    if (input.size() % 2 || sound.sample_rate() != c.rate)
        throw std::runtime_error("Invalid stereo audio or unexpected sample-rate change");
    if (c.audio.size() + output_frames * 2 > static_cast<size_t>(c.rate) * 2)
        throw std::runtime_error("Frontend audio stalled for more than one second");

    const size_t source_frames = input.size() / 2;
    const size_t offset = c.audio.size();
    c.audio.resize(offset + output_frames * 2, 0);
    if (source_frames != 0) {
        const size_t denominator = std::max<size_t>(output_frames, 2) - 1;
        for (size_t frame = 0; frame < output_frames; ++frame) {
            const size_t position = frame * (source_frames - 1);
            const size_t left = position / denominator;
            const size_t right = std::min(left + 1, source_frames - 1);
            const auto fraction = static_cast<s64>(position % denominator);
            for (size_t channel = 0; channel < 2; ++channel) {
                const auto a = static_cast<s64>(input[left * 2 + channel]);
                const auto b = static_cast<s64>(input[right * 2 + channel]);
                c.audio[offset + frame * 2 + channel] = static_cast<s16>(
                    (a * (static_cast<s64>(denominator) - fraction) + b * fraction)
                    / static_cast<s64>(denominator));
            }
        }
    }
    c.output_dc.process(std::span<s16>(c.audio).subspan(offset), c.rate);
    sound.clear_pending_samples();
}
}

extern "C" {
void retro_set_environment(retro_environment_t cb)
{
    environment = cb;
    libretro::option_environment = cb;
    libretro::option_updates_pending = false;
    libretro::option_controls_update = cb ? update_profile_controls : nullptr;
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
    (void)libretro::register_netpacket_interface(environment, log_cb);
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
    if (info) *info = {"SM2-Emu", "0.9.51.2", "zip|7z", true, true};
}
void retro_get_system_av_info(retro_system_av_info* info)
{
    const unsigned scale = content ? content->scale : 1;
    const unsigned maximum_scale = content ? content->requested_scale : 1;
    const float aspect_ratio = content ? content->aspect_ratio : 4.0f / 3.0f;
    if (info) *info = {{(content ? displayed_width(*content) : width) * scale, height * scale,
                       content ? maximum_width(*content) : width, height * maximum_scale, aspect_ratio},
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
        const auto system_path = directory(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY) / "sm2-emu";
        const auto db_path = system_path / "games.xml";
        // RetroArch may already return a core- or content-specific save path.
        // Use it directly, as the frontend owns the .srm location. Optional
        // standalone .nv/.eeprom files are imported from the same directory.
        const auto save_path = directory(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY);
        rom::GameDatabase database;
        if (!database.load(db_path.string()))
            throw std::runtime_error(
                "Missing or invalid sm2-emu/games.xml. Download 'SM2-Emu' from "
                "Online Updater > Core System Files Downloader.");
        auto loaded = rom::RomLoader::load(
            database, game->path, {}, libretro::rom_crc_verification_enabled(),
            system_path.string());
        if (!loaded) throw std::runtime_error("ROM loading failed; see missing-file/CRC details in log");
        // The database name becomes a native-save filename and save-container ID,
        // never an arbitrary path.
        if (loaded->game.name.empty() || loaded->game.name == "." || loaded->game.name == ".."
            || loaded->game.name.find_first_of("/\\") != std::string::npos)
            throw std::runtime_error("Invalid ROM set name in database");
        auto next = std::make_unique<Content>();
        next->game = loaded->game;
        next->crosshair_presentation.configure(next->game, system_path / "crosshairs");
        if (next->game.name == "powsledm")
            message(RETRO_LOG_INFO, "Power Sled Master program: Relay-derived");
        next->power_sled_audio.prepare(database, game->path, next->game.name,
            loaded->roms, libretro::power_sled_relay_audio_enabled());
        const auto original_program = loaded->roms.region("maincpu");
        const std::vector<u8> sled_program = (next->game.name == "powsledm" || next->game.name == "powsledr")
            ? std::vector<u8>(original_program.begin(), original_program.end()) : std::vector<u8>{};
        next->power_sled_standalone_requested = libretro::power_sled_standalone_enabled();
        if ((next->game.name == "powsledm" || next->game.name == "powsledr") && next->power_sled_standalone_requested
            && libretro::linked_cabinets(next->game.name) == 1) {
            next->power_sled_standalone = next->game.name == "powsledr"
                ? libretro::prepare_power_sled_relay_attract(loaded->roms)
                : libretro::prepare_power_sled_standalone(loaded->roms);
            message(RETRO_LOG_INFO, next->power_sled_standalone
                ? (next->game.name == "powsledr"
                    ? "Power Sled Standalone: experimental continuous Relay attract"
                    : "Power Sled Standalone: experimental one rider and three computer opponents")
                : "Power Sled Standalone: unsupported program; keeping original behavior");
        }
        next->power_sled_commands_requested = libretro::power_sled_relay_commands_enabled();
        if (next->game.name == "powsledm" && next->power_sled_commands_requested) {
            next->power_sled_commands = libretro::prepare_power_sled_commands(loaded->roms);
            message(RETRO_LOG_INFO, next->power_sled_commands
                ? "Power Sled Relay Commands On Master: experimental native input remap enabled"
                : "Power Sled Relay Commands On Master: unsupported program; keeping original behavior");
        }
        next->power_sled_ai_finish_speed_fix_requested = libretro::power_sled_ai_finish_speed_fix_enabled();
        if (libretro::power_sled_program_role(next->game.name)
            && next->power_sled_ai_finish_speed_fix_requested) {
            const bool applied = libretro::prepare_power_sled_ai_finish_speed_fix(loaded->roms, next->game.name);
            message(RETRO_LOG_INFO, applied
                ? "Power Sled AI Finish Speed Fix: native late-course clamp bypassed"
                : "Power Sled AI Finish Speed Fix: unsupported program; keeping original behavior");
        }
        if (!sled_program.empty()) {
            const auto current = loaded->roms.region("maincpu");
            next->power_sled_programs[next->power_sled_standalone ? 1 : 0].assign(current.begin(), current.end());
            rom::RomSet alternate;
            alternate.add("maincpu", sled_program);
            bool valid = true;
            if (!next->power_sled_standalone)
                valid = next->game.name == "powsledr" ? libretro::prepare_power_sled_relay_attract(alternate)
                    : libretro::prepare_power_sled_standalone(alternate);
            if (next->game.name == "powsledm" && next->power_sled_commands_requested)
                valid &= libretro::prepare_power_sled_commands(alternate);
            if (next->power_sled_ai_finish_speed_fix_requested)
                valid &= libretro::prepare_power_sled_ai_finish_speed_fix(alternate, next->game.name);
            if (!valid) throw std::runtime_error("Cannot prepare the guarded Power Sled topology variant");
            const auto program = alternate.region("maincpu");
            next->power_sled_programs[next->power_sled_standalone ? 0 : 1].assign(program.begin(), program.end());
        }
        next->machine = hw::create_machine(next->game, std::move(loaded->roms));
        if (!next->machine) throw std::runtime_error("Machine initialization failed");
        next->power_sled_audio.attach(*next->machine);
        if (!next->power_sled_audio.notice().empty())
            message(RETRO_LOG_INFO, next->power_sled_audio.notice().c_str());
        next->manxtt_audio.prepare(database, game->path, next->game.name, *next->machine);
        next->machine->set_i960_round_nearest_even(
            libretro::i960_round_nearest_even_enabled());
        next->machine->sound_board().set_audio_balance_enabled(
            libretro::audio_balance_enabled());
        next->machine->sound_board().set_output_low_pass_enabled(
            libretro::scsp_output_low_pass_enabled());
        next->machine->sound_board().set_music_volume_percent(
            libretro::music_volume_percent());
        configure_audio_sources(next->machine->sound_board());
        next->nvram_game = libretro::nvram::catalog_for_game(
            next->game.name, next->game.parent);
        libretro::set_option_game(next->nvram_game, next->game.name);
        const unsigned linked_cabinets = libretro::linked_cabinets(next->game.name);
        configure_linked_cabinets(*next, linked_cabinets);
        if (linked_cabinets > 1) {
            const std::string link_message = "Linked-cabinet communication selected for "
                + next->game.name + "; awaiting a RetroArch Netplay session";
            message(RETRO_LOG_INFO, link_message.c_str());
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
        next->fps = next->timing != libretro::AVTimingMode::Native
            ? 60.0 : next->native_fps;
        next->cadence_accumulator = 60.0 - next->native_fps;
        next->timing_overlay_font_pixels = libretro::timing_overlay_font_pixels();
        next->timing_overlay = next->timing_overlay_font_pixels != 0;
        refresh_nvram_options(*next);
        next->audio.reserve(static_cast<size_t>(next->rate) * 2);
        const bool rumble_ready = next->rumble.init(environment);
        content = std::move(next);
        libretro::timing_overlay_initialize();
        if (libretro::aspect_ratio_mode() == libretro::AspectRatioMode::SixteenNine)
            content->aspect_ratio = 16.0f / 9.0f;
        libretro::set_option_game(content->nvram_game, content->game.name);
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
        select_renderer(*content);
#endif
        update_aspect_ratio(*content);
        if (content->renderer_api == RendererApi::Software
            && libretro::software_renderer_async_enabled()) {
            content->async_renderer =
                std::make_unique<hw::AsyncSoftRenderer>(content->renderer);
        }
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
        const char* renderer = content->async_renderer
            ? "Renderer: Software (asynchronous, one frame behind)"
            : "Renderer: Software (synchronous)";
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
    libretro::timing_overlay_shutdown();
    libretro::set_option_game({});
    libretro::set_audio_sources({});
    return false;
}
void retro_unload_game() { unload(); }
void retro_reset()
{
    if (!content) return;
    try {
        (void)libretro::consume_option_updates();
        configure_linked_cabinets(*content, libretro::linked_cabinets(content->game.name));
        refresh_nvram_options(*content);
        if (!content->save_ram_initialized) initialize_frontend_save(*content);
        content->reset_pending = true;
        apply_pending_option(*content);
        libretro::update_option_visibility();
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
        if (libretro::consume_option_updates()) {
            if (content->game.name == "powsledm"
                && content->power_sled_audio.requested() != libretro::power_sled_relay_audio_enabled())
                message(RETRO_LOG_INFO, "Power Sled Relay Audio changed: reload content to apply");
            if ((content->game.name == "powsledm" || content->game.name == "powsledr")
                && (content->power_sled_standalone_requested != libretro::power_sled_standalone_enabled()
))
                message(RETRO_LOG_INFO, "Power Sled Standalone changed: reload content to apply");
            if (content->game.name == "powsledm"
                && content->power_sled_commands_requested != libretro::power_sled_relay_commands_enabled())
                message(RETRO_LOG_INFO, "Power Sled Relay Commands On Master changed: reload content to apply");
            if (libretro::power_sled_program_role(content->game.name)
                && content->power_sled_ai_finish_speed_fix_requested != libretro::power_sled_ai_finish_speed_fix_enabled())
                message(RETRO_LOG_INFO, "Power Sled AI Finish Speed Fix changed: reload content to apply");
            const unsigned cabinets = libretro::linked_cabinets(content->game.name);
            if (cabinets != content->linked_cabinets) {
                configure_linked_cabinets(*content, cabinets);
                content->reset_pending = true;
            }
            refresh_nvram_options(*content);
            libretro::update_option_visibility();
            const unsigned overlay_font_pixels = libretro::timing_overlay_font_pixels();
            const bool overlay = overlay_font_pixels != 0;
            if (!overlay) clear_timing_overlay(*content);
            if (overlay && !content->timing_overlay) reset_timing_measurements(*content);
            content->timing_overlay = overlay;
            content->timing_overlay_font_pixels = overlay_font_pixels;
            content->timing_overlay_data.enabled = overlay;
            content->timing_overlay_data.font_pixels = overlay_font_pixels;
            if (!libretro::gamepad_rumble_enabled()) content->rumble.stop();
            content->machine->sound_board().set_audio_balance_enabled(
                libretro::audio_balance_enabled());
            content->machine->sound_board().set_output_low_pass_enabled(
                libretro::scsp_output_low_pass_enabled());
            content->machine->sound_board().set_music_volume_percent(
                libretro::music_volume_percent());
            configure_audio_sources(content->machine->sound_board());
            publish_controls();
        }
        if (!content->save_ram_initialized) initialize_frontend_save(*content);
        apply_pending_option(*content);
        content->first_frame_complete = true;
        const auto cabinet_audio = content->manxtt_audio.update(
            *content->machine, libretro::manxtt_cabinet_audio_enabled());
        if (!cabinet_audio.notice.empty()) message(RETRO_LOG_INFO, cabinet_audio.notice.c_str());
        if (cabinet_audio.changed) {
            content->audio.clear();
            content->output_dc.reset();
            configure_audio_sources(content->machine->sound_board());
        }
        update_aspect_ratio(*content);
        if (input_poll_cb) input_poll_cb();
        s16 rumble_steering = 0;
        if (input_state_cb && devices[0] != RETRO_DEVICE_NONE)
            rumble_steering = input_state_cb(0, RETRO_DEVICE_ANALOG,
                RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
        auto analog_options = libretro::special_analog_options();
        analog_options.widescreen_columns = content->wide_columns;
        libretro::poll_input(content->machine->inputs(), content->game,
                             libretro::profile_devices(devices, libretro::controller_test_service_enabled()),
                             content->input_runtime, libretro::four_speed_h_gate(),
                             input_state_cb, libretro::gun_input_mode(),
                             libretro::offscreen_reload_shortcut_enabled(),
                             libretro::driving_analog_options(),
                             libretro::desert_elevation_options(),
                             libretro::automatic_start_gear_enabled(),
                             libretro::mouse_edge_offscreen_reload_enabled(),
                             analog_options, libretro::gear_shift_neutral_ms(),
                             libretro::analog_aim_absolute());
        const auto machine_start = std::chrono::steady_clock::now();
        content->machine->run_frame();
        if (content->external_link_active) {
            const auto& comm = content->machine->comm();
            const auto role = comm.role_inputs();
            const u64 state = (u64(role[0]) << 32) | (u64(role[1]) << 40)
                | (unsigned(comm.enabled()) << 24)
                | (unsigned(comm.link_alive()) << 16)
                | (unsigned(comm.link_id()) << 8) | unsigned(comm.link_count());
            if (state != content->logged_comm_state) {
                content->logged_comm_state = state;
                if (log_cb) log_cb(RETRO_LOG_INFO,
                    "[SM2-Emu] [NetBoard] Native board %s: enabled=%u alive=%u id=%u count=%u fg=%u role=%u\n",
                    content->game.name.c_str(), unsigned(comm.enabled()),
                    unsigned(comm.link_alive()), unsigned(comm.link_id()),
                    unsigned(comm.link_count()), unsigned(role[0]), unsigned(role[1]));
            }
        }
        if (content->game.lightgun.present) {
            content->machine->video().filter_gun_flash(
                libretro::hide_shot_flash_enabled(), content->machine->inputs().in1);
        }
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
        const auto crosshairs = content->crosshair_presentation.update(
            content->game, content->input_runtime, libretro::crosshair_mask(content->game),
            libretro::crosshair_style(), libretro::crosshair_timeout_seconds(),
            1.0 / content->native_fps);
        if (const auto notice = content->crosshair_presentation.take_notice(); !notice.empty())
            message(RETRO_LOG_WARN, notice.c_str(), true);
        const auto video_start = std::chrono::steady_clock::now();
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
        if (content->renderer_api != RendererApi::Software) {
            if (content->renderer_api == RendererApi::Vulkan) {
#ifdef SM2_LIBRETRO_VULKAN
                if (!content->gpu) throw std::runtime_error("Frontend Vulkan context is not ready");
                content->gpu->render(*content->machine, video_cb,
                    crosshairs,
                    content->timing_overlay_data, content->fps,
                    libretro::texture_filter_quality(), libretro::upscale_2d_mode(),
                    content->wide_columns, wide_background(*content));
#else
                throw std::runtime_error("This core was built without Vulkan support");
#endif
            } else {
#ifdef SM2_LIBRETRO_OPENGL
                if (!content->gl) throw std::runtime_error("Frontend OpenGL context is not ready");
                content->gl->render(*content->machine, video_cb,
                    crosshairs,
                    content->timing_overlay_data, content->fps,
                    libretro::texture_filter_quality(), libretro::upscale_2d_mode(),
                    content->wide_columns, wide_background(*content));
#else
                throw std::runtime_error("This core was built without OpenGL support");
#endif
            }
        } else
#endif
        {
            content->machine->compose_video();
            if (content->async_renderer && !content->machine->render_test_mode()) {
                content->async_renderer->wait();
                if (content->async_frame_ready) {
                    content->frame.swap(content->async_frame);
                    finish_software_frame(*content, content->async_crosshairs,
                                          content->async_timing_overlay_data,
                                          content->async_fps);
                }
                content->async_crosshairs = crosshairs;
                content->async_timing_overlay_data = content->timing_overlay_data;
                content->async_fps = content->fps;
                content->async_renderer->begin(*content->machine, content->async_frame);
                content->async_frame_ready = true;
            } else {
                // Render-test mode must display the current framebuffer bank,
                // so it temporarily uses the synchronous path.
                if (content->async_renderer) content->async_renderer->wait();
                content->async_frame_ready = false;
                content->renderer.render(*content->machine,
                                         content->machine->render_list(),
                                         content->frame);
                finish_software_frame(*content, crosshairs,
                                      content->timing_overlay_data, content->fps);
            }
            if (video_cb) {
                video_cb(content->frame.data(), width, height, width * sizeof(u32));
            }
        }
        const auto video_end = std::chrono::steady_clock::now();
        content->timing_video_ms += elapsed_ms(video_start, video_end);
        const auto audio_start = video_end;
        size_t requested_audio = static_cast<size_t>(-1);
        if (content->timing != libretro::AVTimingMode::Native) {
            content->audio_frame_remainder += content->rate;
            requested_audio = static_cast<size_t>(content->audio_frame_remainder / 60);
            content->audio_frame_remainder %= 60;
            collect_fast_audio(*content, requested_audio);
            content->audio_frames_due += requested_audio;
            requested_audio = content->audio_frames_due;
        }
        const size_t accepted_audio = send_audio(*content, requested_audio);
        if (content->timing != libretro::AVTimingMode::Native)
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
        if (content->async_renderer) content->async_renderer->wait();
        std::vector<u8> state;
        if (!content->machine->save_state(state)
            || state.size() > libretro_state_size - frontend_state_size) {
            message(RETRO_LOG_ERROR, "Save-state buffer capacity exceeded");
            return false;
        }
        std::memcpy(data, state.data(), state.size());
        std::memset(static_cast<u8*>(data) + state.size(), 0,
                    libretro_state_size - state.size());
        save_frontend_state(static_cast<u8*>(data) + libretro_state_size
                            - frontend_state_size, *content);
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
    if (content->game.protection == rom::Protection::Sega315_5838_Doa) {
        state::Header header;
        usize offset = 0;
        if (state::read_header(static_cast<const u8*>(data), size, header, offset)
            && header.game == content->game.name && header.version < 14) {
            message(RETRO_LOG_WARN,
                    "Dead or Alive: this Save State predates the protection update and cannot be loaded. Start a new game.", true);
            return false;
        }
    }
    if (content->game.name == "powsledm") {
        // Retain rejection of development states made with the removed Slave
        // donor. Their program addresses differ; reject before any mutation.
        // Untagged legacy and Relay-derived states remain compatible.
        bool slave = false;
        bool relay_audio = false;
        bool standalone = false;
        bool commands = false;
        if (size >= frontend_state_size) {
            const u8* trailer = static_cast<const u8*>(data) + size - frontend_state_size;
            if (std::memcmp(trailer, frontend_state_magic.data(), frontend_state_magic.size()) == 0) {
                const u8* cursor = trailer + 76;  // Existing gun-flash flags.
                const u32 flags = state_read<u32>(cursor);
                slave = (flags & (1u << 6)) != 0;
                relay_audio = (flags & (1u << 7)) != 0;
                standalone = (flags & (1u << 8)) != 0;
                commands = (flags & (1u << 9)) != 0;
            }
        }
        if (slave) {
            message(RETRO_LOG_ERROR, "Power Sled: Slave-derived Master Save State is unsupported");
            return false;
        }
        if (relay_audio != content->power_sled_audio.active()) {
            message(RETRO_LOG_ERROR, "Power Sled Save State requires the same Relay Audio setting");
            return false;
        }
        if (commands != content->power_sled_commands) {
            message(RETRO_LOG_ERROR, "Power Sled Save State requires the same Relay Commands On Master setting");
            return false;
        }
        if (standalone != content->power_sled_standalone) {
            message(RETRO_LOG_ERROR, "Power Sled Save State requires the same Standalone setting");
            return false;
        }
    }
    try {
        reset_async_software_renderer(*content);
        libretro::InputRuntime input_runtime{};
        double cadence_accumulator = 60.0 - content->native_fps;
        u64 audio_frame_remainder = 0;
        size_t audio_frames_due = 0;
        hw::Model2Video::GunFlashState flash{};
        libretro::OutputDcBlocker output_dc{};
        if (size >= frontend_state_size) {
            load_frontend_state(
                static_cast<const u8*>(data) + size - frontend_state_size,
                input_runtime, cadence_accumulator, audio_frame_remainder,
                audio_frames_due, flash, output_dc);
        }
        if (!content->machine->load_state(static_cast<const u8*>(data), size)) {
            if (content->game.protection == rom::Protection::Sega315_5838_Doa)
                message(RETRO_LOG_WARN, "Dead or Alive: Save State is incompatible or damaged.", true);
            return false;
        }
        reset_frontend_after_state_load(*content);
        content->machine->video().set_gun_flash_state(flash);
        content->output_dc = output_dc;
        content->input_runtime = input_runtime;
        content->cadence_accumulator = cadence_accumulator;
        content->audio_frame_remainder = audio_frame_remainder;
        content->audio_frames_due = audio_frames_due;
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
