// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "core/types.h"

#include <array>
#include <span>
#include <string_view>

namespace sm2::libretro {

constexpr size_t kBackupRamSize = 16 * 1024;
constexpr size_t kEepromSize = 128;
constexpr size_t kSaveHeaderSize = 64;
constexpr size_t kSaveRamSize = kSaveHeaderSize + kBackupRamSize + kEepromSize;
using SaveRam = std::array<u8, kSaveRamSize>;

enum class SaveImportResult { Empty, Loaded, Invalid };

void export_save_ram(std::string_view game, std::span<const u8> backup,
                     std::span<const u8> eeprom, std::span<u8> destination);
[[nodiscard]] SaveImportResult import_save_ram(std::string_view game,
                                               std::span<const u8> source,
                                               std::span<u8> backup,
                                               std::span<u8> eeprom);

[[nodiscard]] u16 crc16_ccitt(std::span<const u8> bytes);

}  // namespace sm2::libretro
