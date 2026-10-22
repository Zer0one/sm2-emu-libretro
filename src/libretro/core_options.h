// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "crosshair.h"
#include "libretro.h"
#include "input.h"
#include "nvram_settings.h"
#include "automatic_network.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace sm2::libretro {
enum class AVTimingMode { Native, FullFrames60Hz };
enum class AspectRatioMode { Automatic, FourThree, SixteenNine };

inline retro_environment_t option_environment = nullptr;
inline bool (*option_controls_update)() = nullptr;
inline bool option_updates_pending = false;
inline std::vector<std::string> option_warnings;
inline std::vector<std::pair<std::string, std::string>> reported_invalid_network_values;
inline std::vector<std::pair<std::string, std::string>> option_corrections;

inline void retain_option_updates()
{
    bool updated = false;
    if (option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated))
        option_updates_pending |= updated;
}
inline bool option_variable(retro_variable& variable)
{
    retain_option_updates();
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable);
}
inline bool consume_option_updates()
{
    retain_option_updates();
    return std::exchange(option_updates_pending, false);
}
inline void invalid_network_option(const std::string& key, const std::string& value,
                                   const std::string& fallback)
{
    const std::pair invalid{key, value};
    if (std::find(reported_invalid_network_values.begin(), reported_invalid_network_values.end(), invalid) != reported_invalid_network_values.end()) return;
    reported_invalid_network_values.push_back(invalid);
    const std::string warning = "Invalid " + key + " value '" + value + "'; using " + fallback;
    if (std::find(option_warnings.begin(), option_warnings.end(), warning) == option_warnings.end())
        option_warnings.push_back(warning);
    if (std::find(option_corrections.begin(), option_corrections.end(), std::pair{key, fallback}) == option_corrections.end())
        option_corrections.emplace_back(key, fallback);
}
inline std::string option_game;
inline std::string option_content_game;
inline std::array<bool, 5> option_audio_sources{};
inline constexpr std::array<const char*, 5> audio_source_keys{{
    "sm2_scsp_gain", "sm2_dsb_gain", "sm2_multipcm1_gain",
    "sm2_multipcm2_gain", "sm2_ym3438_gain"}};
inline bool nvram_master_visible = false;
inline bool nvram_game_visible = false;
inline std::string network_visibility;
inline std::vector<retro_core_option_v2_definition> registered_definitions;
inline std::vector<retro_core_option_v2_definition> compatible_definitions;
inline std::deque<std::string> option_key_storage;
inline std::deque<std::string> option_label_storage;
inline std::deque<std::string> option_info_storage;
inline std::deque<std::string> option_value_label_storage;
inline std::deque<std::string> default_label_storage;
inline std::deque<std::string> legacy_value_storage;
inline std::vector<retro_variable> legacy_definitions;

struct LinkedCabinetGame {
    std::string_view name;
    std::string_view nvram_game;
    unsigned max_cabinets;
};

inline constexpr std::array<LinkedCabinetGame, 32> linked_cabinet_games{{
    {"daytona", "daytona", 8}, {"daytonagtx", "daytonagtx", 8},
    {"daytonas", "daytonas", 8}, {"daytonase", "daytona", 8},
    {"daytonam", "daytona", 8},
    {"stcc", "stcc", 9}, {"stcca", "stcca", 9}, {"stccb", "stccb", 9},
    {"stcco", "stcco", 9},
    {"srallyc", "srallyc", 5}, {"srallycb", "srallyc", 5},
    {"srallycc", "srallyc", 5},
    {"indy500", "indy500", 8}, {"indy500d", "indy500d", 8},
    {"indy500to", "indy500", 8},
    {"motoraid", "motoraid", 5}, {"motoraiddx", "motoraiddx", 5},
    {"waverunr", "waverunr", 4}, {"skisuprg", "skisuprg", 4},
    {"sgt24h", "sgt24h", 4},
    {"overrev", "overrev", 4}, {"overrevb", "overrev", 4},
    {"overrevba", "overrev", 4},
    {"manxtt", "manxtt", 3}, {"manxttc", "manxtt", 3},
    {"von", "von", 3}, {"vonj", "von", 3}, {"vonu", "von", 3},
    {"vonr", "von", 3},
    {"powsledm", "powsledm", 5}, {"powsledr", "powsledr", 5},
    {"powsled", "powsled", 5},
}};

inline constexpr std::array<retro_core_option_value, 9> linked_cabinet_values{{
    {"-1", "1 Cabinet / Disabled"}, {"2", "2 Cabinets"}, {"3", "3 Cabinets"},
    {"4", "4 Cabinets"}, {"5", "5 Cabinets"}, {"6", "6 Cabinets"},
    {"7", "7 Cabinets"}, {"8", "8 Cabinets"}, {"9", "9 Cabinets"},
}};

inline const LinkedCabinetGame* linked_cabinet_game(std::string_view game)
{
    const auto found = std::find_if(linked_cabinet_games.begin(), linked_cabinet_games.end(),
        [game](const auto& candidate) { return candidate.name == game; });
    return found == linked_cabinet_games.end() ? nullptr : &*found;
}

inline unsigned power_sled_program_role(std::string_view game)
{
    return game == "powsledm" ? 1u : game == "powsledr" ? 2u
         : game == "powsled" ? 3u : 0u;
}

inline std::string_view linked_cabinet_network_family(std::string_view game)
{
    if (power_sled_program_role(game)) return "powsled";
    return game == "von" || game == "vonj" || game == "vonu" || game == "vonr"
        ? std::string_view{"von"} : game;
}

inline constexpr const char* linked_cabinets_help =
    "Select the total number of linked cabinets, including a Relay where supported. "
    "1 Cabinet / Disabled disconnects optional communication boards or keeps required boards in local loopback; "
    "it does not change the selected native role. Supported totals and role combinations depend on the game. "
    "Effective changes reset automatically when returning to the game.";

inline std::string retroarch_option_label(std::string_view label)
{
    static constexpr std::string_view acronyms[] = {
        "BGM", "CRT", "DX", "EUR", "EXP", "GT", "ID", "JPN", "NG", "OK", "PK", "SD",
        "SP", "TT", "URL", "US", "USA", "VJCOM", "VJMAN", "VS",
    };
    std::string result;
    result.reserve(label.size() + 2);
    for (size_t i = 0; i < label.size();) {
        const unsigned char current = static_cast<unsigned char>(label[i]);
        if (!std::isalnum(current)) {
            if (label[i] == '(' && !result.empty() && result.back() != ' ')
                result.push_back(' ');
            result.push_back(label[i++]);
            continue;
        }
        size_t end = i;
        bool has_digit = false;
        while (end < label.size() && std::isalnum(static_cast<unsigned char>(label[end]))) {
            has_digit |= std::isdigit(static_cast<unsigned char>(label[end])) != 0;
            ++end;
        }
        const std::string_view word = label.substr(i, end - i);
        const bool acronym = has_digit || std::find(std::begin(acronyms), std::end(acronyms), word)
                                           != std::end(acronyms);
        for (size_t offset = 0; offset < word.size(); ++offset) {
            const unsigned char character = static_cast<unsigned char>(word[offset]);
            result.push_back(static_cast<char>(acronym ? character
                : offset == 0 ? std::toupper(character) : std::tolower(character)));
        }
        i = end;
    }
    return result;
}

inline std::string retroarch_option_info(const nvram::Option& option,
                                         std::string_view display_label)
{
    std::string result = option.description;
    const auto replace_all = [&](std::string_view from, std::string_view to) {
        for (size_t position = 0; (position = result.find(from, position)) != std::string::npos;
             position += to.size())
            result.replace(position, from.size(), to);
    };
    replace_all(option.label, display_label);
    replace_all(std::string(option.game) + "'s", "the game's");
    return result;
}

inline void build_option_definitions()
{
    if (!registered_definitions.empty()) return;
    const auto all = nvram::all_options();
    registered_definitions.reserve(all.size() + 65);

    retro_core_option_v2_definition rom_crc{};
    rom_crc.key = "sm2_rom_crc_verification";
    rom_crc.desc = "ROM CRC Verification (Restart Required)";
    rom_crc.info = "Verify ROM-chip contents against their declared CRC32 checksums.";
    rom_crc.category_key = "system";
    rom_crc.values[0] = {"enabled", "Enabled"};
    rom_crc.values[1] = {"disabled", "Disabled"};
    rom_crc.default_value = "enabled";
    registered_definitions.push_back(rom_crc);

    retro_core_option_v2_definition i960_rounding{};
    i960_rounding.key = "sm2_i960_round_nearest_even";
    i960_rounding.desc = "i960 Round-to-Nearest-Even Fix (Restart Required)";
    i960_rounding.info = "Correct i960 floating-point rounding to use round-to-nearest-even for exact half values.";
    i960_rounding.category_key = "system";
    i960_rounding.values[0] = {"disabled", "Disabled"};
    i960_rounding.values[1] = {"enabled", "Enabled"};
    i960_rounding.default_value = "disabled";
    registered_definitions.push_back(i960_rounding);

    retro_core_option_v2_definition initial_nvram{};
    initial_nvram.key = "sm2_initial_nvram_setup";
    initial_nvram.desc = "Automatic Initial NVRAM Setup (Restart Required)";
    initial_nvram.info = "Initialize missing save data from a validated NVRAM template and apply approved initial settings while preserving existing valid saves.";
    initial_nvram.category_key = "system";
    initial_nvram.values[0] = {"enabled", "Enabled"};
    initial_nvram.values[1] = {"disabled", "Disabled"};
    initial_nvram.default_value = "enabled";
    registered_definitions.push_back(initial_nvram);

    // Only the matching per-set pair (or unsupported pair) is shown.
    for (const auto& game : linked_cabinet_games) {
        option_key_storage.emplace_back("sm2_linked_cabinets_" + std::string(game.name));
        retro_core_option_v2_definition linked{};
        linked.key = option_key_storage.back().c_str();
        linked.desc = "Linked Cabinets";
        linked.info = linked_cabinets_help;
        linked.category_key = "system";
        if (game.name == "vonr") {
            linked.values[0] = linked_cabinet_values[0];
            linked.values[1] = linked_cabinet_values[2];
        } else std::copy_n(linked_cabinet_values.begin(), game.max_cabinets, linked.values);
        linked.default_value = "-1";
        registered_definitions.push_back(linked);

        const bool relay_program = game.name == "vonr" || game.name == "powsledr";
        const std::string_view catalog = relay_program ? game.name : game.nvram_game;
        option_key_storage.emplace_back("sm2_automatic_network_" + std::string(game.name));
        retro_core_option_v2_definition automatic{};
        automatic.key = option_key_storage.back().c_str();
        automatic.desc = "Auto Network Role Settings";
        automatic.info = "Apply the native role and game identity independently of Linked Cabinets, even with NVRAM Settings disabled. Disabled restores manual management. Managed native fields are hidden; unrelated settings are preserved. Effective changes reset automatically when returning to the game.";
        automatic.category_key = "system";
        const auto choices = network::choices(catalog, game.max_cabinets);
        for (size_t index = 0; index < choices.size(); ++index) {
            option_key_storage.push_back(choices[index].key);
            option_value_label_storage.push_back(choices[index].label);
            automatic.values[index] = {option_key_storage.back().c_str(), option_value_label_storage.back().c_str()};
        }
        automatic.default_value = network::default_preset(catalog).data();
        registered_definitions.push_back(automatic);
    }
    for (const auto* key : {"sm2_linked_cabinets_unsupported", "sm2_automatic_network_unsupported"}) {
        retro_core_option_v2_definition unsupported{};
        unsupported.key = key;
        unsupported.desc = std::strcmp(key, "sm2_linked_cabinets_unsupported") == 0
            ? "Linked Cabinets" : "Auto Network Role Settings";
        unsupported.info = "Networking is not supported for this game. This setting has no effect.";
        unsupported.category_key = "system";
        unsupported.values[0] = {"unsupported", "Not Supported"};
        unsupported.default_value = "unsupported";
        registered_definitions.push_back(unsupported);
    }

    retro_core_option_v2_definition sled_standalone{};
    sled_standalone.key = "sm2_power_sled_standalone";
    sled_standalone.desc = "Power Sled: Standalone Mode (Restart Required)";
    sled_standalone.info = "Run the master cabinet alone with native computer opponents, or run the relay alone in attract mode.";
    sled_standalone.category_key = "system";
    sled_standalone.values[0] = {"disabled", "Disabled"};
    sled_standalone.values[1] = {"enabled", "Enabled"};
    sled_standalone.default_value = "enabled";
    registered_definitions.push_back(sled_standalone);

    retro_core_option_v2_definition sled_audio{};
    sled_audio.key = "sm2_power_sled_relay_audio";
    sled_audio.desc = "Power Sled: Relay Audio On Master (Restart Required)";
    sled_audio.info = "Run relay music on an additional sound board within the master cabinet while preserving rider effects.";
    sled_audio.category_key = "system";
    sled_audio.values[0] = {"disabled", "Disabled"};
    sled_audio.values[1] = {"enabled", "Enabled"};
    sled_audio.default_value = "enabled";
    registered_definitions.push_back(sled_audio);

    retro_core_option_v2_definition sled_commands{};
    sled_commands.key = "sm2_power_sled_relay_commands";
    sled_commands.desc = "Power Sled: Relay Commands On Master (Restart Required)";
    sled_commands.info = "Provide relay-style mode selection, race-start and emergency-stop commands from the master cabinet.";
    sled_commands.category_key = "system";
    sled_commands.values[0] = {"disabled", "Disabled"};
    sled_commands.values[1] = {"enabled", "Enabled"};
    sled_commands.default_value = "enabled";
    registered_definitions.push_back(sled_commands);

    retro_core_option_v2_definition sled_speed{};
    sled_speed.key = "sm2_power_sled_ai_finish_speed_fix";
    sled_speed.desc = "Power Sled: AI Finish Speed Fix (Restart Required)";
    sled_speed.info = "Bypass the native computer-opponent speed limit near the course finish.";
    sled_speed.category_key = "system";
    sled_speed.values[0] = {"disabled", "Disabled"};
    sled_speed.values[1] = {"enabled", "Enabled"};
    sled_speed.default_value = "enabled";
    registered_definitions.push_back(sled_speed);

    retro_core_option_v2_definition master{};
    master.key = "sm2_nvram_settings";
    master.desc = "NVRAM Settings";
    master.info = "Apply reviewed operator overrides to saved NVRAM while preserving unrelated fields and calibration.";
    master.category_key = "system";
    master.values[0] = {"disabled", "Disabled"};
    master.values[1] = {"enabled", "Enabled"};
    master.default_value = "disabled";
    registered_definitions.push_back(master);

    for (const auto& option : all) {
        option_key_storage.emplace_back("sm2_nvram_" + std::string(option.game) + "_" + option.suffix);
        option_label_storage.emplace_back(retroarch_option_label(option.label));
        option_info_storage.emplace_back(retroarch_option_info(option, option_label_storage.back()));
        retro_core_option_v2_definition definition{};
        definition.key = option_key_storage.back().c_str();
        definition.desc = option_label_storage.back().c_str();
        definition.info = option_info_storage.back().c_str();
        definition.category_key = "system";
        const size_t count = std::min(option.value_count, std::size(definition.values) - 1);
        for (size_t value = 0; value < count; ++value) {
            option_value_label_storage.emplace_back(
                retroarch_option_label(option.values[value].label));
            const char* label = option_value_label_storage.back().c_str();
            if (std::strcmp(option.values[value].key, option.default_value) == 0) {
                default_label_storage.emplace_back(std::string(label) + " (Default)");
                label = default_label_storage.back().c_str();
            }
            definition.values[value] = {option.values[value].key, label};
        }
        definition.default_value = option.default_value;
        registered_definitions.push_back(definition);
    }

#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
    retro_core_option_v2_definition renderer{};
    renderer.key = "sm2_renderer";
    renderer.desc = "Renderer (Restart Required)";
    renderer.info = "Select hardware-accelerated or software rendering.";
    renderer.category_key = "video";
    size_t renderer_value = 0;
    renderer.values[renderer_value++] = {"auto", "Auto"};
#ifdef SM2_LIBRETRO_VULKAN
    renderer.values[renderer_value++] = {"vulkan", "Vulkan"};
#endif
#ifdef SM2_LIBRETRO_OPENGL
    renderer.values[renderer_value++] = {"opengl", "OpenGL"};
#endif
    renderer.values[renderer_value++] = {"software", "Software"};
    renderer.default_value = "auto";
    registered_definitions.push_back(renderer);

    retro_core_option_v2_definition resolution{};
    resolution.key = "sm2_internal_resolution";
    resolution.desc = "Internal Resolution (Restart Required)";
    resolution.info = "Select the internal rendering resolution to balance image quality and GPU load.";
    resolution.category_key = "video";
    resolution.values[0] = {"1", "1x (496 x 384)"};
    resolution.values[1] = {"2", "2x (992 x 768)"};
    resolution.values[2] = {"3", "3x (1488 x 1152)"};
    resolution.values[3] = {"4", "4x (1984 x 1536)"};
    resolution.values[4] = {"5", "5x (2480 x 1920)"};
    resolution.values[5] = {"6", "6x (2976 x 2304)"};
    resolution.values[6] = {"7", "7x (3472 x 2688)"};
    resolution.values[7] = {"8", "8x (3968 x 3072)"};
    resolution.default_value = "1";
    registered_definitions.push_back(resolution);

    retro_core_option_v2_definition texture_filter{};
    texture_filter.key = "sm2_texture_filter";
    texture_filter.desc = "3D Texture Filtering";
    texture_filter.info = "Select the texture-sampling filter used by the 3D renderer. Applies only to Vulkan / OpenGL renderers.";
    texture_filter.category_key = "video";
    texture_filter.values[0] = {"faithful", "Faithful"};
    texture_filter.values[1] = {"2", "Anisotropic 2x"};
    texture_filter.values[2] = {"4", "Anisotropic 4x"};
    texture_filter.values[3] = {"8", "Anisotropic 8x"};
    texture_filter.values[4] = {"16", "Anisotropic 16x"};
    texture_filter.default_value = "faithful";
    registered_definitions.push_back(texture_filter);

    retro_core_option_v2_definition upscale_2d{};
    upscale_2d.key = "sm2_upscale_2d";
    upscale_2d.desc = "2D Layer Upscaling Filter";
    upscale_2d.info = "Select the upscaling filter applied to 2D tile layers before compositing with 3D.";
    upscale_2d.category_key = "video";
    upscale_2d.values[0] = {"faithful", "Faithful"};
    upscale_2d.values[1] = {"xbr", "xBR"};
    upscale_2d.values[2] = {"scalefx", "ScaleFX"};
    upscale_2d.default_value = "faithful";
    registered_definitions.push_back(upscale_2d);
#endif

    retro_core_option_v2_definition software_threading{};
    software_threading.key = "sm2_software_renderer_threading";
    software_threading.desc = "Software Renderer Threading (Restart Required)";
    software_threading.info = "Select synchronous software rendering or render frames asynchronously on a separate thread.";
    software_threading.category_key = "video";
    software_threading.values[0] = {"asynchronous", "Asynchronous"};
    software_threading.values[1] = {"synchronous", "Synchronous"};
    software_threading.default_value = "asynchronous";
    registered_definitions.push_back(software_threading);

    retro_core_option_v2_definition aspect_ratio{};
    aspect_ratio.key = "sm2_aspect_ratio";
    aspect_ratio.desc = "Aspect Ratio";
    aspect_ratio.info = "Select the displayed aspect ratio or derive it automatically from the native monitor or cabinet configuration.";
    aspect_ratio.category_key = "video";
    aspect_ratio.values[0] = {"auto", "Auto"};
    aspect_ratio.values[1] = {"4_3", "4:3"};
    aspect_ratio.values[2] = {"16_9", "16:9"};
    aspect_ratio.default_value = "auto";
    registered_definitions.push_back(aspect_ratio);

    retro_core_option_v2_definition widescreen{};
    widescreen.key = "sm2_widescreen_mode";
    widescreen.desc = "Widescreen Mode";
    widescreen.info = "Expand the 3D field of view and report 16:9 automatically, with experimental background side fill. Requires hardware rendering and Aspect Ratio Auto or 16:9; 4:3 suppresses expansion. Disabled preserves Aspect Ratio and native monitor settings. Native widescreen is unchanged. Applies on return to game without a reset.";
    widescreen.category_key = "video";
    widescreen.values[0] = {"stretch", "Disabled"};
    widescreen.values[1] = {"expand_3d", "Expand 3D View Only"};
    widescreen.default_value = "stretch";
    registered_definitions.push_back(widescreen);

    retro_core_option_v2_definition timing{};
    timing.key = "sm2_av_timing";
    timing.desc = "A/V Timing (Restart Required)";
    timing.info = "Select the video cadence and corresponding audio timing.";
    timing.category_key = "video";
    timing.values[0] = {"60hz", "60 Hz (Default)"};
    timing.values[1] = {"native", "57.524 Hz (Native Model 2)"};
    timing.default_value = "60hz";
    registered_definitions.push_back(timing);

    retro_core_option_v2_definition overlay{};
    overlay.key = "sm2_timing_overlay";
    overlay.desc = "Timing / FPS Overlay";
    overlay.info = "Display timing and frame-rate measurements to inspect emulation and rendering performance.";
    overlay.category_key = "video";
    overlay.values[0] = {"disabled", "Off"};
    overlay.values[1] = {"auto", "Auto"};
    overlay.values[2] = {"11", "On (11 px)"};
    overlay.values[3] = {"12", "On (12 px)"};
    overlay.values[4] = {"13", "On (13 px)"};
    overlay.values[5] = {"14", "On (14 px)"};
    overlay.default_value = "disabled";
    registered_definitions.push_back(overlay);

    retro_core_option_v2_definition audio_balance{};
    audio_balance.key = "sm2_audio_balance";
    audio_balance.desc = "Enhanced Audio Balance";
    audio_balance.info = "Apply the native audio-balance profile to sources using automatic gain.";
    audio_balance.category_key = "audio";
    audio_balance.values[0] = {"enabled", "Enabled"};
    audio_balance.values[1] = {"disabled", "Disabled"};
    audio_balance.default_value = "enabled";
    registered_definitions.push_back(audio_balance);

    retro_core_option_v2_definition manxtt_audio{};
    manxtt_audio.key = "sm2_manxtt_cabinet_audio";
    manxtt_audio.desc = "Manx TT: Cabinet Audio";
    manxtt_audio.info = "Select cabinet-appropriate sound ROMs and the additional sound board based on the selected cabinet configuration.";
    manxtt_audio.category_key = "audio";
    manxtt_audio.values[0] = {"enabled", "Enabled"};
    manxtt_audio.values[1] = {"disabled", "Disabled"};
    manxtt_audio.default_value = "enabled";
    registered_definitions.push_back(manxtt_audio);

    retro_core_option_v2_definition scsp_low_pass{};
    scsp_low_pass.key = "sm2_scsp_output_low_pass_filter";
    scsp_low_pass.desc = "SCSP Output Low-Pass Filter";
    scsp_low_pass.info = "Apply a low-pass filter to SCSP audio output.";
    scsp_low_pass.category_key = "audio";
    scsp_low_pass.values[0] = {"disabled", "Disabled"};
    scsp_low_pass.values[1] = {"enabled", "Enabled"};
    scsp_low_pass.default_value = "enabled";
    registered_definitions.push_back(scsp_low_pass);

    retro_core_option_v2_definition music_volume{};
    music_volume.key = "sm2_music_volume";
    music_volume.desc = "Music Volume";
    music_volume.info = "Adjust the output level of the separate MPEG music board.";
    music_volume.category_key = "audio";
    music_volume.values[0] = {"0", "0%"};
    music_volume.values[1] = {"10", "10%"};
    music_volume.values[2] = {"20", "20%"};
    music_volume.values[3] = {"30", "30%"};
    music_volume.values[4] = {"40", "40%"};
    music_volume.values[5] = {"50", "50%"};
    music_volume.values[6] = {"60", "60%"};
    music_volume.values[7] = {"70", "70%"};
    music_volume.values[8] = {"80", "80%"};
    music_volume.values[9] = {"90", "90%"};
    music_volume.values[10] = {"100", "100%"};
    music_volume.values[11] = {"110", "110%"};
    music_volume.values[12] = {"120", "120%"};
    music_volume.values[13] = {"130", "130%"};
    music_volume.values[14] = {"140", "140%"};
    music_volume.values[15] = {"150", "150%"};
    music_volume.values[16] = {"160", "160%"};
    music_volume.values[17] = {"170", "170%"};
    music_volume.values[18] = {"180", "180%"};
    music_volume.values[19] = {"190", "190%"};
    music_volume.values[20] = {"200", "200%"};
    music_volume.default_value = "100";
    registered_definitions.push_back(music_volume);

    constexpr std::array<const char*, 6> audio_source_labels{{
        "SCSP Gain", "DSB (MPEG) Gain",
        "MultiPCM 1 Gain", "MultiPCM 2 Gain",
        "FM (YM3438) Gain", "Power Sled: Relay Audio Gain"}};
    for (size_t source = 0; source < audio_source_labels.size(); ++source) {
        retro_core_option_v2_definition gain{};
        gain.key = source < audio_source_keys.size()
            ? audio_source_keys[source] : "sm2_power_sled_relay_audio_gain";
        gain.desc = audio_source_labels[source];
        gain.info = "Adjust this sound source's output gain without stopping its emulation when muted.";
        if (source == audio_source_keys.size())
            gain.info = "Adjust the added relay sound-board output independently of rider audio.";
        gain.category_key = "audio";
        gain.values[0] = {"auto", "Auto"};
        gain.values[1] = {"0", "Mute"};
        for (unsigned step = 1; step <= 10; ++step) {
            option_value_label_storage.push_back(std::to_string(step * 10));
            option_value_label_storage.push_back(std::to_string(step * 10) + "%");
            gain.values[step + 1] = {option_value_label_storage[option_value_label_storage.size()-2].c_str(),
                                 option_value_label_storage.back().c_str()};
        }
        gain.default_value = "auto";
        registered_definitions.push_back(gain);
    }

    retro_core_option_v2_definition controller_profile{};
    controller_profile.key = "sm2_controller_profile_type";
    controller_profile.desc = "Controller Profile Type";
    controller_profile.info = "Select whether controller profiles include Test/Service inputs while retaining gameplay controls and remapping.";
    controller_profile.category_key = "input";
    controller_profile.values[0] = {"normal", "Normal"};
    controller_profile.values[1] = {"test_service", "Normal + Test/Service"};
    controller_profile.default_value = "test_service";
    registered_definitions.push_back(controller_profile);

    retro_core_option_v2_definition hide_shot_flash{};
    hide_shot_flash.key = "sm2_hide_shot_flash";
    hide_shot_flash.desc = "Hide Shot Flash";
    hide_shot_flash.info = "Suppress the white flash displayed when firing.";
    hide_shot_flash.category_key = "input";
    hide_shot_flash.values[0] = {"disabled", "Disabled"};
    hide_shot_flash.values[1] = {"enabled", "Enabled"};
    hide_shot_flash.default_value = "disabled";
    registered_definitions.push_back(hide_shot_flash);

    retro_core_option_v2_definition crosshair{};
    crosshair.key = "sm2_crosshairs";
    crosshair.desc = "Show Crosshair";
    crosshair.info = "Select external crosshair visibility for each player, or use the automatic native-reticle behavior.";
    crosshair.category_key = "input";
    crosshair.values[0] = {"auto", "Automatic"};
    crosshair.values[1] = {"0", "Disabled"};
    crosshair.values[2] = {"1", "Player 1 Only"};
    crosshair.values[3] = {"2", "Player 2 Only"};
    crosshair.values[4] = {"3", "Players 1 & 2"};
    crosshair.default_value = "auto";
    registered_definitions.push_back(crosshair);

    retro_core_option_v2_definition crosshair_style{};
    crosshair_style.key = "sm2_crosshair_style";
    crosshair_style.desc = "Crosshair Style";
    crosshair_style.info = "Select the crosshair shape. Auto uses a custom PNG or SM2-Emu; Custom requires a PNG and reports missing artwork. PNGs: system/sm2-emu/crosshairs/game-p1.png, game-p2.png or game.png; parent and default names are fallbacks. Maximum 1024 x 1024. Reload content after changing PNGs.";
    crosshair_style.category_key = "input";
    crosshair_style.values[0] = {"auto", "Auto"};
    crosshair_style.values[1] = {"sm2", "SM2-Emu"};
    crosshair_style.values[2] = {"supermodel", "Supermodel"};
    crosshair_style.values[3] = {"custom", "Custom"};
    crosshair_style.default_value = "sm2";
    registered_definitions.push_back(crosshair_style);

    retro_core_option_v2_definition crosshair_timeout{};
    crosshair_timeout.key = "sm2_crosshair_timeout";
    crosshair_timeout.desc = "Crosshair Inactivity Timeout";
    crosshair_timeout.info = "Hide each player's external crosshair after the selected time without aiming or firing. Movement or a gun action shows it again. Disabled keeps the selected visibility. Has no effect on native game reticles.";
    crosshair_timeout.category_key = "input";
    crosshair_timeout.values[0] = {"disabled", "Disabled"};
    constexpr std::array<const char*, 12> timeout_values{{"1", "2", "3", "4", "5", "6", "7", "8", "10", "15", "30", "60"}};
    constexpr std::array<const char*, 12> timeout_labels{{"1 Second", "2 Seconds", "3 Seconds", "4 Seconds", "5 Seconds", "6 Seconds", "7 Seconds", "8 Seconds", "10 Seconds", "15 Seconds", "30 Seconds", "60 Seconds"}};
    for (std::size_t i = 0; i < timeout_values.size(); ++i)
        crosshair_timeout.values[i + 1] = {timeout_values[i], timeout_labels[i]};
    crosshair_timeout.default_value = "disabled";
    registered_definitions.push_back(crosshair_timeout);

    {
        retro_core_option_v2_definition mapping{};
        mapping.key = "sm2_gunblade_aim_mapping";
        mapping.desc = "Gunblade NY: Aim Mapping";
        mapping.info = "Select custom horizontal aiming tables and vertical range, or the previous linear mapping.";
        mapping.category_key = "input";
        mapping.values[0] = {"upstream", "Custom (SM2-Emu)"};
        mapping.values[1] = {"previous", "Linear (MAME)"};
        mapping.default_value = "upstream";
        registered_definitions.push_back(mapping);
    }

    {
        retro_core_option_v2_definition mapping{};
        mapping.key = "sm2_rail_chase_aim_mapping";
        mapping.desc = "Rail Chase 2: Aim Mapping";
        mapping.info = "Select calibrated coordinate-table aiming or linear coordinate mapping.";
        mapping.category_key = "input";
        mapping.values[0] = {"upstream", "Custom (SM2-Emu)"};
        mapping.values[1] = {"previous", "Linear (MAME)"};
        mapping.default_value = "upstream";
        registered_definitions.push_back(mapping);
    }

    {
        retro_core_option_v2_definition mapping{};
        mapping.key = "sm2_hotd_prototype_aim_mapping";
        mapping.desc = "House Of The Dead: Prototype Aim Mapping";
        mapping.info = "Select an expanded aiming-coordinate range or the linear coordinate mapping.";
        mapping.category_key = "input";
        mapping.values[0] = {"upstream", "Custom (SM2-Emu)"};
        mapping.values[1] = {"previous", "Linear (MAME)"};
        mapping.default_value = "upstream";
        registered_definitions.push_back(mapping);
    }

    retro_core_option_v2_definition gun_input{};
    gun_input.key = "sm2_gun_input";
    gun_input.desc = "Gun Input Mode";
    gun_input.info = "Select which input devices control the aiming cursor, including combined and dedicated input modes.";
    gun_input.category_key = "input";
    gun_input.values[0] = {"hybrid", "Standard"};
    gun_input.values[1] = {"lightgun", "Lightgun Only"};
    gun_input.values[2] = {"mouse_analog", "Mouse + Analog Stick"};
    gun_input.values[3] = {"mouse", "Mouse Only"};
    gun_input.values[4] = {"analog", "Analog Stick Only"};
    gun_input.default_value = "hybrid";
    registered_definitions.push_back(gun_input);

    retro_core_option_v2_definition analog_aim{};
    analog_aim.key = "sm2_analog_aim";
    analog_aim.desc = "Analog Stick Aim";
    analog_aim.info = "Cursor: the analog stick moves the aiming cursor, which stays where it is when the stick is released. Absolute: the stick's position is the aiming position, as a light gun points, and the cursor returns to the centre when the stick is released.";
    analog_aim.category_key = "input";
    analog_aim.values[0] = {"cursor", "Cursor"};
    analog_aim.values[1] = {"absolute", "Absolute"};
    analog_aim.default_value = "cursor";
    registered_definitions.push_back(analog_aim);

    retro_core_option_v2_definition offscreen_reload{};
    offscreen_reload.key = "sm2_offscreen_reload_shortcut";
    offscreen_reload.desc = "Off-Screen Reload Shortcut";
    offscreen_reload.info = "Enable dedicated inputs that force an off-screen reload while retaining native lightgun off-screen firing.";
    offscreen_reload.category_key = "input";
    offscreen_reload.values[0] = {"enabled", "Enabled"};
    offscreen_reload.values[1] = {"disabled", "Disabled"};
    offscreen_reload.default_value = "enabled";
    registered_definitions.push_back(offscreen_reload);

    retro_core_option_v2_definition mouse_edge_reload{};
    mouse_edge_reload.key = "sm2_mouse_edge_offscreen_reload";
    mouse_edge_reload.desc = "Mouse Edge Off-Screen Reload";
    mouse_edge_reload.info = "Allow a mouse shot at the screen edge to trigger off-screen reload.";
    mouse_edge_reload.category_key = "input";
    mouse_edge_reload.values[0] = {"disabled", "Disabled"};
    mouse_edge_reload.values[1] = {"enabled", "Enabled"};
    mouse_edge_reload.default_value = "disabled";
    registered_definitions.push_back(mouse_edge_reload);

    retro_core_option_v2_definition gamepad_rumble{};
    gamepad_rumble.key = "sm2_gamepad_rumble";
    gamepad_rumble.desc = "Gamepad Rumble";
    gamepad_rumble.info = "Enable gamepad vibration using the game's force-feedback or rumble effects.";
    gamepad_rumble.category_key = "input";
    gamepad_rumble.values[0] = {"enabled", "Enabled"};
    gamepad_rumble.values[1] = {"disabled", "Disabled"};
    gamepad_rumble.default_value = "enabled";
    registered_definitions.push_back(gamepad_rumble);

    retro_core_option_v2_definition shifter{};
    shifter.key = "sm2_four_speed_shifter";
    shifter.desc = "4-Speed Shifter";
    shifter.info = "Select direct directional gear selection or diagonal analog-stick H-gate shifting. Applies only to 4-speed driving games.";
    shifter.category_key = "input";
    shifter.values[0] = {"h_gate", "H-Gate Mode"};
    shifter.values[1] = {"standard", "Standard"};
    shifter.default_value = "h_gate";
    registered_definitions.push_back(shifter);

    retro_core_option_v2_definition neutral_time{};
    neutral_time.key = "sm2_gear_shift_neutral_time";
    neutral_time.desc = "Gear Shift Neutral Time";
    neutral_time.info = "Set the time spent in neutral between gears when using Shift Up/Down in 4-speed driving games. Direct gear selection bypasses the delay. 0 disables it. Applies to subsequent shifts on return to gameplay without a machine reset; 0 also cancels a pending delay.";
    neutral_time.category_key = "input";
    for (unsigned step = 0; step <= 50; ++step) {
        option_value_label_storage.push_back(std::to_string(step * 10));
        const char* value = option_value_label_storage.back().c_str();
        option_value_label_storage.push_back(std::to_string(step * 10) + " ms");
        neutral_time.values[step] = {value, option_value_label_storage.back().c_str()};
    }
    neutral_time.default_value = "100";
    registered_definitions.push_back(neutral_time);

    retro_core_option_v2_definition automatic_start_gear{};
    automatic_start_gear.key = "sm2_automatic_start_gear";
    automatic_start_gear.desc = "Automatic Start Gear (Restart Required)";
    automatic_start_gear.info = "Use the declared initial gear for rolling starts instead of always starting in first gear. Applies only to 4-speed driving games.";
    automatic_start_gear.category_key = "input";
    automatic_start_gear.values[0] = {"enabled", "Enabled"};
    automatic_start_gear.values[1] = {"disabled", "Disabled"};
    automatic_start_gear.default_value = "enabled";
    registered_definitions.push_back(automatic_start_gear);

    retro_core_option_v2_definition desert_elevation_control{};
    desert_elevation_control.key = "sm2_desert_elevation_control";
    desert_elevation_control.desc = "Desert Tank: Elevation Control";
    desert_elevation_control.info = "Select relative movement or absolute-position control for turret elevation.";
    desert_elevation_control.category_key = "input";
    desert_elevation_control.values[0] = {"relative", "Relative"};
    desert_elevation_control.values[1] = {"absolute", "Absolute"};
    desert_elevation_control.default_value = "relative";
    registered_definitions.push_back(desert_elevation_control);

    retro_core_option_v2_definition desert_elevation_speed{};
    desert_elevation_speed.key = "sm2_desert_elevation_speed";
    desert_elevation_speed.desc = "Desert Tank: Elevation Speed (Relative Only)";
    desert_elevation_speed.info = "Adjust the maximum turret elevation speed in relative control mode.";
    desert_elevation_speed.category_key = "input";
    desert_elevation_speed.values[0] = {"10", "10%"};
    desert_elevation_speed.values[1] = {"20", "20%"};
    desert_elevation_speed.values[2] = {"30", "30%"};
    desert_elevation_speed.values[3] = {"40", "40%"};
    desert_elevation_speed.values[4] = {"50", "50%"};
    desert_elevation_speed.values[5] = {"60", "60%"};
    desert_elevation_speed.values[6] = {"70", "70%"};
    desert_elevation_speed.values[7] = {"80", "80%"};
    desert_elevation_speed.values[8] = {"90", "90%"};
    desert_elevation_speed.values[9] = {"100", "100%"};
    desert_elevation_speed.values[10] = {"110", "110%"};
    desert_elevation_speed.values[11] = {"120", "120%"};
    desert_elevation_speed.values[12] = {"130", "130%"};
    desert_elevation_speed.values[13] = {"140", "140%"};
    desert_elevation_speed.values[14] = {"150", "150%"};
    desert_elevation_speed.values[15] = {"160", "160%"};
    desert_elevation_speed.values[16] = {"170", "170%"};
    desert_elevation_speed.values[17] = {"180", "180%"};
    desert_elevation_speed.values[18] = {"190", "190%"};
    desert_elevation_speed.values[19] = {"200", "200%"};
    desert_elevation_speed.default_value = "100";
    registered_definitions.push_back(desert_elevation_speed);

    retro_core_option_v2_definition desert_elevation_axis{};
    desert_elevation_axis.key = "sm2_desert_elevation_axis";
    desert_elevation_axis.desc = "Desert Tank: Elevation Axis Mode";
    desert_elevation_axis.info = "Select normal or inverted vertical input for turret elevation.";
    desert_elevation_axis.category_key = "input";
    desert_elevation_axis.values[0] = {"normal", "Normal"};
    desert_elevation_axis.values[1] = {"inverted", "Inverted"};
    desert_elevation_axis.default_value = "normal";
    registered_definitions.push_back(desert_elevation_axis);

    retro_core_option_v2_definition top_skater_response{};
    top_skater_response.key = "sm2_top_skater_curving_response";
    top_skater_response.desc = "Top Skater: Curving Response";
    top_skater_response.info = "Select a progressive or linear curving response while retaining full control travel.";
    top_skater_response.category_key = "input";
    top_skater_response.values[0] = {"linear", "Linear"};
    top_skater_response.values[1] = {"progressive", "Progressive (Fine Center)"};
    top_skater_response.default_value = "progressive";
    registered_definitions.push_back(top_skater_response);

    retro_core_option_v2_definition steering_response{};
    steering_response.key = "sm2_steering_response";
    steering_response.desc = "Driving Steering Response";
    steering_response.info = "Select the steering response curve to adjust sensitivity around the center while retaining full travel.";
    steering_response.category_key = "input";
    steering_response.values[0] = {"linear", "Linear"};
    steering_response.values[1] = {"progressive", "Progressive (Fine Center)"};
    steering_response.values[2] = {"fbneo", "FBNeo Logarithmic (Fine Center)"};
    steering_response.default_value = "linear";
    registered_definitions.push_back(steering_response);

    const auto add_output_range = [](const char* key, const char* label, const char* info,
                                     bool steering) {
        retro_core_option_v2_definition definition{};
        definition.key = key;
        definition.desc = label;
        definition.info = info;
        definition.category_key = "input";
        static constexpr std::pair<const char*, const char*> steering_values[] = {
            {"50", "50%"}, {"60", "60%"}, {"63", "63% (30-80-D0)"},
            {"70", "70%"}, {"80", "80%"}, {"90", "90%"},
            {"100", "100%"}, {"110", "110%"}, {"120", "120%"},
            {"130", "130%"}, {"140", "140%"}, {"150", "150%"},
        };
        static constexpr std::pair<const char*, const char*> pedal_values[] = {
            {"50", "50%"}, {"60", "60%"}, {"70", "70%"},
            {"75.3", "75.3% (00-C0)"}, {"80", "80%"}, {"90", "90%"},
            {"100", "100%"}, {"110", "110%"}, {"120", "120%"},
            {"130", "130%"}, {"140", "140%"}, {"150", "150%"},
        };
        const auto* values = steering ? steering_values : pedal_values;
        const size_t value_count = steering ? std::size(steering_values)
                                            : std::size(pedal_values);
        for (size_t i = 0; i < value_count; ++i) {
            definition.values[i] = {values[i].first, values[i].second};
        }
        definition.default_value = "100";
        return definition;
    };
    registered_definitions.push_back(add_output_range(
        "sm2_steering_output_range", "Driving Steering Output Range",
        "Scale steering output around its center to adjust the input travel needed for full lock.", true));
    registered_definitions.push_back(add_output_range(
        "sm2_accelerator_output_range", "Driving Accelerator Output Range",
        "Scale accelerator output from its released position to adjust the input travel needed for full acceleration.", false));
    registered_definitions.push_back(add_output_range(
        "sm2_brake_output_range", "Driving Brake Output Range",
        "Scale brake output from its released position to adjust the input travel needed for full braking.", false));

    // Keep functional controls together, then place game-specific controls last.
    // System retains its native/operator order.
    constexpr std::array<std::string_view, 9> video_order{{
        "sm2_renderer", "sm2_software_renderer_threading",
        "sm2_internal_resolution", "sm2_upscale_2d", "sm2_texture_filter",
        "sm2_aspect_ratio", "sm2_widescreen_mode", "sm2_av_timing", "sm2_timing_overlay"}};
    constexpr std::array<std::string_view, 10> audio_order{{
        "sm2_music_volume", "sm2_audio_balance", "sm2_scsp_gain", "sm2_dsb_gain",
        "sm2_multipcm1_gain", "sm2_multipcm2_gain", "sm2_ym3438_gain",
        "sm2_scsp_output_low_pass_filter",
        "sm2_manxtt_cabinet_audio", "sm2_power_sled_relay_audio_gain"}};
    constexpr std::array<std::string_view, 24> input_order{{
        "sm2_controller_profile_type", "sm2_gamepad_rumble",
        "sm2_hide_shot_flash",
        "sm2_crosshairs", "sm2_crosshair_style", "sm2_crosshair_timeout", "sm2_gun_input",
        "sm2_analog_aim", "sm2_offscreen_reload_shortcut", "sm2_mouse_edge_offscreen_reload",
        "sm2_four_speed_shifter", "sm2_gear_shift_neutral_time",
        "sm2_automatic_start_gear", "sm2_steering_response",
        "sm2_steering_output_range", "sm2_accelerator_output_range",
        "sm2_brake_output_range", "sm2_desert_elevation_control",
        "sm2_desert_elevation_speed", "sm2_desert_elevation_axis",
        "sm2_gunblade_aim_mapping", "sm2_hotd_prototype_aim_mapping",
        "sm2_rail_chase_aim_mapping",
        "sm2_top_skater_curving_response"}};
    const auto order_category = [](std::string_view category, const auto& order) {
        const auto first = std::find_if(registered_definitions.begin(),
                                        registered_definitions.end(), [category](const auto& option) {
            return option.category_key && category == option.category_key;
        });
        const auto last = std::find_if(first, registered_definitions.end(),
                                       [category](const auto& option) {
            return !option.category_key || category != option.category_key;
        });
        const auto rank = [&order](const auto& option) {
            return std::find(order.begin(), order.end(), std::string_view(option.key));
        };
        std::stable_sort(first, last, [&rank](const auto& left, const auto& right) {
            return rank(left) < rank(right);
        });
    };
    order_category("video", video_order);
    order_category("audio", audio_order);
    order_category("input", input_order);

    registered_definitions.push_back({});
}

inline AVTimingMode av_timing_mode()
{
    retro_variable option{"sm2_av_timing", nullptr};
    if (!option_environment || !option_variable(option)
        || !option.value) return AVTimingMode::FullFrames60Hz;
    if (std::strcmp(option.value, "native") == 0) return AVTimingMode::Native;
    return AVTimingMode::FullFrames60Hz;
}

inline bool software_renderer_async_enabled()
{
    retro_variable option{"sm2_software_renderer_threading", nullptr};
    return !(option_environment
             && option_variable(option)
             && option.value && std::strcmp(option.value, "synchronous") == 0);
}

inline bool rom_crc_verification_enabled()
{
    retro_variable option{"sm2_rom_crc_verification", nullptr};
    return !(option_environment
             && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool i960_round_nearest_even_enabled()
{
    retro_variable option{"sm2_i960_round_nearest_even", nullptr};
    return option_environment && option_variable(option)
           && option.value && std::strcmp(option.value, "enabled") == 0;
}

inline bool expand_3d_view()
{
    retro_variable option{"sm2_widescreen_mode", nullptr};
    return option_environment && option_variable(option) && option.value
        && std::strcmp(option.value, "expand_3d") == 0;
}

inline AspectRatioMode aspect_ratio_mode()
{
    retro_variable option{"sm2_aspect_ratio", nullptr};
    if (!(option_environment && option_variable(option))
        || !option.value)
        return AspectRatioMode::Automatic;
    if (std::strcmp(option.value, "4_3") == 0) return AspectRatioMode::FourThree;
    if (std::strcmp(option.value, "16_9") == 0) return AspectRatioMode::SixteenNine;
    return AspectRatioMode::Automatic;
}

inline bool expand_3d_view_for(const rom::GameSpec& game, bool hardware)
{
    const std::string_view family = game.parent.empty() ? game.name : game.parent;
    return hardware && aspect_ratio_mode() != AspectRatioMode::FourThree && expand_3d_view()
        && family != "indy500" && family != "stcc" && family != "sgt24h";
}

inline unsigned timing_overlay_font_pixels()
{
    retro_variable option{"sm2_timing_overlay", nullptr};
    if (!(option_environment && option_variable(option))
        || !option.value)
        return 0;
    if (std::strcmp(option.value, "auto") == 0) return 13;
    if (std::strcmp(option.value, "11") == 0) return 11;
    if (std::strcmp(option.value, "12") == 0) return 12;
    if (std::strcmp(option.value, "13") == 0) return 13;
    if (std::strcmp(option.value, "14") == 0) return 14;
    return 0;
}

inline bool hide_shot_flash_enabled()
{
    retro_variable option{"sm2_hide_shot_flash", nullptr};
    return option_environment && option_variable(option)
        && option.value && std::strcmp(option.value, "enabled") == 0;
}

inline unsigned crosshair_mask(const rom::GameSpec& game)
{
    retro_variable option{"sm2_crosshairs", nullptr};
    if (!(option_environment && option_variable(option))
        || !option.value)
        return game.lightgun.present ? 1u : 0u;
    if (std::strcmp(option.value, "auto") == 0)
        return game.lightgun.present ? 1u : 0u;
    if (std::strcmp(option.value, "0") == 0) return 0;
    if (std::strcmp(option.value, "1") == 0) return 1;
    if (std::strcmp(option.value, "2") == 0) return 2;
    if (std::strcmp(option.value, "3") == 0) return 3;
    return game.lightgun.present ? 1u : 0u;
}

inline CrosshairStyle crosshair_style()
{
    retro_variable option{"sm2_crosshair_style", nullptr};
    if (option_environment && option_variable(option) && option.value) {
        if (std::strcmp(option.value, "supermodel") == 0) return CrosshairStyle::Supermodel;
        if (std::strcmp(option.value, "auto") == 0) return CrosshairStyle::Auto;
        if (std::strcmp(option.value, "custom") == 0) return CrosshairStyle::Custom;
    }
    return CrosshairStyle::Sm2;
}

inline unsigned crosshair_timeout_seconds()
{
    retro_variable option{"sm2_crosshair_timeout", nullptr};
    if (option_environment && option_variable(option) && option.value)
        for (unsigned seconds : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 10u, 15u, 30u, 60u})
            if (std::to_string(seconds) == option.value) return seconds;
    return 0;
}

inline bool initial_nvram_setup_enabled()
{
    retro_variable option{"sm2_initial_nvram_setup", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool four_speed_h_gate()
{
    retro_variable option{"sm2_four_speed_shifter", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "standard") == 0);
}

inline unsigned gear_shift_neutral_ms()
{
    retro_variable option{"sm2_gear_shift_neutral_time", nullptr};
    if (option_environment && option_variable(option) && option.value) {
        char* end = nullptr;
        const unsigned long value = std::strtoul(option.value, &end, 10);
        if (option.value[0] >= '0' && option.value[0] <= '9'
            && end && *end == '\0' && value <= 500 && value % 10 == 0)
            return static_cast<unsigned>(value);
    }
    return 100;
}

inline bool automatic_start_gear_enabled()
{
    retro_variable option{"sm2_automatic_start_gear", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool gamepad_rumble_enabled()
{
    retro_variable option{"sm2_gamepad_rumble", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool audio_balance_enabled()
{
    retro_variable option{"sm2_audio_balance", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool manxtt_cabinet_audio_enabled()
{
    retro_variable option{"sm2_manxtt_cabinet_audio", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool power_sled_relay_audio_enabled()
{
    retro_variable option{"sm2_power_sled_relay_audio", nullptr};
    return !(option_environment && option_variable(option)
        && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool power_sled_standalone_enabled()
{
    retro_variable option{"sm2_power_sled_standalone", nullptr};
    return !(option_environment && option_variable(option)
        && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool power_sled_ai_finish_speed_fix_enabled()
{
    retro_variable option{"sm2_power_sled_ai_finish_speed_fix", nullptr};
    return !(option_environment && option_variable(option)
        && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool power_sled_relay_commands_enabled()
{
    retro_variable option{"sm2_power_sled_relay_commands", nullptr};
    if (option_environment && option_variable(option)
        && option.value)
        return std::strcmp(option.value, "enabled") == 0;
    return true;
}

inline bool scsp_output_low_pass_enabled()
{
    retro_variable option{"sm2_scsp_output_low_pass_filter", nullptr};
    if (option_environment && option_variable(option)
        && option.value) {
        return std::strcmp(option.value, "disabled") != 0;
    }
    return true;
}

inline unsigned music_volume_percent()
{
    retro_variable option{"sm2_music_volume", nullptr};
    const char* value = option_environment
            && option_variable(option)
            && option.value ? option.value : "100";
    return static_cast<unsigned>(std::clamp(std::atoi(value), 0, 200));
}

inline bool audio_gain_automatic(const char* key)
{
    retro_variable option{key, nullptr};
    return !(option_environment && option_variable(option)
             && option.value) || std::strcmp(option.value, "auto") == 0;
}

inline unsigned audio_gain(const char* key)
{
    retro_variable option{key, nullptr};
    const char* value = option_environment
        && option_variable(option)
        && option.value ? option.value : "auto";
    if (std::strcmp(value, "auto") == 0) return 100;
    if (std::strcmp(value, "mute") == 0 || std::strcmp(value, "Mute") == 0
        || std::strcmp(value, "0") == 0) return 0;
    char* end = nullptr;
    const long percent = std::strtol(value, &end, 10);
    if (end == value || *end || percent < 0 || percent > 200 || percent % 10) return 100;
    return static_cast<unsigned>(percent);
}

inline bool audio_source_gain_automatic(size_t source)
{
    return source >= audio_source_keys.size() || audio_gain_automatic(audio_source_keys[source]);
}

inline unsigned audio_source_gain(size_t source)
{
    return source >= audio_source_keys.size() ? 100 : audio_gain(audio_source_keys[source]);
}

inline SpecialAnalogOptions special_analog_options()
{
    SpecialAnalogOptions options;
    retro_variable response{"sm2_top_skater_curving_response", nullptr};
    if (option_environment && option_variable(response)
        && response.value)
        options.top_skater_progressive = std::strcmp(response.value, "linear") != 0;
    for (auto [key, enabled] : {std::pair{"sm2_gunblade_aim_mapping", &options.gunblade_upstream},
                                std::pair{"sm2_rail_chase_aim_mapping", &options.rail_chase_upstream},
                                std::pair{"sm2_hotd_prototype_aim_mapping", &options.hotd_prototype_upstream}}) {
        retro_variable mapping{key, nullptr};
        if (option_environment && option_variable(mapping)
            && mapping.value)
            *enabled = std::strcmp(mapping.value, "previous") != 0;
    }
    return options;
}

inline DrivingAnalogOptions driving_analog_options()
{
    const auto value = [](const char* key, const char* fallback) {
        retro_variable option{key, nullptr};
        return option_environment && option_variable(option)
                && option.value
            ? option.value : fallback;
    };
    DrivingAnalogOptions options;
    const char* response = value("sm2_steering_response", "linear");
    options.steering_response = std::strcmp(response, "progressive") == 0
        ? SteeringResponse::Progressive
        : std::strcmp(response, "fbneo") == 0
            ? SteeringResponse::FBNeoLogarithmic : SteeringResponse::Linear;
    options.steering_output_range = std::atoi(value("sm2_steering_output_range", "100"));
    const char* accelerator = value("sm2_accelerator_output_range", "100");
    const char* brake = value("sm2_brake_output_range", "100");
    options.accelerator_output_range_per_mille =
        std::strcmp(accelerator, "75.3") == 0 ? 753 : std::atoi(accelerator) * 10;
    options.brake_output_range_per_mille =
        std::strcmp(brake, "75.3") == 0 ? 753 : std::atoi(brake) * 10;
    return options;
}

inline DesertElevationOptions desert_elevation_options()
{
    const auto value = [](const char* key, const char* fallback) {
        retro_variable option{key, nullptr};
        return option_environment && option_variable(option)
                && option.value
            ? option.value : fallback;
    };
    DesertElevationOptions options;
    options.control = std::strcmp(value("sm2_desert_elevation_control", "relative"),
                                  "absolute") == 0
        ? DesertElevationControl::Absolute : DesertElevationControl::Relative;
    options.speed_percent = std::atoi(value("sm2_desert_elevation_speed", "100"));
    options.inverted = std::strcmp(value("sm2_desert_elevation_axis", "normal"),
                                   "inverted") == 0;
    return options;
}

inline unsigned texture_filter_quality()
{
    retro_variable option{"sm2_texture_filter", nullptr};
    if (!(option_environment && option_variable(option))
        || !option.value || std::strcmp(option.value, "faithful") == 0)
        return 0;
    const unsigned quality = static_cast<unsigned>(std::atoi(option.value));
    return quality == 2 || quality == 4 || quality == 8 || quality == 16 ? quality : 0;
}

inline unsigned upscale_2d_mode()
{
    retro_variable option{"sm2_upscale_2d", nullptr};
    if (!(option_environment && option_variable(option))
        || !option.value)
        return 0;
    if (std::strcmp(option.value, "xbr") == 0) return 1;
    if (std::strcmp(option.value, "scalefx") == 0) return 2;
    return 0;
}

inline bool controller_test_service_enabled()
{
    retro_variable option{"sm2_controller_profile_type", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "normal") == 0);
}

inline GunInputMode gun_input_mode()
{
    retro_variable option{"sm2_gun_input", nullptr};
    if (!(option_environment && option_variable(option))
        || !option.value)
        return GunInputMode::Hybrid;
    if (std::strcmp(option.value, "lightgun") == 0) return GunInputMode::Lightgun;
    if (std::strcmp(option.value, "mouse") == 0) return GunInputMode::Mouse;
    if (std::strcmp(option.value, "mouse_analog") == 0) return GunInputMode::MouseAnalog;
    if (std::strcmp(option.value, "analog") == 0) return GunInputMode::AnalogSticks;
    return GunInputMode::Hybrid;
}

inline bool analog_aim_absolute()
{
    retro_variable option{"sm2_analog_aim", nullptr};
    return option_environment && option_variable(option)
        && option.value && std::strcmp(option.value, "absolute") == 0;
}

inline bool offscreen_reload_shortcut_enabled()
{
    retro_variable option{"sm2_offscreen_reload_shortcut", nullptr};
    return !(option_environment && option_variable(option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool mouse_edge_offscreen_reload_enabled()
{
    retro_variable option{"sm2_mouse_edge_offscreen_reload", nullptr};
    return option_environment && option_variable(option)
        && option.value && std::strcmp(option.value, "enabled") == 0;
}

inline unsigned linked_cabinets(std::string_view game)
{
    const auto* linked = linked_cabinet_game(game);
    if (!linked) return 1;
    const std::string key = "sm2_linked_cabinets_" + std::string(game);
    retro_variable variable{key.c_str(), nullptr};
    if (!option_variable(variable) || !variable.value) return 1;
    const std::string value(variable.value);
    if (value == "-1" || value == "0" || value == "1") return 1;
    const unsigned minimum = game == "vonr" ? 3u : 2u;
    if (value.size() == 1 && value[0] >= static_cast<char>('0' + minimum) && value[0] <= static_cast<char>('0' + linked->max_cabinets))
        return static_cast<unsigned>(value[0] - '0');
    invalid_network_option(key, value, "-1");
    return 1;
}

inline std::string automatic_network_preset(std::string_view game)
{
    const auto* linked = linked_cabinet_game(game);
    if (!linked) return "-1";
    const auto catalog = game == "vonr" || game == "powsledr" ? game : linked->nvram_game;
    const std::string fallback(network::default_preset(catalog));
    const std::string key = "sm2_automatic_network_" + std::string(game);
    retro_variable variable{key.c_str(), nullptr};
    if (!option_variable(variable) || !variable.value) return fallback;
    std::string selected(variable.value);
    if (selected == "l" || selected == "s") selected = "r";
    const auto choices = network::choices(catalog, linked->max_cabinets);
    if (std::any_of(choices.begin(), choices.end(), [&](const auto& choice) { return choice.key == selected; }))
        return selected;
    invalid_network_option(key, selected, fallback);
    return fallback;
}

inline bool update_option_visibility()
{
    if (!option_environment) return false;
    retain_option_updates();
    const bool master_visible = !nvram::options_for_game(option_game).empty();
    const bool supported_network = linked_cabinet_game(option_content_game) != nullptr;
    const std::string network_selection = supported_network ? automatic_network_preset(option_content_game) : "-1";
    const bool automated = supported_network && network_selection != "-1";
    const std::string network_tag = option_content_game + ":" + network_selection;
    const auto network_catalog = option_content_game == "vonr" ? std::string_view{"vonr"} : std::string_view{option_game};
    retro_variable enabled_option{"sm2_nvram_settings", nullptr};
    const bool enabled = option_variable(enabled_option) &&
                         enabled_option.value && std::string(enabled_option.value) == "enabled";
    const bool game_visible = master_visible && enabled;
    retro_core_option_display master{"sm2_nvram_settings", master_visible};
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &master);
    for (const auto& definition : registered_definitions) {
        if (!definition.key || std::strcmp(definition.key, "sm2_nvram_settings") == 0) continue;
        const std::string_view key = definition.key;
        if (key == "sm2_power_sled_relay_audio" || key == "sm2_power_sled_standalone"
            || key == "sm2_power_sled_relay_commands" || key == "sm2_power_sled_ai_finish_speed_fix") {
            const unsigned role = power_sled_program_role(option_content_game);
            retro_core_option_display display{definition.key,
                role == 1 || (role == 2 && key == "sm2_power_sled_standalone")};
            option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            continue;
        }
        if (key.starts_with("sm2_linked_cabinets_")) {
            retro_core_option_display display{definition.key,
                key.substr(std::strlen("sm2_linked_cabinets_")) == (supported_network ? option_content_game : "unsupported")};
            option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            continue;
        }
        if (key.starts_with("sm2_automatic_network_")) {
            retro_core_option_display display{definition.key,
                key.substr(std::strlen("sm2_automatic_network_")) == (supported_network ? option_content_game : "unsupported")};
            option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            continue;
        }
        const auto source = std::find_if(audio_source_keys.begin(), audio_source_keys.end(),
            [&](const char* candidate) { return key == candidate; });
        if (source != audio_source_keys.end()) {
            retro_core_option_display display{definition.key,
                option_audio_sources[static_cast<size_t>(source - audio_source_keys.begin())]};
            option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            continue;
        }
        if (!std::string_view(definition.key).starts_with("sm2_nvram_")) {
            retro_core_option_display display{definition.key, true};
            option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            continue;
        }
        const std::string prefix = "sm2_nvram_" + option_game + "_";
        retro_core_option_display display{definition.key,
            game_visible && key.starts_with(prefix)
                && !(automated && network::managed_field(network_catalog, key.substr(prefix.size())))};
        option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
    }
    const bool changed = master_visible != nvram_master_visible || game_visible != nvram_game_visible
        || network_tag != network_visibility;
    network_visibility = network_tag;
    nvram_master_visible = master_visible;
    nvram_game_visible = game_visible;
    return changed;
}

inline void set_option_game(std::string game, std::string content_game = {})
{
    option_game = std::move(game);
    option_content_game = content_game.empty() ? option_game : std::move(content_game);
    update_option_visibility();
}

inline void set_audio_sources(std::array<bool, 5> present)
{
    option_audio_sources = present;
    update_option_visibility();
}

inline std::vector<std::string> selected_nvram_values(std::string_view game)
{
    std::vector<std::string> selected;
    for (const auto& option : nvram::options_for_game(game)) {
        const std::string key = "sm2_nvram_" + std::string(option.game) + "_" + option.suffix;
        retro_variable variable{key.c_str(), nullptr};
        if (option_environment && option_variable(variable) &&
            variable.value)
            selected.emplace_back(variable.value);
        else
            selected.emplace_back(option.default_value);
    }
    return selected;
}

inline bool nvram_settings_enabled()
{
    retro_variable option{"sm2_nvram_settings", nullptr};
    return option_environment && option_variable(option) &&
           option.value && std::string(option.value) == "enabled";
}

inline void prepare_gain_compatibility(bool probe_all)
{
    compatible_definitions = registered_definitions;
    for (auto& definition : compatible_definitions) {
        if (!definition.key || std::find(audio_source_keys.begin(), audio_source_keys.end(),
                std::string_view(definition.key)) == audio_source_keys.end()) continue;
        size_t end = 0;
        while (definition.values[end].value) ++end;
        retro_variable selected{definition.key, nullptr};
        const char* saved = !probe_all && option_environment
            && option_variable(selected)
            ? selected.value : nullptr;
        for (unsigned percent = 110; percent <= 200; percent += 10) {
            const std::string value = std::to_string(percent);
            if (!probe_all && (!saved || value != saved)) continue;
            option_value_label_storage.push_back(value);
            const char* raw = option_value_label_storage.back().c_str();
            option_value_label_storage.push_back(value + "% (Legacy)");
            definition.values[end++] = {raw, option_value_label_storage.back().c_str()};
        }
        for (const char* value : {"mute", "Mute"}) {
            if (probe_all || (saved && std::strcmp(value, saved) == 0))
                definition.values[end++] = {value, "Mute (Legacy)"};
        }
    }
}

inline bool update_option_display()
{
    const bool visibility_changed = update_option_visibility();
    const bool controls_changed = option_controls_update && option_controls_update();
    return visibility_changed || controls_changed;
}

inline void register_core_options(retro_environment_t env)
{
    option_environment = env;
    option_updates_pending = false;
    option_warnings.clear();
    reported_invalid_network_values.clear();
    option_corrections.clear();
    build_option_definitions();
    static retro_core_option_v2_category categories[] = {
        {"system", "System", "Machine settings stored in battery-backed memory."},
        {"video", "Video", "Renderer, resolution, A/V cadence and diagnostics."},
        {"audio", "Audio", "Sound emulation and per-game mixing."},
        {"input", "Input", "Controller profiles and input response."},
        {nullptr, nullptr, nullptr}};
    unsigned version = 0;
    if (env(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
        // Register old values first so RetroArch can read them from existing .opt files.
        prepare_gain_compatibility(true);
        retro_core_options_v2 options{categories, compatible_definitions.data()};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        prepare_gain_compatibility(false);
        options.definitions = compatible_definitions.data();
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        retro_core_options_update_display_callback callback{update_option_display};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &callback);
        update_option_visibility();
        return;
    }

    for (bool probe_all : {true, false}) {
        prepare_gain_compatibility(probe_all);
        legacy_definitions.clear();
        legacy_value_storage.clear();
        legacy_definitions.reserve(registered_definitions.size());
        for (const auto& definition : compatible_definitions) {
            if (!definition.key) break;
            std::string line = std::string(definition.desc) + "; ";
            const bool network_option = std::string_view(definition.key).starts_with("sm2_linked_cabinets_")
                || std::string_view(definition.key).starts_with("sm2_automatic_network_");
            bool first = true;
            // The legacy ABI encodes its default as the first token.
            if (network_option) { line += definition.default_value; first = false; }
            for (const auto& value : definition.values) {
                if (!value.value) break;
                if (network_option && std::strcmp(value.value, definition.default_value) == 0) continue;
                if (!first) line += '|';
                line += value.value;
                first = false;
            }
            legacy_value_storage.push_back(std::move(line));
            legacy_definitions.push_back({definition.key, legacy_value_storage.back().c_str()});
        }
        legacy_definitions.push_back({nullptr, nullptr});
        env(RETRO_ENVIRONMENT_SET_VARIABLES, legacy_definitions.data());
    }
}
}  // namespace sm2::libretro
