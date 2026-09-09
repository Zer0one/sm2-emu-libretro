// SPDX-License-Identifier: BSD-3-Clause
#include "input.h"

namespace sm2::libretro {
namespace {
struct Binding { unsigned id; u8 bit; const char* label; };
constexpr Binding bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x01, "Button 1 (South)"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x02, "Button 2 (East)"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x04, "Button 3 (West)"},
    {RETRO_DEVICE_ID_JOYPAD_L, 0x04, "Button 3 (LB alias)"},
    {RETRO_DEVICE_ID_JOYPAD_UP, 0x20, "Up"},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 0x10, "Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 0x80, "Left"},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 0x40, "Right"},
};
bool player_enabled(const rom::GameSpec& game, unsigned player)
{
    return rom::has_input(game.inputs, player == 0 ? rom::InputFlags::Joystick1
                                                   : rom::InputFlags::Joystick2);
}
}
bool digital_profile(const rom::GameSpec& game)
{
    if (game.name == "von" || game.parent == "von") return false;
    if (rom::has_input(game.inputs, rom::InputFlags::Vehicle | rom::InputFlags::Gun1
                                    | rom::InputFlags::Gun2)) return false;
    for (const auto& channel : game.analog)
        if (channel.control != rom::AnalogControl::None) return false;
    return rom::has_input(game.inputs, rom::InputFlags::Joystick1);
}
std::vector<retro_input_descriptor> descriptors(const rom::GameSpec& game)
{
    std::vector<retro_input_descriptor> result;
    for (unsigned p = 0; p < 2; ++p) {
        const auto add = [&](unsigned id, const char* label) {
            result.push_back({p, RETRO_DEVICE_JOYPAD, 0, id, label});
        };
        add(RETRO_DEVICE_ID_JOYPAD_SELECT, "Insert Coin");
        add(RETRO_DEVICE_ID_JOYPAD_START, "Start");
        if (digital_profile(game) && player_enabled(game, p)) {
            for (const auto& b : bindings) add(b.id, b.label);
            if (!rom::has_input(game.inputs, rom::InputFlags::Buttons3)) {
                add(RETRO_DEVICE_ID_JOYPAD_X, "Button 4 (North)");
                add(RETRO_DEVICE_ID_JOYPAD_R, "Button 4 (RB alias)");
            }
        }
        if (p == 0) {
            add(RETRO_DEVICE_ID_JOYPAD_L3, "Test");
            add(RETRO_DEVICE_ID_JOYPAD_R3, "Service");
        }
    }
    result.push_back({});
    return result;
}
void poll_input(hw::Inputs& inputs, const rom::GameSpec& game,
                const std::array<unsigned, 2>& devices, retro_input_state_t state)
{
    inputs.in0 = inputs.in1 = inputs.in2 = 0xff;
    // Preserve calibrated idle analogue/gun positions initialized by the machine.
    if (!state) return;
    for (unsigned p = 0; p < 2; ++p) {
        if (devices[p] != RETRO_DEVICE_JOYPAD) continue;
        const auto pressed = [&](unsigned id) { return state(p, RETRO_DEVICE_JOYPAD, 0, id) != 0; };
        const auto clear = [](u8& port, u8 bits) { port &= static_cast<u8>(~bits); };
        if (pressed(RETRO_DEVICE_ID_JOYPAD_SELECT)) clear(inputs.in0, p == 0 ? 0x01 : 0x02);
        if (pressed(RETRO_DEVICE_ID_JOYPAD_START))
            clear(inputs.in0, p == 0 ? (game.start1_bit ? game.start1_bit : 0x10) : 0x20);
        if (p == 0) {
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L3)) clear(inputs.in0, 0x04);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R3)) clear(inputs.in0, 0x08);
        }
        if (!digital_profile(game) || !player_enabled(game, p)) continue;
        u8& port = p == 0 ? inputs.in1 : inputs.in2;
        for (const auto& b : bindings) if (pressed(b.id)) clear(port, b.bit);
        if (!rom::has_input(game.inputs, rom::InputFlags::Buttons3)
            && (pressed(RETRO_DEVICE_ID_JOYPAD_X) || pressed(RETRO_DEVICE_ID_JOYPAD_R)))
            clear(port, 0x08);
    }
}
}
