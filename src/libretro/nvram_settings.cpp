// SPDX-License-Identifier: BSD-3-Clause
#include "nvram_settings.h"
#include "save_ram.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>

namespace sm2::libretro::nvram {
namespace {
#include "nvram_settings_data.inc"

bool crc_bank_valid(std::span<const u8> memory, size_t start)
{
    if (memory.size() < start + 0x80) return false;
    const u16 stored = static_cast<u16>(memory[start + 8] |
                                        (static_cast<u16>(memory[start + 9]) << 8));
    return stored == crc16_ccitt(memory.subspan(start + 10, 0x76));
}

void update_crc_bank(std::span<u8> memory, size_t start)
{
    const u16 crc = crc16_ccitt(memory.subspan(start + 10, 0x76));
    memory[start + 8] = static_cast<u8>(crc);
    memory[start + 9] = static_cast<u8>(crc >> 8);
}

bool named_layout_valid(std::span<const u8> backup, std::string_view title,
                        size_t settings_size)
{
    if (backup.size() != kBackupRamSize || settings_size > 32) return false;
    if (backup[0x3306] != 0x18 || backup[0x3307] != 0) return false;
    return std::equal(title.begin(), title.end(), backup.begin() + 0x3308);
}

bool nonblank(std::span<const u8> bytes)
{
    const bool zero = std::all_of(bytes.begin(), bytes.end(), [](u8 b) { return b == 0; });
    const bool erased = std::all_of(bytes.begin(), bytes.end(), [](u8 b) { return b == 0xff; });
    return !zero && !erased;
}

u16 sega_crc_add(std::span<const u8> bytes, u32 initial, bool append_zero)
{
    u32 crc = initial;
    const auto add_byte = [&crc](u8 value) {
        crc = (crc & 0xffffff00u) + value;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80000000u) ? (crc << 1) + 0x10210000u : crc << 1;
    };
    for (u8 value : bytes) add_byte(value);
    if (append_zero) add_byte(0);
    return static_cast<u16>(crc >> 16);
}

u16 sega_crc_or(std::span<const u8> bytes, bool post_shift_test)
{
    u32 crc = 0xdebdeb00u;
    const auto add_byte = [&crc, post_shift_test](u8 value) {
        crc = (crc & 0xffffff00u) | value;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (post_shift_test) {
                crc <<= 1;
                if (crc & 0x80000000u) crc ^= 0x10210000u;
            } else {
                const bool high = crc & 0x80000000u;
                crc <<= 1;
                if (high) crc ^= 0x10210000u;
            }
        }
    };
    for (u8 value : bytes) add_byte(value);
    add_byte(0);
    return static_cast<u16>(crc >> 16);
}

u16 crc16_ccitt_inverted(std::span<const u8> bytes)
{
    u16 crc = 0xffff;
    for (u8 value : bytes) {
        crc ^= static_cast<u16>(value) << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = static_cast<u16>((crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1);
    }
    return static_cast<u16>(~crc);
}

u16 gunblade_crc(std::span<const u8> bytes)
{
    // The title applies a fixed post-transform to the common inverted CCITT result.
    return static_cast<u16>(crc16_ccitt_inverted(bytes) ^ 0x1d0f);
}

u16 virtual_on_crc(std::span<const u8> bytes)
{
    u32 crc = 0xdebdeb00u;
    const auto table = [](u8 index) {
        u16 value = static_cast<u16>(index) << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            value = static_cast<u16>((value & 0x8000) ? (value << 1) ^ 0x1021 : value << 1);
        return value;
    };
    for (u8 value : bytes)
        crc = ((crc + value) << 8) ^ (static_cast<u32>(table(crc >> 24)) << 16);
    return static_cast<u16>(table(crc >> 24) ^ ((crc << 8) >> 16));
}

u16 load_u16(std::span<const u8> memory, size_t offset)
{
    return static_cast<u16>(memory[offset] | (static_cast<u16>(memory[offset + 1]) << 8));
}

void store_u16(std::span<u8> memory, size_t offset, u16 value)
{
    memory[offset] = static_cast<u8>(value);
    memory[offset + 1] = static_cast<u8>(value >> 8);
}

bool mirrored_sega_bank_valid(std::span<const u8> eeprom, size_t first,
                              size_t bank_size, u32 initial, bool append_zero)
{
    const size_t mirror = first + bank_size;
    return mirror + bank_size <= eeprom.size() &&
           std::equal(eeprom.begin() + first, eeprom.begin() + mirror,
                      eeprom.begin() + mirror) &&
           load_u16(eeprom, first) == sega_crc_add(eeprom.subspan(first + 2, bank_size - 2),
                                                   initial, append_zero);
}

bool mirrored_sega_bank_valid(std::span<const u8> eeprom, size_t bank_size,
                              u32 initial, bool append_zero)
{
    return mirrored_sega_bank_valid(eeprom, 0x08, bank_size, initial, append_zero);
}

void update_mirrored_sega_bank(std::span<u8> eeprom, size_t first,
                               size_t bank_size, u32 initial, bool append_zero)
{
    store_u16(eeprom, first, sega_crc_add(eeprom.subspan(first + 2, bank_size - 2),
                                          initial, append_zero));
    std::copy_n(eeprom.begin() + first, bank_size, eeprom.begin() + first + bank_size);
}

void update_mirrored_sega_bank(std::span<u8> eeprom, size_t bank_size,
                               u32 initial, bool append_zero)
{
    update_mirrored_sega_bank(eeprom, 0x08, bank_size, initial, append_zero);
}

bool sgt24h_link_bank_valid(std::span<const u8> eeprom)
{
    constexpr size_t first = 0x08;
    constexpr size_t bank_size = 32;
    constexpr size_t mirror = first + bank_size;
    const u16 checksum = static_cast<u16>(std::accumulate(
        eeprom.begin() + 0x0a, eeprom.begin() + 0x28, 0u));
    return std::equal(eeprom.begin() + first, eeprom.begin() + mirror,
                      eeprom.begin() + mirror) &&
           load_u16(eeprom, first) == checksum && load_u16(eeprom, 0x10) == 0xad85;
}

void update_sgt24h_link_bank(std::span<u8> eeprom)
{
    store_u16(eeprom, 0x08, static_cast<u16>(std::accumulate(
        eeprom.begin() + 0x0a, eeprom.begin() + 0x28, 0u)));
    std::copy_n(eeprom.begin() + 0x08, 32, eeprom.begin() + 0x28);
}

u16 bel_checksum(std::span<const u8> bank)
{
    u32 sum = 0x000c;
    for (size_t offset = 2; offset < bank.size(); offset += 2)
        sum += static_cast<u16>(bank[offset] |
                                (static_cast<u16>(bank[offset + 1]) << 8));
    return static_cast<u16>(sum);
}

bool bel_bank_valid(std::span<const u8> bank)
{
    return bank.size() == 0x40 && load_u16(bank, 0) == bel_checksum(bank);
}

void update_bel_eeprom(std::span<u8> eeprom)
{
    auto bank = eeprom.first(0x40);
    store_u16(bank, 0, bel_checksum(bank));
    std::copy_n(bank.begin(), bank.size(), eeprom.begin() + 0x40);
}

bool layout_ready(std::string_view game, std::span<const u8> backup,
                  std::span<const u8> eeprom)
{
    if (backup.size() != kBackupRamSize || eeprom.size() != kEepromSize) return false;
    if (game == "daytona" || game == "daytona93" || game == "daytonas")
        return std::equal(backup.begin(), backup.begin() + 0x80, backup.begin() + 0x80) &&
               crc_bank_valid(backup, 0) && crc_bank_valid(backup, 0x80);
    if (game == "desert") return crc_bank_valid(backup, 0);
    if (game == "doa") {
        const auto first = eeprom.subspan(0x08, 36);
        const auto mirror = eeprom.subspan(0x2c, 36);
        return std::equal(first.begin(), first.end(), mirror.begin()) &&
               first[0] == static_cast<u8>(std::accumulate(first.begin() + 1, first.end(), 0u));
    }
    if (game == "bel")
        return std::equal(eeprom.begin(), eeprom.begin() + 0x40, eeprom.begin() + 0x40) &&
               std::equal(eeprom.begin() + 4, eeprom.begin() + 8, "LMC!") &&
               bel_bank_valid(eeprom.first(0x40)) && bel_bank_valid(eeprom.subspan(0x40));
    if (game == "gunblade")
        return std::equal(eeprom.begin(), eeprom.begin() + 8, "SEGAGBNY") &&
               load_u16(eeprom, 0x08) == gunblade_crc(eeprom.subspan(0x10, 0x4a)) &&
               load_u16(eeprom, 0x10) == 0x0058;
    if (game == "fvipers")
        return backup[0x3306] == 0x12 && backup[0x3307] == 0
            && std::equal(backup.begin() + 0x3308, backup.begin() + 0x3318,
                          "VIRTUA FIGHTER 2")
            && static_cast<u16>(backup[0x3302] | (static_cast<u16>(backup[0x3303]) << 8)) ==
                crc16_ccitt(backup.subspan(0x3340, 29));
    if (game == "vf2a" || game == "vf2o")
        return backup[0x3306] == (game == "vf2a" ? 0x13 : 0x12)
            && backup[0x3307] == 0
            && std::equal(backup.begin() + 0x3308, backup.begin() + 0x3317,
                          "VIRTUA FIGHTER 2")
            && static_cast<u16>(backup[0x3302] | (static_cast<u16>(backup[0x3303]) << 8)) ==
                crc16_ccitt(backup.subspan(0x3340, 29));
    if (game == "vf2")
        return named_layout_valid(backup, "VIRTUA FIGHTER 2", 29) &&
               static_cast<u16>(backup[0x3302] | (static_cast<u16>(backup[0x3303]) << 8)) ==
                   crc16_ccitt(backup.subspan(0x3340, 29));
    if (game == "schamp" || game == "sfight")
        return nonblank(backup.subspan(0x3340, 32)) &&
               static_cast<u16>(backup[0x3302] | (static_cast<u16>(backup[0x3303]) << 8)) ==
                   crc16_ccitt(backup.subspan(0x3340, 32));
    if (game == "vcop")
        return std::equal(backup.begin(), backup.begin() + 0x80, backup.begin() + 0x80) &&
               backup[0] == 'S' && backup[1] == 'E' && backup[2] == 'G' && backup[3] == 'A';
    if (game == "vstrikero")
        return std::equal(backup.begin(), backup.begin() + 0x80, backup.begin() + 0x80)
            && std::equal(backup.begin(), backup.begin() + 4, "SEGA")
            && backup[0x06] == 0x01 && backup[0x08] == 0x0a && backup[0x09] == 0x00;
    if (game == "vcop2" || game == "vstriker")
        return std::equal(backup.begin(), backup.begin() + 0x80, backup.begin() + 0x80) &&
               crc_bank_valid(backup, 0) && crc_bank_valid(backup, 0x80);
    if (game == "airwlkrs")
        return std::equal(eeprom.begin() + 0x08, eeprom.begin() + 0x20,
                          eeprom.begin() + 0x20) &&
               load_u16(eeprom, 0x08) == sega_crc_or(eeprom.subspan(0x0a, 22), true);
    if (game == "hotdp")
        return mirrored_sega_bank_valid(eeprom, 24, 0xdebdeb00u, true);
    if (game == "dynabb" || game == "dynabb97")
        return mirrored_sega_bank_valid(eeprom, 28, 0xdebdeb00u, false);
    if (game == "hpyagu98") {
        constexpr std::array<u8, 4> protection = {0xfa, 0xe3, 0xa6, 0x29};
        return std::equal(protection.begin(), protection.end(), eeprom.begin() + 0x08) &&
               mirrored_sega_bank_valid(eeprom, 0x0c, 28, 0xdebdeb00u, false);
    }
    if (game == "indy500d")
        return mirrored_sega_bank_valid(eeprom, 44, 0xdebdec00u, false);
    if (game == "indy500" || game == "motoraid" || game == "motoraiddx" ||
        game == "waverunr")
        return mirrored_sega_bank_valid(eeprom, 36, 0xdebdec00u, false);
    if (game == "zerogun" || game == "zeroguna")
        return mirrored_sega_bank_valid(eeprom, 20, 0xdebdeb00u, false);
    if (game == "overrev")
        return mirrored_sega_bank_valid(eeprom, 52, 0xdebdeb00u, false);
    if (game == "pltkids")
        return std::equal(eeprom.begin(), eeprom.begin() + 4, "S32A") &&
               mirrored_sega_bank_valid(eeprom, 28, 0xdebdeb00u, false);
    if (game == "segawski")
        return mirrored_sega_bank_valid(eeprom, 32, 0xdebdec00u, false);
    if (game == "dynamcop" || game == "dyndeka2" || game == "dyndeka2b" ||
        game == "hotd" || game == "skisuprg" || game == "skytargt")
        return mirrored_sega_bank_valid(eeprom, 40, 0xdebdeb00u, true);
    if (game == "lastbrnx")
        return std::equal(eeprom.begin() + 0x08, eeprom.begin() + 0x44,
                          eeprom.begin() + 0x44) &&
               load_u16(eeprom, 0x08) == sega_crc_or(eeprom.subspan(0x0a, 58), false);
    if (game == "manxtt" || game == "manxttdx")
        return eeprom[2] == 0x38 && eeprom[3] == 0 &&
               load_u16(eeprom, 0) == crc16_ccitt_inverted(eeprom.subspan(2, 0x36));
    if (game == "rchase2" || game == "srallycdxa")
        return eeprom[2] == 0x2c && eeprom[3] == 0 &&
               load_u16(eeprom, 0) == crc16_ccitt_inverted(eeprom.subspan(2, 0x2a));
    if (game == "srallyc" || game == "srallycdx")
        return eeprom[2] == 0x24 && eeprom[3] == 0 &&
               load_u16(eeprom, 0) == crc16_ccitt_inverted(eeprom.subspan(2, 0x22));
    if (game == "sgt24h")
        return load_u16(backup, 0) == 0xad85 && sgt24h_link_bank_valid(eeprom);
    if (game == "stcc" || game == "stcca" || game == "stccb" || game == "stcco")
        return std::equal(eeprom.begin(), eeprom.begin() + 4, "SEGA") &&
               load_u16(eeprom, 0x08) == crc16_ccitt_inverted(eeprom.subspan(0x10, 0x70));
    if (game == "topskatr")
        return std::equal(eeprom.begin(), eeprom.begin() + 8, "SEGATPSK") &&
               load_u16(eeprom, 0x08) == crc16_ccitt_inverted(eeprom.subspan(0x10, 0x48));
    if (game == "von")
        return std::equal(eeprom.begin(), eeprom.begin() + 0x3c,
                          eeprom.begin() + 0x3c) &&
               std::equal(backup.begin() + 0x14, backup.begin() + 0x34,
                          backup.begin() + 0x220) &&
               load_u16(eeprom, 0x08) == virtual_on_crc(eeprom.subspan(0x0c, 38)) &&
               load_u16(backup, 0x14) == virtual_on_crc(backup.subspan(0x16, 34));
    return false;
}

void sync_integrity(std::string_view game, std::span<u8> backup,
                    std::span<u8> eeprom)
{
    if (game == "daytona" || game == "daytona93" || game == "daytonas") {
        update_crc_bank(backup, 0);
        std::copy_n(backup.begin(), 0x80, backup.begin() + 0x80);
        for (size_t i = 0; i < 0x80; i += 2) {
            eeprom[i] = backup[i + 1];
            eeprom[i + 1] = backup[i];
        }
    } else if (game == "desert") {
        update_crc_bank(backup, 0);
        for (size_t i = 0; i < 0x80; i += 2) {
            eeprom[i] = backup[i + 1];
            eeprom[i + 1] = backup[i];
        }
    } else if (game == "doa") {
        eeprom[0x08] = static_cast<u8>(std::accumulate(eeprom.begin() + 0x09,
                                                       eeprom.begin() + 0x2c, 0u));
        std::copy_n(eeprom.begin() + 0x08, 36, eeprom.begin() + 0x2c);
    } else if (game == "bel") {
        update_bel_eeprom(eeprom);
    } else if (game == "gunblade") {
        store_u16(eeprom, 0x08, gunblade_crc(eeprom.subspan(0x10, 0x4a)));
    } else if (game == "fvipers" || game == "vf2" || game == "vf2a" || game == "vf2o") {
        const u16 crc = crc16_ccitt(backup.subspan(0x3340, 29));
        backup[0x3302] = static_cast<u8>(crc);
        backup[0x3303] = static_cast<u8>(crc >> 8);
    } else if (game == "schamp" || game == "sfight") {
        const u16 crc = crc16_ccitt(backup.subspan(0x3340, 32));
        backup[0x3302] = static_cast<u8>(crc);
        backup[0x3303] = static_cast<u8>(crc >> 8);
    } else if (game == "vcop") {
        std::copy_n(backup.begin(), 0x80, backup.begin() + 0x80);
    } else if (game == "vstrikero") {
        std::copy_n(backup.begin(), 0x80, backup.begin() + 0x80);
    } else if (game == "vcop2" || game == "vstriker") {
        update_crc_bank(backup, 0);
        std::copy_n(backup.begin(), 0x80, backup.begin() + 0x80);
    } else if (game == "airwlkrs") {
        store_u16(eeprom, 0x08, sega_crc_or(eeprom.subspan(0x0a, 22), true));
        std::copy_n(eeprom.begin() + 0x08, 24, eeprom.begin() + 0x20);
    } else if (game == "hotdp") {
        update_mirrored_sega_bank(eeprom, 24, 0xdebdeb00u, true);
    } else if (game == "dynabb" || game == "dynabb97") {
        update_mirrored_sega_bank(eeprom, 28, 0xdebdeb00u, false);
    } else if (game == "hpyagu98") {
        update_mirrored_sega_bank(eeprom, 0x0c, 28, 0xdebdeb00u, false);
    } else if (game == "indy500d") {
        update_mirrored_sega_bank(eeprom, 44, 0xdebdec00u, false);
    } else if (game == "indy500" || game == "motoraid" || game == "motoraiddx" ||
               game == "waverunr") {
        update_mirrored_sega_bank(eeprom, 36, 0xdebdec00u, false);
    } else if (game == "zerogun" || game == "zeroguna") {
        update_mirrored_sega_bank(eeprom, 20, 0xdebdeb00u, false);
    } else if (game == "overrev") {
        update_mirrored_sega_bank(eeprom, 52, 0xdebdeb00u, false);
    } else if (game == "pltkids") {
        update_mirrored_sega_bank(eeprom, 28, 0xdebdeb00u, false);
    } else if (game == "segawski") {
        update_mirrored_sega_bank(eeprom, 32, 0xdebdec00u, false);
    } else if (game == "dynamcop" || game == "dyndeka2" || game == "dyndeka2b" ||
               game == "hotd" || game == "skisuprg" || game == "skytargt") {
        update_mirrored_sega_bank(eeprom, 40, 0xdebdeb00u, true);
    } else if (game == "lastbrnx") {
        store_u16(eeprom, 0x08, sega_crc_or(eeprom.subspan(0x0a, 58), false));
        std::copy_n(eeprom.begin() + 0x08, 60, eeprom.begin() + 0x44);
    } else if (game == "manxtt" || game == "manxttdx") {
        store_u16(eeprom, 0, crc16_ccitt_inverted(eeprom.subspan(2, 0x36)));
    } else if (game == "rchase2" || game == "srallycdxa") {
        store_u16(eeprom, 0, crc16_ccitt_inverted(eeprom.subspan(2, 0x2a)));
    } else if (game == "srallyc" || game == "srallycdx") {
        store_u16(eeprom, 0, crc16_ccitt_inverted(eeprom.subspan(2, 0x22)));
    } else if (game == "sgt24h") {
        update_sgt24h_link_bank(eeprom);
    } else if (game == "stcc" || game == "stcca" || game == "stccb"
               || game == "stcco") {
        store_u16(eeprom, 0x08, crc16_ccitt_inverted(eeprom.subspan(0x10, 0x70)));
    } else if (game == "topskatr") {
        store_u16(eeprom, 0x08, crc16_ccitt_inverted(eeprom.subspan(0x10, 0x48)));
    } else if (game == "von") {
        store_u16(eeprom, 0x08, virtual_on_crc(eeprom.subspan(0x0c, 38)));
        std::copy_n(eeprom.begin(), 0x3c, eeprom.begin() + 0x3c);
        store_u16(backup, 0x14, virtual_on_crc(backup.subspan(0x16, 34)));
        std::copy_n(backup.begin() + 0x14, 32, backup.begin() + 0x220);
    }
}

}  // namespace

std::span<const Option> all_options() { return kOptions; }

std::span<const Option> options_for_game(std::string_view game)
{
    const auto first = std::find_if(std::begin(kOptions), std::end(kOptions),
                                    [game](const Option& option) { return option.game == game; });
    if (first == std::end(kOptions)) return {};
    const auto last = std::find_if(first, std::end(kOptions),
                                   [game](const Option& option) { return option.game != game; });
    return {first, static_cast<size_t>(last - first)};
}

std::string_view catalog_for_game(std::string_view game, std::string_view parent)
{
    if (!options_for_game(game).empty()) return game;
    return options_for_game(parent).empty() ? std::string_view{} : parent;
}

std::vector<std::string> initial_values(std::string_view game)
{
    const auto options = options_for_game(game);
    std::vector<std::string> selected(options.size());
    static constexpr std::pair<std::string_view, std::string_view> offline[] = {
        {"daytona", "link_id"},
        {"daytonas", "link_id"},
        {"motoraiddx", "network_type"},
        {"indy500d", "network_type"},
        {"manxtt", "link_type"},
        {"overrev", "link_max"},
        {"sgt24h", "link_type"},
        {"srallyc", "link_type"},
        {"stcc", "link_type"},
        {"stcca", "link_type"},
        {"stccb", "link_type"},
        {"stcco", "link_type"},
        {"von", "network_link_attribute"},
    };
    static constexpr std::pair<std::string_view, std::string_view> initial_defaults[] = {
        {"daytona", "cabinet"},
        {"daytona93", "cabinet"},
        {"daytonas", "cabinet"},
        {"manxtt", "cabinet_type"},
        {"sgt24h", "io_type"},
    };
    for (size_t i = 0; i < options.size(); ++i) {
        const std::string_view suffix = options[i].suffix;
        const bool country = suffix == "country" || suffix == "nation";
        const bool safe_offline = std::find(std::begin(offline), std::end(offline),
            std::pair{game, suffix}) != std::end(offline);
        const bool title_default = std::find(std::begin(initial_defaults),
            std::end(initial_defaults), std::pair{game, suffix}) != std::end(initial_defaults);
        if (country || safe_offline || title_default)
            selected[i] = options[i].default_value;
    }
    return selected;
}

ApplyResult apply(std::string_view game, std::span<u8> backup,
                  std::span<u8> eeprom, std::span<const std::string> selections)
{
    const auto options = options_for_game(game);
    if (options.empty() || selections.size() != options.size()) return ApplyResult::Unsupported;
    if (!layout_ready(game, backup, eeprom)) return ApplyResult::LayoutNotReady;

    std::string_view sgt24h_link_type;
    if (game == "sgt24h") {
        for (size_t i = 0; i < options.size(); ++i)
            if (options[i].suffix == std::string_view("link_type"))
                sgt24h_link_type = selections[i];
    }

    bool changed = false;
    for (size_t i = 0; i < options.size(); ++i) {
        const Option& option = options[i];
        if (game == "sgt24h" && option.suffix == std::string_view("link_max")
            && sgt24h_link_type != "car_no1_master")
            continue;
        const auto value = std::find_if(option.values, option.values + option.value_count,
            [&](const Value& candidate) { return selections[i] == candidate.key; });
        if (value == option.values + option.value_count) continue;
        for (size_t patch = 0; patch < option.patch_count; ++patch) {
            const Patch& target = option.patches[patch];
            const bool use_eeprom = target.memory == MemoryKind::Eeprom || game == "doa";
            std::span<u8> memory = use_eeprom ? eeprom : backup;
            const u8 old_value = memory[target.offset];
            const u8 new_value = static_cast<u8>((old_value & ~option.patches[patch].mask) |
                (value->desired[patch] & option.patches[patch].mask));
            changed |= old_value != new_value;
            memory[target.offset] = new_value;
        }
    }
    if (game == "sgt24h" && sgt24h_link_type == "not_link") {
        changed |= eeprom[0x19] != 1;
        eeprom[0x19] = 1;
    }
    if (!changed) return ApplyResult::Unchanged;
    sync_integrity(game, backup, eeprom);
    return ApplyResult::Changed;
}

}  // namespace sm2::libretro::nvram
