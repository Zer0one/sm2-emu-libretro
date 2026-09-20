// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "crosshair.h"
#include "libretro.h"
#include "input.h"
#include "nvram_settings.h"

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
enum class AVTimingMode { Native, Compatibility60Hz };
enum class AspectRatioMode { Automatic, FourThree, SixteenNine };

inline retro_environment_t option_environment = nullptr;
inline std::string option_game;
inline std::string option_content_game;
inline bool nvram_master_visible = false;
inline bool nvram_game_visible = false;
inline std::vector<retro_core_option_v2_definition> registered_definitions;
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

inline constexpr std::array<LinkedCabinetGame, 28> linked_cabinet_games{{
    {"daytona", "daytona", 8},
    {"daytonas", "daytonas", 8}, {"daytonase", "daytona", 8},
    {"daytonam", "daytona", 8},
    {"stcc", "stcc", 9}, {"stcca", "stcca", 9}, {"stccb", "stccb", 9},
    {"stcco", "stcco", 9},
    {"srallyc", "srallyc", 5}, {"srallycb", "srallyc", 5},
    {"srallycc", "srallyc", 5},
    {"indy500", "indy500", 8}, {"indy500d", "indy500d", 8},
    {"indy500to", "indy500", 8},
    {"motoraid", "motoraid", 4}, {"motoraiddx", "motoraiddx", 4},
    {"waverunr", "waverunr", 4}, {"skisuprg", "skisuprg", 4},
    {"sgt24h", "sgt24h", 4},
    {"overrev", "overrev", 4}, {"overrevb", "overrev", 4},
    {"overrevba", "overrev", 4},
    {"manxtt", "manxtt", 3}, {"manxttc", "manxtt", 3},
    {"von", "von", 3}, {"vonj", "von", 3}, {"vonu", "von", 3},
    {"vonr", "von", 3},
}};

inline constexpr std::array<retro_core_option_value, 9> linked_cabinet_values{{
    {"disabled", "Disabled"}, {"2", "2 Cabinets"}, {"3", "3 Cabinets"},
    {"4", "4 Cabinets"}, {"5", "5 Cabinets"}, {"6", "6 Cabinets"},
    {"7", "7 Cabinets"}, {"8", "8 Cabinets"}, {"9", "9 Cabinets"},
}};

inline const LinkedCabinetGame* linked_cabinet_game(std::string_view game)
{
    const auto found = std::find_if(linked_cabinet_games.begin(), linked_cabinet_games.end(),
        [game](const auto& candidate) { return candidate.name == game; });
    return found == linked_cabinet_games.end() ? nullptr : &*found;
}

inline std::string_view linked_cabinet_network_family(std::string_view game)
{
    return game == "von" || game == "vonj" || game == "vonu" || game == "vonr"
        ? std::string_view{"von"} : game;
}

inline std::string_view linked_cabinet_rules(const LinkedCabinetGame& linked_game)
{
    if (linked_game.name == "vonr")
        return "This dedicated Relay program is valid only in a 3 Cabinets session with one Master Twin and one Slave Twin; it displays the linked match and is not playable.";
    const std::string_view game = linked_game.nvram_game;
    if (game == "daytona" || game == "daytonas")
        return "Use exactly one Master and one Slave for every remaining cabinet. Car Numbers must be unique and sequential from 1 through the selected total. Relay is not available.";
    if (game == "stcc" || game == "stcca" || game == "stccb" || game == "stcco")
        return "Use unique, sequential Car roles from Car 1 through the number of playable cabinets. One optional Relay may be included in the selected total; never configure more than one Relay.";
    if (game == "srallyc")
        return "Use unique, sequential Car roles from Car 1 through the number of playable cabinets. One optional Relay may be included in the selected total; never configure more than one Relay.";
    if (game == "manxtt")
        return "For 2 Cabinets use Master and Slave. For 3 Cabinets use Master, Slave and one Relay.";
    if (game == "von")
        return "For 2 Cabinets use one Twin set as Master and one Twin set as Slave. For 3 Cabinets add exactly one dedicated vonr Relay set; the Relay displays the linked match and is not a playable cabinet.";
    if (game == "sgt24h")
        return "Use Car No.1 Master and sequential Slave roles for the remaining cabinets. Link Max must equal the selected total. Relay is not available.";
    if (game == "overrev")
        return "Use Master CarNo.1 and sequential Slave roles for the remaining cabinets. Link Max must equal the selected total. Relay is not available.";
    if (game == "motoraid" || game == "motoraiddx")
        return "Use exactly one Master and sequential Slave cabinets with unique IDs. Live is the relay/live-monitor role: it is included in the selected total and at most one cabinet should use it.";
    return "Use exactly one Master and one Slave for every remaining cabinet. Cabinet IDs must be unique and sequential from 1 through the selected total. Relay is not available.";
}

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
    registered_definitions.reserve(all.size() + 29);

    retro_core_option_v2_definition initial_nvram{};
    initial_nvram.key = "sm2_initial_nvram_setup";
    initial_nvram.desc = "Automatic Initial NVRAM Setup";
    initial_nvram.info = "When no frontend .srm or valid standalone .nv/.eeprom exists, initialize supported sets from a validated game sample or a byte-identical parent sample before the first emulated frame, then apply the selected country, safe offline/link values and core defaults such as Daytona's Deluxe cabinet and Super GT 24h's I/O Type C. Automatic setup never replaces existing saves; normal game writes still persist. Delete the game's save data to regenerate the initial setup.";
    initial_nvram.category_key = "system";
    initial_nvram.values[0] = {"enabled", "Enabled"};
    initial_nvram.values[1] = {"disabled", "Disabled"};
    initial_nvram.default_value = "enabled";
    registered_definitions.push_back(initial_nvram);

    for (const auto& game : linked_cabinet_games) {
        option_key_storage.emplace_back("sm2_linked_cabinets_" + std::string(game.name));
        retro_core_option_v2_definition linked_cabinets{};
        linked_cabinets.key = option_key_storage.back().c_str();
        linked_cabinets.desc = "Linked Cabinets (Restart Required)";
        option_info_storage.emplace_back(
            "Set the total number of cabinets expected for this game in the RetroArch Netplay session. All displayed totals are supported; there is no even-number requirement. Disabled preserves the standalone one-node loopback. Every instance must use the same core and value, then restart content. Use the same ROM unless the game-specific rules below require a dedicated Relay program. "
            + std::string(linked_cabinet_rules(game)));
        linked_cabinets.info = option_info_storage.back().c_str();
        linked_cabinets.category_key = "system";
        if (game.name == "vonr") {
            linked_cabinets.values[0] = linked_cabinet_values[0];
            linked_cabinets.values[1] = linked_cabinet_values[2];
        } else {
            std::copy_n(linked_cabinet_values.begin(), game.max_cabinets,
                        linked_cabinets.values);
        }
        linked_cabinets.default_value = "disabled";
        registered_definitions.push_back(linked_cabinets);
    }

    retro_core_option_v2_definition ski_drive_board_bypass{};
    ski_drive_board_bypass.key = "sm2_skisuprg_drive_board_bypass";
    ski_drive_board_bypass.desc = "Drive Board Error Bypass (Restart Required)";
    ski_drive_board_bypass.info = "For Sega Ski Super G, automatically press Test once when the game reaches the DRIVE BOARD TROUBLE CODE: FF screen. This allows play without the unimplemented external Drive Board. Disabled preserves the original SM2-Emu startup behaviour.";
    ski_drive_board_bypass.category_key = "system";
    ski_drive_board_bypass.values[0] = {"disabled", "Disabled"};
    ski_drive_board_bypass.values[1] = {"enabled", "Enabled"};
    ski_drive_board_bypass.default_value = "disabled";
    registered_definitions.push_back(ski_drive_board_bypass);

    retro_core_option_v2_definition master{};
    master.key = "sm2_nvram_settings";
    master.desc = "NVRAM Settings";
    master.info = "Let RetroArch manage supported operator settings for the loaded game. When enabled, every displayed value is applied at startup and overrides later Service Menu changes. When disabled, the core does not modify these fields.";
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
    renderer.info = "Auto follows a supported hardware context preferred by the frontend, otherwise software. Vulkan requires Vulkan 1.3; OpenGL requires OpenGL 4.3 core or OpenGL ES 3.1. Reload content after changing this option.";
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
    resolution.info = "Hardware rendering resolution for Vulkan and OpenGL. Tilemaps keep their native detail. Software always uses 496 x 384. Reload content after changing this option.";
    resolution.category_key = "video";
    resolution.values[0] = {"1", "1x (496 x 384)"};
    resolution.values[1] = {"2", "2x (992 x 768)"};
    resolution.values[2] = {"3", "3x (1488 x 1152)"};
    resolution.values[3] = {"4", "4x (1984 x 1536)"};
    resolution.default_value = "1";
    registered_definitions.push_back(resolution);

    retro_core_option_v2_definition texture_filter{};
    texture_filter.key = "sm2_texture_filter";
    texture_filter.desc = "3D Texture Filtering";
    texture_filter.info = "Select the native SM2-Emu texture filter used by the 3D renderer. Faithful preserves the original single-sample path. Anisotropic levels add progressively more samples along oblique surfaces. Applies only to Vulkan and OpenGL and takes effect immediately.";
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
    upscale_2d.info = "Select the native SM2-Emu filter applied to 2D tile layers before they are composited with 3D. Faithful keeps the original crisp pixels; xBR and ScaleFX smooth pixel-art edges. Applies only to Vulkan and OpenGL and takes effect immediately.";
    upscale_2d.category_key = "video";
    upscale_2d.values[0] = {"faithful", "Faithful"};
    upscale_2d.values[1] = {"xbr", "xBR"};
    upscale_2d.values[2] = {"scalefx", "ScaleFX"};
    upscale_2d.default_value = "faithful";
    registered_definitions.push_back(upscale_2d);
#endif

    retro_core_option_v2_definition aspect_ratio{};
    aspect_ratio.key = "sm2_aspect_ratio";
    aspect_ratio.desc = "Aspect Ratio";
    aspect_ratio.info = "Automatic follows the original cabinet configuration stored in NVRAM: Twin cabinets use 4:3 and supported Deluxe cabinets use 16:9. The explicit values override the cabinet setting. Changes take effect immediately.";
    aspect_ratio.category_key = "video";
    aspect_ratio.values[0] = {"auto", "Auto"};
    aspect_ratio.values[1] = {"4_3", "4:3"};
    aspect_ratio.values[2] = {"16_9", "16:9"};
    aspect_ratio.default_value = "auto";
    registered_definitions.push_back(aspect_ratio);

    retro_core_option_v2_definition timing{};
    timing.key = "sm2_av_timing";
    timing.desc = "A/V Timing (Restart Required)";
    timing.info = "Native reports the Model 2 hardware cadence of 57.524160 Hz. 60 Hz Compatibility preserves machine speed while fitting output to a 60 Hz frontend cadence, occasionally duplicating a video frame and packetizing audio at 60 Hz. Reload content after changing this option.";
    timing.category_key = "video";
    timing.values[0] = {"native", "Native (57.524160 Hz)"};
    timing.values[1] = {"60hz", "60 Hz Compatibility"};
    timing.default_value = "native";
    registered_definitions.push_back(timing);

    retro_core_option_v2_definition overlay{};
    overlay.key = "sm2_timing_overlay";
    overlay.desc = "Timing / FPS Overlay";
    overlay.info = "Show 61-frame averages for machine, video and audio work, total retro_run time, worst frame, actual frontend cadence and estimated processing capacity. The frontend renders the status overlay. Takes effect immediately.";
    overlay.category_key = "video";
    overlay.values[0] = {"disabled", "Disabled"};
    overlay.values[1] = {"enabled", "Enabled"};
    overlay.default_value = "disabled";
    registered_definitions.push_back(overlay);

    retro_core_option_v2_definition audio_balance{};
    audio_balance.key = "sm2_audio_balance";
    audio_balance.desc = "Enhanced Audio Balance";
    audio_balance.info = "Apply the SM2-Emu 0.9.7 audio balance automatically to every supported game, including VF2's separate music, effects, announcer and voice levels. Disabled preserves unity gain and the unbalanced emulated output. Changes take effect immediately.";
    audio_balance.category_key = "audio";
    audio_balance.values[0] = {"enabled", "Enabled"};
    audio_balance.values[1] = {"disabled", "Disabled"};
    audio_balance.default_value = "enabled";
    registered_definitions.push_back(audio_balance);

    retro_core_option_v2_definition music_volume{};
    music_volume.key = "sm2_music_volume";
    music_volume.desc = "Music Volume";
    music_volume.info = "Adjust the separate DSB/DSB2 MPEG music volume in supported games. Other audio remains unchanged. Changes take effect immediately.";
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

    retro_core_option_v2_definition crosshair{};
    crosshair.key = "sm2_crosshairs";
    crosshair.desc = "Show Crosshair";
    crosshair.info = "Automatic follows SM2-Emu: Player 1 in serial lightgun games and the game's own reticle in positional-gun games. Explicit player selections also enable the external crosshair for positional guns. Changes take effect immediately.";
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
    crosshair_style.info = "Choose the upstream SM2-Emu circle and cardinal lines or the Supermodel four-wedge vector reticle. Changes take effect immediately.";
    crosshair_style.category_key = "input";
    crosshair_style.values[0] = {"sm2", "SM2-Emu"};
    crosshair_style.values[1] = {"supermodel", "Supermodel"};
    crosshair_style.default_value = "sm2";
    registered_definitions.push_back(crosshair_style);

    retro_core_option_v2_definition gun_input{};
    gun_input.key = "sm2_gun_input";
    gun_input.desc = "Gun Input Mode";
    gun_input.info = "Input source for gun games. Standard accepts RetroArch Lightgun, Mouse, and the left Analog Stick through one virtual cursor. Mouse + Analog Stick excludes Lightgun coordinates while retaining both relative cursor sources. Dedicated modes restrict input to the selected source. Changes take effect immediately.";
    gun_input.category_key = "input";
    gun_input.values[0] = {"hybrid", "Standard"};
    gun_input.values[1] = {"lightgun", "Lightgun Only"};
    gun_input.values[2] = {"mouse_analog", "Mouse + Analog Stick"};
    gun_input.values[3] = {"mouse", "Mouse Only"};
    gun_input.values[4] = {"analog", "Analog Stick Only"};
    gun_input.default_value = "hybrid";
    registered_definitions.push_back(gun_input);

    retro_core_option_v2_definition offscreen_reload{};
    offscreen_reload.key = "sm2_offscreen_reload_shortcut";
    offscreen_reload.desc = "Off-Screen Reload Shortcut";
    offscreen_reload.info = "For Virtua Cop, Virtua Cop 2 and The House of the Dead, enables explicit forced off-screen reload inputs on RetroPad East/LB, Mouse Right and Lightgun Reload. A physical Lightgun off-screen Trigger remains an intrinsic cabinet action and always works. Takes effect immediately.";
    offscreen_reload.category_key = "input";
    offscreen_reload.values[0] = {"enabled", "Enabled"};
    offscreen_reload.values[1] = {"disabled", "Disabled"};
    offscreen_reload.default_value = "enabled";
    registered_definitions.push_back(offscreen_reload);

    retro_core_option_v2_definition mouse_edge_reload{};
    mouse_edge_reload.key = "sm2_mouse_edge_offscreen_reload";
    mouse_edge_reload.desc = "Mouse Edge Off-Screen Reload";
    mouse_edge_reload.info = "Provides off-screen reload when using a Mouse, because the Libretro Mouse interface does not report whether the pointer is off-screen. In Standard, Mouse + Analog Stick or Mouse Only mode, moving the virtual cursor within the outer five percent of the screen and pressing Mouse Left sends an off-screen shot in Virtua Cop, Virtua Cop 2 and The House of the Dead. A Lightgun already supplies its native off-screen status and does not require this option. Takes effect immediately.";
    mouse_edge_reload.category_key = "input";
    mouse_edge_reload.values[0] = {"disabled", "Disabled"};
    mouse_edge_reload.values[1] = {"enabled", "Enabled"};
    mouse_edge_reload.default_value = "disabled";
    registered_definitions.push_back(mouse_edge_reload);

    retro_core_option_v2_definition gamepad_rumble{};
    gamepad_rumble.key = "sm2_gamepad_rumble";
    gamepad_rumble.desc = "Gamepad Rumble";
    gamepad_rumble.info = "Enable the SM2-Emu gamepad vibration model for driving games. Drive-board impacts produce short jolts and steering deflection produces a lighter cornering vibration. Other games remain silent. Changes take effect immediately.";
    gamepad_rumble.category_key = "input";
    gamepad_rumble.values[0] = {"enabled", "Enabled"};
    gamepad_rumble.values[1] = {"disabled", "Disabled"};
    gamepad_rumble.default_value = "enabled";
    registered_definitions.push_back(gamepad_rumble);

    retro_core_option_v2_definition shifter{};
    shifter.key = "sm2_four_speed_shifter";
    shifter.desc = "4-Speed Shifter";
    shifter.info = "Applied only to recognized 4-Speed driving games. H-Gate selects gears from the four diagonal positions of the right analog stick. Standard assigns first through fourth gear to Up, Down, Left and Right. West remains Neutral and L1/R1 remain sequential Shift Down/Up.";
    shifter.category_key = "input";
    shifter.values[0] = {"h_gate", "H-Gate Mode"};
    shifter.values[1] = {"standard", "Standard"};
    shifter.default_value = "h_gate";
    registered_definitions.push_back(shifter);

    retro_core_option_v2_definition automatic_start_gear{};
    automatic_start_gear.key = "sm2_automatic_start_gear";
    automatic_start_gear.desc = "Automatic Start Gear (Restart Required)";
    automatic_start_gear.info = "Use the initial gear declared by SM2-Emu for rolling starts. Enabled starts the Daytona USA family in fourth gear; Disabled starts four-speed games in first gear. Reload content after changing this option.";
    automatic_start_gear.category_key = "input";
    automatic_start_gear.values[0] = {"enabled", "Enabled"};
    automatic_start_gear.values[1] = {"disabled", "Disabled"};
    automatic_start_gear.default_value = "enabled";
    registered_definitions.push_back(automatic_start_gear);

    retro_core_option_v2_definition desert_elevation_control{};
    desert_elevation_control.key = "sm2_desert_elevation_control";
    desert_elevation_control.desc = "Desert Tank Elevation Control";
    desert_elevation_control.info = "Relative treats the centered Left Analog Y axis as a proportional movement control: releasing the stick holds the current turret elevation. Absolute maps the stick position directly to the full elevation range. Applied only to Desert Tank. Changes take effect immediately.";
    desert_elevation_control.category_key = "input";
    desert_elevation_control.values[0] = {"relative", "Relative"};
    desert_elevation_control.values[1] = {"absolute", "Absolute"};
    desert_elevation_control.default_value = "relative";
    registered_definitions.push_back(desert_elevation_control);

    retro_core_option_v2_definition desert_elevation_speed{};
    desert_elevation_speed.key = "sm2_desert_elevation_speed";
    desert_elevation_speed.desc = "Desert Tank Elevation Speed (Relative Only)";
    desert_elevation_speed.info = "Adjust the maximum turret elevation movement speed in Relative mode. Stick displacement continues to control the actual speed proportionally. Applied only to Desert Tank. Changes take effect immediately.";
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
    desert_elevation_axis.desc = "Desert Tank Elevation Axis Mode";
    desert_elevation_axis.info = "Normal maps Left Analog Y up/down to turret elevation up/down. Inverted reverses the axis in both Relative and Absolute control modes. Applied only to Desert Tank. Changes take effect immediately.";
    desert_elevation_axis.category_key = "input";
    desert_elevation_axis.values[0] = {"normal", "Normal"};
    desert_elevation_axis.values[1] = {"inverted", "Inverted"};
    desert_elevation_axis.default_value = "normal";
    registered_definitions.push_back(desert_elevation_axis);

    retro_core_option_v2_definition steering_response{};
    steering_response.key = "sm2_steering_response";
    steering_response.desc = "Driving Steering Response";
    steering_response.info = "Applied only to games recognized as Driving. Select the steering response curve; Progressive and FBNeo Logarithmic reduce sensitivity around the center while retaining the available output range. Changes take effect immediately.";
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
        "Applied only to games recognized as Driving. Scale steering around its center. Values below 100% reduce the emulated wheel range; values above 100% reach full lock with less physical stick travel. The 63% preset maps full travel to the 30-80-D0 ADC range. Changes take effect immediately.", true));
    registered_definitions.push_back(add_output_range(
        "sm2_accelerator_output_range", "Driving Accelerator Output Range",
        "Applied only to games recognized as Driving. Scale the accelerator from its released position. Values below 100% reduce the maximum emulated pedal output; values above 100% reach full output with less physical trigger travel. The 75.3% preset maps full travel to the 00-C0 ADC range. Changes take effect immediately.", false));
    registered_definitions.push_back(add_output_range(
        "sm2_brake_output_range", "Driving Brake Output Range",
        "Applied only to games recognized as Driving. Scale the brake from its released position. Values below 100% reduce the maximum emulated pedal output; values above 100% reach full output with less physical trigger travel. The 75.3% preset maps full travel to the 00-C0 ADC range. Changes take effect immediately.", false));

    registered_definitions.push_back({});
}

inline AVTimingMode av_timing_mode()
{
    retro_variable option{"sm2_av_timing", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option) &&
                   option.value && std::strcmp(option.value, "60hz") == 0
               ? AVTimingMode::Compatibility60Hz
               : AVTimingMode::Native;
}

inline AspectRatioMode aspect_ratio_mode()
{
    retro_variable option{"sm2_aspect_ratio", nullptr};
    if (!(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option))
        || !option.value)
        return AspectRatioMode::Automatic;
    if (std::strcmp(option.value, "4_3") == 0) return AspectRatioMode::FourThree;
    if (std::strcmp(option.value, "16_9") == 0) return AspectRatioMode::SixteenNine;
    return AspectRatioMode::Automatic;
}

inline bool timing_overlay_enabled()
{
    retro_variable option{"sm2_timing_overlay", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option) &&
           option.value && std::strcmp(option.value, "enabled") == 0;
}

inline unsigned crosshair_mask(const rom::GameSpec& game)
{
    retro_variable option{"sm2_crosshairs", nullptr};
    if (!(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option))
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
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
               && option.value && std::strcmp(option.value, "supermodel") == 0
           ? CrosshairStyle::Supermodel
           : CrosshairStyle::Sm2;
}

inline bool initial_nvram_setup_enabled()
{
    retro_variable option{"sm2_initial_nvram_setup", nullptr};
    return !(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool four_speed_h_gate()
{
    retro_variable option{"sm2_four_speed_shifter", nullptr};
    return !(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
             && option.value && std::strcmp(option.value, "standard") == 0);
}

inline bool automatic_start_gear_enabled()
{
    retro_variable option{"sm2_automatic_start_gear", nullptr};
    return !(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool gamepad_rumble_enabled()
{
    retro_variable option{"sm2_gamepad_rumble", nullptr};
    return !(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool audio_balance_enabled()
{
    retro_variable option{"sm2_audio_balance", nullptr};
    return !(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline unsigned music_volume_percent()
{
    retro_variable option{"sm2_music_volume", nullptr};
    const char* value = option_environment
            && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
            && option.value ? option.value : "100";
    return static_cast<unsigned>(std::clamp(std::atoi(value), 0, 200));
}

inline DrivingAnalogOptions driving_analog_options()
{
    const auto value = [](const char* key, const char* fallback) {
        retro_variable option{key, nullptr};
        return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
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
        return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
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
    if (!(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option))
        || !option.value || std::strcmp(option.value, "faithful") == 0)
        return 0;
    const unsigned quality = static_cast<unsigned>(std::atoi(option.value));
    return quality == 2 || quality == 4 || quality == 8 || quality == 16 ? quality : 0;
}

inline unsigned upscale_2d_mode()
{
    retro_variable option{"sm2_upscale_2d", nullptr};
    if (!(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option))
        || !option.value)
        return 0;
    if (std::strcmp(option.value, "xbr") == 0) return 1;
    if (std::strcmp(option.value, "scalefx") == 0) return 2;
    return 0;
}

inline GunInputMode gun_input_mode()
{
    retro_variable option{"sm2_gun_input", nullptr};
    if (!(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option))
        || !option.value)
        return GunInputMode::Hybrid;
    if (std::strcmp(option.value, "lightgun") == 0) return GunInputMode::Lightgun;
    if (std::strcmp(option.value, "mouse") == 0) return GunInputMode::Mouse;
    if (std::strcmp(option.value, "mouse_analog") == 0) return GunInputMode::MouseAnalog;
    if (std::strcmp(option.value, "analog") == 0) return GunInputMode::AnalogSticks;
    return GunInputMode::Hybrid;
}

inline bool offscreen_reload_shortcut_enabled()
{
    retro_variable option{"sm2_offscreen_reload_shortcut", nullptr};
    return !(option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
             && option.value && std::strcmp(option.value, "disabled") == 0);
}

inline bool mouse_edge_offscreen_reload_enabled()
{
    retro_variable option{"sm2_mouse_edge_offscreen_reload", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
        && option.value && std::strcmp(option.value, "enabled") == 0;
}

inline unsigned linked_cabinets(std::string_view game)
{
    const auto* linked_game = linked_cabinet_game(game);
    if (!linked_game) return 1;
    const std::string key = "sm2_linked_cabinets_" + std::string(game);
    retro_variable option{key.c_str(), nullptr};
    if (!option_environment || !option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
        || !option.value || std::strcmp(option.value, "disabled") == 0)
        return 1;
    const unsigned cabinets = static_cast<unsigned>(option.value[0] - '0');
    const unsigned minimum = game == "vonr" ? 3u : 2u;
    return cabinets >= minimum && cabinets <= linked_game->max_cabinets
        && option.value[1] == '\0'
        ? cabinets : 1;
}

inline bool ski_super_g_drive_board_bypass_enabled()
{
    retro_variable option{"sm2_skisuprg_drive_board_bypass", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option)
        && option.value && std::strcmp(option.value, "enabled") == 0;
}

inline bool update_option_visibility()
{
    if (!option_environment) return false;
    const bool master_visible = !nvram::options_for_game(option_game).empty();
    retro_variable enabled_option{"sm2_nvram_settings", nullptr};
    const bool enabled = option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &enabled_option) &&
                         enabled_option.value && std::string(enabled_option.value) == "enabled";
    const bool game_visible = master_visible && enabled;
    retro_core_option_display master{"sm2_nvram_settings", master_visible};
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &master);
    for (const auto& definition : registered_definitions) {
        if (!definition.key || std::strcmp(definition.key, "sm2_nvram_settings") == 0) continue;
        const std::string_view key = definition.key;
        if (key.starts_with("sm2_linked_cabinets_")) {
            retro_core_option_display display{definition.key,
                key.substr(std::strlen("sm2_linked_cabinets_")) == option_content_game};
            option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            continue;
        }
        if (key == "sm2_skisuprg_drive_board_bypass") {
            retro_core_option_display display{definition.key,
                                               option_content_game == "skisuprg"};
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
            game_visible && std::strncmp(definition.key, prefix.c_str(), prefix.size()) == 0};
        option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
    }
    const bool changed = master_visible != nvram_master_visible || game_visible != nvram_game_visible;
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

inline std::vector<std::string> selected_nvram_values(std::string_view game)
{
    std::vector<std::string> selected;
    for (const auto& option : nvram::options_for_game(game)) {
        const std::string key = "sm2_nvram_" + std::string(option.game) + "_" + option.suffix;
        retro_variable variable{key.c_str(), nullptr};
        if (option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) &&
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
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option) &&
           option.value && std::string(option.value) == "enabled";
}

inline void register_core_options(retro_environment_t env)
{
    option_environment = env;
    build_option_definitions();
    static retro_core_option_v2_category categories[] = {
        {"system", "System", "Machine settings stored in battery-backed memory."},
        {"video", "Video", "Renderer, resolution, A/V cadence and diagnostics."},
        {"audio", "Audio", "Sound emulation and per-game mixing."},
        {"input", "Input", "Controller profiles and input response."},
        {nullptr, nullptr, nullptr}};
    unsigned version = 0;
    if (env(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
        retro_core_options_v2 options{categories, registered_definitions.data()};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        retro_core_options_update_display_callback callback{update_option_visibility};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &callback);
        update_option_visibility();
        return;
    }

    legacy_definitions.clear();
    legacy_value_storage.clear();
    legacy_definitions.reserve(registered_definitions.size());
    for (const auto& definition : registered_definitions) {
        if (!definition.key) break;
        std::string line = std::string(definition.desc) + "; ";
        bool first = true;
        for (const auto& value : definition.values) {
            if (!value.value) break;
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
}  // namespace sm2::libretro
