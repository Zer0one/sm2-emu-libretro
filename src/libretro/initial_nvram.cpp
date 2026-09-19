// SPDX-License-Identifier: BSD-3-Clause
#include "initial_nvram.h"

#include "save_ram.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace sm2::libretro::initial_nvram {
namespace {

struct Template {
    std::string_view game;
    const u8* encoded;
    size_t encoded_size;
    u16 payload_crc;
};

#include "initial_nvram_templates.inc"

const Template* find(std::string_view game)
{
    const auto found = std::find_if(std::begin(kTemplates), std::end(kTemplates),
        [game](const Template& candidate) { return candidate.game == game; });
    return found == std::end(kTemplates) ? nullptr : found;
}

bool decode(const Template& source, std::span<u8> destination)
{
    size_t input = 0;
    size_t output = 0;
    while (input < source.encoded_size) {
        const u8 command = source.encoded[input++];
        const size_t count = static_cast<size_t>(command & 0x7f) + 1;
        if (output + count > destination.size()) return false;
        if (command & 0x80) {
            if (input >= source.encoded_size) return false;
            std::fill_n(destination.begin() + output, count, source.encoded[input++]);
        } else {
            if (input + count > source.encoded_size) return false;
            std::copy_n(source.encoded + input, count, destination.begin() + output);
            input += count;
        }
        output += count;
    }
    return output == destination.size() && crc16_ccitt(destination) == source.payload_crc;
}

}  // namespace

bool has_template(std::string_view game) { return find(game) != nullptr; }

bool can_use_parent_template(std::string_view game)
{
    static constexpr std::array<std::string_view, 21> compatible_clones = {
        "daytonam", "fvipersa", "fvipersb", "hotdo", "lastbrnxj", "lastbrnxu",
        "overrevb", "overrevba", "pltkidsa", "rchase2a", "srallycb", "srallycc",
        "topskatrj", "topskatru", "topskatruo", "vcopa", "vonj", "vonr", "vonu",
        "zerogunaj", "zerogunj",
    };
    return std::find(compatible_clones.begin(), compatible_clones.end(), game)
        != compatible_clones.end();
}

SeedResult seed(std::string_view game, std::span<u8> backup, std::span<u8> eeprom)
{
    const Template* source = find(game);
    if (!source) return SeedResult::Unsupported;
    if (backup.size() != kBackupRamSize || eeprom.size() != kEepromSize)
        return SeedResult::InvalidTemplate;

    std::array<u8, kBackupRamSize + kEepromSize> payload{};
    if (!decode(*source, payload)) return SeedResult::InvalidTemplate;
    std::copy_n(payload.begin(), backup.size(), backup.begin());
    std::copy_n(payload.begin() + backup.size(), eeprom.size(), eeprom.begin());
    return SeedResult::Loaded;
}

}  // namespace sm2::libretro::initial_nvram
