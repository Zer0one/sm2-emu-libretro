// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"
#include "hw/model2_machine_base.h"
#include <array>
#include <vector>

namespace sm2::libretro {
// Milestone 2: digital joystick cabinets. Other cabinets retain idle axes.
bool digital_profile(const rom::GameSpec& game);
std::vector<retro_input_descriptor> descriptors(const rom::GameSpec& game);
void poll_input(hw::Inputs& inputs, const rom::GameSpec& game,
                const std::array<unsigned, 2>& devices, retro_input_state_t state);
}
