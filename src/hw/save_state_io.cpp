//  ____  __  __  ____         _____ __  __ _   _
// / ___||  \/  ||___ \       | ____|  \/  | | | |
// \___ \| |\/| |  __) |_____ |  _| | |\/| | | | |
//  ___) | |  | | / __/|_____|| |___| |  | | |_| |
// |____/|_|  |_||_____|      |_____|_|  |_|\___/
//
// A Sega Model 2 arcade emulator.
// Copyright (c) 2025+ Daniel Martin (dmanlfc)
// SPDX-License-Identifier: BSD-3-Clause
//
// This header must not be removed. The source files in this project may not be
// used to contribute to commercial projects or for monetary gain without the
// express written permission of the author.
//
#include "hw/save_state_io.h"

#include "core/archive.h"
#include "core/log.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <vector>

namespace sm2::hw {

bool save_state_to_buffer(std::vector<u8>& out, const std::string& game, u32 board,
                          bool in_frame, const SerializeFn& serialize)
{
    if (in_frame) {
        SM2_ERROR("save_state called mid-frame; only allowed between frames");
        return false;
    }

    out.clear();
    Archive       ar(out);
    state::Header header;
    header.game  = game;
    header.board = board;
    state::write_header(ar, header);
    serialize(ar);
    return true;
}

bool load_state_from_buffer(const u8* data, usize size, const std::string& game,
                            u32 board, bool in_frame, const SerializeFn& serialize)
{
    if (in_frame) {
        SM2_ERROR("load_state called mid-frame; only allowed between frames");
        return false;
    }
    if (data == nullptr || size == 0) {
        SM2_ERROR("save-state: empty input buffer");
        return false;
    }

    state::Header parsed;
    usize         payload_offset = 0;
    if (!state::read_header(data, size, parsed, payload_offset)) {
        return false;
    }
    if (parsed.game != game) {
        SM2_ERROR("save-state: image is for '%s', running '%s' — refused",
                  parsed.game.c_str(), game.c_str());
        return false;
    }
    if (parsed.board != board) {
        SM2_ERROR("save-state: board mismatch — refused");
        return false;
    }

    // A scratch machine is not viable (create_machine consumes the RomSet),
    // so snapshot the current state and roll it back if the incoming payload
    // runs past its end.
    std::vector<u8> rollback;
    {
        Archive save_ar(rollback);
        serialize(save_ar);
    }

    Archive load_ar(data + payload_offset, size - payload_offset);
    serialize(load_ar);
    if (load_ar.failed()) {
        SM2_ERROR("save-state: image is truncated or corrupt — rolling back");
        Archive restore_ar(rollback.data(), rollback.size());
        serialize(restore_ar);
        return false;
    }
    return true;
}

bool save_state_to_file(const std::string& path, const std::string& game, u32 board,
                        bool in_frame, const SerializeFn& serialize)
{
    std::vector<u8> buffer;
    if (!save_state_to_buffer(buffer, game, board, in_frame, serialize)) {
        return false;
    }

    // Create the parent directory (e.g. the states dir) on first use, the way
    // the screenshot path does; fopen("wb") will not make it.
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }

    std::FILE* handle = std::fopen(path.c_str(), "wb");
    if (handle == nullptr) {
        SM2_ERROR("save-state: could not open '%s' for writing", path.c_str());
        return false;
    }
    const usize written = std::fwrite(buffer.data(), 1, buffer.size(), handle);
    std::fclose(handle);
    if (written != buffer.size()) {
        SM2_ERROR("save-state: short write to '%s'", path.c_str());
        return false;
    }
    SM2_INFO("save-state: wrote %zu bytes to '%s'", buffer.size(), path.c_str());
    return true;
}

bool load_state_from_file(const std::string& path, const std::string& game, u32 board,
                          bool in_frame, const SerializeFn& serialize)
{
    if (in_frame) {
        SM2_ERROR("load_state called mid-frame; only allowed between frames");
        return false;
    }
    std::FILE* handle = std::fopen(path.c_str(), "rb");
    if (handle == nullptr) {
        SM2_ERROR("save-state: could not open '%s' for reading", path.c_str());
        return false;
    }
    std::fseek(handle, 0, SEEK_END);
    const long size = std::ftell(handle);
    std::fseek(handle, 0, SEEK_SET);
    if (size <= 0) {
        std::fclose(handle);
        SM2_ERROR("save-state: '%s' is empty or unreadable", path.c_str());
        return false;
    }
    std::vector<u8> buffer(static_cast<usize>(size));
    const usize     read = std::fread(buffer.data(), 1, buffer.size(), handle);
    std::fclose(handle);
    if (read != buffer.size()) {
        SM2_ERROR("save-state: short read from '%s'", path.c_str());
        return false;
    }

    if (!load_state_from_buffer(buffer.data(), buffer.size(), game, board,
                                in_frame, serialize)) {
        return false;
    }

    SM2_INFO("save-state: loaded '%s'", path.c_str());
    return true;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

std::string state_slot_path(const std::string& dir, const std::string& game,
                            const std::string& slot)
{
    return (std::filesystem::path(dir) / (game + "." + slot + ".sm2state")).string();
}

std::vector<SlotInfo> list_state_slots(const std::string& dir, const std::string& game)
{
    std::vector<SlotInfo> slots;
    slots.reserve(1 + kNumberedSlots);

    auto add = [&](const std::string& name) {
        SlotInfo info;
        info.slot = name;
        std::error_code ec;
        const std::filesystem::path p = state_slot_path(dir, game, name);
        if (std::filesystem::exists(p, ec)) {
            info.occupied = true;
            const auto ftime = std::filesystem::last_write_time(p, ec);
            if (!ec) {
                // file_clock -> system_clock -> time_t, portably enough for a
                // display stamp (exact epoch alignment does not matter here).
                const auto sctime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now()
                    + std::chrono::system_clock::now());
                const std::time_t t = std::chrono::system_clock::to_time_t(sctime);
                std::tm tm{};
#if defined(_WIN32)
                localtime_s(&tm, &t);
#else
                localtime_r(&t, &tm);
#endif
                char stamp[32];
                std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);
                info.timestamp = stamp;
            }
        }
        slots.push_back(std::move(info));
    };

    add(kQuickSlot);
    for (int i = 1; i <= kNumberedSlots; ++i) {
        add(std::to_string(i));
    }
    return slots;
}

bool delete_state_slot(const std::string& dir, const std::string& game,
                       const std::string& slot)
{
    const std::filesystem::path p = state_slot_path(dir, game, slot);
    std::error_code ec;
    std::filesystem::remove(p, ec);  // false + no error means it was already gone
    if (ec) {
        SM2_ERROR("save-state: could not delete '%s': %s", p.string().c_str(),
                  ec.message().c_str());
        return false;
    }
    return true;
}

}  // namespace sm2::hw
