// SPDX-License-Identifier: BSD-3-Clause
#include "save_ram.h"
#include "core_options.h"
#include "initial_nvram.h"
#include "nvram_settings.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace sm2;

int failures = 0;
std::map<std::string, std::string> option_labels;
std::map<std::string, std::string> option_infos;
std::map<std::string, bool> option_visibility;
std::map<std::string, std::string> option_values;

bool option_environment(unsigned command, void* data)
{
    if (command == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION) {
        *static_cast<unsigned*>(data) = 2;
        return true;
    }
    if (command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) {
        const auto* options = static_cast<retro_core_options_v2*>(data);
        for (auto* definition = options->definitions; definition && definition->key; ++definition) {
            option_labels[definition->key] = definition->desc ? definition->desc : "";
            option_infos[definition->key] = definition->info ? definition->info : "";
        }
        return true;
    }
    if (command == RETRO_ENVIRONMENT_GET_VARIABLE) {
        auto* variable = static_cast<retro_variable*>(data);
        const auto selected = option_values.find(variable->key);
        if (selected != option_values.end()) {
            variable->value = selected->second.c_str();
            return true;
        }
        if (std::string_view(variable->key) == "sm2_nvram_settings") {
            variable->value = "enabled";
            return true;
        }
        return false;
    }
    if (command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY) {
        const auto* display = static_cast<retro_core_option_display*>(data);
        option_visibility[display->key] = display->visible;
        return true;
    }
    return command == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK;
}
void expect(bool condition, const char* description)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
        ++failures;
    }
}

void expect_game(bool condition, std::string_view game, const char* description)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL [%.*s]: %s\n", static_cast<int>(game.size()),
                     game.data(), description);
        ++failures;
    }
}

libretro::SaveRam valid_vf2_save(std::array<u8, libretro::kBackupRamSize>& backup,
                                 std::array<u8, libretro::kEepromSize>& eeprom)
{
    backup.fill(0xff);
    eeprom.fill(0xff);
    backup[0x3306] = 0x18;
    backup[0x3307] = 0;
    constexpr std::string_view title = "VIRTUA FIGHTER 2";
    std::copy(title.begin(), title.end(), backup.begin() + 0x3308);
    const u16 crc = libretro::crc16_ccitt(std::span<const u8>(backup).subspan(0x3340, 29));
    backup[0x3302] = static_cast<u8>(crc);
    backup[0x3303] = static_cast<u8>(crc >> 8);
    libretro::SaveRam save{};
    libretro::export_save_ram("vf2", backup, eeprom, save);
    return save;
}
}

int main()
{
    expect(libretro::retroarch_option_label("LINK ID") == "Link ID",
           "RetroArch option labels use title case and preserve ID");
    expect(libretro::retroarch_option_label("VJCOM DIFFICULTY") == "VJCOM Difficulty",
           "RetroArch option labels preserve game acronyms");
    expect(libretro::retroarch_option_label("ENERGY(1P)") == "Energy (1P)",
           "RetroArch option labels separate parenthesized player abbreviations");
    expect(libretro::retroarch_option_label("ENERGY(VS)") == "Energy (VS)",
           "RetroArch option labels separate parenthesized acronyms");
    expect(libretro::retroarch_option_label("VERY HARD") == "Very Hard"
               && libretro::retroarch_option_label("USA") == "USA"
               && libretro::retroarch_option_label("BLUE (No.2)") == "Blue (No.2)",
           "RetroArch value labels use title case while preserving acronyms and punctuation");
    expect(libretro::retroarch_option_label("URL ADDRESS") == "URL Address",
           "RetroArch option labels preserve URL");
    libretro::register_core_options(option_environment);
    expect(option_labels["sm2_nvram_settings"] == "NVRAM Settings",
           "NVRAM master label matches the Supermodel convention");
    expect(option_labels["sm2_nvram_daytona_link_id"] == "Link ID"
               && option_labels["sm2_nvram_daytona_advertise_sound"] == "Advertise Sound",
           "registered Daytona option labels use RetroArch title case");
    const auto* definitions = libretro::registered_definitions.data();
    const auto find_definition = [definitions](std::string_view key) {
        for (const auto* definition = definitions; definition->key; ++definition)
            if (key == definition->key) return definition;
        return static_cast<const retro_core_option_v2_definition*>(nullptr);
    };
    const auto* difficulty = find_definition("sm2_nvram_daytona_difficulty");
    const auto* cabinet = find_definition("sm2_nvram_daytona_cabinet");
    const auto* manxtt_cabinet = find_definition("sm2_nvram_manxtt_cabinet_type");
    const auto* stcc_cabinet = find_definition("sm2_nvram_stcc_cabinet_type");
    const auto* sgt24h_io_type = find_definition("sm2_nvram_sgt24h_io_type");
    const auto* steering_response = find_definition("sm2_steering_response");
    const auto* steering_range = find_definition("sm2_steering_output_range");
    const auto* accelerator_range = find_definition("sm2_accelerator_output_range");
    const auto* brake_range = find_definition("sm2_brake_output_range");
    const auto* gamepad_rumble = find_definition("sm2_gamepad_rumble");
    const auto* audio_balance = find_definition("sm2_audio_balance");
    const auto* crosshairs = find_definition("sm2_crosshairs");
    const auto* linked_cabinets = find_definition("sm2_linked_cabinets_daytona");
    const auto* stcc_linked_cabinets = find_definition("sm2_linked_cabinets_stcc");
    const auto* rally_linked_cabinets = find_definition("sm2_linked_cabinets_srallyc");
    const auto* indy_linked_cabinets = find_definition("sm2_linked_cabinets_indy500");
    const auto* motor_linked_cabinets = find_definition("sm2_linked_cabinets_motoraid");
    const auto* sgt24h_linked_cabinets = find_definition("sm2_linked_cabinets_sgt24h");
    const auto* overrev_linked_cabinets = find_definition("sm2_linked_cabinets_overrev");
    const auto* manxtt_linked_cabinets = find_definition("sm2_linked_cabinets_manxtt");
    const auto* von_linked_cabinets = find_definition("sm2_linked_cabinets_von");
    const auto* vonr_linked_cabinets = find_definition("sm2_linked_cabinets_vonr");
    const auto* ski_drive_board_bypass =
        find_definition("sm2_skisuprg_drive_board_bypass");
    const auto* nvram_settings = find_definition("sm2_nvram_settings");
    expect(difficulty && std::string_view(difficulty->values[0].label) == "Normal (Default)"
               && std::string_view(difficulty->values[1].label) == "Hard",
           "registered NVRAM values use RetroArch title case without changing keys");
    expect(cabinet && std::string_view(cabinet->default_value) == "deluxe"
               && std::string_view(cabinet->values[2].label) == "Deluxe (Default)",
           "Daytona Cabinet core option defaults to Deluxe");
    expect(manxtt_cabinet && std::string_view(manxtt_cabinet->default_value) == "twin"
               && std::string_view(manxtt_cabinet->values[1].label) == "Twin (Default)",
           "Manx TT Cabinet Type core option defaults to Twin");
    expect(stcc_cabinet && std::string_view(stcc_cabinet->values[1].value) == "delux"
               && std::string_view(stcc_cabinet->values[1].label) == "Deluxe",
           "STCC displays Deluxe without changing its native NVRAM key");
    expect(sgt24h_io_type && std::string_view(sgt24h_io_type->desc) == "I/O Type"
               && std::string_view(sgt24h_io_type->default_value) == "c"
               && std::string_view(sgt24h_io_type->values[2].label) == "C (Default)",
           "Super GT 24h I/O Type core option defaults to C");
    expect(steering_response && std::string_view(steering_response->default_value) == "linear"
               && std::string_view(steering_response->values[1].label)
                    == "Progressive (Fine Center)"
               && std::string_view(steering_response->values[2].label)
                    == "FBNeo Logarithmic (Fine Center)",
           "Driving Steering Response matches the Supermodel convention");
    expect(steering_range && accelerator_range && brake_range
               && std::string_view(steering_range->default_value) == "100"
               && std::string_view(accelerator_range->default_value) == "100"
               && std::string_view(brake_range->default_value) == "100"
               && std::string_view(steering_range->values[0].label) == "50%"
               && std::string_view(steering_range->values[2].label) == "63% (30-80-D0)"
               && std::string_view(steering_range->values[11].label) == "150%"
               && std::string_view(accelerator_range->values[3].label) == "75.3% (00-C0)"
               && std::string_view(brake_range->values[3].label) == "75.3% (00-C0)",
           "Driving analog ranges expose the Supermodel calibration presets");
    expect(gamepad_rumble
               && std::string_view(gamepad_rumble->desc) == "Gamepad Rumble"
               && std::string_view(gamepad_rumble->default_value) == "enabled",
           "Gamepad rumble is enabled by default");
    const auto* automatic_start_gear = find_definition("sm2_automatic_start_gear");
    expect(automatic_start_gear
               && std::string_view(automatic_start_gear->desc)
                    == "Automatic Start Gear (Restart Required)"
               && std::string_view(automatic_start_gear->default_value) == "enabled"
               && std::string_view(automatic_start_gear->values[0].label) == "Enabled"
               && std::string_view(automatic_start_gear->values[1].label) == "Disabled",
           "Automatic Start Gear exposes the upstream rolling-start behaviour");
    expect(audio_balance
               && std::string_view(audio_balance->desc) == "Enhanced Audio Balance"
               && std::string_view(audio_balance->default_value) == "enabled"
               && std::string_view(audio_balance->category_key) == "audio",
           "upstream audio balance is enabled globally by default in Audio");
    expect(crosshairs && std::string_view(crosshairs->default_value) == "auto"
               && std::string_view(crosshairs->values[0].label) == "Automatic",
           "crosshair option uses one game-aware Automatic default");
    expect(linked_cabinets && stcc_linked_cabinets && rally_linked_cabinets
               && indy_linked_cabinets
               && motor_linked_cabinets && sgt24h_linked_cabinets
               && overrev_linked_cabinets && manxtt_linked_cabinets
               && von_linked_cabinets && vonr_linked_cabinets
               && nvram_settings
               && linked_cabinets < stcc_linked_cabinets
               && stcc_linked_cabinets < rally_linked_cabinets
               && rally_linked_cabinets < indy_linked_cabinets
               && indy_linked_cabinets < motor_linked_cabinets
               && motor_linked_cabinets < nvram_settings
               && std::string_view(linked_cabinets->default_value) == "disabled"
               && std::string_view(linked_cabinets->values[0].value) == "disabled"
               && std::string_view(linked_cabinets->values[0].label) == "Disabled"
               && std::string_view(linked_cabinets->values[1].value) == "2"
               && std::string_view(linked_cabinets->values[1].label) == "2 Cabinets"
               && std::string_view(linked_cabinets->values[7].value) == "8"
               && std::string_view(linked_cabinets->values[7].label) == "8 Cabinets"
               && std::string_view(indy_linked_cabinets->values[7].label) == "8 Cabinets"
               && std::string_view(stcc_linked_cabinets->values[8].label) == "9 Cabinets"
               && std::string_view(rally_linked_cabinets->values[4].label) == "5 Cabinets"
               && rally_linked_cabinets->values[5].value == nullptr
               && std::string_view(motor_linked_cabinets->values[3].label) == "4 Cabinets"
               && motor_linked_cabinets->values[4].value == nullptr
               && std::string_view(sgt24h_linked_cabinets->values[3].label) == "4 Cabinets"
               && sgt24h_linked_cabinets->values[4].value == nullptr
               && std::string_view(overrev_linked_cabinets->values[3].label) == "4 Cabinets"
               && overrev_linked_cabinets->values[4].value == nullptr
               && std::string_view(manxtt_linked_cabinets->values[2].label) == "3 Cabinets"
               && manxtt_linked_cabinets->values[3].value == nullptr
               && std::string_view(von_linked_cabinets->values[2].label) == "3 Cabinets"
               && von_linked_cabinets->values[3].value == nullptr
               && std::string_view(vonr_linked_cabinets->values[1].label) == "3 Cabinets"
               && vonr_linked_cabinets->values[2].value == nullptr,
           "Linked Cabinets precedes NVRAM Settings and applies each game's cabinet limit");
    expect(std::string_view(stcc_linked_cabinets->info).find("One optional Relay")
               != std::string_view::npos,
           "STCC Linked Cabinets documents its optional single Relay");
    expect(std::string_view(rally_linked_cabinets->info).find("One optional Relay")
               != std::string_view::npos,
           "Sega Rally Linked Cabinets documents its optional single Relay");
    expect(std::string_view(manxtt_linked_cabinets->info).find(
               "For 3 Cabinets use Master, Slave and one Relay") != std::string_view::npos,
           "Manx TT Linked Cabinets documents its fixed three-participant combination");
    expect(std::string_view(von_linked_cabinets->info).find(
               "one dedicated vonr Relay set") != std::string_view::npos,
           "Virtual On Linked Cabinets documents its dedicated Relay program");
    expect(std::string_view(vonr_linked_cabinets->info).find(
               "valid only in a 3 Cabinets session") != std::string_view::npos,
           "Virtual On Relay documents its only valid participant count");
    expect(std::string_view(linked_cabinets->info).find("Relay is not available")
               != std::string_view::npos,
           "Daytona Linked Cabinets documents that Relay is unavailable");
    expect(ski_drive_board_bypass
               && std::string_view(ski_drive_board_bypass->desc)
                    == "Drive Board Error Bypass (Restart Required)"
               && std::string_view(ski_drive_board_bypass->default_value) == "disabled"
               && std::string_view(ski_drive_board_bypass->values[0].label) == "Disabled"
               && std::string_view(ski_drive_board_bypass->values[1].label) == "Enabled",
           "Sega Ski Super G Drive Board bypass is optional and disabled by default");
    option_values["sm2_linked_cabinets_daytona"] = "2";
    expect(libretro::linked_cabinets("daytona") == 2,
           "Linked Cabinets reads the validated two-cabinet value");
    option_values["sm2_linked_cabinets_daytona"] = "8";
    expect(libretro::linked_cabinets("daytona") == 8,
           "Linked Cabinets reads the eight-cabinet value");
    expect(libretro::linked_cabinets("daytona93") == 1,
           "Daytona 1993 stays excluded because its menu has no link settings");
    option_values["sm2_linked_cabinets_daytona93"] = "4";
    expect(libretro::linked_cabinets("daytona93") == 1,
           "A stale Daytona 1993 option cannot enable linked cabinets");
    option_values["sm2_linked_cabinets_stcc"] = "9";
    expect(libretro::linked_cabinets("stcc") == 9,
           "STCC accepts eight cars plus its Relay cabinet");
    expect(libretro::linked_cabinets("stcca") == 1,
           "STCC clone values remain independent from the parent");
    option_values["sm2_linked_cabinets_srallycc"] = "5";
    expect(libretro::linked_cabinets("srallycc") == 5,
           "Sega Rally revision C accepts four cars plus its Relay cabinet");
    expect(libretro::linked_cabinets("srallycdx") == 1
               && libretro::linked_cabinets("srallycdxa") == 1,
           "Sega Rally Deluxe sets stay excluded from linked play");
    option_values["sm2_linked_cabinets_indy500to"] = "8";
    expect(libretro::linked_cabinets("indy500to") == 8,
           "Indy 500 clone accepts the family's eight-cabinet limit");
    option_values["sm2_linked_cabinets_motoraid"] = "4";
    expect(libretro::linked_cabinets("motoraid") == 4,
           "Motor Raid accepts four linked cabinets");
    option_values["sm2_linked_cabinets_motoraid"] = "5";
    expect(libretro::linked_cabinets("motoraid") == 1,
           "Motor Raid rejects a cabinet count above its hardware limit");
    option_values["sm2_linked_cabinets_sgt24h"] = "4";
    expect(libretro::linked_cabinets("sgt24h") == 4,
           "Super GT 24h accepts its four-cabinet limit");
    option_values["sm2_linked_cabinets_overrevba"] = "4";
    expect(libretro::linked_cabinets("overrevba") == 4,
           "Over Rev Revision A accepts its four-cabinet limit");
    option_values["sm2_linked_cabinets_manxttc"] = "3";
    expect(libretro::linked_cabinets("manxttc") == 3,
           "Manx TT Revision C accepts Master, Slave and Relay");
    expect(libretro::linked_cabinets("manxttdx") == 1,
           "Manx TT Deluxe stays excluded because its menu has no Link Type");
    option_values["sm2_linked_cabinets_von"] = "3";
    option_values["sm2_linked_cabinets_vonr"] = "3";
    expect(libretro::linked_cabinets("von") == 3
               && libretro::linked_cabinets("vonr") == 3,
           "Virtual On Twin and dedicated Relay programs share a three-participant limit");
    option_values["sm2_linked_cabinets_vonr"] = "2";
    expect(libretro::linked_cabinets("vonr") == 1,
           "Virtual On Relay rejects a session without both Twin participants");
    expect(libretro::linked_cabinet_network_family("von") == "von"
               && libretro::linked_cabinet_network_family("vonj") == "von"
               && libretro::linked_cabinet_network_family("vonu") == "von"
               && libretro::linked_cabinet_network_family("vonr") == "von"
               && libretro::linked_cabinet_network_family("daytona") == "daytona",
           "Virtual On programs use one compatible Netpacket family");
    expect(libretro::linked_cabinets("vf2") == 1,
           "Linked Cabinets does not leak into an unrelated game");
    option_values["sm2_linked_cabinets_daytona"] = "enabled";
    expect(libretro::linked_cabinets("daytona") == 1,
           "Linked Cabinets rejects the obsolete Enabled value");
    option_values.erase("sm2_linked_cabinets_daytona");
    option_values.erase("sm2_linked_cabinets_daytona93");
    option_values.erase("sm2_linked_cabinets_stcc");
    option_values.erase("sm2_linked_cabinets_srallycc");
    option_values.erase("sm2_linked_cabinets_indy500to");
    option_values.erase("sm2_linked_cabinets_motoraid");
    option_values.erase("sm2_linked_cabinets_sgt24h");
    option_values.erase("sm2_linked_cabinets_overrevba");
    option_values.erase("sm2_linked_cabinets_manxttc");
    option_values.erase("sm2_linked_cabinets_von");
    option_values.erase("sm2_linked_cabinets_vonr");
    expect(!libretro::ski_super_g_drive_board_bypass_enabled(),
           "Sega Ski Super G Drive Board bypass defaults to disabled");
    option_values["sm2_skisuprg_drive_board_bypass"] = "enabled";
    expect(libretro::ski_super_g_drive_board_bypass_enabled(),
           "Sega Ski Super G Drive Board bypass reads the enabled value");
    option_values.erase("sm2_skisuprg_drive_board_bypass");
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
    const auto* texture_filter = find_definition("sm2_texture_filter");
    const auto* upscale_2d = find_definition("sm2_upscale_2d");
    expect(texture_filter && std::string_view(texture_filter->desc) == "3D Texture Filtering"
               && std::string_view(texture_filter->default_value) == "faithful"
               && std::string_view(texture_filter->values[1].label) == "Anisotropic 2x"
               && std::string_view(texture_filter->values[4].label) == "Anisotropic 16x",
           "3D texture filter exposes the upstream quality levels with a faithful default");
    expect(upscale_2d && std::string_view(upscale_2d->desc) == "2D Layer Upscaling Filter"
               && std::string_view(upscale_2d->default_value) == "faithful"
               && std::string_view(upscale_2d->values[1].label) == "xBR"
               && std::string_view(upscale_2d->values[2].label) == "ScaleFX",
           "2D layer upscaling exposes the upstream filters with a faithful default");
#endif
    option_values = {
        {"sm2_steering_response", "fbneo"},
        {"sm2_steering_output_range", "140"},
        {"sm2_accelerator_output_range", "75.3"},
        {"sm2_brake_output_range", "80"},
        {"sm2_gamepad_rumble", "disabled"},
        {"sm2_audio_balance", "disabled"},
    };
    const auto driving_options = libretro::driving_analog_options();
    expect(driving_options.steering_response == libretro::SteeringResponse::FBNeoLogarithmic
               && driving_options.steering_output_range == 140
               && driving_options.accelerator_output_range_per_mille == 753
               && driving_options.brake_output_range_per_mille == 800,
           "Driving analog options read the frontend values independently");
    expect(!libretro::gamepad_rumble_enabled(),
           "gamepad rumble switch reads the frontend value");
    expect(!libretro::audio_balance_enabled(),
           "per-game audio balance switch reads the frontend value");
    option_values["sm2_texture_filter"] = "8";
    option_values["sm2_upscale_2d"] = "scalefx";
    expect(libretro::texture_filter_quality() == 8 && libretro::upscale_2d_mode() == 2,
           "renderer enhancement options read the selected frontend values");
    option_values["sm2_texture_filter"] = "invalid";
    option_values["sm2_upscale_2d"] = "invalid";
    expect(libretro::texture_filter_quality() == 0 && libretro::upscale_2d_mode() == 0,
           "invalid renderer enhancement values fall back to faithful rendering");
    option_values.clear();
    expect(option_infos["sm2_nvram_daytona_link_id"].find("LINK ID") == std::string::npos
               && option_infos["sm2_nvram_daytona_link_id"].find("daytona's") == std::string::npos,
           "registered option descriptions use frontend-facing text");
    option_visibility.clear();
    libretro::set_option_game("daytona");
    size_t visible_daytona = 0;
    size_t visible_other_games = 0;
    for (const auto& [key, visible] : option_visibility) {
        if (!visible || key == "sm2_nvram_settings") continue;
        if (key.starts_with("sm2_nvram_daytona_")) ++visible_daytona;
        else if (key.starts_with("sm2_nvram_")) ++visible_other_games;
    }
    expect(visible_daytona == 8 && visible_other_games == 0,
           "only the loaded game's NVRAM option group is visible");
    for (const auto* definition = definitions; definition->key; ++definition) {
        const std::string_view key = definition->key;
        if (key != "sm2_nvram_settings" && !key.starts_with("sm2_nvram_")
            && !key.starts_with("sm2_linked_cabinets_")
            && key != "sm2_skisuprg_drive_board_bypass")
            expect(option_visibility[definition->key],
                   "all non-NVRAM options remain visible for Daytona");
    }
    expect(option_visibility["sm2_linked_cabinets_daytona"],
           "Linked Cabinets is visible for the supported Daytona family");
    expect(!option_visibility["sm2_skisuprg_drive_board_bypass"],
           "Sega Ski Super G Drive Board bypass is hidden for Daytona");
    option_visibility.clear();
    libretro::set_option_game("von", "vonr");
    expect(option_visibility["sm2_linked_cabinets_vonr"]
               && !option_visibility["sm2_linked_cabinets_von"],
           "Virtual On Relay exposes only its own Linked Cabinets option");
    option_values["sm2_nvram_settings"] = "disabled";
    std::set<std::string> nvram_games;
    for (const auto& option : libretro::nvram::all_options())
        nvram_games.emplace(option.game);
    for (const auto& game : nvram_games) {
        option_visibility.clear();
        libretro::set_option_game(game);
        for (const auto* definition = definitions; definition->key; ++definition) {
            const std::string_view key = definition->key;
            if (key.starts_with("sm2_nvram_") && key != "sm2_nvram_settings")
                expect_game(!option_visibility[definition->key], game,
                            "NVRAM child option is hidden when NVRAM Settings is disabled");
        }
        for (const auto& linked_game : libretro::linked_cabinet_games) {
            const std::string key = "sm2_linked_cabinets_" + std::string(linked_game.name);
            expect_game(option_visibility[key] == (game == linked_game.name), game,
                        "only the loaded game's Linked Cabinets option is visible");
        }
    }
    option_values.clear();
    libretro::set_option_game("daytona");
    for (const auto& game : libretro::linked_cabinet_games) {
        option_visibility.clear();
        libretro::set_option_game(std::string(game.nvram_game), std::string(game.name));
        for (const auto& linked_game : libretro::linked_cabinet_games) {
            const std::string key = "sm2_linked_cabinets_" + std::string(linked_game.name);
            expect_game(option_visibility[key] == (game.name == linked_game.name), game.name,
                        "parent and clones expose only their own networking option");
        }
    }
    option_visibility.clear();
    libretro::set_option_game("skisuprg", "skisuprg");
    expect(option_visibility["sm2_skisuprg_drive_board_bypass"],
           "Sega Ski Super G exposes its Drive Board bypass");
    option_visibility.clear();
    libretro::set_option_game("vf2", "vf2");
    expect(!option_visibility["sm2_skisuprg_drive_board_bypass"],
           "Sega Ski Super G Drive Board bypass does not leak to other games");
    libretro::set_option_game("daytona");
    {
        const auto options = libretro::nvram::all_options();
        std::set<std::string> keys;
        expect(options.size() == 293, "reviewed NVRAM option catalog has 293 entries");
        for (const auto& option : options) {
            const std::string key = std::string(option.game) + ":" + option.suffix;
            expect(keys.insert(key).second, "NVRAM option keys are unique");
            const auto default_value = std::find_if(option.values, option.values + option.value_count,
                [&](const auto& value) { return std::string_view(value.key) == option.default_value; });
            expect(default_value != option.values + option.value_count,
                   "every NVRAM option default belongs to its value list");
        }
        expect(libretro::nvram::options_for_game("airwlkrs").size() == 4,
               "Air Walkers exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("von").size() == 8,
               "Virtual On exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("sgt24h").size() == 8,
               "Super GT 24h exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("gunblade").size() == 5,
               "Gunblade NY exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("bel").size() == 3,
               "Behind Enemy Lines exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("hpyagu98").size() == 4,
               "Hanguk Pro Yagu 98 exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("pltkids").size() == 3,
               "Pilot Kids exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("daytona93").size() == 4,
               "Daytona USA '93 exposes its reduced Game System menu");
        expect(libretro::nvram::options_for_game("daytonas").size() == 9,
               "Daytona USA Saturn advertisement exposes Promote Saturn");
        expect(libretro::nvram::options_for_game("stccb").size() == 9,
               "STCC revision B exposes its clone-specific Country layout");
        expect(libretro::nvram::options_for_game("stcca").size() == 8,
               "STCC revision A omits Default View from the parent option set");
        expect(libretro::nvram::options_for_game("stcco").size() == 8,
               "STCC older revision omits Default View from the parent option set");
        expect(libretro::nvram::options_for_game("dyndeka2").size() == 3
                   && libretro::nvram::options_for_game("dyndeka2b").size() == 3,
               "Dynamite Deka 2 revisions expose the three reviewed editable settings");
        const auto motoraiddx_options = libretro::nvram::options_for_game("motoraiddx");
        expect(motoraiddx_options.size() == 8,
               "Motor Raid Deluxe omits read-only Engine Volume and adds Cabinet Type");
        static constexpr std::string_view motoraiddx_order[] = {
            "game_difficulty", "race_mode", "enemy_level", "advertise_sound",
            "country", "network_type", "cabinet_id", "cabinet_type",
        };
        expect(std::equal(motoraiddx_options.begin(), motoraiddx_options.end(),
                          std::begin(motoraiddx_order), std::end(motoraiddx_order),
                          [](const auto& option, const auto suffix) {
                              return std::string_view(option.suffix) == suffix;
                          }),
               "Motor Raid Deluxe options follow the acquired Service Menu order");
        expect(libretro::nvram::options_for_game("srallycdx").size() == 4,
               "Sega Rally Deluxe exposes its reduced Game Assignments menu");
        expect(libretro::nvram::options_for_game("srallycdxa").size() == 4,
               "Sega Rally Deluxe revision A exposes its reduced Game Assignments menu");
        expect(libretro::nvram::options_for_game("hotdp").size() == 4,
               "House of the Dead prototype exposes its reviewed option subset");

        std::set<std::string> games;
        for (const auto& option : options) games.insert(option.game);
        expect(games.size() == 50, "initial NVRAM catalog covers 50 reviewed sets");
        const std::set<std::pair<std::string, std::string>> offline = {
            {"daytona", "link_id"}, {"daytonas", "link_id"},
            {"manxtt", "link_type"}, {"motoraiddx", "network_type"}, {"indy500d", "network_type"},
            {"overrev", "link_max"}, {"sgt24h", "link_type"},
            {"srallyc", "link_type"}, {"stcc", "link_type"},
            {"stcca", "link_type"}, {"stccb", "link_type"},
            {"stcco", "link_type"},
            {"von", "network_link_attribute"},
        };
        const std::set<std::pair<std::string, std::string>> title_defaults = {
            {"daytona", "cabinet"}, {"daytona93", "cabinet"},
            {"daytonas", "cabinet"},
            {"manxtt", "cabinet_type"},
            {"sgt24h", "io_type"},
        };
        const std::set<std::pair<std::string, std::string>> compound_defaults = {
            // The visible native default is C.R.T.; selecting it also writes the
            // captured companion calibration fields, so byte identity is not a
            // valid default check for this option.
            {"lastbrnx", "display_type"},
        };
        for (const auto& game : games) {
            expect_game(libretro::initial_nvram::has_template(game), game,
                        "reviewed parent has an initial NVRAM template");
            std::array<u8, libretro::kBackupRamSize> initial_backup{};
            std::array<u8, libretro::kEepromSize> initial_eeprom{};
            expect_game(libretro::initial_nvram::seed(game, initial_backup, initial_eeprom) ==
                            libretro::initial_nvram::SeedResult::Loaded,
                        game, "validated initial NVRAM template decodes");
            const auto game_options = libretro::nvram::options_for_game(game);
            const auto selected = libretro::nvram::initial_values(game);
            expect(selected.size() == game_options.size(),
                   "initial NVRAM selection matches the game option layout");
            for (size_t i = 0; i < game_options.size(); ++i) {
                const std::string suffix = game_options[i].suffix;
                const bool expected = suffix == "country" || suffix == "nation"
                    || offline.contains({game, suffix})
                    || title_defaults.contains({game, suffix});
                expect(selected[i].empty() != expected,
                       "initial NVRAM changes only country, offline and explicit core-default fields");
                if (!expected && !compound_defaults.contains({game, suffix})) {
                    auto default_backup = initial_backup;
                    auto default_eeprom = initial_eeprom;
                    std::vector<std::string> default_selection(game_options.size());
                    default_selection[i] = game_options[i].default_value;
                    const std::string default_description =
                        "Core Option default matches native NVRAM: " + suffix;
                    expect_game(libretro::nvram::apply(
                                    game, default_backup, default_eeprom,
                                    default_selection) ==
                                    libretro::nvram::ApplyResult::Unchanged,
                                game,
                                default_description.c_str());
                }
            }
            const auto applied = libretro::nvram::apply(
                game, initial_backup, initial_eeprom, selected);
            expect_game(applied == libretro::nvram::ApplyResult::Changed
                            || applied == libretro::nvram::ApplyResult::Unchanged,
                        game, "initial NVRAM defaults apply before the first frame");
            if (game == "daytona" || game == "daytona93" || game == "daytonas") {
                expect(initial_backup[0x1a] == 0 && initial_backup[0x9a] == 0,
                       "Daytona initial NVRAM applies DELUXE to both settings banks");
                expect(initial_eeprom[0x1a] == initial_backup[0x1b]
                           && initial_eeprom[0x1b] == initial_backup[0x1a],
                           "Daytona initial NVRAM synchronizes the DELUXE setting to EEPROM");
            } else if (game == "manxtt") {
                expect(initial_eeprom[0x0a] == 1,
                       "Manx TT initial NVRAM applies TWIN cabinet type");
            }
            libretro::SaveRam initial_save{};
            libretro::export_save_ram(game, initial_backup, initial_eeprom, initial_save);
            std::array<u8, libretro::kBackupRamSize> roundtrip_backup{};
            std::array<u8, libretro::kEepromSize> roundtrip_eeprom{};
            expect(libretro::import_save_ram(game, initial_save, roundtrip_backup,
                                             roundtrip_eeprom) ==
                       libretro::SaveImportResult::Loaded
                       && roundtrip_backup == initial_backup
                       && roundtrip_eeprom == initial_eeprom,
                   "initial NVRAM persists through the frontend save container");
        }
        const std::set<std::string_view> compatible_clones = {
            "daytonam", "fvipersa", "fvipersb", "hotdo", "lastbrnxj", "lastbrnxu",
            "overrevb", "overrevba", "pltkidsa", "rchase2a", "srallycb", "srallycc",
            "topskatrj", "topskatru", "topskatruo", "vcopa", "vonj", "vonr", "vonu",
            "zerogunaj", "zerogunj",
        };
        for (const auto game : compatible_clones)
            expect_game(libretro::initial_nvram::can_use_parent_template(game), game,
                        "byte-identical clone may inherit its parent template");

        static constexpr std::pair<std::string_view, std::string_view> parent_catalog_clones[] = {
            {"daytonam", "daytona"}, {"fvipersa", "fvipers"},
            {"fvipersb", "fvipers"}, {"hotdo", "hotd"},
            {"lastbrnxj", "lastbrnx"}, {"lastbrnxu", "lastbrnx"},
            {"overrevb", "overrev"}, {"overrevba", "overrev"},
            {"pltkidsa", "pltkids"}, {"rchase2a", "rchase2"},
            {"srallycb", "srallyc"}, {"srallycc", "srallyc"},
            {"topskatrj", "topskatr"}, {"topskatru", "topskatr"},
            {"topskatruo", "topskatr"}, {"vcopa", "vcop"},
            {"vonj", "von"}, {"vonr", "von"}, {"vonu", "von"},
            {"zerogunaj", "zeroguna"}, {"zerogunj", "zerogun"},
            {"daytonase", "daytona"}, {"indy500to", "indy500"},
            {"manxttc", "manxtt"}, {"sfight", "schamp"},
            {"doaa", "doa"},
            {"doaab", "doa"}, {"doaae", "doa"}, {"doab", "doa"},
            {"dynamcopb", "dynamcop"}, {"dynamcopc", "dynamcop"},
            {"manxttdx", "manxtt"}, {"vf2b", "vf2"},
        };
        expect(std::size(parent_catalog_clones) == 33,
               "33 clone Service Menus reuse the reviewed parent catalog");
        for (const auto& [clone, parent] : parent_catalog_clones) {
            expect_game(libretro::nvram::options_for_game(clone).empty(), clone,
                        "parent-compatible clone does not shadow the parent catalog");
            expect_game(!libretro::nvram::options_for_game(parent).empty(), clone,
                        "parent-compatible clone resolves to a reviewed parent catalog");
        }

        static constexpr std::string_view clone_specific_catalogs[] = {
            "daytona93", "daytonas", "dyndeka2", "dyndeka2b", "hotdp",
            "indy500d", "motoraiddx", "srallycdx", "srallycdxa", "stcca", "stccb",
            "stcco", "vf2a", "vf2o", "vstrikero",
        };
        expect(std::size(clone_specific_catalogs) == 15,
               "15 clone Service Menus use a reviewed clone-specific catalog");
        for (const auto clone : clone_specific_catalogs)
            expect_game(!libretro::nvram::options_for_game(clone).empty(), clone,
                        "clone-specific catalog is available");

        static constexpr std::pair<std::string_view, std::string_view> clone_templates[] = {
            {"daytonase", "daytona"}, {"indy500to", "indy500"},
            {"manxttc", "manxtt"}, {"sfight", "schamp"},
            {"srallycdx", "srallycdx"},
            {"doaa", "doa"}, {"doaab", "doa"}, {"doaae", "doa"}, {"doab", "doa"},
            {"dynamcopb", "dynamcop"}, {"dynamcopc", "dynamcop"},
            {"dyndeka2", "dyndeka2"}, {"dyndeka2b", "dyndeka2b"},
            {"manxttdx", "manxtt"}, {"motoraiddx", "motoraiddx"},
            {"stcca", "stcca"}, {"stcco", "stcco"}, {"vf2b", "vf2"},
            {"daytona93", "daytona93"}, {"daytonas", "daytonas"},
            {"stccb", "stccb"}, {"vf2a", "vf2a"}, {"vf2o", "vf2o"},
            {"indy500d", "indy500d"}, {"vstrikero", "vstrikero"},
            {"srallycdxa", "srallycdxa"}, {"hotdp", "hotdp"},
        };
        for (const auto& [clone, parent] : clone_templates) {
            expect_game(libretro::initial_nvram::has_template(clone), clone,
                        "validated clone has a dedicated initial NVRAM template");
            std::array<u8, libretro::kBackupRamSize> clone_backup{};
            std::array<u8, libretro::kEepromSize> clone_eeprom{};
            expect_game(libretro::initial_nvram::seed(clone, clone_backup, clone_eeprom) ==
                            libretro::initial_nvram::SeedResult::Loaded,
                        clone, "dedicated clone template decodes");
            const auto parent_options = libretro::nvram::options_for_game(parent);
            const auto initial = libretro::nvram::initial_values(parent);
            const auto initial_result = libretro::nvram::apply(
                parent, clone_backup, clone_eeprom, initial);
            expect_game(initial_result == libretro::nvram::ApplyResult::Changed
                            || initial_result == libretro::nvram::ApplyResult::Unchanged,
                        clone, "parent initial defaults apply to clone template");
            expect_game(libretro::nvram::apply(parent, clone_backup, clone_eeprom, initial) ==
                            libretro::nvram::ApplyResult::Unchanged,
                        clone, "clone retains parent initial defaults");
            for (size_t option_index = 0; option_index < parent_options.size(); ++option_index) {
                for (size_t value_index = 0;
                     value_index < parent_options[option_index].value_count; ++value_index) {
                    expect_game(libretro::initial_nvram::seed(
                                    clone, clone_backup, clone_eeprom) ==
                                    libretro::initial_nvram::SeedResult::Loaded,
                                clone, "clone template resets before option value check");
                    std::vector<std::string> selection(parent_options.size());
                    selection[option_index] = parent_options[option_index].values[value_index].key;
                    const auto write = libretro::nvram::apply(
                        parent, clone_backup, clone_eeprom, selection);
                    expect_game(write == libretro::nvram::ApplyResult::Changed
                                    || write == libretro::nvram::ApplyResult::Unchanged,
                                clone, "every parent option value applies to clone template");
                    if (clone == "vf2a" || clone == "vf2o")
                        expect_game(clone_backup[0x3001] == 0xff
                                        && clone_backup[0x3306] == (clone == "vf2a" ? 0x13 : 0x12)
                                        && clone_backup[0x3318] == 0x2f,
                                    clone, "option writes preserve the clone revision bytes");
                    expect_game(libretro::nvram::apply(
                                    parent, clone_backup, clone_eeprom,
                                    std::vector<std::string>(parent_options.size())) ==
                                    libretro::nvram::ApplyResult::Unchanged,
                                clone, "clone integrity remains valid after parent option write");
                }
            }
            if (clone == "vf2a" || clone == "vf2o") {
                clone_backup[0x3318] = 0xce;
                clone_backup[0x3319] = 0x0b;
                expect_game(libretro::nvram::apply(
                                parent, clone_backup, clone_eeprom,
                                std::vector<std::string>(parent_options.size())) ==
                                libretro::nvram::ApplyResult::Unchanged,
                            clone, "runtime header data is not a fixed revision marker");
            }
            if (clone == "vstrikero") {
                expect_game(clone_backup[0x06] == 0x01, clone,
                            "original revision identifier is preserved");
                expect_game(clone_backup[0x21] == 0x02 && clone_backup[0x0a] == 0x00,
                            clone, "native fields outside the selected clone catalog are preserved");
                expect_game(clone_backup[0x08] == 0x0a && clone_backup[0x09] == 0x00,
                            clone, "fixed original-revision integrity marker is preserved");
                expect_game(std::equal(clone_backup.begin(), clone_backup.begin() + 0x80,
                                       clone_backup.begin() + 0x80),
                            clone, "original-revision settings mirror remains synchronized");
                expect_game(parent_options.size() == 9, clone,
                            "original revision omits One Match Mode from Core Options");

                expect_game(libretro::initial_nvram::seed(
                                clone, clone_backup, clone_eeprom) ==
                                libretro::initial_nvram::SeedResult::Loaded,
                            clone, "original revision resets before offset check");
                std::vector<std::string> advertise_selection(parent_options.size());
                const auto advertise = std::find_if(
                    parent_options.begin(), parent_options.end(), [](const auto& option) {
                        return std::string_view(option.suffix) == "advertise_sound";
                    });
                expect_game(advertise != parent_options.end(), clone,
                            "Advertise Sound is exposed");
                if (advertise != parent_options.end())
                    advertise_selection[static_cast<size_t>(
                        advertise - parent_options.begin())] = "off";
                expect_game(libretro::nvram::apply(
                                clone, clone_backup, clone_eeprom,
                                advertise_selection) == libretro::nvram::ApplyResult::Changed,
                            clone, "Advertise Sound applies to the clone layout");
                expect_game(clone_backup[0x17] == 1 && clone_backup[0x97] == 1
                                && clone_backup[0x19] == 0 && clone_backup[0x99] == 0,
                            clone, "Advertise Sound uses the older revision offset only");
            }
            if (clone == "indy500d") {
                std::array<u8, libretro::kBackupRamSize> parent_backup{};
                std::array<u8, libretro::kEepromSize> parent_eeprom{};
                expect(libretro::initial_nvram::seed("indy500", parent_backup, parent_eeprom) ==
                           libretro::initial_nvram::SeedResult::Loaded,
                       "parent comparison template decodes");
                expect_game(libretro::nvram::apply(
                                clone, parent_backup, parent_eeprom,
                                std::vector<std::string>(parent_options.size())) ==
                                libretro::nvram::ApplyResult::LayoutNotReady,
                            clone, "Deluxe rejects the parent's 36-byte bank");
                expect_game(parent_options.size() == 7, clone,
                            "Deluxe omits Engine Volume and Default View");
            }
            if (clone == "srallycdxa") {
                expect_game(clone_eeprom[2] == 0x2c && clone_eeprom[3] == 0,
                            clone, "Deluxe revision A preserves its 44-byte layout");
                expect_game(parent_options.size() == 4, clone,
                            "Deluxe revision A omits Cabinet Type and Link Type");
                expect_game(clone_eeprom[0x0a] == 0 && clone_eeprom[0x0b] == 0,
                            clone, "fields absent from the menu remain at native values");
            }
            if (clone == "hotdp") {
                expect_game(parent_options.size() == 4, clone,
                            "prototype exposes only the four reviewed Core Options");
                const auto blood = std::find_if(
                    parent_options.begin(), parent_options.end(), [](const auto& option) {
                        return std::string_view(option.suffix) == "blood_color";
                    });
                expect_game(blood != parent_options.end() && blood->value_count == 2,
                            clone, "prototype Blood Color exposes only Red and Green");
                expect_game(clone_eeprom[0x1a] == 0 && clone_eeprom[0x1d] == 9
                                && clone_eeprom[0x1f] == 1,
                            clone, "excluded Cabinet, Life and Blowback fields retain native defaults");
                expect_game(std::equal(clone_eeprom.begin() + 0x08,
                                       clone_eeprom.begin() + 0x20,
                                       clone_eeprom.begin() + 0x20),
                            clone, "prototype 24-byte settings bank remains mirrored");
            }
            libretro::SaveRam clone_save{};
            libretro::export_save_ram(clone, clone_backup, clone_eeprom, clone_save);
            expect_game(libretro::import_save_ram(clone, clone_save,
                            clone_backup, clone_eeprom) == libretro::SaveImportResult::Loaded,
                        clone, "dedicated clone NVRAM persists in its own save container");
        }

        {
            std::array<u8, libretro::kBackupRamSize> clone_backup{};
            std::array<u8, libretro::kEepromSize> clone_eeprom{};
            expect_game(libretro::initial_nvram::seed(
                            "motoraiddx", clone_backup, clone_eeprom) ==
                            libretro::initial_nvram::SeedResult::Loaded,
                        "motoraiddx", "dedicated template decodes for cabinet test");
            const auto motor_options = libretro::nvram::options_for_game("motoraiddx");
            std::vector<std::string> selections(motor_options.size());
            const auto cabinet_option = std::find_if(
                motor_options.begin(), motor_options.end(),
                [](const auto& option) {
                    return std::string_view(option.suffix) == "cabinet_type";
                });
            expect_game(cabinet_option != motor_options.end(), "motoraiddx",
                        "Cabinet Type is exposed");
            if (cabinet_option != motor_options.end())
                selections[static_cast<size_t>(
                    cabinet_option - motor_options.begin())] = "twin";
            expect_game(libretro::nvram::apply(
                            "motoraiddx", clone_backup, clone_eeprom, selections) ==
                            libretro::nvram::ApplyResult::Changed,
                        "motoraiddx", "Twin applies to the clone layout");
            expect((clone_eeprom[0x0c] & 0x03) == 0x02
                       && clone_eeprom[0x17] == 0x00
                       && (clone_eeprom[0x30] & 0x03) == 0x02
                       && clone_eeprom[0x3b] == 0x00,
                   "Motor Raid Deluxe Twin matches the acquired compound encoding");
            expect_game(libretro::nvram::apply(
                            "motoraiddx", clone_backup, clone_eeprom,
                            std::vector<std::string>(motor_options.size())) ==
                            libretro::nvram::ApplyResult::Unchanged,
                        "motoraiddx", "Twin keeps checksum and mirror valid");
        }

    }

    std::array<u8, libretro::kBackupRamSize> backup{};
    std::array<u8, libretro::kEepromSize> eeprom{};
    auto save = valid_vf2_save(backup, eeprom);

    std::array<u8, libretro::kBackupRamSize> restored_backup{};
    std::array<u8, libretro::kEepromSize> restored_eeprom{};
    expect(libretro::import_save_ram("vf2", save, restored_backup, restored_eeprom) ==
           libretro::SaveImportResult::Loaded, "versioned save imports");
    expect(restored_backup == backup && restored_eeprom == eeprom, "save round trip preserves payload");
    expect(libretro::import_save_ram("daytona", save, restored_backup, restored_eeprom) ==
           libretro::SaveImportResult::Invalid, "save from another game is rejected");
    save.back() ^= 1;
    expect(libretro::import_save_ram("vf2", save, restored_backup, restored_eeprom) ==
           libretro::SaveImportResult::Invalid, "corrupt payload is rejected");

    valid_vf2_save(backup, eeprom);
    std::vector<std::string> vf2_selections;
    const auto vf2_options = libretro::nvram::options_for_game("vf2");
    for (const auto& option : vf2_options) vf2_selections.emplace_back(option.default_value);
    auto select_vf2 = [&](std::string_view suffix, std::string value) {
        const auto option = std::find_if(vf2_options.begin(), vf2_options.end(),
            [suffix](const auto& candidate) { return suffix == candidate.suffix; });
        expect(option != vf2_options.end(), "requested VF2 option exists");
        if (option != vf2_options.end())
            vf2_selections[static_cast<size_t>(option - vf2_options.begin())] = std::move(value);
    };
    select_vf2("country", "usa");
    select_vf2("drink", "ok");
    select_vf2("difficulty", "hardest");
    select_vf2("display_type", "crt");
    expect(vf2_selections.size() == 8, "VF2 exposes the reviewed option set");
    expect(libretro::nvram::apply("vf2", backup, eeprom, vf2_selections) ==
           libretro::nvram::ApplyResult::Changed, "VF2 reviewed values apply to a valid layout");
    expect((backup[0x3350] & 0x03) == 1, "VF2 defaults to USA");
    expect((backup[0x3351] & 0x08) == 0, "VF2 Country and Drink remain independent");
    expect((backup[0x3342] & 0x03) == 3 && (backup[0x334f] & 0x1f) == 13 &&
           (backup[0x3352] & 0x30) == 0 && (backup[0x3354] & 0x7c) == 0x20,
           "Hardest applies the verified dependent fields");
    expect((backup[0x3351] & 0x04) && (backup[0x3356] & 0x35) == 0x35 &&
           (backup[0x3359] & 0x07) == 0x02,
           "CRT applies the verified calibration fields");
    const u16 vf2_stored_crc = static_cast<u16>(backup[0x3302] | (backup[0x3303] << 8));
    expect(vf2_stored_crc ==
           libretro::crc16_ccitt(std::span<const u8>(backup).subspan(0x3340, 29)),
           "VF2 settings CRC is regenerated");
    expect(libretro::nvram::apply("vf2", backup, eeprom, vf2_selections) ==
           libretro::nvram::ApplyResult::Unchanged, "reapplying VF2 values is idempotent");
    auto invalid = backup;
    invalid[0x3308] = 0;
    expect(libretro::nvram::apply("vf2", invalid, eeprom, vf2_selections) ==
           libretro::nvram::ApplyResult::LayoutNotReady, "unknown VF2 layout is not modified");
    expect(libretro::nvram::apply("vf2a", backup, eeprom, vf2_selections) ==
           libretro::nvram::ApplyResult::LayoutNotReady, "VF2 revision rejects a mismatched parent header");

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        auto make_sega_bank = [&](size_t start) {
            settings_backup[start + 0] = 'S'; settings_backup[start + 1] = 'E';
            settings_backup[start + 2] = 'G'; settings_backup[start + 3] = 'A';
            const u16 crc = libretro::crc16_ccitt(
                std::span<const u8>(settings_backup).subspan(start + 10, 118));
            settings_backup[start + 8] = static_cast<u8>(crc);
            settings_backup[start + 9] = static_cast<u8>(crc >> 8);
        };
        make_sega_bank(0);
        std::copy_n(settings_backup.begin(), 0x80, settings_backup.begin() + 0x80);
        std::vector<std::string> selections;
        for (const auto& option : libretro::nvram::options_for_game("daytona"))
            selections.emplace_back(option.default_value);
        expect(selections.size() == 8, "Daytona exposes the reviewed option set");
        expect(libretro::nvram::apply("daytona", settings_backup, settings_eeprom, selections) ==
               libretro::nvram::ApplyResult::Changed, "Daytona defaults apply to a valid layout");
        expect(settings_backup[0x0b] == 0, "Daytona defaults to SINGLE for offline boot");
        expect(settings_backup[0x1a] == 0, "Daytona defaults to DELUXE cabinet");
        expect(settings_backup[0x1b] == 0, "Daytona defaults to USA");
        expect(std::equal(settings_backup.begin(), settings_backup.begin() + 0x80,
                          settings_backup.begin() + 0x80), "Daytona settings mirror is synchronized");
        expect(static_cast<u16>(settings_backup[8] | (settings_backup[9] << 8)) ==
               libretro::crc16_ccitt(std::span<const u8>(settings_backup).subspan(10, 118)),
               "Daytona CRC is regenerated");
        expect(settings_eeprom[0] == settings_backup[1] &&
               settings_eeprom[1] == settings_backup[0], "Daytona EEPROM copy uses word-swapped order");
    }

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        settings_eeprom[0x08] = 0;
        std::copy_n(settings_eeprom.begin() + 0x08, 36, settings_eeprom.begin() + 0x2c);
        std::vector<std::string> selections;
        for (const auto& option : libretro::nvram::options_for_game("doa"))
            selections.emplace_back(option.default_value);
        expect(libretro::nvram::apply("doa", settings_backup, settings_eeprom, selections) ==
               libretro::nvram::ApplyResult::Changed, "DOA defaults apply to a valid EEPROM layout");
        expect(settings_eeprom[0x1e] == 1, "DOA Nation defaults to USA");
        expect(settings_eeprom[0x08] == static_cast<u8>(std::accumulate(
                   settings_eeprom.begin() + 0x09, settings_eeprom.begin() + 0x2c, 0u)),
               "DOA additive checksum is regenerated");
        expect(std::equal(settings_eeprom.begin() + 0x08, settings_eeprom.begin() + 0x2c,
                          settings_eeprom.begin() + 0x2c), "DOA EEPROM mirror is synchronized");
    }

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        settings_backup[0] = 0x85;
        settings_backup[1] = 0xad;
        settings_backup[0x68] = 0x12;
        settings_backup[0x69] = 0x34;
        settings_backup[0x6a] = 0x56;
        settings_backup[0x6b] = 0x78;
        settings_backup[0x14] = 3;
        settings_backup[0x0a] = 1;
        settings_backup[0x1a] = 0;
        settings_backup[0x1c] = 1;
        settings_backup[0x1e] = 1;
        settings_backup[0x20] = 1;
        settings_eeprom[0x10] = 0x85;
        settings_eeprom[0x11] = 0xad;
        settings_eeprom[0x19] = 4;
        settings_eeprom[0x1a] = 0;
        settings_eeprom[0x1b] = 0;
        settings_eeprom[0x1c] = 3;
        const u16 initial_sum = static_cast<u16>(std::accumulate(
            settings_eeprom.begin() + 0x0a, settings_eeprom.begin() + 0x28, 0u));
        settings_eeprom[0x08] = static_cast<u8>(initial_sum);
        settings_eeprom[0x09] = static_cast<u8>(initial_sum >> 8);
        std::copy_n(settings_eeprom.begin() + 0x08, 32,
                    settings_eeprom.begin() + 0x28);

        std::vector<std::string> selections;
        for (const auto& option : libretro::nvram::options_for_game("sgt24h"))
            selections.emplace_back(option.default_value);
        expect(selections.size() == 8, "Super GT 24h has eight approved selections");
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed,
               "Super GT 24h defaults apply to valid native layouts");
        expect(settings_eeprom[0x19] == 1 && settings_eeprom[0x1a] == 1 &&
               settings_eeprom[0x1b] == 1 && settings_eeprom[0x1c] == 0,
               "Super GT 24h NOT LINK retains the required internal Link Max sentinel");
        expect(settings_backup[0x1a] == 1, "Super GT 24h defaults to USA");
        expect(settings_backup[0x14] == 0 && settings_backup[0x1c] == 0 &&
               settings_backup[0x1e] == 0 && settings_backup[0x20] == 0,
               "Super GT 24h gameplay and sound defaults are stored");
        expect(settings_backup[0x0a] == 0,
               "Super GT 24h defaults to I/O Type C");
        expect(settings_backup[0x68] == 0x12 && settings_backup[0x69] == 0x34 &&
               settings_backup[0x6a] == 0x56 && settings_backup[0x6b] == 0x78,
               "Super GT 24h runtime counter is not treated as integrity data");
        const u16 stored_sum = static_cast<u16>(settings_eeprom[0x08] |
                                                (settings_eeprom[0x09] << 8));
        expect(stored_sum == static_cast<u16>(std::accumulate(
                   settings_eeprom.begin() + 0x0a, settings_eeprom.begin() + 0x28, 0u)),
               "Super GT 24h additive checksum is regenerated");
        expect(std::equal(settings_eeprom.begin() + 0x08,
                          settings_eeprom.begin() + 0x28,
                          settings_eeprom.begin() + 0x28),
               "Super GT 24h EEPROM mirror is synchronized");
        selections[0] = "car_no1_master";
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed
                   && settings_eeprom[0x19] == 2 && settings_eeprom[0x1a] == 0
                   && settings_eeprom[0x1b] == 1 && settings_eeprom[0x1c] == 0,
               "Super GT 24h applies Link Max 2 only for CAR NO1 Master");
        selections[0] = "not_link";
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed
                   && settings_eeprom[0x19] == 1,
               "Super GT 24h restores the NOT LINK sentinel after leaving master mode");
        selections.back() = "a";
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed
                   && settings_backup[0x0a] == 1,
               "Super GT 24h I/O Type A writes its captured NVRAM value");
        selections.back() = "b";
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed
                   && settings_backup[0x0a] == 3,
               "Super GT 24h I/O Type B writes its captured NVRAM value");
        selections.back() = "c";
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed
                   && settings_backup[0x0a] == 0,
               "Super GT 24h I/O Type C writes its captured NVRAM value");
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Unchanged,
               "reapplying Super GT 24h values is idempotent");
    }

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        std::copy_n("SEGAGBNY", 8, settings_eeprom.begin());
        settings_eeprom[0x08] = 0x41;
        settings_eeprom[0x09] = 0x8f;
        settings_eeprom[0x10] = 0x58;
        settings_eeprom[0x14] = 1;
        settings_eeprom[0x17] = 3;
        settings_eeprom[0x19] = 3;
        settings_eeprom[0x1a] = 3;
        settings_eeprom[0x1d] = 1;
        const auto gunblade_checksum = [](std::span<const u8> bytes) {
            u16 crc = 0xffff;
            for (u8 value : bytes) {
                crc ^= static_cast<u16>(value) << 8;
                for (unsigned bit = 0; bit < 8; ++bit)
                    crc = static_cast<u16>((crc & 0x8000) ?
                        (crc << 1) ^ 0x1021 : crc << 1);
            }
            return static_cast<u16>((~crc) ^ 0x1d0f);
        };
        std::vector<std::string> selections;
        for (const auto& option : libretro::nvram::options_for_game("gunblade"))
            selections.emplace_back(option.default_value);
        expect(selections.size() == 5, "Gunblade NY has five approved selections");
        expect(libretro::nvram::apply("gunblade", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed,
               "Gunblade NY defaults apply to a valid native layout");
        expect(settings_eeprom[0x14] == 1 && settings_eeprom[0x15] == 1 &&
               settings_eeprom[0x19] == 3 && settings_eeprom[0x17] == 3 &&
               settings_eeprom[0x1e] == 0,
               "Gunblade NY approved defaults are stored");
        expect(static_cast<u16>(settings_eeprom[0x08] |
                                (settings_eeprom[0x09] << 8)) == 0xef58,
               "Gunblade NY known EEPROM integrity word is regenerated");
        expect(gunblade_checksum(std::span<const u8>(settings_eeprom).subspan(
                   0x10, 0x4a)) == 0xef58,
               "Gunblade NY regenerated integrity word validates independently");
        expect(libretro::nvram::apply("gunblade", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Unchanged,
               "reapplying Gunblade NY values is idempotent");
    }

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        settings_eeprom.fill(0xff);
        std::copy_n("LMC!", 4, settings_eeprom.begin() + 4);
        settings_eeprom[0x13] = 0;
        settings_eeprom[0x15] = 2;
        settings_eeprom[0x22] = 9;
        const auto bel_checksum = [](std::span<const u8> bank) {
            u32 sum = 0x000c;
            for (size_t offset = 2; offset < bank.size(); offset += 2)
                sum += static_cast<u16>(bank[offset] |
                    (static_cast<u16>(bank[offset + 1]) << 8));
            return static_cast<u16>(sum);
        };
        auto first = std::span<u8>(settings_eeprom).first(0x40);
        const u16 initial_checksum = bel_checksum(first);
        first[0] = static_cast<u8>(initial_checksum);
        first[1] = static_cast<u8>(initial_checksum >> 8);
        std::copy_n(first.begin(), first.size(), settings_eeprom.begin() + 0x40);
        std::vector<std::string> selections;
        for (const auto& option : libretro::nvram::options_for_game("bel"))
            selections.emplace_back(option.default_value);
        expect(selections.size() == 3, "Behind Enemy Lines has three approved selections");
        expect(libretro::nvram::apply("bel", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed,
               "Behind Enemy Lines defaults apply to a valid native layout");
        expect(settings_eeprom[0x15] == 1 && settings_eeprom[0x13] == 1 &&
               settings_eeprom[0x22] == 5,
               "Behind Enemy Lines approved defaults are stored");
        expect(std::equal(settings_eeprom.begin(), settings_eeprom.begin() + 0x40,
                          settings_eeprom.begin() + 0x40),
               "Behind Enemy Lines EEPROM mirror is synchronized");
        expect(static_cast<u16>(settings_eeprom[0] | (settings_eeprom[1] << 8)) ==
                   bel_checksum(std::span<const u8>(settings_eeprom).first(0x40)),
               "Behind Enemy Lines additive checksum is regenerated");
        expect(libretro::nvram::apply("bel", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Unchanged,
               "reapplying Behind Enemy Lines values is idempotent");
    }

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        settings_eeprom.fill(0xff);
        const std::array<u8, 4> protection = {0xfa, 0xe3, 0xa6, 0x29};
        std::copy(protection.begin(), protection.end(), settings_eeprom.begin() + 0x08);
        settings_eeprom[0x1c] = 1;
        settings_eeprom[0x1d] = 1;
        settings_eeprom[0x1e] = 8;
        settings_eeprom[0x1f] = 2;
        const auto sega_crc = [](std::span<const u8> bytes) {
            u32 crc = 0xdebdeb00u;
            for (u8 value : bytes) {
                crc = (crc & 0xffffff00u) + value;
                for (unsigned bit = 0; bit < 8; ++bit)
                    crc = (crc & 0x80000000u) ?
                        (crc << 1) + 0x10210000u : crc << 1;
            }
            return static_cast<u16>(crc >> 16);
        };
        const u16 initial_crc = sega_crc(
            std::span<const u8>(settings_eeprom).subspan(0x0e, 26));
        settings_eeprom[0x0c] = static_cast<u8>(initial_crc);
        settings_eeprom[0x0d] = static_cast<u8>(initial_crc >> 8);
        std::copy_n(settings_eeprom.begin() + 0x0c, 28,
                    settings_eeprom.begin() + 0x28);

        const auto options = libretro::nvram::options_for_game("hpyagu98");
        std::vector<std::string> selections;
        for (const auto& option : options) selections.emplace_back(option.default_value);
        auto select = [&](std::string_view suffix, std::string value) {
            const auto option = std::find_if(options.begin(), options.end(),
                [suffix](const auto& candidate) { return suffix == candidate.suffix; });
            expect(option != options.end(), "requested Hanguk Pro Yagu 98 option exists");
            if (option != options.end())
                selections[static_cast<size_t>(option - options.begin())] = std::move(value);
        };
        select("game_difficulty", "hardest");
        select("advertise_sound", "off");
        select("cabinet_type", "megalo");
        select("favorite", "tigers");
        expect(libretro::nvram::apply("hpyagu98", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed,
               "Hanguk Pro Yagu 98 values apply to a valid EEPROM layout");
        expect(settings_eeprom[0x1c] == 3 && settings_eeprom[0x1d] == 0 &&
               settings_eeprom[0x1e] == 7 && settings_eeprom[0x1f] == 1,
               "Hanguk Pro Yagu 98 approved values are stored");
        expect(std::equal(protection.begin(), protection.end(), settings_eeprom.begin() + 0x08),
               "Hanguk Pro Yagu 98 protection prefix is preserved");
        expect(std::equal(settings_eeprom.begin() + 0x0c,
                          settings_eeprom.begin() + 0x28,
                          settings_eeprom.begin() + 0x28),
               "Hanguk Pro Yagu 98 EEPROM mirror is synchronized");
        expect(static_cast<u16>(settings_eeprom[0x0c] |
                                (settings_eeprom[0x0d] << 8)) ==
                   sega_crc(std::span<const u8>(settings_eeprom).subspan(0x0e, 26)),
               "Hanguk Pro Yagu 98 CRC is regenerated");
        expect(libretro::nvram::apply("hpyagu98", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Unchanged,
               "reapplying Hanguk Pro Yagu 98 values is idempotent");
        auto invalid_hpyagu98 = settings_eeprom;
        invalid_hpyagu98[0x08] ^= 1;
        expect(libretro::nvram::apply("hpyagu98", settings_backup, invalid_hpyagu98, selections) ==
                   libretro::nvram::ApplyResult::LayoutNotReady,
               "invalid Hanguk Pro Yagu 98 protection prefix is not modified");
    }

    {
        std::array<u8, libretro::kBackupRamSize> settings_backup{};
        std::array<u8, libretro::kEepromSize> settings_eeprom{};
        settings_eeprom.fill(0xff);
        std::copy_n("S32A", 4, settings_eeprom.begin());
        settings_eeprom[0x11] = 1;
        settings_eeprom[0x12] = 0;
        settings_eeprom[0x17] = 0;
        const auto sega_crc = [](std::span<const u8> bytes) {
            u32 crc = 0xdebdeb00u;
            for (u8 value : bytes) {
                crc = (crc & 0xffffff00u) + value;
                for (unsigned bit = 0; bit < 8; ++bit)
                    crc = (crc & 0x80000000u) ?
                        (crc << 1) + 0x10210000u : crc << 1;
            }
            return static_cast<u16>(crc >> 16);
        };
        const u16 initial_crc = sega_crc(
            std::span<const u8>(settings_eeprom).subspan(0x0a, 26));
        settings_eeprom[0x08] = static_cast<u8>(initial_crc);
        settings_eeprom[0x09] = static_cast<u8>(initial_crc >> 8);
        std::copy_n(settings_eeprom.begin() + 0x08, 28,
                    settings_eeprom.begin() + 0x24);

        const auto options = libretro::nvram::options_for_game("pltkids");
        std::vector<std::string> selections;
        for (const auto& option : options) selections.emplace_back(option.default_value);
        auto select = [&](std::string_view suffix, std::string value) {
            const auto option = std::find_if(options.begin(), options.end(),
                [suffix](const auto& candidate) { return suffix == candidate.suffix; });
            expect(option != options.end(), "requested Pilot Kids option exists");
            if (option != options.end())
                selections[static_cast<size_t>(option - options.begin())] = std::move(value);
        };
        select("difficulty", "more_difficult");
        select("demo_sound", "on");
        select("continue", "off");
        expect(libretro::nvram::apply("pltkids", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed,
               "Pilot Kids values apply to a valid EEPROM layout");
        expect(settings_eeprom[0x11] == 3 && settings_eeprom[0x12] == 1 &&
               settings_eeprom[0x17] == 1,
               "Pilot Kids approved values are stored");
        expect(std::equal(settings_eeprom.begin(), settings_eeprom.begin() + 4, "S32A"),
               "Pilot Kids EEPROM signature is preserved");
        expect(std::equal(settings_eeprom.begin() + 0x08,
                          settings_eeprom.begin() + 0x24,
                          settings_eeprom.begin() + 0x24),
               "Pilot Kids EEPROM mirror is synchronized");
        expect(static_cast<u16>(settings_eeprom[0x08] |
                                (settings_eeprom[0x09] << 8)) ==
                   sega_crc(std::span<const u8>(settings_eeprom).subspan(0x0a, 26)),
               "Pilot Kids CRC is regenerated");
        expect(libretro::nvram::apply("pltkids", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Unchanged,
               "reapplying Pilot Kids values is idempotent");
        auto invalid_pltkids = settings_eeprom;
        invalid_pltkids[0] = 0;
        expect(libretro::nvram::apply("pltkids", settings_backup, invalid_pltkids, selections) ==
                   libretro::nvram::ApplyResult::LayoutNotReady,
               "invalid Pilot Kids signature is not modified");
    }

    if (failures) return 1;
    std::puts("All Libretro save RAM checks passed");
    return 0;
}
