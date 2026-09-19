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
#include "osd/drive_command.h"

namespace sm2::osd {
namespace {

using Effect = DriveCommand::Effect;

DriveCommand make(Effect effect, int steps, int full_steps, bool held = false)
{
    return {effect, steps * kDriveFull / full_steps, held};
}

// Daytona, Indy 500 and Touring Car: effect in the high nibble,
// strength in the low one.
//   0x1x  spring       0..7
//   0x2x  friction     0..7
//   0x3x  centring     0..12
//   0x4x  vibration    0..7
//   0x5x  push left    0 releases, up to 7
//   0x6x  push right   0 releases, up to 7
// 0x0x and 0x7x are the boot handshake. Indy 500 follows each effect with two
// parameter bytes, 0xbx then 0xax, not yet understood.
DriveCommand decode_daytona(u8 value)
{
    constexpr DriveCommand kOther{Effect::Other};
    const int low = value & 0x0f;
    switch (value & 0xf0) {
        case 0x10: return low <= 7  ? make(Effect::Spring, low + 1, 8) : kOther;
        case 0x20: return low <= 7  ? make(Effect::Friction, low + 1, 8) : kOther;
        case 0x30: return low <= 12 ? make(Effect::Spring, low + 1, 13) : kOther;
        case 0x40: return low <= 7  ? make(Effect::Vibrate, low + 1, 8) : kOther;
        case 0x50: return low <= 7  ? make(Effect::PushLeft, low, 7) : kOther;
        case 0x60: return low <= 7  ? make(Effect::PushRight, low, 7) : kOther;
        default:   return kOther;
    }
}

// Touring Car uses Daytona's bytes but streams its pushes every frame as a
// centring torque worked out from the car, which lags the wheel: swinging from
// one turn into the next, it still pushes the old way and throws the wheel. So
// only its strength is used, as a spring about the wheel's own position. It
// idles at 1, so strength runs from 1 (none) to 7 (full).
DriveCommand decode_stcc(u8 value)
{
    DriveCommand command = decode_daytona(value);
    if (command.is_push()) {
        const int low    = value & 0x0f;
        command.effect   = Effect::Spring;
        command.strength = low <= 1 ? 0 : (low - 1) * kDriveFull / 6;
        command.held     = true;
    }
    return command;
}

// Sega Rally streams a torque every frame, strength in the low five bits:
//   0x80..0x9f  push right  1..32
//   0xc0..0xdf  push left   1..32
// Holding the wheel over sends a push back towards centre. 0x00 releases;
// 0x10 and 0x15 are not forces.
DriveCommand decode_rally(u8 value)
{
    const int low = value & 0x1f;
    switch (value & 0xe0) {
        case 0x80: return make(Effect::PushRight, low + 1, 32, true);
        case 0xc0: return make(Effect::PushLeft, low + 1, 32, true);
        default:   return value == 0x00 ? DriveCommand{} : DriveCommand{Effect::Other};
    }
}

}  // namespace

DriveCommand decode_drive_command(rom::DriveProtocol protocol, u8 value)
{
    switch (protocol) {
        case rom::DriveProtocol::Stcc:  return decode_stcc(value);
        case rom::DriveProtocol::Rally: return decode_rally(value);
        case rom::DriveProtocol::Daytona:
        default: return decode_daytona(value);
    }
}

}  // namespace sm2::osd
