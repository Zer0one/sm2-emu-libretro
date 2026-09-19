// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"
#include "osd/drive_command.h"
#include "rom/game.h"

#include <span>

namespace sm2::libretro {

/// Adapts SM2-Emu's gamepad-rumble model to the frontend-owned Libretro motors.
/// The game provides one cabinet/drive-board stream, so it targets player one.
class GamepadRumble {
public:
    [[nodiscard]] bool init(retro_environment_t environment);
    void update(const rom::GameSpec& game, std::span<const u8> drive_writes,
                s16 steering, bool enabled);
    void stop();

private:
    void apply(u16 strong, u16 weak);
    void clear_state();

    retro_set_rumble_state_t m_set_state = nullptr;
    int m_level = 0;
    int m_direction = 0;
    int m_hold = 0;
    u16 m_strong = 0;
    u16 m_weak = 0;
    osd::DriveCommand m_drive_command;
};

}  // namespace sm2::libretro
