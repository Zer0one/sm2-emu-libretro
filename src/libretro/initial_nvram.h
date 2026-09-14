// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "core/types.h"

#include <span>
#include <string_view>

namespace sm2::libretro::initial_nvram {

enum class SeedResult { Unsupported, InvalidTemplate, Loaded };

[[nodiscard]] SeedResult seed(std::string_view game, std::span<u8> backup,
                              std::span<u8> eeprom);
[[nodiscard]] bool has_template(std::string_view game);

}  // namespace sm2::libretro::initial_nvram
