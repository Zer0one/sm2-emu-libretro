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
    const auto* steering_response = find_definition("sm2_steering_response");
    const auto* steering_range = find_definition("sm2_steering_output_range");
    const auto* accelerator_range = find_definition("sm2_accelerator_output_range");
    const auto* brake_range = find_definition("sm2_brake_output_range");
    const auto* water_ski_slide_axis = find_definition("sm2_water_ski_slide_axis");
    const auto* ski_swing_axis = find_definition("sm2_ski_super_g_swing_axis");
    const auto* top_skater_curving_axis = find_definition("sm2_top_skater_curving_axis");
    expect(difficulty && std::string_view(difficulty->values[0].label) == "Normal (Default)"
               && std::string_view(difficulty->values[1].label) == "Hard",
           "registered NVRAM values use RetroArch title case without changing keys");
    expect(cabinet && std::string_view(cabinet->default_value) == "deluxe"
               && std::string_view(cabinet->values[2].label) == "Deluxe (Default)",
           "Daytona Cabinet core option defaults to Deluxe");
    expect(manxtt_cabinet && std::string_view(manxtt_cabinet->default_value) == "twin"
               && std::string_view(manxtt_cabinet->values[1].label) == "Twin (Default)",
           "Manx TT Cabinet Type core option defaults to Twin");
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
    expect(water_ski_slide_axis
               && std::string_view(water_ski_slide_axis->default_value) == "inverted"
               && std::string_view(water_ski_slide_axis->values[0].label) == "Inverted"
               && std::string_view(water_ski_slide_axis->values[1].label) == "Normal",
           "Sega Water Ski Slide Axis Mode defaults to Inverted");
    option_values.clear();
    expect(libretro::water_ski_slide_inverted(),
           "Sega Water Ski Slide Axis Mode defaults to Inverted when unset");
    option_values["sm2_water_ski_slide_axis"] = "normal";
    expect(!libretro::water_ski_slide_inverted(),
           "Sega Water Ski Slide Axis Mode reads Normal");
    option_values.clear();
    expect(ski_swing_axis
               && std::string_view(ski_swing_axis->default_value) == "inverted"
               && std::string_view(ski_swing_axis->values[0].label) == "Inverted"
               && std::string_view(ski_swing_axis->values[1].label) == "Normal",
           "Sega Ski Super G Swing Axis Mode defaults to Inverted");
    option_values.clear();
    expect(libretro::ski_super_g_swing_inverted(),
           "Sega Ski Super G Swing Axis Mode defaults to Inverted when unset");
    option_values["sm2_ski_super_g_swing_axis"] = "normal";
    expect(!libretro::ski_super_g_swing_inverted(),
           "Sega Ski Super G Swing Axis Mode reads Normal");
    option_values.clear();
    expect(top_skater_curving_axis
               && std::string_view(top_skater_curving_axis->default_value) == "inverted"
               && std::string_view(top_skater_curving_axis->values[0].label) == "Inverted"
               && std::string_view(top_skater_curving_axis->values[1].label) == "Normal",
           "Top Skater Curving Axis Mode defaults to Inverted");
    expect(libretro::top_skater_curving_inverted(),
           "Top Skater Curving Axis Mode defaults to Inverted when unset");
    option_values["sm2_top_skater_curving_axis"] = "normal";
    expect(!libretro::top_skater_curving_inverted(),
           "Top Skater Curving Axis Mode reads Normal");
    option_values.clear();
    option_values = {
        {"sm2_steering_response", "fbneo"},
        {"sm2_steering_output_range", "140"},
        {"sm2_accelerator_output_range", "75.3"},
        {"sm2_brake_output_range", "80"},
    };
    const auto driving_options = libretro::driving_analog_options();
    expect(driving_options.steering_response == libretro::SteeringResponse::FBNeoLogarithmic
               && driving_options.steering_output_range == 140
               && driving_options.accelerator_output_range_per_mille == 753
               && driving_options.brake_output_range_per_mille == 800,
           "Driving analog options read the frontend values independently");
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
        if (key != "sm2_nvram_settings" && !key.starts_with("sm2_nvram_"))
            expect(option_visibility[definition->key],
                   "all non-NVRAM options remain visible for Daytona");
    }
    {
        const auto options = libretro::nvram::all_options();
        std::set<std::string> keys;
        expect(options.size() == 196, "reviewed NVRAM option catalog has 196 entries");
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
        expect(libretro::nvram::options_for_game("sgt24h").size() == 7,
               "Super GT 24h exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("gunblade").size() == 5,
               "Gunblade NY exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("bel").size() == 3,
               "Behind Enemy Lines exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("hpyagu98").size() == 4,
               "Hanguk Pro Yagu 98 exposes the reviewed option set");
        expect(libretro::nvram::options_for_game("pltkids").size() == 3,
               "Pilot Kids exposes the reviewed option set");

        std::set<std::string> games;
        for (const auto& option : options) games.insert(option.game);
        expect(games.size() == 35, "initial NVRAM catalog covers 35 reviewed parents");
        const std::set<std::pair<std::string, std::string>> offline = {
            {"daytona", "link_id"}, {"manxtt", "link_type"},
            {"overrev", "link_max"}, {"sgt24h", "link_type"},
            {"sgt24h", "link_max"}, {"srallyc", "link_type"},
            {"stcc", "link_type"}, {"von", "network_link_attribute"},
        };
        const std::set<std::pair<std::string, std::string>> title_defaults = {
            {"daytona", "cabinet"},
            {"manxtt", "cabinet_type"},
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
            }
            const auto applied = libretro::nvram::apply(
                game, initial_backup, initial_eeprom, selected);
            expect_game(applied == libretro::nvram::ApplyResult::Changed
                            || applied == libretro::nvram::ApplyResult::Unchanged,
                        game, "initial NVRAM defaults apply before the first frame");
            if (game == "daytona") {
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
        expect(!libretro::initial_nvram::has_template("vcopa"),
               "unverified clone does not reuse a parent template implicitly");
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
           libretro::nvram::ApplyResult::Unsupported, "options are restricted to the parent set");

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
        expect(selections.size() == 7, "Super GT 24h has seven approved selections");
        expect(libretro::nvram::apply("sgt24h", settings_backup, settings_eeprom,
                                     selections) == libretro::nvram::ApplyResult::Changed,
               "Super GT 24h defaults apply to valid native layouts");
        expect(settings_eeprom[0x19] == 2 && settings_eeprom[0x1a] == 1 &&
               settings_eeprom[0x1b] == 1 && settings_eeprom[0x1c] == 0,
               "Super GT 24h defaults to NOT LINK with Link Max 2");
        expect(settings_backup[0x1a] == 1, "Super GT 24h defaults to USA");
        expect(settings_backup[0x14] == 0 && settings_backup[0x1c] == 0 &&
               settings_backup[0x1e] == 0 && settings_backup[0x20] == 0,
               "Super GT 24h gameplay and sound defaults are stored");
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
