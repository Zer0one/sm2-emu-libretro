// SPDX-License-Identifier: BSD-3-Clause
#include "input.h"
#include <array>
#include <stdexcept>
#include <cstdio>
using namespace sm2;
namespace {
std::array<unsigned,2> pressed{};
int16_t state(unsigned port,unsigned device,unsigned index,unsigned id)
{
    if (port >= 2 || device != RETRO_DEVICE_JOYPAD || index != 0 || id >= 16)
        throw std::runtime_error("Invalid frontend input query");
    return (pressed[port] & (1u << id)) != 0;
}
void check(bool okay,const char* what) { if(!okay) throw std::runtime_error(what); }
}
int main()
{
    rom::GameSpec game;
    game.inputs=rom::InputFlags::Common | rom::InputFlags::Joystick1
        | rom::InputFlags::Joystick2 | rom::InputFlags::Buttons3;
    hw::Inputs inputs;
    inputs.analog[0]=0x77; inputs.gun_p1x=0x123;
    std::array<unsigned,2> devices{1,1};
    auto poll=[&]{libretro::poll_input(inputs,game,devices,state);};
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_SELECT,1u<<RETRO_DEVICE_ID_JOYPAD_START}; poll();
    check(inputs.in0==0xde,"Independent P1 coin and P2 start");
    game.start1_bit=0x80; pressed={1u<<RETRO_DEVICE_ID_JOYPAD_START,0};poll();
    check(inputs.in0==0x7f,"Game-specific start wiring");
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_Y,0};poll(); const auto canonical=inputs.in1;
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_L,0};poll();check(inputs.in1==canonical,"Alias must preserve canonical action");
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_Y | 1u<<RETRO_DEVICE_ID_JOYPAD_L,0};poll();
    check(inputs.in1==canonical,"Simultaneous alias must not toggle action");
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_X,0};poll();check(inputs.in1==0xff,"Three-button profile must not invent a fourth action");
    pressed={0,1u<<RETRO_DEVICE_ID_JOYPAD_B};poll();check(inputs.in1==0xff && inputs.in2==0xfe,"P2 remains independent");
    devices[1]=RETRO_DEVICE_NONE;poll();check(inputs.in2==0xff,"Disconnected device releases controls");
    devices[1]=1;pressed={0,0};poll();check(inputs.in0==0xff && inputs.in1==0xff && inputs.in2==0xff,"Released controls stay released");
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_L3 | 1u<<RETRO_DEVICE_ID_JOYPAD_R3,0};poll();
    check(inputs.in0==0xf3,"Test and Service remain independent cabinet bits");
    check(inputs.analog[0]==0x77 && inputs.gun_p1x==0x123,"Digital adapter must preserve calibrated analogue idle state");
    game.inputs=rom::InputFlags::Vehicle | rom::InputFlags::Joystick1;
    pressed={1u<<RETRO_DEVICE_ID_JOYPAD_B,0};poll();check(inputs.in1==0xff,"Unsupported cabinet must not inherit fighter controls");
    game.inputs=rom::InputFlags::Joystick1;game.name="von";
    check(!libretro::digital_profile(game),"Twin-stick cabinet requires its own profile");
    auto desc=libretro::descriptors(game);
    check(desc.back().description==nullptr,"Descriptor list terminates");
    for(const auto& d:desc)check(!d.description || d.id!=RETRO_DEVICE_ID_JOYPAD_B,"Unsupported action must not be advertised");
    std::puts("Input behavior checks passed");
}
