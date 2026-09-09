// SPDX-License-Identifier: BSD-3-Clause
// Frontend-free validation of the machine, software video and native-rate audio.

#include "hw/machine_factory.h"
#include "hw/model2_debug.h"
#include "hw/model2_softrender.h"
#include "hw/sound_board.h"
#include "rom/game_db.h"
#include "rom/rom_loader.h"

#include <charconv>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace sm2;

struct Options {
    std::string archive;
    std::string game;
    std::string database;
    std::filesystem::path output;
    u32 frames = 0;
    bool help = false;
};

bool parse(int argc, char** argv, Options& out)
{
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            out.help = true;
            return true;
        }
        if (arg == "--frames" || arg == "--game" || arg == "--database"
            || arg == "--output") {
            if (++i == argc) return false;
            const std::string_view value = argv[i];
            if (arg == "--frames") {
                const auto result = std::from_chars(value.data(), value.data() + value.size(),
                                                    out.frames);
                if (result.ec != std::errc{} || result.ptr != value.data() + value.size()
                    || out.frames == 0) return false;
            } else if (arg == "--game") {
                out.game = value;
            } else if (arg == "--database") {
                out.database = value;
            } else {
                out.output = value;
            }
        } else if (arg.starts_with('-') || !out.archive.empty()) {
            return false;
        } else {
            out.archive = arg;
        }
    }
    return !out.archive.empty() && !out.output.empty() && out.frames != 0;
}

bool write_frame(const std::filesystem::path& path, std::span<const u32> frame)
{
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << hw::SoftRenderer::kWidth << ' ' << hw::SoftRenderer::kHeight << "\n255\n";
    for (const u32 pixel : frame) {
        const char rgb[] = {static_cast<char>(pixel), static_cast<char>(pixel >> 8),
                            static_cast<char>(pixel >> 16)};
        out.write(rgb, sizeof(rgb));
    }
    out.close();
    return !out.fail();
}

int run(const Options& options)
{
    rom::GameDatabase database;
    const auto database_path = options.database.empty()
        ? rom::GameDatabase::locate() : std::optional<std::string>(options.database);
    if (!database_path || !database.load(*database_path)) return 1;
    auto loaded = rom::RomLoader::load(database, options.archive, options.game);
    if (!loaded) return 1;
    auto machine = hw::create_machine(loaded->game, std::move(loaded->roms));
    if (!machine) return 1;

    // Each run starts with fresh NVRAM; refuse to overwrite a previous run.
    if (std::filesystem::exists(options.output)) {
        std::fprintf(stderr, "Output directory already exists: %s\n",
                     options.output.string().c_str());
        return 1;
    }
    std::filesystem::create_directories(options.output / "nvram");
    machine->set_nvram_directory((options.output / "nvram").string());
    machine->load_nvram();
    machine->reset(); // Same order as the standalone loader: restore NVRAM, then reset.

    hw::SoftRenderer renderer;
    std::vector<u32> video(static_cast<usize>(hw::SoftRenderer::kWidth)
                           * hw::SoftRenderer::kHeight);
    std::vector<s16> audio;
    auto& sound = machine->sound_board();
    const u32 rate = sound.sample_rate();
    if (rate == 0) {
        std::fprintf(stderr, "Sound board reported a zero sample rate\n");
        return 1;
    }
    for (u32 frame = 0; frame < options.frames; ++frame) {
        // Idle inputs intentionally match upstream --boot-test without --coin-at.
        machine->run_frame();
        const auto samples = sound.pending_samples();
        if (samples.size() % 2 != 0 || sound.sample_rate() != rate) {
            std::fprintf(stderr, "Invalid stereo audio or sample rate changed\n");
            return 1;
        }
        // The shared WAV writer uses a 32-bit RIFF length.
        constexpr usize max_samples = (std::numeric_limits<u32>::max() - 36U) / sizeof(s16);
        if (samples.size() > max_samples - audio.size()) {
            std::fprintf(stderr, "Recording exceeds WAV size limit\n");
            return 1;
        }
        audio.insert(audio.end(), samples.begin(), samples.end());
        sound.clear_pending_samples();

        const auto status = machine->main_cpu_status();
        if (status.faulted) {
            std::fprintf(stderr, "CPU fault at frame %u: %s\n", frame,
                         status.fault_message.c_str());
            return 1;
        }
        machine->compose_video();
        renderer.render(*machine, machine->render_list(), video);
    }
    if (!write_frame(options.output / "software_frame.ppm", video)
        || !hw::write_wav((options.output / "audio.wav").string(), audio, rate)) {
        std::fprintf(stderr, "Could not write video or audio output\n");
        return 1;
    }
    machine->save_nvram();
    const auto status = machine->main_cpu_status();
    std::printf("game              : %s\nframes run        : %llu\nmaster cycles     : %llu\n"
                "cpu state         : %s%s\naudio rate        : %u\naudio frames      : %zu\n",
                loaded->game.name.c_str(), static_cast<unsigned long long>(machine->frames()),
                static_cast<unsigned long long>(machine->cycles()), status.state_string.c_str(),
                status.halted ? " HALTED" : "", rate, audio.size() / 2);
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    Options options;
    const bool valid = parse(argc, argv, options);
    if (!valid || options.help) {
        std::printf("Usage: sm2-headless --frames N --output NEW_DIRECTORY "
                    "[--game SET] [--database games.xml] ROM.zip|ROM.7z\n"
                    "Runs without SDL or GPU; writes software_frame.ppm, audio.wav and nvram/.\n");
        return valid ? 0 : 2;
    }
    try {
        return run(options);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "sm2-headless: %s\n", error.what());
        return 1;
    }
}
