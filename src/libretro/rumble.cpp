// SPDX-License-Identifier: BSD-3-Clause
#include "rumble.h"

#include <algorithm>
#include <cstdlib>

namespace sm2::libretro {
namespace {
constexpr int kSteeringDeadzone = 7000;
constexpr int kHoldFrames = 12;
}

bool GamepadRumble::init(retro_environment_t environment)
{
    stop();
    m_set_state = nullptr;
    retro_rumble_interface interface{};
    if (!environment
        || !environment(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &interface)
        || !interface.set_rumble_state)
        return false;
    m_set_state = interface.set_rumble_state;
    return true;
}

void GamepadRumble::apply(u16 strong, u16 weak)
{
    if (strong == m_strong && weak == m_weak) return;
    if (m_set_state) {
        m_set_state(0, RETRO_RUMBLE_STRONG, strong);
        m_set_state(0, RETRO_RUMBLE_WEAK, weak);
    }
    m_strong = strong;
    m_weak = weak;
}

void GamepadRumble::clear_state()
{
    m_level = 0;
    m_direction = 0;
    m_hold = 0;
    m_drive_command = {};
}

void GamepadRumble::stop()
{
    if (m_set_state) {
        m_set_state(0, RETRO_RUMBLE_STRONG, 0);
        m_set_state(0, RETRO_RUMBLE_WEAK, 0);
    }
    m_strong = 0;
    m_weak = 0;
    clear_state();
}

void GamepadRumble::update(const rom::GameSpec& game, std::span<const u8> drive_writes,
                           s16 steering, bool enabled)
{
    for (const u8 value : drive_writes) {
        const osd::DriveCommand command = osd::decode_drive_command(game.drive_protocol, value);
        if (command.effect == osd::DriveCommand::Effect::Other) continue;
        if (command.effect == osd::DriveCommand::Effect::Spring && !command.held
            && m_drive_command.held)
            continue;
        m_drive_command = command;
    }

    constexpr int ceiling = 65535;
    const bool active = enabled && game.has_steering();
    if (!active) {
        apply(0, 0);
        clear_state();
        return;
    }

    // Short directional pushes and vibration commands become gamepad jolts.
    int impact = 0;
    if (m_drive_command.strength > 0 && !m_drive_command.held
        && (m_drive_command.is_push()
            || m_drive_command.effect == osd::DriveCommand::Effect::Vibrate)) {
        const int minimum_felt = ceiling / 4;
        impact = minimum_felt
               + (ceiling - minimum_felt) * m_drive_command.strength / osd::kDriveFull;
        if (m_drive_command.is_push()) {
            const int direction =
                m_drive_command.effect == osd::DriveCommand::Effect::PushLeft ? 1 : -1;
            if (direction != m_direction) {
                impact = std::min(impact * 3 / 2, 65535);
                m_direction = direction;
            }
        }
    }

    // A gamepad cannot reproduce the directional centring force, so upstream
    // represents steering load as a lighter vibration instead.
    int cornering = 0;
    const int deflection = std::abs(static_cast<int>(steering));
    if (deflection > kSteeringDeadzone) {
        constexpr int span = 32767 - kSteeringDeadzone;
        const int over = std::min(deflection - kSteeringDeadzone, span);
        cornering = (ceiling * 2 / 5) * over / span;
    }

    const int target = std::max(impact, cornering);
    if (target > 0) {
        m_level = target;
        m_hold = kHoldFrames;
    } else if (m_hold > 0 && --m_hold == 0) {
        m_level = 0;
        m_direction = 0;
    }

    const auto strong = static_cast<u16>(std::clamp(m_level, 0, 65535));
    const auto weak = static_cast<u16>(std::clamp(m_level / 2, 0, 65535));
    apply(strong, weak);
}

}  // namespace sm2::libretro
