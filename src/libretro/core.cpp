// SPDX-License-Identifier: BSD-3-Clause
// Libretro owns presentation and pacing; the upstream machine owns emulation.
#include "libretro.h"
#include "input.h"
#ifdef SM2_LIBRETRO_VULKAN
#include "gpu.h"
#include "video_options.h"
#endif
#include "hw/machine_factory.h"
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
std::array<unsigned, 2> devices{RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
constexpr unsigned width = hw::SoftRenderer::kWidth;
constexpr unsigned height = hw::SoftRenderer::kHeight;

struct Content {
    rom::GameSpec game;
    std::unique_ptr<hw::Model2MachineBase> machine;
    hw::SoftRenderer renderer;
    std::vector<u32> frame = std::vector<u32>(width * height);
    std::vector<s16> audio;
    std::vector<retro_input_descriptor> input_descriptors;
    double fps = 0;
    u32 rate = 0;
    bool failed = false;
    bool hardware = false;
    unsigned scale = 1;
#ifdef SM2_LIBRETRO_VULKAN
    std::unique_ptr<libretro::VulkanRenderer> gpu;
#endif
};
std::unique_ptr<Content> content;

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
    if (content) content->failed = true;
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
#ifdef SM2_LIBRETRO_VULKAN
void context_destroy()
{
    if (content) content->gpu.reset();
    message(RETRO_LOG_INFO, "GPU context resources released");
}
void context_reset()
{
    if (!content || !content->hardware) return;
    try {
        content->gpu.reset();
        auto gpu = std::make_unique<libretro::VulkanRenderer>();
        gpu->init(environment, content->scale, log_cb);
        content->gpu = std::move(gpu);
    } catch (const std::exception& e) { failure(e.what()); }
    catch (...) { failure("Unexpected GPU context failure"); }
}
void select_renderer(Content& c)
{
    retro_variable renderer{"sm2_renderer", nullptr};
    environment(RETRO_ENVIRONMENT_GET_VARIABLE, &renderer);
    const std::string value = renderer.value ? renderer.value : "auto";
    unsigned preferred = RETRO_HW_CONTEXT_NONE;
    if (value == "auto") environment(RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER, &preferred);
    c.hardware = value == "vulkan" || (value == "auto" && preferred == RETRO_HW_CONTEXT_VULKAN);
    if (!c.hardware) return;
    retro_variable resolution{"sm2_internal_resolution", nullptr};
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &resolution) && resolution.value &&
        resolution.value[0] >= '1' && resolution.value[0] <= '4' && resolution.value[1] == '\0')
        c.scale = static_cast<unsigned>(resolution.value[0] - '0');
    if (!libretro::request_vulkan(environment, context_reset, context_destroy, log_cb))
        throw std::runtime_error("Vulkan context negotiation v2 unavailable; select Software and reload content.");
}
#endif
void unload()
{
    if (content) {
        try { content->machine->save_nvram(); }
        catch (const std::exception& error) { message(RETRO_LOG_ERROR, error.what()); }
        content.reset();
    }
    if (environment) {
        static const retro_input_descriptor empty[] = {{}};
        environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(empty));
    }
}
void send_audio(Content& c)
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
    const size_t frames = c.audio.size() / 2;
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
}
}

extern "C" {
void retro_set_environment(retro_environment_t cb)
{
    environment = cb;
    if (cb) {
        bool no_game = false;
        cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
#ifdef SM2_LIBRETRO_VULKAN
        libretro::register_video_options(cb);
#endif
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
    devices.fill(RETRO_DEVICE_JOYPAD);
}
void retro_deinit() { unload(); log_cb = nullptr; }
void retro_get_system_info(retro_system_info* info)
{
    if (info) *info = {"SM2-Emu", "0.9.4-libretro-dev", "zip|7z", true, true};
}
void retro_get_system_av_info(retro_system_av_info* info)
{
    const unsigned scale = content ? content->scale : 1;
    if (info) *info = {{width * scale, height * scale, width * scale, height * scale, 4.0f / 3.0f},
                      {content ? content->fps : board_fps<hw::Model2>(),
                       content ? static_cast<double>(content->rate) : 44100.0}};
}
void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port < devices.size()) devices[port] = device & RETRO_DEVICE_MASK;
}
bool retro_load_game(const retro_game_info* game)
{
    unload();
    try {
        if (!game || !game->path || !*game->path) throw std::runtime_error("A ROM archive path is required");
        enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
        if (!environment || !environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format))
            throw std::runtime_error("Frontend does not support XRGB8888 video");
        const auto db_path = directory(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY) / "sm2-emu" / "games.xml";
        const auto save_root = directory(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) / "sm2-emu";
        rom::GameDatabase database;
        if (!database.load(db_path.string()))
            throw std::runtime_error("Cannot load system/sm2-emu/games.xml");
        auto loaded = rom::RomLoader::load(database, game->path);
        if (!loaded) throw std::runtime_error("ROM loading failed; see missing-file/CRC details in log");
        // The database name becomes a directory component, never an arbitrary path.
        if (loaded->game.name.empty() || loaded->game.name == "." || loaded->game.name == ".."
            || loaded->game.name.find_first_of("/\\") != std::string::npos)
            throw std::runtime_error("Invalid ROM set name in database");
        auto next = std::make_unique<Content>();
        next->game = loaded->game;
        next->machine = hw::create_machine(next->game, std::move(loaded->roms));
        if (!next->machine) throw std::runtime_error("Machine initialization failed");
        const auto save_path = save_root / next->game.name;
        std::filesystem::create_directories(save_path);
        next->machine->set_nvram_directory(save_path.string());
        next->machine->load_nvram();
        next->machine->reset();
        next->rate = next->machine->sound_board().sample_rate();
        if (!next->rate) throw std::runtime_error("Sound board reported zero sample rate");
        next->fps = game_fps(next->game.board);
        next->input_descriptors = libretro::descriptors(next->game);
        next->audio.reserve(static_cast<size_t>(next->rate) * 2);
        content = std::move(next);
#ifdef SM2_LIBRETRO_VULKAN
        select_renderer(*content);
#endif
        environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, content->input_descriptors.data());
        if (!libretro::digital_profile(content->game))
            message(RETRO_LOG_WARN, "This cabinet's gameplay controls are not implemented yet; Coin/Start/Test/Service only.");
        char report[256];
        std::snprintf(report, sizeof(report), "Loaded %s: %ux%u, %.9f Hz, %u Hz stereo",
                      content->game.name.c_str(), width, height, content->fps, content->rate);
        message(RETRO_LOG_INFO, report);
        message(RETRO_LOG_INFO, content->hardware ? "Renderer: Vulkan (awaiting frontend context)" : "Renderer: Software");
        return true;
    } catch (const std::exception& error) { message(RETRO_LOG_ERROR, error.what()); }
    catch (...) { message(RETRO_LOG_ERROR, "Unexpected error while loading content"); }
    content.reset();
    return false;
}
void retro_unload_game() { unload(); }
void retro_reset()
{
    if (!content) return;
    try {
        content->machine->reset();
        content->machine->sound_board().clear_pending_samples();
        content->audio.clear();
        content->failed = false;
#ifdef SM2_LIBRETRO_VULKAN
        // A machine reset can reuse generation numbers: discard GPU caches.
        if (content->hardware && content->gpu) context_reset();
#endif
    } catch (const std::exception& error) { failure(error.what()); }
    catch (...) { failure("Unexpected reset failure"); }
}
void retro_run()
{
    if (!content || content->failed) return;
    try {
        if (input_poll_cb) input_poll_cb();
        libretro::poll_input(content->machine->inputs(), content->game, devices, input_state_cb);
        content->machine->run_frame();
        const auto status = content->machine->main_cpu_status();
        if (status.faulted) throw std::runtime_error(status.fault_message);
#ifdef SM2_LIBRETRO_VULKAN
        if (content->hardware) {
            if (!content->gpu) throw std::runtime_error("Frontend Vulkan context is not ready");
            content->gpu->render(*content->machine, video_cb);
        } else
#endif
        {
            content->machine->compose_video();
            content->renderer.render(*content->machine, content->machine->render_list(), content->frame);
            // Upstream packs R in bits 0..7. Libretro XRGB8888 packs R in 16..23.
            for (auto& pixel : content->frame)
                pixel = ((pixel & 0xffu) << 16) | (pixel & 0xff00u) | ((pixel >> 16) & 0xffu);
            if (video_cb) video_cb(content->frame.data(), width, height, width * sizeof(u32));
        }
        send_audio(*content);
    } catch (const std::exception& error) { failure(error.what()); }
    catch (...) { failure("Unexpected frame execution failure"); }
}
size_t retro_serialize_size() { return 0; }
bool retro_serialize(void*, size_t) { return false; }
bool retro_unserialize(const void*, size_t) { return false; }
void retro_cheat_reset() {}
void retro_cheat_set(unsigned, bool, const char*) {}
bool retro_load_game_special(unsigned, const retro_game_info*, size_t) { return false; }
unsigned retro_get_region() { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned) { return nullptr; }
size_t retro_get_memory_size(unsigned) { return 0; }
}
