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

}  // namespace sm2::libretro
