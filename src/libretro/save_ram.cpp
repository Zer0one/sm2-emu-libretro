// SPDX-License-Identifier: BSD-3-Clause
#include "save_ram.h"

#include <algorithm>
#include <stdexcept>

namespace sm2::libretro {
namespace {
constexpr std::array<u8, 8> kMagic{'S', 'M', '2', 'S', 'R', 'A', 'M', 0};
constexpr u32 kVersion = 1;
constexpr size_t kVersionOffset = 8;
constexpr size_t kBackupSizeOffset = 12;
constexpr size_t kEepromSizeOffset = 16;
constexpr size_t kPayloadCrcOffset = 20;
constexpr size_t kGameOffset = 24;
constexpr size_t kGameCapacity = 32;

void write_u32(std::span<u8> bytes, size_t offset, u32 value)
{
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<u8>(value >> (8 * i));
}
u32 read_u32(std::span<const u8> bytes, size_t offset)
{
    u32 value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= static_cast<u32>(bytes[offset + i]) << (8 * i);
    return value;
}
bool all_zero(std::span<const u8> bytes)
{
    return std::all_of(bytes.begin(), bytes.end(), [](u8 byte) { return byte == 0; });
}

bool vf2_layout_ready(std::span<const u8> backup)
{
    constexpr size_t version_offset = 0x3306;
    constexpr size_t title_offset = 0x3308;
    constexpr std::string_view title = "VIRTUA FIGHTER 2";
    return backup.size() == kBackupRamSize && backup[version_offset] == 0x18 &&
           backup[version_offset + 1] == 0 &&
           std::equal(title.begin(), title.end(), backup.begin() + title_offset);
}

void update_vf2_crc(std::span<u8> backup)
{
    constexpr size_t settings_offset = 0x3340;
    constexpr size_t settings_size = 29;
    constexpr size_t crc_offset = 0x3302;
    const u16 crc = crc16_ccitt(backup.subspan(settings_offset, settings_size));
    backup[crc_offset] = static_cast<u8>(crc);
    backup[crc_offset + 1] = static_cast<u8>(crc >> 8);
}
}

u16 crc16_ccitt(std::span<const u8> bytes)
{
    u16 crc = 0;
    for (const u8 byte : bytes) {
        crc ^= static_cast<u16>(byte) << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? static_cast<u16>((crc << 1) ^ 0x1021)
                                 : static_cast<u16>(crc << 1);
    }
    return crc;
}

void export_save_ram(std::string_view game, std::span<const u8> backup,
                     std::span<const u8> eeprom, std::span<u8> destination)
{
    if (backup.size() != kBackupRamSize || eeprom.size() != kEepromSize ||
        destination.size() != kSaveRamSize || game.empty() || game.size() >= kGameCapacity)
        throw std::invalid_argument("Invalid frontend save RAM layout");
    std::fill(destination.begin(), destination.end(), 0);
    std::copy(kMagic.begin(), kMagic.end(), destination.begin());
    write_u32(destination, kVersionOffset, kVersion);
    write_u32(destination, kBackupSizeOffset, static_cast<u32>(backup.size()));
    write_u32(destination, kEepromSizeOffset, static_cast<u32>(eeprom.size()));
    std::copy(game.begin(), game.end(), destination.begin() + kGameOffset);
    std::copy(backup.begin(), backup.end(), destination.begin() + kSaveHeaderSize);
    std::copy(eeprom.begin(), eeprom.end(), destination.begin() + kSaveHeaderSize + kBackupRamSize);
    write_u32(destination, kPayloadCrcOffset,
              crc16_ccitt(destination.subspan(kSaveHeaderSize)));
}

SaveImportResult import_save_ram(std::string_view game, std::span<const u8> source,
                                 std::span<u8> backup, std::span<u8> eeprom)
{
    if (source.size() != kSaveRamSize || backup.size() != kBackupRamSize ||
        eeprom.size() != kEepromSize)
        return SaveImportResult::Invalid;
    if (all_zero(source)) return SaveImportResult::Empty;
    if (!std::equal(kMagic.begin(), kMagic.end(), source.begin()) ||
        read_u32(source, kVersionOffset) != kVersion ||
        read_u32(source, kBackupSizeOffset) != kBackupRamSize ||
        read_u32(source, kEepromSizeOffset) != kEepromSize)
        return SaveImportResult::Invalid;
    const auto* game_begin = reinterpret_cast<const char*>(source.data() + kGameOffset);
    const auto* game_end = std::find(game_begin, game_begin + kGameCapacity, '\0');
    const auto stored_game = std::string_view(game_begin, static_cast<size_t>(game_end - game_begin));
    if (stored_game != game ||
        read_u32(source, kPayloadCrcOffset) != crc16_ccitt(source.subspan(kSaveHeaderSize)))
        return SaveImportResult::Invalid;
    std::copy_n(source.begin() + kSaveHeaderSize, kBackupRamSize, backup.begin());
    std::copy_n(source.begin() + kSaveHeaderSize + kBackupRamSize, kEepromSize, eeprom.begin());
    return SaveImportResult::Loaded;
}

OptionApplyResult apply_vf2_country(std::string_view game, std::span<u8> backup,
                                    Vf2Country country)
{
    if (game != "vf2") return OptionApplyResult::NotApplicable;
    constexpr size_t country_offset = 0x3350;
    if (!vf2_layout_ready(backup))
        return OptionApplyResult::LayoutNotReady;

    const u8 value = country == Vf2Country::Japan ? 0 :
                     country == Vf2Country::Usa ? 1 : 2;
    if (backup[country_offset] == value) return OptionApplyResult::Unchanged;
    backup[country_offset] = value;
    update_vf2_crc(backup);
    return OptionApplyResult::Changed;
}

OptionApplyResult apply_vf2_drink(std::string_view game, std::span<u8> backup,
                                  Vf2Drink drink)
{
    if (game != "vf2") return OptionApplyResult::NotApplicable;
    constexpr size_t flags_offset = 0x3351;
    if (!vf2_layout_ready(backup)) return OptionApplyResult::LayoutNotReady;
    const u8 flags = drink == Vf2Drink::Ng
        ? static_cast<u8>(backup[flags_offset] | 0x08)
        : static_cast<u8>(backup[flags_offset] & ~0x08);
    if (backup[flags_offset] == flags) return OptionApplyResult::Unchanged;
    backup[flags_offset] = flags;
    update_vf2_crc(backup);
    return OptionApplyResult::Changed;
}

OptionApplyResult apply_vf2_difficulty(std::string_view game, std::span<u8> backup,
                                       Vf2Difficulty difficulty)
{
    if (game != "vf2") return OptionApplyResult::NotApplicable;
    if (!vf2_layout_ready(backup)) return OptionApplyResult::LayoutNotReady;
    struct Values { u8 difficulty; u8 stage_width; u8 energy_1p; u8 energy_vs; };
    const Values values = difficulty == Vf2Difficulty::Easy ? Values{0, 16, 176, 220} :
                          difficulty == Vf2Difficulty::Normal ? Values{1, 15, 160, 200} :
                          difficulty == Vf2Difficulty::Hard ? Values{2, 14, 144, 180} :
                          Values{3, 13, 128, 160};
    const bool changed = backup[0x3342] != values.difficulty ||
                         backup[0x334f] != values.stage_width ||
                         backup[0x3352] != values.energy_1p ||
                         backup[0x3354] != values.energy_vs;
    if (!changed) return OptionApplyResult::Unchanged;
    backup[0x3342] = values.difficulty;
    backup[0x334f] = values.stage_width;
    backup[0x3352] = values.energy_1p;
    backup[0x3354] = values.energy_vs;
    update_vf2_crc(backup);
    return OptionApplyResult::Changed;
}

OptionApplyResult apply_vf2_display_type(std::string_view game, std::span<u8> backup,
                                         Vf2DisplayType display_type)
{
    if (game != "vf2") return OptionApplyResult::NotApplicable;
    if (!vf2_layout_ready(backup)) return OptionApplyResult::LayoutNotReady;
    const bool crt = display_type == Vf2DisplayType::Crt;
    const u8 flags = crt ? static_cast<u8>(backup[0x3351] | 0x04)
                         : static_cast<u8>(backup[0x3351] & ~0x04);
    const u8 bias = crt ? 117 : 64;
    const u8 gain = crt ? 34 : 37;
    bool changed = backup[0x3351] != flags;
    for (size_t offset = 0x3356; offset <= 0x3358; ++offset)
        changed |= backup[offset] != bias;
    for (size_t offset = 0x3359; offset <= 0x335b; ++offset)
        changed |= backup[offset] != gain;
    changed |= backup[0x335c] != 31;
    if (!changed) return OptionApplyResult::Unchanged;
    backup[0x3351] = flags;
    std::fill(backup.begin() + 0x3356, backup.begin() + 0x3359, bias);
    std::fill(backup.begin() + 0x3359, backup.begin() + 0x335c, gain);
    backup[0x335c] = 31;
    update_vf2_crc(backup);
    return OptionApplyResult::Changed;
}

}  // namespace sm2::libretro
