// SPDX-License-Identifier: BSD-3-Clause
#include "save_ram.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

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
    libretro::SaveRam save{};
    libretro::export_save_ram("vf2", backup, eeprom, save);
    return save;
}
}

int main()
{
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

    auto check_country = [&](libretro::Vf2Country country, u8 expected) {
        valid_vf2_save(backup, eeprom);
        backup[0x3351] = 0x08;
        expect(libretro::apply_vf2_country("vf2", backup, country) ==
               libretro::OptionApplyResult::Changed, "VF2 country changes");
        expect(backup[0x3350] == expected, "VF2 country byte matches selection");
        expect((backup[0x3351] & 0x08) != 0, "country selection preserves Drink");
        const u16 stored = static_cast<u16>(backup[0x3302] | (backup[0x3303] << 8));
        expect(stored == libretro::crc16_ccitt(std::span<const u8>(backup).subspan(0x3340, 29)),
               "VF2 settings CRC is updated");
    };
    check_country(libretro::Vf2Country::Japan, 0);
    check_country(libretro::Vf2Country::Usa, 1);
    check_country(libretro::Vf2Country::Export, 2);

    valid_vf2_save(backup, eeprom);
    backup[0x3351] = 0x08;
    expect(libretro::apply_vf2_drink("vf2", backup, libretro::Vf2Drink::Ok) ==
           libretro::OptionApplyResult::Changed, "Drink OK changes independently");
    expect((backup[0x3351] & 0x08) == 0, "Drink OK clears its flag");
    expect(libretro::apply_vf2_drink("vf2", backup, libretro::Vf2Drink::Ng) ==
           libretro::OptionApplyResult::Changed, "Drink NG changes independently");
    expect((backup[0x3351] & 0x08) != 0, "Drink NG sets its flag");
    expect(libretro::apply_vf2_difficulty("vf2", backup, libretro::Vf2Difficulty::Hardest) ==
           libretro::OptionApplyResult::Changed, "Difficulty changes independently");
    expect(backup[0x3342] == 3 && backup[0x334f] == 13 &&
           backup[0x3352] == 128 && backup[0x3354] == 160,
           "Hardest applies verified dependent values");
    expect(libretro::apply_vf2_display_type("vf2", backup, libretro::Vf2DisplayType::Crt) ==
           libretro::OptionApplyResult::Changed, "Display type changes independently");
    expect((backup[0x3351] & 0x04) && backup[0x3356] == 117 &&
           backup[0x3359] == 34 && backup[0x335c] == 31,
           "CRT applies verified calibration values");
    auto invalid = backup;
    invalid[0x3308] = 0;
    expect(libretro::apply_vf2_country("vf2", invalid, libretro::Vf2Country::Usa) ==
           libretro::OptionApplyResult::LayoutNotReady, "unknown VF2 layout is not modified");
    expect(libretro::apply_vf2_country("vf2a", backup, libretro::Vf2Country::Usa) ==
           libretro::OptionApplyResult::NotApplicable, "option is restricted to the vf2 parent");

    if (failures) return 1;
    std::puts("All Libretro save RAM checks passed");
    return 0;
}
