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
#pragma once

#include "core/types.h"

#include <functional>
#include <string>
#include <vector>

namespace sm2 {
class Archive;
}

// Shared save-state file I/O for all four boards.
//
// Every board's save_state/load_state is the same envelope around its own
// serialize(Archive&): write a header + payload on save; on load validate the
// header (magic/version/game/board), then apply with a snapshot-rollback so a
// truncated or corrupt file leaves the running machine untouched (requirement
// 3.2). Only the per-board serialize differs, so it is passed in as a visitor
// and the envelope lives here once rather than being copied per board.

namespace sm2::hw {

/// A board's serialize walk. Called for both save and load; the Archive's mode
/// decides the direction.
using SerializeFn = std::function<void(Archive&)>;

/// Build a complete SM2STATE image in memory. This is the frontend-neutral
/// path used by Libretro; file saves use the same function before writing.
[[nodiscard]] bool save_state_to_buffer(std::vector<u8>& out,
                                        const std::string& game, u32 board,
                                        bool in_frame,
                                        const SerializeFn& serialize);

/// Validate and apply an in-memory SM2STATE image transactionally. A rejected
/// or truncated image leaves the running machine untouched.
[[nodiscard]] bool load_state_from_buffer(const u8* data, usize size,
                                          const std::string& game, u32 board,
                                          bool in_frame,
                                          const SerializeFn& serialize);

/// Write `serialize`'s payload, behind an SM2STATE header keyed by `game` /
/// `board`, to `path`. `in_frame` must be false (save only between frames).
/// Returns false (logged) on a bad state or an I/O error.
[[nodiscard]] bool save_state_to_file(const std::string& path, const std::string& game,
                                      u32 board, bool in_frame,
                                      const SerializeFn& serialize);

/// Validate and apply a state file at `path` into the live machine via
/// `serialize`. Rejects (logged, machine untouched) on header mismatch against
/// `game`/`board`, on `in_frame`, or on a truncated payload (rolled back).
/// Returns false on any rejection. On success the caller still runs its own
/// fix-ups (generation bumps etc.).
[[nodiscard]] bool load_state_from_file(const std::string& path, const std::string& game,
                                        u32 board, bool in_frame,
                                        const SerializeFn& serialize);

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
// A game gets one reserved quick-save slot plus a few numbered slots. Files are
// named `<game>.<slot>.sm2state` under the states directory (data dir / states).

/// The reserved quick-save slot name.
inline constexpr const char* kQuickSlot = "quick";

/// How many numbered slots ("1".."kNumberedSlots") a game has.
inline constexpr int kNumberedSlots = 4;

/// Absolute path of one slot's file: `<dir>/<game>.<slot>.sm2state`. `dir` is
/// the states directory (resolve_default_paths derives it as data dir/states).
[[nodiscard]] std::string state_slot_path(const std::string& dir, const std::string& game,
                                          const std::string& slot);

/// One slot's status for the overlay.
struct SlotInfo {
    std::string slot;                ///< "quick", "1", ...
    bool        occupied = false;
    std::string timestamp;           ///< local "YYYY-MM-DD HH:MM" when occupied, else empty
};

/// The quick slot followed by the numbered slots, each with its occupied state
/// and (where occupied) a local-time modification stamp, for `game` under `dir`.
[[nodiscard]] std::vector<SlotInfo> list_state_slots(const std::string& dir,
                                                     const std::string& game);

/// Delete one slot's file. Returns true if the file was removed; false (logged)
/// on an I/O error. A missing file is treated as success (nothing to do).
[[nodiscard]] bool delete_state_slot(const std::string& dir, const std::string& game,
                                     const std::string& slot);

}  // namespace sm2::hw
