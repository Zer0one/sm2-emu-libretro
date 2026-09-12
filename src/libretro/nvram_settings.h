// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "core/types.h"

#include <span>
#include <string>
#include <string_view>

namespace sm2::libretro::nvram {

struct Value {
    const char* key;
    const char* label;
    const u8* desired;
};

enum class MemoryKind { BackupRam, Eeprom };

struct Patch {
    size_t offset;
    u8 mask;
    MemoryKind memory = MemoryKind::BackupRam;
};

struct Option {
    const char* game;
    const char* suffix;
    const char* label;
    const char* description;
    const char* default_value;
    const Value* values;
    size_t value_count;
    const Patch* patches;
    size_t patch_count;
};

enum class ApplyResult { Unsupported, LayoutNotReady, Unchanged, Changed };

[[nodiscard]] std::span<const Option> all_options();
[[nodiscard]] std::span<const Option> options_for_game(std::string_view game);
[[nodiscard]] ApplyResult apply(std::string_view game, std::span<u8> backup,
                                std::span<u8> eeprom,
                                std::span<const std::string> selections);

}  // namespace sm2::libretro::nvram
