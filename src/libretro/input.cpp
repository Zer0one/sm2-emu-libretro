// SPDX-License-Identifier: BSD-3-Clause
#include "input.h"
#include <algorithm>
#include <cmath>
#include <string_view>

namespace sm2::libretro {
namespace {
struct ProfileEntry { std::string_view root; InputProfile profile; };
constexpr ProfileEntry profiles[] = {
    {"dynamcop", InputProfile::Action},
    {"fvipers", InputProfile::Fighting},
    {"lastbrnx", InputProfile::Fighting},
    {"vf2", InputProfile::Fighting},
    {"doa", InputProfile::FightingDeadOrAlive},
    {"schamp", InputProfile::FightingSonicChampionship},
    {"pltkids", InputProfile::Shooter},
    {"zerogun", InputProfile::Shooter},
    {"zeroguna", InputProfile::Shooter},
    {"hpyagu98", InputProfile::BaseballHangukProYagu98},
    {"vstriker", InputProfile::Soccer},
    {"von", InputProfile::Twin},
};

struct Binding { unsigned id; u8 bit; const char* label; };
constexpr Binding directions[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, 0x20, "Joystick Up"},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 0x10, "Joystick Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 0x80, "Joystick Left"},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 0x40, "Joystick Right"},
};
constexpr Binding action_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x02, "Kick"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x01, "Punch"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x04, "Jump"},
};
constexpr Binding fighting_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x02, "Kick"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x01, "Punch"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x04, "Guard"},
};
constexpr Binding doa_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x04, "Kick"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x02, "Punch"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x01, "Hold"},
};
constexpr Binding schamp_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x02, "Kick"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x01, "Punch"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x04, "Barrier"},
};
constexpr Binding shooter_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x01, "Button 1"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x02, "Button 2"},
};
constexpr Binding baseball_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x01, "Button 1"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x02, "Button 2"},
};
constexpr Binding generic_three_button_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x01, "Button 1"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x02, "Button 2"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x04, "Button 3"},
};
constexpr Binding soccer_bindings[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, 0x04, "Short Pass"},
    {RETRO_DEVICE_ID_JOYPAD_A, 0x01, "Long Pass"},
    {RETRO_DEVICE_ID_JOYPAD_Y, 0x02, "Shoot"},
};

std::string_view root_name(const rom::GameSpec& game)
{
    return game.parent.empty() ? std::string_view(game.name) : std::string_view(game.parent);
}
bool expected_digital_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Joystick2)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected) return false;
    return std::none_of(game.analog.begin(), game.analog.end(), [](const auto& channel) {
        return channel.control != rom::AnalogControl::None;
    });
}
bool expected_single_player_digital_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    return std::none_of(game.analog.begin(), game.analog.end(), [](const auto& channel) {
        return channel.control != rom::AnalogControl::None;
    });
}
bool expected_driving_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Vehicle);
    if (static_cast<u32>(game.inputs) != expected) return false;
    unsigned steer = 0, accel = 0, brake = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Steer: ++steer; break;
        case rom::AnalogControl::Accel: ++accel; break;
        case rom::AnalogControl::Brake: ++brake; break;
        default: ++other; break;
        }
    }
    return steer == 1 && accel == 1 && brake == 1 && other == 0;
}
bool expected_sequential_driving_signature(const rom::GameSpec& game)
{
    return expected_driving_signature(game) && game.shift_buttons && !game.gearbox;
}
bool expected_motorcycle_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Vehicle);
    if (static_cast<u32>(game.inputs) != expected || !game.shift_buttons || game.gearbox)
        return false;
    unsigned bank = 0, throttle = 0, brake = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Bank: ++bank; break;
        case rom::AnalogControl::Throttle: ++throttle; break;
        case rom::AnalogControl::Brake: ++brake; break;
        default: ++other; break;
        }
    }
    return bank == 1 && throttle == 1 && brake == 1 && other == 0;
}
bool expected_sky_target_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned stick_x = 0, stick_y = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::StickX: ++stick_x; break;
        case rom::AnalogControl::StickY: ++stick_y; break;
        default: ++other; break;
        }
    }
    return stick_x == 1 && stick_y == 1 && other == 0;
}
bool expected_baseball_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Joystick2)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned bat1 = 0, bat2 = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Bat1: ++bat1; break;
        case rom::AnalogControl::Bat2: ++bat2; break;
        default: ++other; break;
        }
    }
    return bat1 == 1 && bat2 == 1 && other == 0;
}
bool expected_water_ski_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Vehicle);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned slide = 0, other = 0;
    for (const auto& channel : game.analog) {
        if (channel.control == rom::AnalogControl::None) continue;
        if (channel.control == rom::AnalogControl::Slide) ++slide;
        else ++other;
    }
    return slide == 1 && other == 0;
}
bool expected_ski_super_g_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned swing = 0, inclining = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Swing: ++swing; break;
        case rom::AnalogControl::Inclining: ++inclining; break;
        default: ++other; break;
        }
    }
    return swing == 1 && inclining == 1 && other == 0;
}
bool expected_top_skater_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned curving = 0, slide = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Curving: ++curving; break;
        case rom::AnalogControl::Slide: ++slide; break;
        default: ++other; break;
        }
    }
    return curving == 1 && slide == 1 && other == 0;
}
bool expected_wave_runner_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Vehicle);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned handle = 0, roll = 0, throttle = 0, pitch = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Handle: ++handle; break;
        case rom::AnalogControl::Roll: ++roll; break;
        case rom::AnalogControl::Throttle: ++throttle; break;
        case rom::AnalogControl::Pitch: ++pitch; break;
        default: ++other; break;
        }
    }
    return handle == 1 && roll == 1 && throttle == 1 && pitch == 1 && other == 0;
}
bool expected_desert_tank_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Joystick1)
        | static_cast<u32>(rom::InputFlags::Buttons3);
    if (static_cast<u32>(game.inputs) != expected || game.gearbox || game.shift_buttons)
        return false;
    unsigned steer = 0, accel = 0, elevation = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Steer: ++steer; break;
        case rom::AnalogControl::Accel: ++accel; break;
        case rom::AnalogControl::Elevation: ++elevation; break;
        default: ++other; break;
        }
    }
    return steer == 1 && accel == 1 && elevation == 1 && other == 0;
}
bool expected_positional_gun_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Gun1)
        | static_cast<u32>(rom::InputFlags::Gun2);
    if (static_cast<u32>(game.inputs) != expected || game.lightgun.present
        || game.gearbox || game.shift_buttons)
        return false;
    unsigned p1x = 0, p1y = 0, p2x = 0, p2y = 0, other = 0;
    for (const auto& channel : game.analog) {
        switch (channel.control) {
        case rom::AnalogControl::None: break;
        case rom::AnalogControl::Gun1X: ++p1x; break;
        case rom::AnalogControl::Gun1Y: ++p1y; break;
        case rom::AnalogControl::Gun2X: ++p2x; break;
        case rom::AnalogControl::Gun2Y: ++p2y; break;
        default: ++other; break;
        }
    }
    return p1x == 1 && p1y == 1 && p2x == 1 && p2y == 1 && other == 0;
}
bool expected_serial_gun_signature(const rom::GameSpec& game)
{
    constexpr auto expected = static_cast<u32>(rom::InputFlags::Common)
        | static_cast<u32>(rom::InputFlags::Gun1)
        | static_cast<u32>(rom::InputFlags::Gun2);
    return static_cast<u32>(game.inputs) == expected && game.lightgun.present
        && !game.gun_missile && !game.gearbox && !game.shift_buttons
        && std::none_of(game.analog.begin(), game.analog.end(), [](const auto& channel) {
            return channel.control != rom::AnalogControl::None;
        });
}
bool gameplay_port(InputProfile profile, unsigned player)
{
    return player < profile_players(profile);
}
const Binding* profile_bindings(InputProfile profile, size_t& count)
{
    switch (profile) {
    case InputProfile::Action: count = std::size(action_bindings); return action_bindings;
    case InputProfile::Fighting: count = std::size(fighting_bindings); return fighting_bindings;
    case InputProfile::FightingDeadOrAlive: count = std::size(doa_bindings); return doa_bindings;
    case InputProfile::FightingSonicChampionship: count = std::size(schamp_bindings); return schamp_bindings;
    case InputProfile::Shooter: count = std::size(shooter_bindings); return shooter_bindings;
    case InputProfile::BaseballDynamite: count = std::size(baseball_bindings); return baseball_bindings;
    case InputProfile::BasketballAirWalkers:
    case InputProfile::HorseRacingRoyalAscotII:
        count = std::size(generic_three_button_bindings);
        return generic_three_button_bindings;
    case InputProfile::BaseballHangukProYagu98:
        count = std::size(generic_three_button_bindings);
        return generic_three_button_bindings;
    case InputProfile::Soccer: count = std::size(soccer_bindings); return soccer_bindings;
    default: count = 0; return nullptr;
    }
}
bool service_enabled(unsigned device)
{
    return device == RETRO_DEVICE_JOYPAD;
}
bool joypad_enabled(unsigned device)
{
    return (device & RETRO_DEVICE_MASK) == RETRO_DEVICE_JOYPAD;
}
void clear(u8& port, u8 bits)
{
    port &= static_cast<u8>(~bits);
}
void map_twin_axis(u8& port, s16 x, s16 y)
{
    constexpr s16 threshold = 0x4000;
    if (x < -threshold) clear(port, 0x80);
    if (x > threshold) clear(port, 0x40);
    if (y < -threshold) clear(port, 0x20);
    if (y > threshold) clear(port, 0x10);
}
u8 scale_axis(const rom::AnalogChannel& channel, s16 raw, bool pedal)
{
    if (!pedal && raw == 0)
        return channel.rest;
    float fraction = pedal
        ? static_cast<float>(std::max<int>(raw, 0)) / 32767.0f
        : static_cast<float>(static_cast<int>(raw) + 32768) / 65535.0f;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    const float span = static_cast<float>(channel.maximum - channel.minimum);
    float value = static_cast<float>(channel.minimum) + fraction * span;
    if (channel.reverse)
        value = static_cast<float>(channel.maximum) - (value - channel.minimum);
    return static_cast<u8>(value + 0.5f);
}
static constexpr u8 kFBNeoLogarithmicSteeringCurve[0x100] = {
    0x00, 0x01, 0x13, 0x1d, 0x25, 0x2b, 0x2f, 0x33, 0x37, 0x3a, 0x3d, 0x3f, 0x41, 0x44, 0x46, 0x47,
    0x49, 0x4b, 0x4c, 0x4d, 0x4f, 0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x59, 0x5a,
    0x5b, 0x5c, 0x5d, 0x5d, 0x5e, 0x5f, 0x60, 0x60, 0x61, 0x62, 0x62, 0x63, 0x63, 0x64, 0x65, 0x65,
    0x66, 0x66, 0x67, 0x67, 0x68, 0x68, 0x69, 0x69, 0x6a, 0x6a, 0x6b, 0x6b, 0x6c, 0x6c, 0x6c, 0x6d,
    0x6d, 0x6e, 0x6e, 0x6e, 0x6f, 0x6f, 0x70, 0x70, 0x70, 0x71, 0x71, 0x71, 0x72, 0x72, 0x72, 0x73,
    0x73, 0x73, 0x74, 0x74, 0x74, 0x75, 0x75, 0x75, 0x76, 0x76, 0x76, 0x76, 0x77, 0x77, 0x77, 0x78,
    0x78, 0x78, 0x78, 0x79, 0x79, 0x79, 0x79, 0x7a, 0x7a, 0x7a, 0x7a, 0x7b, 0x7b, 0x7b, 0x7b, 0x7c,
    0x7c, 0x7c, 0x7c, 0x7d, 0x7d, 0x7d, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x80,
    0x80, 0x81, 0x81, 0x81, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x83, 0x83, 0x83, 0x84, 0x84, 0x84,
    0x84, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x86, 0x86, 0x87, 0x87, 0x87, 0x87, 0x88, 0x88, 0x88,
    0x88, 0x89, 0x89, 0x89, 0x8a, 0x8a, 0x8a, 0x8a, 0x8b, 0x8b, 0x8b, 0x8c, 0x8c, 0x8c, 0x8d, 0x8d,
    0x8d, 0x8e, 0x8e, 0x8e, 0x8f, 0x8f, 0x8f, 0x90, 0x90, 0x90, 0x91, 0x91, 0x92, 0x92, 0x92, 0x93,
    0x93, 0x94, 0x94, 0x94, 0x95, 0x95, 0x96, 0x96, 0x97, 0x97, 0x98, 0x98, 0x99, 0x99, 0x9a, 0x9a,
    0x9b, 0x9b, 0x9c, 0x9d, 0x9d, 0x9e, 0x9e, 0x9f, 0xa0, 0xa0, 0xa1, 0xa2, 0xa3, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xae, 0xaf, 0xb0, 0xb1, 0xb3, 0xb4, 0xb5, 0xb7,
    0xb9, 0xba, 0xbc, 0xbf, 0xc1, 0xc3, 0xc6, 0xc9, 0xcd, 0xd1, 0xd5, 0xdb, 0xe3, 0xed, 0xff, 0xff,
};
u8 tune_steering(u8 value, const DrivingAnalogOptions& options)
{
    int target = value;
    if (options.steering_response == SteeringResponse::Progressive) {
        const int offset = target - 0x80;
        const int magnitude = std::abs(offset);
        const int side_range = offset < 0 ? 0x80 : 0x7f;
        const int curved = (magnitude * magnitude + side_range / 2) / side_range;
        target = 0x80 + (offset < 0 ? -curved : curved);
    } else if (options.steering_response == SteeringResponse::FBNeoLogarithmic) {
        target = kFBNeoLogarithmicSteeringCurve[target];
    }
    const int output_range = std::clamp(options.steering_output_range, 50, 150);
    return static_cast<u8>(std::clamp(
        0x80 + ((target - 0x80) * output_range) / 100, 0, 0xff));
}
u8 scale_fraction(const rom::AnalogChannel& channel, float fraction)
{
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    if (channel.reverse) fraction = 1.0f - fraction;
    const float span = static_cast<float>(channel.maximum - channel.minimum);
    return static_cast<u8>(static_cast<float>(channel.minimum) + fraction * span + 0.5f);
}
u8 scale_pedal(const rom::AnalogChannel& channel, s16 raw, int output_range_per_mille)
{
    float fraction = static_cast<float>(std::max<int>(raw, 0)) / 32767.0f;
    fraction *= static_cast<float>(std::clamp(output_range_per_mille, 500, 1500)) / 1000.0f;
    return scale_fraction(channel, fraction);
}
u16 scale_lightgun(float fraction, const rom::LightgunAxis& axis)
{
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    const float span = static_cast<float>(axis.maximum - axis.minimum);
    return static_cast<u16>(static_cast<float>(axis.minimum) + fraction * span + 0.5f);
}
void set_analog_control(hw::Inputs& inputs, const rom::GameSpec& game,
                        rom::AnalogControl control, s16 raw, bool pedal,
                        const DrivingAnalogOptions* driving_options = nullptr)
{
    for (size_t channel = 0; channel < game.analog.size(); ++channel) {
        if (game.analog[channel].control != control) continue;
        u8 value = scale_axis(game.analog[channel], raw, pedal);
        if (driving_options && control == rom::AnalogControl::Steer)
            value = tune_steering(value, *driving_options);
        else if (driving_options && pedal && (control == rom::AnalogControl::Accel
                                              || control == rom::AnalogControl::Throttle))
            value = scale_pedal(game.analog[channel], raw,
                                driving_options->accelerator_output_range_per_mille);
        else if (driving_options && pedal && control == rom::AnalogControl::Brake)
            value = scale_pedal(game.analog[channel], raw,
                                driving_options->brake_output_range_per_mille);
        inputs.analog[channel] = value;
    }
}
void set_analog_fraction(hw::Inputs& inputs, const rom::GameSpec& game,
                         rom::AnalogControl control, float fraction)
{
    for (size_t channel = 0; channel < game.analog.size(); ++channel)
        if (game.analog[channel].control == control)
            inputs.analog[channel] = scale_fraction(game.analog[channel], fraction);
}
}
InputProfile recognize_profile(const rom::GameSpec& game)
{
    const auto root = root_name(game);
    if (expected_desert_tank_signature(game) && root == "desert")
        return InputProfile::SpecialDesertTank;
    if (expected_single_player_digital_signature(game)) {
        if (root == "airwlkrs") return InputProfile::BasketballAirWalkers;
        if (root == "rascot2") return InputProfile::HorseRacingRoyalAscotII;
    }
    if (expected_wave_runner_signature(game) && root == "waverunr")
        return InputProfile::SpecialWaveRunner;
    if (expected_top_skater_signature(game) && root == "topskatr")
        return InputProfile::SpecialTopSkater;
    if (expected_water_ski_signature(game) && root == "segawski")
        return InputProfile::SpecialWaterSki;
    if (expected_ski_super_g_signature(game) && root == "skisuprg")
        return InputProfile::SpecialSkiSuperG;
    if (expected_baseball_signature(game) && (root == "dynabb" || root == "dynabb97"))
        return InputProfile::BaseballDynamite;
    if (expected_sky_target_signature(game) && root == "skytargt")
        return InputProfile::JoystickAnalogSkyTarget;
    if (expected_positional_gun_signature(game) && game.gun_missile && root == "bel")
        return InputProfile::GunBehindEnemyLines;
    if (((expected_positional_gun_signature(game) && !game.gun_missile
          && (root == "gunblade" || root == "rchase2"))
         || (expected_serial_gun_signature(game)
             && (root == "vcop" || root == "vcop2" || root == "hotd"))))
        return InputProfile::Gun;
    if (expected_sequential_driving_signature(game)
        && (root == "indy500" || root == "overrev" || root == "stcc"))
        return InputProfile::DrivingSequentialVR2;
    if (expected_sequential_driving_signature(game) && root == "sgt24h")
        return InputProfile::DrivingSequentialVR1;
    if (expected_motorcycle_signature(game) && root == "manxtt")
        return InputProfile::DrivingSequentialManxTT;
    if (expected_motorcycle_signature(game) && root == "motoraid")
        return InputProfile::DrivingSequentialMotorRaid;
    if (expected_driving_signature(game)) {
        if (game.gearbox && !game.shift_buttons
            && root == "daytona" && game.start1_bit == 0x10
            && game.wheel_button_bits[0] == std::pair<u8, u8>{0, 0x20}
            && game.wheel_button_bits[1] == std::pair<u8, u8>{0, 0x40}
            && game.wheel_button_bits[2] == std::pair<u8, u8>{0, 0x80}
            && game.wheel_button_bits[3] == std::pair<u8, u8>{1, 0x01})
            return InputProfile::Driving4SpeedVR4;
        if (game.gearbox && !game.shift_buttons
            && root == "srallyc" && game.start1_bit == 0x40
            && game.wheel_button_bits[0] == std::pair<u8, u8>{0, 0x20})
            return InputProfile::Driving4SpeedVR1Handbrake;
    }
    if (!expected_digital_signature(game)) return InputProfile::Unsupported;
    for (const auto& entry : profiles)
        if (entry.root == root) return entry.profile;
    return InputProfile::Unsupported;
}
const char* profile_name(InputProfile profile)
{
    switch (profile) {
    case InputProfile::Action: return "Joystick (Standard): Action";
    case InputProfile::Fighting: return "Joystick (Standard): Fighting";
    case InputProfile::FightingDeadOrAlive: return "Joystick (Standard): Fighting (Dead or Alive)";
    case InputProfile::FightingSonicChampionship: return "Joystick (Standard): Fighting (Sonic Championship)";
    case InputProfile::Shooter: return "Joystick (Standard): Shooter";
    case InputProfile::Soccer: return "Joystick (Standard): Soccer";
    case InputProfile::Twin: return "Joystick (Twin)";
    case InputProfile::Driving4SpeedVR4: return "Driving: 4-Speed + VR4";
    case InputProfile::Driving4SpeedVR1Handbrake: return "Driving: 4-Speed + VR1 + Handbrake";
    case InputProfile::DrivingSequentialVR2: return "Driving: Sequential + VR2";
    case InputProfile::DrivingSequentialVR1: return "Driving: Sequential + VR1";
    case InputProfile::DrivingSequentialManxTT: return "Driving: Sequential (Manx TT Superbike)";
    case InputProfile::DrivingSequentialMotorRaid: return "Driving: Sequential (Motor Raid)";
    case InputProfile::JoystickAnalogSkyTarget: return "Joystick (Analog): Sky Target";
    case InputProfile::BaseballDynamite: return "Joystick (Standard): Baseball (Dynamite Baseball)";
    case InputProfile::SpecialWaterSki: return "Special: Water Ski";
    case InputProfile::SpecialSkiSuperG: return "Special: Ski Super G";
    case InputProfile::SpecialTopSkater: return "Special: Top Skater";
    case InputProfile::SpecialWaveRunner: return "Special: Wave Runner";
    case InputProfile::BasketballAirWalkers: return "Joystick (Standard): Basketball (Air Walkers)";
    case InputProfile::HorseRacingRoyalAscotII: return "Joystick (Standard): Horse Racing (Royal Ascot II)";
    case InputProfile::SpecialDesertTank: return "Joystick (Analog): Desert Tank + VR3";
    case InputProfile::BaseballHangukProYagu98: return "Joystick (Standard): Baseball (Hanguk Pro Yagu 98)";
    case InputProfile::Gun: return "Gun";
    case InputProfile::GunBehindEnemyLines: return "Gun: Behind Enemy Lines";
    default: return "Common Controls";
    }
}
unsigned profile_players(InputProfile profile)
{
    if (profile == InputProfile::Unsupported) return 0;
    return profile == InputProfile::Twin || profile == InputProfile::Driving4SpeedVR4
        || profile == InputProfile::Driving4SpeedVR1Handbrake
        || profile == InputProfile::DrivingSequentialVR2
        || profile == InputProfile::DrivingSequentialVR1
        || profile == InputProfile::DrivingSequentialManxTT
        || profile == InputProfile::DrivingSequentialMotorRaid
        || profile == InputProfile::JoystickAnalogSkyTarget
        || profile == InputProfile::SpecialWaterSki
        || profile == InputProfile::SpecialSkiSuperG
        || profile == InputProfile::SpecialTopSkater
        || profile == InputProfile::SpecialWaveRunner
        || profile == InputProfile::HorseRacingRoyalAscotII
        || profile == InputProfile::SpecialDesertTank ? 1 : 2;
}
bool digital_profile(const rom::GameSpec& game)
{
    return recognize_profile(game) != InputProfile::Unsupported;
}
namespace {
std::string gun_profile_name(InputProfile profile, GunInputMode mode)
{
    std::string name = profile_name(profile);
    switch (mode) {
    case GunInputMode::Hybrid: return name;
    case GunInputMode::Lightgun: return name + " (Lightgun)";
    case GunInputMode::Mouse: return name + " (Mouse)";
    case GunInputMode::MouseAnalog: return name + " (Mouse + Analog Stick)";
    case GunInputMode::AnalogSticks: return name + " (Analog Sticks)";
    }
    return name;
}
bool gun_profile(InputProfile profile)
{
    return profile == InputProfile::Gun || profile == InputProfile::GunBehindEnemyLines;
}
}
void configure_controllers(const rom::GameSpec& game, ControllerConfiguration& configuration,
                           GunInputMode gun_mode)
{
    const auto profile = recognize_profile(game);
    for (unsigned port = 0; port < 2; ++port) {
        const std::string game_profile = gun_profile(profile)
            ? gun_profile_name(profile, gun_mode) : profile_name(profile);
        configuration.base_names[port] = game_profile;
        const char* base = configuration.base_names[port].c_str();
        configuration.full_names[port] = configuration.base_names[port] + " + Test/Service slots";
        configuration.descriptions[port][0] = {
            configuration.full_names[port].c_str(), RETRO_DEVICE_JOYPAD};
        configuration.descriptions[port][1] = {base, kNoServiceDevice};
        configuration.ports[port] = {configuration.descriptions[port].data(), 2};
    }
    configuration.ports[2] = {nullptr, 0};
}
std::vector<retro_input_descriptor> descriptors(
    const rom::GameSpec& game, const std::array<unsigned, 2>& devices,
    GunInputMode gun_mode, bool offscreen_reload_shortcut)
{
    std::vector<retro_input_descriptor> result;
    const auto profile = recognize_profile(game);
    for (unsigned p = 0; p < 2; ++p) {
        const auto add = [&](unsigned device, unsigned index, unsigned id, const char* label) {
            result.push_back({p, device, index, id, label});
        };
        add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Coin");
        add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,
            profile == InputProfile::DrivingSequentialManxTT
                || profile == InputProfile::DrivingSequentialMotorRaid ? "Start / VR"
                : profile == InputProfile::SpecialWaterSki ? "Start / Select Down"
                : "Start");
        if (service_enabled(devices[p])) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "Service A");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "Test A");
        }
        if (!gameplay_port(profile, p)) continue;
        if (gun_profile(profile)) {
            const bool missile = profile == InputProfile::GunBehindEnemyLines;
            const bool reload = game.lightgun.present && offscreen_reload_shortcut;
            const auto add_analog = [&](bool qualified) {
                add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                    RETRO_DEVICE_ID_ANALOG_X,
                    qualified ? "Gun Yaw (Analog Cursor)" : "Gun Yaw");
                add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                    RETRO_DEVICE_ID_ANALOG_Y,
                    qualified ? "Gun Pitch (Analog Cursor)" : "Gun Pitch");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Shot");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Shot");
                if (missile || reload)
                    add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,
                        missile ? "Missile" : "Reload Offscreen");
                if (missile || reload)
                    add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,
                        missile ? "Missile" : "Reload Offscreen");
            };
            const auto add_lightgun = [&](bool qualified) {
                add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X,
                    qualified ? "Gun Yaw (Lightgun)" : "Gun Yaw");
                add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y,
                    qualified ? "Gun Pitch (Lightgun)" : "Gun Pitch");
                add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Shot");
                if (reload)
                    add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD,
                        "Reload Offscreen");
                if (missile) {
                    add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_A, "Missile");
                    add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD, "Missile");
                }
                add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SELECT, "Coin");
                add(RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START, "Start");
            };
            const auto add_mouse = [&](bool qualified) {
                add(RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X,
                    qualified ? "Gun Yaw (Mouse)" : "Gun Yaw");
                add(RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y,
                    qualified ? "Gun Pitch (Mouse)" : "Gun Pitch");
                add(RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT, "Shot");
                if (missile || reload)
                    add(RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT,
                        missile ? "Missile" : "Reload Offscreen");
            };
            switch (gun_mode) {
            case GunInputMode::Hybrid:
                add_lightgun(true);
                add_mouse(true);
                add_analog(true);
                break;
            case GunInputMode::Lightgun: add_lightgun(false); break;
            case GunInputMode::Mouse: add_mouse(false); break;
            case GunInputMode::MouseAnalog:
                add_mouse(true);
                add_analog(true);
                break;
            case GunInputMode::AnalogSticks: add_analog(false); break;
            }
            continue;
        }
        if (profile == InputProfile::JoystickAnalogSkyTarget) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Machine Gun");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Machine Gun");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Missile");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Missile");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "View Change");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Analog Joystick X");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_Y, "Analog Joystick Y");
            continue;
        }
        if (profile == InputProfile::BaseballDynamite) {
            for (const auto& binding : directions)
                add(RETRO_DEVICE_JOYPAD, 0, binding.id, binding.label);
            for (const auto& binding : baseball_bindings)
                add(RETRO_DEVICE_JOYPAD, 0, binding.id, binding.label);
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_Y, "Bat Swing");
            continue;
        }
        if (profile == InputProfile::SpecialWaterSki) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Select Up");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Select Down");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Pitch Left");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Pitch Right");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Set");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Pitch Left");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Pitch Right");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Slide");
            continue;
        }
        if (profile == InputProfile::SpecialSkiSuperG) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Zoom In");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Zoom Out");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Foot Sensor Left");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Foot Sensor Right");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Select 2");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Select 3");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Select 1");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Swing");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_X, "Inclining");
            continue;
        }
        if (profile == InputProfile::SpecialTopSkater) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Select Left");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Select Right");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Jump Front");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Jump Tail");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Curving");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_X, "Slide");
            continue;
        }
        if (profile == InputProfile::SpecialWaveRunner) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "View");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Throttle");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Handle");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_Y, "Pitch");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_X, "Roll");
            continue;
        }
        if (profile == InputProfile::SpecialDesertTank) {
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "VR1 (Blue)");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "VR2 (Green)");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "VR3 (Red)");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Machine Gun");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Machine Gun");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Cannon");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Cannon");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Shift");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Steering");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_Y, "Elevation");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_Y, "Elevation");
            continue;
        }
        if (profile == InputProfile::Twin) {
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Joystick X");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Joystick Y");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Joystick X");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Joystick Y");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Left Dash (Turbo)");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Right Dash (Turbo)");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Left Shot Trigger");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Right Shot Trigger");
            continue;
        }
        if (profile == InputProfile::Driving4SpeedVR4
            || profile == InputProfile::Driving4SpeedVR1Handbrake) {
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Steering");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_X, "H-Gate: Left / Right");
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                RETRO_DEVICE_ID_ANALOG_Y, "H-Gate: Up / Down");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "4-Speed: Neutral");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Shift Down");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Shift Up");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Brake");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator");
            if (profile == InputProfile::Driving4SpeedVR4) {
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "VR1 (Red)");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "VR2 (Blue)");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "VR3 (Yellow)");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "VR4 (Green)");
            } else {
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "VR1");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Handbrake (Analog)");
            }
            continue;
        }
        if (profile == InputProfile::DrivingSequentialVR2
            || profile == InputProfile::DrivingSequentialVR1) {
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Steering");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,
                profile == InputProfile::DrivingSequentialVR1 ? "VR1" : "View 1");
            if (profile == InputProfile::DrivingSequentialVR2)
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "View 2");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Shift Down");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Shift Up");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Brake");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator");
            continue;
        }
        if (profile == InputProfile::DrivingSequentialManxTT
            || profile == InputProfile::DrivingSequentialMotorRaid) {
            add(RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X, "Bank");
            if (profile == InputProfile::DrivingSequentialMotorRaid) {
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Kick");
                add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Punch");
            }
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Shift Down");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Shift Up");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Brake");
            add(RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator");
            continue;
        }
        for (const auto& binding : directions)
            add(RETRO_DEVICE_JOYPAD, 0, binding.id, binding.label);
        size_t count = 0;
        const auto* bindings = profile_bindings(profile, count);
        for (size_t i = 0; i < count; ++i)
            add(RETRO_DEVICE_JOYPAD, 0, bindings[i].id, bindings[i].label);
    }
    result.push_back({});
    return result;
}
void poll_input(hw::Inputs& inputs, const rom::GameSpec& game,
                const std::array<unsigned, 2>& devices, InputRuntime& runtime,
                bool h_gate_shifter, retro_input_state_t state,
                GunInputMode gun_mode, bool offscreen_reload_shortcut,
                DrivingAnalogOptions driving_options,
                DesertElevationOptions desert_elevation_options,
                bool water_ski_slide_inverted,
                bool ski_super_g_swing_inverted,
                bool top_skater_curving_inverted)
{
    inputs.in0 = inputs.in1 = inputs.in2 = 0xff;
    const auto profile = recognize_profile(game);
    if (profile == InputProfile::Driving4SpeedVR1Handbrake) inputs.in2 = 0x00;
    if (profile == InputProfile::SpecialSkiSuperG) inputs.in2 = 0x00;
    if (profile == InputProfile::SpecialWaveRunner) inputs.in2 = 0xf7;
    // Preserve calibrated idle analogue/gun positions initialized by the machine.
    if (!state) return;
    for (unsigned p = 0; p < 2; ++p) {
        if (!joypad_enabled(devices[p])) continue;
        const auto pressed = [&](unsigned id) { return state(p, RETRO_DEVICE_JOYPAD, 0, id) != 0; };
        if (pressed(RETRO_DEVICE_ID_JOYPAD_SELECT)) clear(inputs.in0, p == 0 ? 0x01 : 0x02);
        if (pressed(RETRO_DEVICE_ID_JOYPAD_START)) {
            const u8 start = profile == InputProfile::SpecialWaterSki
                ? 0x40 : (game.start1_bit ? game.start1_bit : 0x10);
            clear(inputs.in0, p == 0 ? start : 0x20);
        }
        if (service_enabled(devices[p])) {
            const bool bel = profile == InputProfile::GunBehindEnemyLines;
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L3)) clear(inputs.in0, bel ? 0x04 : 0x08);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R3)) clear(inputs.in0, bel ? 0x08 : 0x04);
        }
        if (!gameplay_port(profile, p)) continue;
        if (gun_profile(profile)) {
            constexpr int width = 496;
            constexpr int height = 384;
            constexpr s16 deadzone = 6000;
            const bool allow_lightgun = gun_mode == GunInputMode::Hybrid
                || gun_mode == GunInputMode::Lightgun;
            const bool allow_mouse = gun_mode == GunInputMode::Hybrid
                || gun_mode == GunInputMode::Mouse
                || gun_mode == GunInputMode::MouseAnalog;
            const bool allow_analog = gun_mode == GunInputMode::Hybrid
                || gun_mode == GunInputMode::MouseAnalog
                || gun_mode == GunInputMode::AnalogSticks;
            const auto lightgun = [&](unsigned id) {
                return allow_lightgun ? state(p, RETRO_DEVICE_LIGHTGUN, 0, id) : 0;
            };
            const auto mouse = [&](unsigned id) {
                return allow_mouse ? state(p, RETRO_DEVICE_MOUSE, 0, id) : 0;
            };
            const auto analog = [&](unsigned id) {
                return allow_analog
                    ? state(p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, id)
                    : 0;
            };
            const s16 lg_x = static_cast<s16>(lightgun(RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X));
            const s16 lg_y = static_cast<s16>(lightgun(RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y));
            const bool lg_trigger = lightgun(RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) != 0;
            const bool lg_reload = lightgun(RETRO_DEVICE_ID_LIGHTGUN_RELOAD) != 0;
            const bool lg_aux = lightgun(RETRO_DEVICE_ID_LIGHTGUN_AUX_A) != 0;
            const bool lg_offscreen = lightgun(RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) != 0;
            const bool lg_action = lg_trigger || lg_reload || lg_aux || lg_offscreen;
            const bool lg_moved = runtime.gun_lightgun_initialized[p]
                ? lg_x != runtime.gun_lightgun_x[p] || lg_y != runtime.gun_lightgun_y[p]
                : lg_x != 0 || lg_y != 0;
            if (allow_lightgun) {
                runtime.gun_lightgun_x[p] = lg_x;
                runtime.gun_lightgun_y[p] = lg_y;
                runtime.gun_lightgun_initialized[p] = true;
            }
            const int mouse_x = mouse(RETRO_DEVICE_ID_MOUSE_X);
            const int mouse_y = mouse(RETRO_DEVICE_ID_MOUSE_Y);
            const s16 stick_x = static_cast<s16>(analog(RETRO_DEVICE_ID_ANALOG_X));
            const s16 stick_y = static_cast<s16>(analog(RETRO_DEVICE_ID_ANALOG_Y));
            const auto stick_step = [=](s16 value) {
                const int magnitude = std::abs(static_cast<int>(value));
                if (magnitude <= deadzone) return 0;
                const float normalized = static_cast<float>(magnitude - deadzone)
                    / static_cast<float>(32767 - deadzone);
                const int speed = 1 + static_cast<int>(normalized * 11.0f + 0.5f);
                return value < 0 ? -speed : speed;
            };
            const bool use_lightgun = gun_mode == GunInputMode::Lightgun
                || (gun_mode == GunInputMode::Hybrid && (lg_moved || lg_action));
            if (use_lightgun) {
                runtime.gun_cursor_x[p] = static_cast<int>(
                    (static_cast<unsigned>(static_cast<int>(lg_x) + 32768) * (width - 1)) / 65535u);
                runtime.gun_cursor_y[p] = static_cast<int>(
                    (static_cast<unsigned>(static_cast<int>(lg_y) + 32768) * (height - 1)) / 65535u);
            } else {
                runtime.gun_cursor_x[p] += mouse_x + stick_step(stick_x);
                runtime.gun_cursor_y[p] += mouse_y + stick_step(stick_y);
            }
            runtime.gun_cursor_x[p] = std::clamp(runtime.gun_cursor_x[p], 0, width - 1);
            runtime.gun_cursor_y[p] = std::clamp(runtime.gun_cursor_y[p], 0, height - 1);
            float x = static_cast<float>(runtime.gun_cursor_x[p]) / (width - 1);
            float y = static_cast<float>(runtime.gun_cursor_y[p]) / (height - 1);

            bool shot = (allow_analog && (pressed(RETRO_DEVICE_ID_JOYPAD_B)
                                         || pressed(RETRO_DEVICE_ID_JOYPAD_R)))
                || (allow_mouse && mouse(RETRO_DEVICE_ID_MOUSE_LEFT)) || lg_trigger;
            const bool secondary_source = allow_mouse && mouse(RETRO_DEVICE_ID_MOUSE_RIGHT);
            const bool secondary = profile == InputProfile::GunBehindEnemyLines
                ? secondary_source
                    || (allow_analog && (pressed(RETRO_DEVICE_ID_JOYPAD_A)
                                        || pressed(RETRO_DEVICE_ID_JOYPAD_L)))
                    || lg_aux || lg_reload
                : offscreen_reload_shortcut
                    && (secondary_source
                        || (allow_analog && (pressed(RETRO_DEVICE_ID_JOYPAD_A)
                                            || pressed(RETRO_DEVICE_ID_JOYPAD_L)))
                        || lg_reload);
            const bool offscreen_shot = game.lightgun.present && lg_offscreen && lg_trigger;
            runtime.gun_aim_active[p] = true;
            runtime.gun_aim_offscreen[p] = game.lightgun.present
                && (lg_offscreen || secondary);
            if (game.lightgun.present && (secondary || offscreen_shot)) {
                x = y = 0.0f;
                shot = true;
            }
            if (allow_lightgun && lightgun(RETRO_DEVICE_ID_LIGHTGUN_SELECT))
                clear(inputs.in0, p == 0 ? 0x01 : 0x02);
            if (allow_lightgun && lightgun(RETRO_DEVICE_ID_LIGHTGUN_START))
                clear(inputs.in0, p == 0 ? (game.start1_bit ? game.start1_bit : 0x10) : 0x20);

            if (game.lightgun.present) {
                const auto& gx = p == 0 ? game.lightgun.p1x : game.lightgun.p2x;
                const auto& gy = p == 0 ? game.lightgun.p1y : game.lightgun.p2y;
                if (p == 0) {
                    inputs.gun_p1x = scale_lightgun(x, gx);
                    inputs.gun_p1y = scale_lightgun(y, gy);
                } else {
                    inputs.gun_p2x = scale_lightgun(x, gx);
                    inputs.gun_p2y = scale_lightgun(y, gy);
                }
            } else {
                set_analog_fraction(inputs, game,
                    p == 0 ? rom::AnalogControl::Gun1X : rom::AnalogControl::Gun2X, x);
                set_analog_fraction(inputs, game,
                    p == 0 ? rom::AnalogControl::Gun1Y : rom::AnalogControl::Gun2Y, y);
            }
            if (shot) {
                if (p == 0) clear(inputs.in1, 0x01);
                else if (game.lightgun.p2_trigger_on_in2) clear(inputs.in2, 0x01);
                else clear(inputs.in1, 0x02);
            }
            if (profile == InputProfile::GunBehindEnemyLines && secondary)
                clear(inputs.in1, p == 0 ? 0x10 : 0x20);
            continue;
        }
        if (profile == InputProfile::JoystickAnalogSkyTarget) {
            if (p != 0) continue;
            set_analog_control(inputs, game, rom::AnalogControl::StickX,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                      RETRO_DEVICE_ID_ANALOG_X), false);
            set_analog_control(inputs, game, rom::AnalogControl::StickY,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                      RETRO_DEVICE_ID_ANALOG_Y), false);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_B)
                || pressed(RETRO_DEVICE_ID_JOYPAD_R)) clear(inputs.in1, 0x10);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_A)
                || pressed(RETRO_DEVICE_ID_JOYPAD_L)) clear(inputs.in1, 0x20);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) clear(inputs.in0, 0x20);
            continue;
        }
        if (profile == InputProfile::BaseballDynamite) {
            const s16 bat = state(p, RETRO_DEVICE_ANALOG,
                                  RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                  RETRO_DEVICE_ID_ANALOG_Y);
            const float swing = static_cast<float>(std::max(0, static_cast<int>(bat)))
                / 32767.0f;
            set_analog_fraction(inputs, game,
                p == 0 ? rom::AnalogControl::Bat1 : rom::AnalogControl::Bat2,
                swing);
            u8& port = p == 0 ? inputs.in1 : inputs.in2;
            for (const auto& binding : directions)
                if (pressed(binding.id)) clear(port, binding.bit);
            for (const auto& binding : baseball_bindings)
                if (pressed(binding.id)) clear(port, binding.bit);
            continue;
        }
        if (profile == InputProfile::SpecialWaterSki) {
            if (p != 0) continue;
            int slide = static_cast<int>(state(
                0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X));
            if (water_ski_slide_inverted) slide = -slide;
            set_analog_control(inputs, game, rom::AnalogControl::Slide,
                static_cast<s16>(std::clamp(slide, -32768, 32767)), false);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) clear(inputs.in1, 0x02);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_DOWN)) clear(inputs.in0, 0x40);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L)
                || pressed(RETRO_DEVICE_ID_JOYPAD_Y)) clear(inputs.in1, 0x04);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R)
                || pressed(RETRO_DEVICE_ID_JOYPAD_A)) clear(inputs.in1, 0x08);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_B)) clear(inputs.in1, 0x01);
            continue;
        }
        if (profile == InputProfile::SpecialSkiSuperG) {
            if (p != 0) continue;
            int swing = static_cast<int>(state(
                0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X));
            if (ski_super_g_swing_inverted) swing = -swing;
            set_analog_control(inputs, game, rom::AnalogControl::Swing,
                static_cast<s16>(std::clamp(swing, -32768, 32767)), false);
            set_analog_control(inputs, game, rom::AnalogControl::Inclining,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                      RETRO_DEVICE_ID_ANALOG_X), false);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) clear(inputs.in0, 0x20);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_DOWN)) clear(inputs.in1, 0x01);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L)) inputs.in2 |= 0xf0;
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R)) inputs.in2 |= 0x0f;
            if (pressed(RETRO_DEVICE_ID_JOYPAD_B)) clear(inputs.in0, 0x80);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_A)) clear(inputs.in0, 0x10);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_Y)) clear(inputs.in0, 0x40);
            continue;
        }
        if (profile == InputProfile::SpecialTopSkater) {
            if (p != 0) continue;
            int curving = static_cast<int>(state(
                0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                RETRO_DEVICE_ID_ANALOG_X));
            if (top_skater_curving_inverted) curving = -curving;
            set_analog_control(inputs, game, rom::AnalogControl::Curving,
                static_cast<s16>(std::clamp(curving, -32768, 32767)), false);
            set_analog_control(inputs, game, rom::AnalogControl::Slide,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                      RETRO_DEVICE_ID_ANALOG_X), false);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_LEFT)) clear(inputs.in0, 0x80);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_RIGHT)) clear(inputs.in0, 0x10);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_B)) clear(inputs.in0, 0x20);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_A)) clear(inputs.in1, 0x01);
            continue;
        }
        if (profile == InputProfile::SpecialWaveRunner) {
            if (p != 0) continue;
            set_analog_control(inputs, game, rom::AnalogControl::Handle,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                      RETRO_DEVICE_ID_ANALOG_X), false);
            set_analog_control(inputs, game, rom::AnalogControl::Pitch,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                      RETRO_DEVICE_ID_ANALOG_Y), false);
            set_analog_control(inputs, game, rom::AnalogControl::Roll,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                      RETRO_DEVICE_ID_ANALOG_X), false);
            set_analog_control(inputs, game, rom::AnalogControl::Throttle,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                      RETRO_DEVICE_ID_JOYPAD_R2), false);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) clear(inputs.in1, 0x01);
            continue;
        }
        if (profile == InputProfile::SpecialDesertTank) {
            if (p != 0) continue;
            set_analog_control(inputs, game, rom::AnalogControl::Steer,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                      RETRO_DEVICE_ID_ANALOG_X), false);
            set_analog_control(inputs, game, rom::AnalogControl::Accel,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                      RETRO_DEVICE_ID_JOYPAD_R2), true);
            const s16 elevation_left = state(0, RETRO_DEVICE_ANALOG,
                                             RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                             RETRO_DEVICE_ID_ANALOG_Y);
            const s16 elevation_right = state(0, RETRO_DEVICE_ANALOG,
                                              RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                              RETRO_DEVICE_ID_ANALOG_Y);
            const s16 elevation = std::abs(static_cast<int>(elevation_right))
                    > std::abs(static_cast<int>(elevation_left))
                ? elevation_right : elevation_left;
            int elevation_axis = static_cast<int>(elevation);
            if (desert_elevation_options.inverted) elevation_axis = -elevation_axis;
            elevation_axis = std::clamp(elevation_axis, -32768, 32767);
            if (desert_elevation_options.control == DesertElevationControl::Absolute) {
                set_analog_control(inputs, game, rom::AnalogControl::Elevation,
                                   static_cast<s16>(elevation_axis), false);
                for (size_t channel = 0; channel < game.analog.size(); ++channel)
                    if (game.analog[channel].control == rom::AnalogControl::Elevation)
                        runtime.desert_elevation = static_cast<float>(inputs.analog[channel]);
            } else {
                constexpr int dead_zone = 6000;
                constexpr float maximum_step = 3.0f;
                const int magnitude = std::abs(elevation_axis);
                if (magnitude > dead_zone) {
                    const float normalized = static_cast<float>(magnitude - dead_zone)
                        / static_cast<float>(32767 - dead_zone);
                    const float speed = (1.0f + normalized * (maximum_step - 1.0f))
                        * static_cast<float>(std::clamp(
                            desert_elevation_options.speed_percent, 10, 200)) / 100.0f;
                    runtime.desert_elevation = std::clamp(
                        runtime.desert_elevation + (elevation_axis < 0 ? -speed : speed),
                        0.0f, 255.0f);
                }
                set_analog_fraction(inputs, game, rom::AnalogControl::Elevation,
                                    runtime.desert_elevation / 255.0f);
            }
            if (pressed(RETRO_DEVICE_ID_JOYPAD_DOWN)) clear(inputs.in0, 0x20);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_LEFT)) clear(inputs.in0, 0x40);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) clear(inputs.in0, 0x80);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_B)
                || pressed(RETRO_DEVICE_ID_JOYPAD_R)) clear(inputs.in1, 0x10);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_A)
                || pressed(RETRO_DEVICE_ID_JOYPAD_L)) clear(inputs.in1, 0x20);
            const bool shift = pressed(RETRO_DEVICE_ID_JOYPAD_Y);
            if (shift && !runtime.desert_shift_held)
                runtime.desert_shift = !runtime.desert_shift;
            runtime.desert_shift_held = shift;
            if (runtime.desert_shift) clear(inputs.in1, 0x01);
            continue;
        }
        if (profile == InputProfile::Twin) {
            if (p != 0) continue;
            map_twin_axis(inputs.in1,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X),
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y));
            map_twin_axis(inputs.in2,
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X),
                state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y));
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L2)) clear(inputs.in1, 0x01);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R2)) clear(inputs.in2, 0x01);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L)) clear(inputs.in1, 0x02);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R)) clear(inputs.in2, 0x02);
            continue;
        }
        if (profile == InputProfile::Driving4SpeedVR4
            || profile == InputProfile::Driving4SpeedVR1Handbrake) {
            if (p != 0) continue;
            const auto analog = [&](unsigned index, unsigned id) {
                return state(0, RETRO_DEVICE_ANALOG, index, id);
            };
            set_analog_control(inputs, game, rom::AnalogControl::Steer,
                               analog(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X),
                               false, &driving_options);
            set_analog_control(inputs, game, rom::AnalogControl::Brake,
                               analog(RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                                      RETRO_DEVICE_ID_JOYPAD_L2), true, &driving_options);
            set_analog_control(inputs, game, rom::AnalogControl::Accel,
                               analog(RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                                      RETRO_DEVICE_ID_JOYPAD_R2), true, &driving_options);

            u8* ports[] = {&inputs.in0, &inputs.in1, &inputs.in2};
            const auto press_wheel_button = [&](unsigned index) {
                const auto [port, bit] = game.wheel_button_bits[index];
                if (port < std::size(ports)) clear(*ports[port], bit);
            };
            if (profile == InputProfile::Driving4SpeedVR4) {
                if (pressed(RETRO_DEVICE_ID_JOYPAD_DOWN)) press_wheel_button(0);
                if (pressed(RETRO_DEVICE_ID_JOYPAD_LEFT)) press_wheel_button(1);
                if (pressed(RETRO_DEVICE_ID_JOYPAD_RIGHT)) press_wheel_button(2);
                if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) press_wheel_button(3);
            } else {
                if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) press_wheel_button(0);
                if (pressed(RETRO_DEVICE_ID_JOYPAD_B)) inputs.in2 = 0xff;
            }

            const s16 gate_x = analog(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
            const s16 gate_y = analog(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
            constexpr s16 gate_threshold = 0x4000;
            unsigned gate = 0;
            if (h_gate_shifter) {
                if (gate_x < -gate_threshold && gate_y < -gate_threshold) gate = 1;
                else if (gate_x < -gate_threshold && gate_y > gate_threshold) gate = 2;
                else if (gate_x > gate_threshold && gate_y < -gate_threshold) gate = 3;
                else if (gate_x > gate_threshold && gate_y > gate_threshold) gate = 4;
            } else {
                constexpr s16 standard_threshold = 0x5555;
                if (gate_y < -standard_threshold) gate = 1;
                else if (gate_y > standard_threshold) gate = 2;
                else if (gate_x < -standard_threshold) gate = 3;
                else if (gate_x > standard_threshold) gate = 4;
            }
            if (gate != 0) runtime.gear = gate;
            const bool down = pressed(RETRO_DEVICE_ID_JOYPAD_L);
            const bool up = pressed(RETRO_DEVICE_ID_JOYPAD_R);
            if (down && !runtime.shift_down_held && runtime.gear > 0) --runtime.gear;
            if (up && !runtime.shift_up_held && runtime.gear < 4) ++runtime.gear;
            runtime.shift_down_held = down;
            runtime.shift_up_held = up;
            if (pressed(RETRO_DEVICE_ID_JOYPAD_Y)) runtime.gear = 0;
            inputs.gears = static_cast<u8>(1u << runtime.gear);
            continue;
        }
        if (profile == InputProfile::DrivingSequentialVR2
            || profile == InputProfile::DrivingSequentialVR1) {
            if (p != 0) continue;
            const auto analog = [&](unsigned index, unsigned id) {
                return state(0, RETRO_DEVICE_ANALOG, index, id);
            };
            set_analog_control(inputs, game, rom::AnalogControl::Steer,
                               analog(RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                      RETRO_DEVICE_ID_ANALOG_X), false, &driving_options);
            set_analog_control(inputs, game, rom::AnalogControl::Brake,
                               analog(RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                                      RETRO_DEVICE_ID_JOYPAD_L2), true, &driving_options);
            set_analog_control(inputs, game, rom::AnalogControl::Accel,
                               analog(RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                                      RETRO_DEVICE_ID_JOYPAD_R2), true, &driving_options);
            u8* ports[] = {&inputs.in0, &inputs.in1, &inputs.in2};
            const auto press_wheel_button = [&](unsigned index) {
                const auto [port, bit] = game.wheel_button_bits[index];
                if (port < std::size(ports)) clear(*ports[port], bit);
            };
            if (pressed(RETRO_DEVICE_ID_JOYPAD_UP)) press_wheel_button(0);
            if (profile == InputProfile::DrivingSequentialVR2
                && pressed(RETRO_DEVICE_ID_JOYPAD_DOWN))
                press_wheel_button(1);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R)) clear(inputs.in1, 0x10);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L)) clear(inputs.in1, 0x20);
            continue;
        }
        if (profile == InputProfile::DrivingSequentialManxTT
            || profile == InputProfile::DrivingSequentialMotorRaid) {
            if (p != 0) continue;
            const auto analog = [&](unsigned index, unsigned id) {
                return state(0, RETRO_DEVICE_ANALOG, index, id);
            };
            set_analog_control(inputs, game, rom::AnalogControl::Bank,
                               analog(RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                      RETRO_DEVICE_ID_ANALOG_X), false);
            set_analog_control(inputs, game, rom::AnalogControl::Brake,
                               analog(RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                                      RETRO_DEVICE_ID_JOYPAD_L2), true, &driving_options);
            set_analog_control(inputs, game, rom::AnalogControl::Throttle,
                               analog(RETRO_DEVICE_INDEX_ANALOG_BUTTON,
                                      RETRO_DEVICE_ID_JOYPAD_R2), true, &driving_options);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_R)
                || (profile == InputProfile::DrivingSequentialMotorRaid
                    && pressed(RETRO_DEVICE_ID_JOYPAD_A)))
                clear(inputs.in1, 0x10);
            if (pressed(RETRO_DEVICE_ID_JOYPAD_L)
                || (profile == InputProfile::DrivingSequentialMotorRaid
                    && pressed(RETRO_DEVICE_ID_JOYPAD_B)))
                clear(inputs.in1, 0x20);
            continue;
        }
        u8& port = p == 0 ? inputs.in1 : inputs.in2;
        for (const auto& binding : directions)
            if (pressed(binding.id)) clear(port, binding.bit);
        size_t count = 0;
        const auto* bindings = profile_bindings(profile, count);
        for (size_t i = 0; i < count; ++i)
            if (pressed(bindings[i].id)) clear(port, bindings[i].bit);
    }
}
}
