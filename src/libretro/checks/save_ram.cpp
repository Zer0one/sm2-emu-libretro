// SPDX-License-Identifier: BSD-3-Clause
#include "save_ram.h"
#include "nvram_settings.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <numeric>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace sm2;

int failures = 0;
void expect(bool condition, const char* description)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
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
    {
        const auto options = libretro::nvram::all_options();
        std::set<std::string> keys;
        expect(options.size() == 181, "reviewed NVRAM option catalog has 181 entries");
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
        expect(libretro::nvram::options_for_game("bel").empty() &&
               libretro::nvram::options_for_game("gunblade").empty(),
               "unresolved layouts are excluded from Core Options");
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

    if (failures) return 1;
    std::puts("All Libretro save RAM checks passed");
    return 0;
}
