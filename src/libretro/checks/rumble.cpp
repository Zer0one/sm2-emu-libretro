// SPDX-License-Identifier: BSD-3-Clause
#include "rumble.h"

#include <array>
#include <span>
#include <cstdlib>
#include <cstdio>

using namespace sm2;
namespace {
std::array<u16, 2> motors{};
unsigned calls = 0;

bool set_rumble(unsigned port, retro_rumble_effect effect, u16 strength)
{
    if (port != 0 || effect > RETRO_RUMBLE_WEAK) return false;
    motors[effect] = strength;
    ++calls;
    return true;
}

bool environment(unsigned command, void* data)
{
    if (command != RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE) return false;
    static_cast<retro_rumble_interface*>(data)->set_rumble_state = set_rumble;
    return true;
}

void expect(bool condition, const char* description)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
        std::exit(1);
    }
}

rom::GameSpec driving_game()
{
    rom::GameSpec game;
    game.analog[0].control = rom::AnalogControl::Steer;
    return game;
}
}

int main()
{
    libretro::GamepadRumble rumble;
    expect(rumble.init(environment), "frontend rumble interface is acquired");
    auto driving = driving_game();

    const std::array<u8, 1> daytona_push{0x57};
    const std::array<u8, 1> daytona_release{0x50};
    rumble.update(driving, daytona_push, 0, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 65535
               && motors[RETRO_RUMBLE_WEAK] == 32767,
           "upstream directional jolt drives strong and weak motors");

    rumble.update(driving, daytona_release, 32767, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 26214
               && motors[RETRO_RUMBLE_WEAK] == 13107,
           "a current steering load replaces the previous drive-board jolt");
    rumble.update(driving, daytona_release, 0, true);
    for (int frame = 0; frame < 12; ++frame) rumble.update(driving, {}, 0, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 0 && motors[RETRO_RUMBLE_WEAK] == 0,
           "jolt ends after the upstream hold interval");

    rumble.update(driving, {}, 32767, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 26214
               && motors[RETRO_RUMBLE_WEAK] == 13107,
           "full steering deflection produces the upstream cornering vibration");
    rumble.update(driving, {}, 32767, false);
    expect(motors[RETRO_RUMBLE_STRONG] == 0 && motors[RETRO_RUMBLE_WEAK] == 0,
           "disabling rumble stops both motors immediately");

    rom::GameSpec non_driving;
    const unsigned before = calls;
    rumble.update(non_driving, daytona_push, 32767, true);
    expect(calls == before, "non-driving games remain silent");

    rumble.stop();
    driving.drive_protocol = rom::DriveProtocol::Stcc;
    const std::array<u8, 1> stcc_force{0x57};
    rumble.update(driving, stcc_force, 0, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 0 && motors[RETRO_RUMBLE_WEAK] == 0,
           "STCC continuous centring force is not misread as a gamepad impact");

    rumble.stop();
    driving.drive_protocol = rom::DriveProtocol::Rally;
    const std::array<u8, 1> rally_force{0x9f};
    rumble.update(driving, rally_force, 0, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 0 && motors[RETRO_RUMBLE_WEAK] == 0,
           "Sega Rally streamed torque is not misread as a Daytona impact");

    rumble.stop();
    driving.drive_protocol = rom::DriveProtocol::Daytona;
    const std::array<u8, 3> indy_sequence{0x57, 0xb0, 0xa0};
    rumble.update(driving, indy_sequence, 0, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 65535
               && motors[RETRO_RUMBLE_WEAK] == 32767,
           "Indy 500 parameter bytes retain the preceding impact command");

    rumble.stop();
    const std::array<u8, 1> vibration{0x47};
    rumble.update(driving, vibration, 0, true);
    expect(motors[RETRO_RUMBLE_STRONG] == 65535
               && motors[RETRO_RUMBLE_WEAK] == 32767,
           "Daytona vibration command drives both gamepad motors");

    rumble.stop();
    expect(motors[RETRO_RUMBLE_STRONG] == 0 && motors[RETRO_RUMBLE_WEAK] == 0,
           "explicit shutdown leaves both motors stopped");
    std::puts("SM2 Libretro gamepad rumble checks passed");
}
