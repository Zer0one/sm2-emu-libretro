// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"
#include "hw/model2_machine_base.h"
#include <array>
#include <string>
#include <vector>

namespace sm2::libretro {
constexpr unsigned kNoServiceDevice = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0);

enum class InputProfile {
    Unsupported,
    Action,
    Fighting,
    FightingDeadOrAlive,
    FightingSonicChampionship,
    Shooter,
    Soccer,
    Twin,
    Driving4SpeedVR4,
    Driving4SpeedVR1Handbrake,
    DrivingSequentialVR2,
    DrivingSequentialVR1,
    DrivingSequentialManxTT,
    DrivingSequentialMotorRaid,
    JoystickAnalogSkyTarget,
    BaseballDynamite,
    SpecialWaterSki,
    SpecialSkiSuperG,
    SpecialTopSkater,
    SpecialWaveRunner,
    BasketballAirWalkers,
    HorseRacingRoyalAscotII,
    SpecialDesertTank,
    BaseballHangukProYagu98,
    Gun,
    GunBehindEnemyLines,
};

enum class GunInputMode {
    Hybrid,
    Lightgun,
    Mouse,
    MouseAnalog,
    AnalogSticks,
};

enum class SteeringResponse {
    Linear,
    Progressive,
    FBNeoLogarithmic,
};

enum class DesertElevationControl {
    Relative,
    Absolute,
};

struct DesertElevationOptions {
    DesertElevationControl control = DesertElevationControl::Relative;
    int speed_percent = 100;
    bool inverted = false;
};

struct DrivingAnalogOptions {
    SteeringResponse steering_response = SteeringResponse::Linear;
    int steering_output_range = 100;
    int accelerator_output_range_per_mille = 1000;
    int brake_output_range_per_mille = 1000;
};

struct InputRuntime {
    unsigned gear = 0;
    bool gear_initialized = false;
    bool shift_up_held = false;
    bool shift_down_held = false;
    bool desert_shift = false;
    bool desert_shift_held = false;
    float desert_elevation = 128.0f;
    std::array<int, 2> gun_cursor_x{248, 248};
    std::array<int, 2> gun_cursor_y{192, 192};
    std::array<s16, 2> gun_lightgun_x{};
    std::array<s16, 2> gun_lightgun_y{};
    std::array<bool, 2> gun_lightgun_initialized{};
    std::array<bool, 2> gun_aim_active{};
    std::array<bool, 2> gun_aim_offscreen{};
};

struct ControllerConfiguration {
    std::array<std::string, 2> base_names;
    std::array<std::string, 2> full_names;
    std::array<std::array<retro_controller_description, 2>, 2> descriptions{};
    std::array<retro_controller_info, 3> ports{};
};

InputProfile recognize_profile(const rom::GameSpec& game);
const char* profile_name(InputProfile profile);
unsigned profile_players(InputProfile profile);
bool digital_profile(const rom::GameSpec& game);
void configure_controllers(const rom::GameSpec& game, ControllerConfiguration& configuration,
                           GunInputMode gun_mode = GunInputMode::Hybrid);
std::vector<retro_input_descriptor> descriptors(
    const rom::GameSpec& game, const std::array<unsigned, 2>& devices,
    GunInputMode gun_mode = GunInputMode::Hybrid,
    bool offscreen_reload_shortcut = true);
void poll_input(hw::Inputs& inputs, const rom::GameSpec& game,
                const std::array<unsigned, 2>& devices, InputRuntime& runtime,
                bool h_gate_shifter, retro_input_state_t state,
                GunInputMode gun_mode = GunInputMode::Hybrid,
                bool offscreen_reload_shortcut = true,
                DrivingAnalogOptions driving_options = {},
                DesertElevationOptions desert_elevation_options = {},
                bool automatic_start_gear = true);
}
