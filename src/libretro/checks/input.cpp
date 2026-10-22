// SPDX-License-Identifier: BSD-3-Clause
#include "input.h"
#include "crosshair.h"
#include "rom/game_db.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace sm2;
namespace {
std::array<unsigned, libretro::kMaxInputPorts> pressed{};
std::array<std::array<std::array<s16, 2>, 2>, libretro::kMaxInputPorts> axes{};
std::array<std::array<s16, 16>, libretro::kMaxInputPorts> analog_buttons{};
std::array<std::array<s16, 2>, libretro::kMaxInputPorts> mouse_axes{};
std::array<unsigned, libretro::kMaxInputPorts> mouse_buttons{};
std::array<std::array<s16, 2>, libretro::kMaxInputPorts> lightgun_axes{};
std::array<unsigned, libretro::kMaxInputPorts> lightgun_buttons{};

int16_t state(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port >= libretro::kMaxInputPorts)
        throw std::runtime_error("Invalid frontend input port");
    if (device == RETRO_DEVICE_JOYPAD) {
        if (index != 0 || id >= 16) throw std::runtime_error("Invalid joypad query");
        return (pressed[port] & (1u << id)) != 0;
    }
    if (device == RETRO_DEVICE_ANALOG) {
        if (index == RETRO_DEVICE_INDEX_ANALOG_BUTTON) {
            if (id >= 16) throw std::runtime_error("Invalid analog button query");
            return analog_buttons[port][id];
        }
        if (index > 1 || id > 1) throw std::runtime_error("Invalid analog axis query");
        return axes[port][index][id];
    }
    if (device == RETRO_DEVICE_MOUSE) {
        if (index != 0) throw std::runtime_error("Invalid mouse query");
        if (id == RETRO_DEVICE_ID_MOUSE_X || id == RETRO_DEVICE_ID_MOUSE_Y)
            return mouse_axes[port][id];
        return id < 32 && (mouse_buttons[port] & (1u << id)) != 0;
    }
    if (device == RETRO_DEVICE_LIGHTGUN) {
        if (index != 0) throw std::runtime_error("Invalid lightgun query");
        if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X)
            return lightgun_axes[port][0];
        if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y)
            return lightgun_axes[port][1];
        return id < 32 && (lightgun_buttons[port] & (1u << id)) != 0;
    }
    throw std::runtime_error("Invalid frontend input device");
}
void check(bool okay, const char* what)
{
    if (!okay) throw std::runtime_error(what);
}
rom::GameSpec game(std::string_view name, std::string_view parent = {})
{
    rom::GameSpec result;
    result.name = name;
    result.parent = parent;
    result.inputs = rom::InputFlags::Common | rom::InputFlags::Joystick1
        | rom::InputFlags::Joystick2 | rom::InputFlags::Buttons3;
    return result;
}
bool has_descriptor(const std::vector<retro_input_descriptor>& descriptors,
                    unsigned port, unsigned device, unsigned index, unsigned id,
                    std::string_view label)
{
    for (const auto& descriptor : descriptors) {
        if (descriptor.description && descriptor.port == port && descriptor.device == device
            && descriptor.index == index && descriptor.id == id
            && descriptor.description == label)
            return true;
    }
    return false;
}
bool has_descriptor_id(const std::vector<retro_input_descriptor>& descriptors,
                       unsigned port, unsigned device, unsigned index, unsigned id)
{
    for (const auto& descriptor : descriptors)
        if (descriptor.description && descriptor.port == port && descriptor.device == device
            && descriptor.index == index && descriptor.id == id)
            return true;
    return false;
}
void check_profile(std::string_view root, libretro::InputProfile expected, const char* name)
{
    auto parent = game(root);
    check(libretro::recognize_profile(parent) == expected, name);
    auto clone = game(std::string(root) + "_clone", root);
    check(libretro::recognize_profile(clone) == expected, "Clone must inherit parent profile");
}
}

int main()
{
    check_profile("dynamcop", libretro::InputProfile::Action, "Dynamite Cop profile");
    check_profile("fvipers", libretro::InputProfile::Fighting, "Fighting Vipers profile");
    check_profile("lastbrnx", libretro::InputProfile::Fighting, "Last Bronx profile");
    check_profile("vf2", libretro::InputProfile::Fighting, "Virtua Fighter 2 profile");
    check_profile("doa", libretro::InputProfile::FightingDeadOrAlive, "Dead or Alive profile");
    check_profile("schamp", libretro::InputProfile::FightingSonicChampionship, "Sonic Championship profile");
    check_profile("pltkids", libretro::InputProfile::Shooter, "Pilot Kids profile");
    check_profile("zerogun", libretro::InputProfile::Shooter, "Zero Gunner profile");
    check_profile("zeroguna", libretro::InputProfile::Shooter, "Zero Gunner A profile");
    check_profile("vstriker", libretro::InputProfile::Soccer, "Virtua Striker profile");
    check_profile("von", libretro::InputProfile::Twin, "Virtual On profile");

    struct CatalogEntry { const char* name; libretro::InputProfile profile; };
    constexpr CatalogEntry catalog[] = {
        {"vf2", libretro::InputProfile::Fighting},
        {"vf2b", libretro::InputProfile::Fighting},
        {"vf2a", libretro::InputProfile::Fighting},
        {"vf2o", libretro::InputProfile::Fighting},
        {"zeroguna", libretro::InputProfile::Shooter},
        {"zerogunaj", libretro::InputProfile::Shooter},
        {"doaa", libretro::InputProfile::FightingDeadOrAlive},
        {"doaab", libretro::InputProfile::FightingDeadOrAlive},
        {"doa", libretro::InputProfile::FightingDeadOrAlive},
        {"doaae", libretro::InputProfile::FightingDeadOrAlive},
        {"doab", libretro::InputProfile::FightingDeadOrAlive},
        {"dynamcop", libretro::InputProfile::Action},
        {"dynamcopb", libretro::InputProfile::Action},
        {"dynamcopc", libretro::InputProfile::Action},
        {"dyndeka2", libretro::InputProfile::Action},
        {"dyndeka2b", libretro::InputProfile::Action},
        {"fvipers", libretro::InputProfile::Fighting},
        {"fvipersa", libretro::InputProfile::Fighting},
        {"fvipersb", libretro::InputProfile::Fighting},
        {"lastbrnx", libretro::InputProfile::Fighting},
        {"lastbrnxj", libretro::InputProfile::Fighting},
        {"lastbrnxu", libretro::InputProfile::Fighting},
        {"pltkids", libretro::InputProfile::Shooter},
        {"pltkidsa", libretro::InputProfile::Shooter},
        {"schamp", libretro::InputProfile::FightingSonicChampionship},
        {"sfight", libretro::InputProfile::FightingSonicChampionship},
        {"von", libretro::InputProfile::Twin},
        {"vonj", libretro::InputProfile::Twin},
        {"vonr", libretro::InputProfile::Twin},
        {"vonu", libretro::InputProfile::Twin},
        {"vstriker", libretro::InputProfile::Soccer},
        {"vstrikero", libretro::InputProfile::Soccer},
        {"zerogun", libretro::InputProfile::Shooter},
        {"zerogunj", libretro::InputProfile::Shooter},
        {"daytona", libretro::InputProfile::Driving4SpeedVR4},
        {"daytona93", libretro::InputProfile::Driving4SpeedVR4},
        {"daytonas", libretro::InputProfile::Driving4SpeedVR4},
        {"daytonase", libretro::InputProfile::Driving4SpeedVR4},
        {"daytonam", libretro::InputProfile::Driving4SpeedVR4},
        {"daytonagtx", libretro::InputProfile::Driving4SpeedVR4},
        {"srallyc", libretro::InputProfile::Driving4SpeedVR1Handbrake},
        {"srallycb", libretro::InputProfile::Driving4SpeedVR1Handbrake},
        {"srallycc", libretro::InputProfile::Driving4SpeedVR1Handbrake},
        {"srallycdx", libretro::InputProfile::Driving4SpeedVR1Handbrake},
        {"srallycdxa", libretro::InputProfile::Driving4SpeedVR1Handbrake},
        {"indy500", libretro::InputProfile::DrivingSequentialVR2},
        {"indy500d", libretro::InputProfile::DrivingSequentialVR2},
        {"indy500to", libretro::InputProfile::DrivingSequentialVR2},
        {"overrev", libretro::InputProfile::DrivingSequentialVR2},
        {"overrevb", libretro::InputProfile::DrivingSequentialVR2},
        {"overrevba", libretro::InputProfile::DrivingSequentialVR2},
        {"stcc", libretro::InputProfile::DrivingSequentialVR2},
        {"stcca", libretro::InputProfile::DrivingSequentialVR2},
        {"stccb", libretro::InputProfile::DrivingSequentialVR2},
        {"stcco", libretro::InputProfile::DrivingSequentialVR2},
        {"sgt24h", libretro::InputProfile::DrivingSequentialVR2},
        {"manxtt", libretro::InputProfile::DrivingSequentialManxTT},
        {"manxttc", libretro::InputProfile::DrivingSequentialManxTT},
        {"manxttdx", libretro::InputProfile::DrivingSequentialManxTT},
        {"motoraid", libretro::InputProfile::DrivingSequentialMotorRaid},
        {"motoraiddx", libretro::InputProfile::DrivingSequentialMotorRaid},
        {"skytargt", libretro::InputProfile::JoystickAnalogSkyTarget},
        {"dynabb", libretro::InputProfile::BaseballDynamite},
        {"dynabb97", libretro::InputProfile::BaseballDynamite},
        {"segawski", libretro::InputProfile::SpecialWaterSki},
        {"skisuprg", libretro::InputProfile::SpecialSkiSuperG},
        {"topskatr", libretro::InputProfile::SpecialTopSkater},
        {"topskatrj", libretro::InputProfile::SpecialTopSkater},
        {"topskatru", libretro::InputProfile::SpecialTopSkater},
        {"topskatruo", libretro::InputProfile::SpecialTopSkater},
        {"waverunr", libretro::InputProfile::SpecialWaveRunner},
        {"airwlkrs", libretro::InputProfile::BasketballAirWalkers},
        {"rascot2", libretro::InputProfile::HorseRacingRoyalAscotII},
        {"desert", libretro::InputProfile::SpecialDesertTank},
        {"hpyagu98", libretro::InputProfile::BaseballHangukProYagu98},
        {"bel", libretro::InputProfile::GunBehindEnemyLines},
        {"gunblade", libretro::InputProfile::Gun},
        {"rchase2", libretro::InputProfile::Gun},
        {"rchase2a", libretro::InputProfile::Gun},
        {"vcop", libretro::InputProfile::Gun},
        {"vcopa", libretro::InputProfile::Gun},
        {"vcop2", libretro::InputProfile::Gun},
        {"hotd", libretro::InputProfile::Gun},
        {"hotdo", libretro::InputProfile::Gun},
        {"hotdp", libretro::InputProfile::Gun},
        {"powsled", libretro::InputProfile::SpecialPowerSled},
        {"powsledm", libretro::InputProfile::SpecialPowerSled},
        {"powsledr", libretro::InputProfile::SpecialPowerSled},

    };
    rom::GameDatabase database;
    check(database.load(SM2_GAMES_XML_PATH), "Load catalog games.xml");
    check(database.find("von") && database.find("vonr")
              && database.find("von")->texture_wait == 37
              && database.find("vonr")->texture_wait == 37,
          "Virtual-On texture waits are inherited by its clone");
    for (const auto& entry : catalog) {
        const auto* catalog_game = database.find(entry.name);
        check(catalog_game != nullptr, "Cataloged digital set exists");
        check(libretro::recognize_profile(*catalog_game) == entry.profile,
              "Cataloged digital set resolves to reviewed profile");
    }
    size_t recognized_sets = 0;
    for (const auto& catalog_game : database.games())
        if (libretro::recognize_profile(catalog_game) != libretro::InputProfile::Unsupported)
            ++recognized_sets;
    check(recognized_sets == std::size(catalog),
          "Recognizer coverage must match the reviewed digital catalog exactly");

    struct StandardDescriptions {
        const char* root;
        const char* profile;
        const char* south;
        const char* east;
        const char* west;
    };
    constexpr StandardDescriptions approved_descriptions[] = {
        {"dynamcop", "Joystick (Standard): Action", "Kick", "Punch", "Jump"},
        {"vf2", "Joystick (Standard): Fighting", "Kick", "Punch", "Guard"},
        {"doa", "Joystick (Standard): Fighting (Dead or Alive)", "Kick", "Punch", "Hold"},
        {"schamp", "Joystick (Standard): Fighting (Sonic Championship)", "Kick", "Punch", "Barrier"},
        {"pltkids", "Joystick (Standard): Shooter", "Button 1", "Button 2", nullptr},
        {"hpyagu98", "Joystick (Standard): Baseball (Hanguk Pro Yagu 98)",
         "Button 1", "Button 2", "Button 3"},
        {"vstriker", "Joystick (Standard): Soccer", "Short Pass", "Long Pass", "Shoot"},
    };
    libretro::InputDevices full_devices{
        RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD,
        RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD,
    };
    for (const auto& approved : approved_descriptions) {
        auto approved_game = game(approved.root);
        check(std::string_view(libretro::profile_name(libretro::recognize_profile(approved_game)))
                  == approved.profile,
              "Profile name must match the approved workbook");
        const auto approved_desc = libretro::descriptors(approved_game, full_devices);
        check(has_descriptor(approved_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_B, approved.south),
              "South description must match the approved workbook");
        check(has_descriptor(approved_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_A, approved.east),
              "East description must match the approved workbook");
        const bool west_present = has_descriptor(approved_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                                  RETRO_DEVICE_ID_JOYPAD_Y,
                                                  approved.west ? approved.west : "");
        check(west_present == (approved.west != nullptr),
              "West description must match the approved workbook");
    }

    auto unknown = game("unknown");
    check(libretro::recognize_profile(unknown) == libretro::InputProfile::Unsupported,
          "Unknown set must use explicit fallback");
    auto changed = game("vf2");
    changed.inputs = changed.inputs | rom::InputFlags::Vehicle;
    check(libretro::recognize_profile(changed) == libretro::InputProfile::Unsupported,
          "Changed catalog signature must require review");

    libretro::ControllerConfiguration controllers;
    auto vf2 = game("vf2");
    libretro::configure_controllers(vf2, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Standard): Fighting + Test/Service slots",
          "Full profile device name");
    check(controllers.descriptions[0][0].id == RETRO_DEVICE_JOYPAD
              && controllers.ports[0].num_types == 1,
          "Controls exposes only one profile; Core Option owns the variant");
    check(std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Fighting + Test/Service slots"
              && controllers.ports[1].num_types == 1
              && controllers.ports[1].num_types == 1,
          "Player 2 exposes one controller type");

    for (const auto& entry : database.games()) {
        if (libretro::recognize_profile(entry) == libretro::InputProfile::Unsupported) continue;
        const libretro::InputDevices frontend{RETRO_DEVICE_JOYPAD, libretro::kNoServiceDevice,
                                             RETRO_DEVICE_NONE, RETRO_DEVICE_JOYPAD};
        for (bool enabled : {false, true}) {
            const auto effective = libretro::profile_devices(frontend, enabled);
            check(effective[0] == effective[1] && effective[1] == effective[3]
                      && effective[2] == RETRO_DEVICE_NONE,
                  "Global profile overrides legacy per-port variants and preserves None");
            libretro::configure_controllers(entry, controllers, libretro::GunInputMode::Hybrid, enabled);
            for (const auto& port : controllers.ports)
                check(port.num_types <= 1, "Controls cannot select another profile variant");
            const auto global_desc = libretro::descriptors(entry, effective);
            check(has_descriptor(global_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                 RETRO_DEVICE_ID_JOYPAD_R3, "Test A") == enabled,
                  "Global variant controls Test descriptor for every supported game");
        }
    }
    libretro::configure_controllers(vf2, controllers, libretro::GunInputMode::Hybrid, false);
    check(std::string_view(controllers.descriptions[0][0].desc) == "Joystick (Standard): Fighting",
          "Normal controller type uses the game-specific name without Test/Service");
    libretro::InputDevices devices{
        RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD,
        RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD,
    };
    auto desc = libretro::descriptors(vf2, devices);
    check(has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "Joystick Up"),
          "Excel direction description");
    check(has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_B, "Kick"),
          "Excel fighting description");
    check(has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_L3, "Service A")
              && has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R3, "Test A"),
          "Default profile exposes Service on L3 and Test on R3");
    check(has_descriptor(desc, 1, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_L3, "Service A")
              && has_descriptor(desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R3, "Test A"),
          "Player 2 exposes virtual aliases for the Model 2 Service and Test lines");
    devices[0] = libretro::kNoServiceDevice;
    desc = libretro::descriptors(vf2, devices);
    check(!has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                          RETRO_DEVICE_ID_JOYPAD_L3, "Service A")
              && !has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                 RETRO_DEVICE_ID_JOYPAD_R3, "Test A"),
          "Reduced profile hides Service A and Test A");
    check(has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_B, "Kick"),
          "Reduced profile preserves gameplay binding");

    hw::Inputs inputs;
    libretro::InputRuntime runtime;
    // MAME wires independent rider handlebars to mux channels 1/3 and 5/7.
    for (const auto* name : {"powsled", "powsledm", "powsledr"}) {
        const auto* sled = database.find(name);
        check(sled && sled->preliminary, "Power Sled variants remain preliminary");
        devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
        const auto sled_desc = libretro::descriptors(*sled, devices);
        check(has_descriptor(sled_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                  RETRO_DEVICE_ID_JOYPAD_R3, "Cancel Network Check"),
              "Power Sled exposes upstream network-wait cancel");
        check(has_descriptor(sled_desc, 0, RETRO_DEVICE_ANALOG,
                  RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,
                  "P1 Right Handlebar"), "Power Sled handlebar descriptor");
        check(has_descriptor(sled_desc, 1, RETRO_DEVICE_ANALOG,
                  RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,
                  "P2 Right Handlebar"), "Power Sled second-rider descriptor");
        for (unsigned player = 0; player < 2; ++player) {
            check(has_descriptor(sled_desc, player, RETRO_DEVICE_JOYPAD, 0,
                      RETRO_DEVICE_ID_JOYPAD_B, player == 0 ? "P1 Entry" : "P2 Entry")
                      && has_descriptor(sled_desc, player, RETRO_DEVICE_JOYPAD, 0,
                          RETRO_DEVICE_ID_JOYPAD_A, player == 0 ? "P1 Call" : "P2 Call"),
                  "Power Sled maps Xbox A / RetroPad B to Entry and Xbox B / RetroPad A to Call");
            check(has_descriptor(sled_desc, player, RETRO_DEVICE_ANALOG,
                      RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,
                      player == 0 ? "P1 Left Handlebar" : "P2 Left Handlebar")
                      && !has_descriptor_id(sled_desc, player, RETRO_DEVICE_ANALOG,
                          RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X)
                      && !has_descriptor_id(sled_desc, player, RETRO_DEVICE_ANALOG,
                          RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X),
                  "Power Sled exposes only Y axes for both riders' handles");
        }
        check(has_descriptor(sled_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                  RETRO_DEVICE_ID_JOYPAD_L3, "Service A / Select"),
              "Power Sled service selection belongs to P1");
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L3, 0};
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in0 == 0xbf && inputs.in1 == 0xff && inputs.in3 == 0xff,
              "Power Sled P1 Service A clears only the selection line");
        devices[0] = libretro::kNoServiceDevice;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in0 == 0xff, "Power Sled reduced P1 disables Service");
        devices[0] = RETRO_DEVICE_JOYPAD;
        for (const auto button : {RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN}) {
            for (unsigned port = 0; port < 2; ++port) {
                pressed = {}; pressed[port] = 1u << button;
                libretro::poll_input(inputs, *sled, devices, runtime, true, state);
                check(inputs.in0 == 0xff && inputs.in1 == (port == 0 ? 0xfe : 0xfb),
                      "D-Pad Entry aliases do not send Shutdown or Service");
            }
        }
        for (const auto button : {RETRO_DEVICE_ID_JOYPAD_L3, RETRO_DEVICE_ID_JOYPAD_R3,
                                  RETRO_DEVICE_ID_JOYPAD_X}) {
            pressed = {}; pressed[1] = 1u << button;
            libretro::poll_input(inputs, *sled, devices, runtime, true, state);
            check(inputs.in0 == (button == RETRO_DEVICE_ID_JOYPAD_L3 ? 0x7f
                                  : button == RETRO_DEVICE_ID_JOYPAD_X ? 0xf7 : 0xff)
                      && inputs.in3 == (button == RETRO_DEVICE_ID_JOYPAD_R3 ? 0xfd : 0xff)
                      && inputs.in1 == 0xff,
                  "P2 Shutdown and Cancel inputs never send Service, Test or Call");
        }
        check(!has_descriptor(sled_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                  RETRO_DEVICE_ID_JOYPAD_L3, "Service A / Select")
              && !has_descriptor(sled_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                  RETRO_DEVICE_ID_JOYPAD_R3, "Test A")
              && has_descriptor(sled_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                  RETRO_DEVICE_ID_JOYPAD_X, "Shutdown Button"),
              "P2 descriptors have Shutdown and Cancel without duplicate Service/Test");
        // Native operator command is available on Main/Relay Port 1 only.
        for (unsigned port = 0; port < 2; ++port) {
            pressed = {};
            pressed[port] = 1u << RETRO_DEVICE_ID_JOYPAD_START;
            const bool operator_start = port == 0 && sled->name != "powsled";
            libretro::poll_input(inputs, *sled, devices, runtime, true, state);
            check(inputs.in1 == (operator_start ? 0xef : 0xff)
                      && inputs.in0 == (port == 0 ? 0xef : 0xdf)
                      && inputs.in3 == 0xff,
                  "Power Sled Start preserves its old line and isolates native operator input");
            devices[port] = libretro::kNoServiceDevice;
            libretro::poll_input(inputs, *sled, devices, runtime, true, state);
            check(inputs.in1 == (operator_start ? 0xef : 0xff),
                  "Power Sled operator Start remains a gameplay action without Service");
            devices[port] = RETRO_DEVICE_NONE;
            libretro::poll_input(inputs, *sled, devices, runtime, true, state);
            check(inputs.in0 == 0xff && inputs.in1 == 0xff,
                  "Power Sled disabled port cannot send operator Start");
            devices[port] = RETRO_DEVICE_JOYPAD;
        }
        pressed = {};
        analog_buttons = {};
        pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_B;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in1 == static_cast<u8>(0xff & ~0x01), "Power Sled P1 Entry wiring");
        pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_A;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in1 == static_cast<u8>(0xff & ~0x02), "Power Sled P1 Call wiring");
        for (const auto button : {RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_X}) {
            const bool entry = button == RETRO_DEVICE_ID_JOYPAD_Y;
            pressed = {};
            pressed[0] = 1u << button;
            devices[1] = RETRO_DEVICE_NONE;
            libretro::poll_input(inputs, *sled, devices, runtime, true, state);
            check(inputs.in1 == (entry ? 0xfb : 0xf7)
                      && has_descriptor(sled_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                          button, entry ? "P2 Entry" : "P2 Call"),
                  "Power Sled P1 West/North independently supply P2 Entry/Call without controller 2");
            devices[1] = RETRO_DEVICE_JOYPAD;
            pressed = {};
            pressed[1] = 1u << button;
            libretro::poll_input(inputs, *sled, devices, runtime, true, state);
            check(inputs.in1 == 0xff && inputs.in0 == (entry ? 0xff : 0xf7),
                  "P2 North is Shutdown, not the P1 secondary Call alias");
        }
        pressed = {};
        pressed[1] = 1u << RETRO_DEVICE_ID_JOYPAD_B;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in1 == 0xfb, "Power Sled P2 Entry wiring");
        pressed[1] = 1u << RETRO_DEVICE_ID_JOYPAD_A;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in1 == 0xf7, "Power Sled P2 Call wiring");
        pressed = {};
        pressed[1] = 1u << RETRO_DEVICE_ID_JOYPAD_L3;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in0 == static_cast<u8>(0xff & ~0x80), "Power Sled Cancel Error wiring");
        pressed[1] = 1u << RETRO_DEVICE_ID_JOYPAD_R3;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in3 == static_cast<u8>(0xff & ~0x02), "Power Sled Cancel Network Check wiring");
        pressed = {};
        axes = {};
        axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = 32767;
        axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.analog[1] == 0 && inputs.analog[3] == 0xff,
              "Power Sled inverted Y handles reach both endpoints on mux 1 and 3");
        pressed[1] = (1u << RETRO_DEVICE_ID_JOYPAD_B) | (1u << RETRO_DEVICE_ID_JOYPAD_A);
        axes[1][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = -16384;
        axes[1][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = 16384;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in1 == 0xf3 && inputs.analog[5] == 0xbf && inputs.analog[7] == 0x40
                  && inputs.analog[1] == 0 && inputs.analog[3] == 0xff,
              "Power Sled second rider is independent and preserves P1");
        // Exercise both signed extremes on both riders, including -32768.
        for (unsigned player = 0; player < 2; ++player) {
            axes[player][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
            axes[player][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = 32767;
        }
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.analog[1] == 0xff && inputs.analog[3] == 0
                  && inputs.analog[5] == 0xff && inputs.analog[7] == 0,
              "Power Sled inverted extremes do not wrap on either controller");
        devices[1] = RETRO_DEVICE_NONE;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in1 == 0xff && inputs.analog[5] == 0x80 && inputs.analog[7] == 0x80,
              "Power Sled disabled second port returns its handles to center");
        devices[1] = RETRO_DEVICE_JOYPAD;
        pressed = {};
        axes = {};
        // Old trigger bindings must not contribute a second handle source.
        for (unsigned player = 0; player < 2; ++player) {
            axes[player][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
            axes[player][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
        }
        analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
        analog_buttons[1][RETRO_DEVICE_ID_JOYPAD_L2] = 32767;
        libretro::poll_input(inputs, *sled, devices, runtime, true, state);
        check(inputs.in0 == 0xff && inputs.in1 == 0xff && inputs.in3 == 0xff
                  && inputs.analog[1] == 0x80 && inputs.analog[3] == 0x80
                  && inputs.analog[5] == 0x80 && inputs.analog[7] == 0x80,
              "Power Sled centers Y handles and ignores X axes and triggers");
        analog_buttons = {};
        axes = {};

    }
    inputs.analog[0] = 0x77;
    inputs.gun_p1x = 0x123;
    auto poll = [&](const rom::GameSpec& selected) {
        libretro::poll_input(inputs, selected, devices, runtime, true, state);
    };
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_SELECT,
               1u << RETRO_DEVICE_ID_JOYPAD_START};
    poll(vf2);
    check(inputs.in0 == 0xde, "Independent P1 coin and P2 start");
    pressed = {0, 1u << RETRO_DEVICE_ID_JOYPAD_B};
    poll(vf2);
    check(inputs.in1 == 0xff && inputs.in2 == 0xfd, "P1/P2 gameplay is independent");
    devices[1] = RETRO_DEVICE_NONE;
    poll(vf2);
    check(inputs.in2 == 0xff, "Disconnected device releases controls");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L3 | 1u << RETRO_DEVICE_ID_JOYPAD_R3, 0};
    poll(vf2);
    check(inputs.in0 == 0xf3, "Default profile maps Test and Service");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L3, 0};
    poll(vf2);
    check(inputs.in0 == 0xf7, "L3 maps Service on standard hardware");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R3, 0};
    poll(vf2);
    check(inputs.in0 == 0xfb, "R3 maps Test on standard hardware");
    pressed = {0, (1u << RETRO_DEVICE_ID_JOYPAD_L3)
                    | (1u << RETRO_DEVICE_ID_JOYPAD_R3)};
    poll(vf2);
    check(inputs.in0 == 0xf3, "Player 2 aliases drive the same Service A and Test A lines");
    devices[1] = libretro::kNoServiceDevice;
    poll(vf2);
    check(inputs.in0 == 0xff, "Player 2 reduced profile disables the virtual cabinet aliases");
    devices[0] = libretro::kNoServiceDevice;
    poll(vf2);
    check(inputs.in0 == 0xff, "Reduced profiles do not map Test or Service");
    check(inputs.analog[0] == 0x77 && inputs.gun_p1x == 0x123,
          "Digital adapter preserves calibrated analog and gun state");

    struct PunchKickLayout {
        const char* root;
        u8 south_kick_bit;
        u8 east_punch_bit;
        u8 west_other_bit;
    };
    constexpr PunchKickLayout punch_kick_layouts[] = {
        {"dynamcop", 0x02, 0x01, 0x04},
        {"fvipers", 0x02, 0x01, 0x04},
        {"lastbrnx", 0x02, 0x01, 0x04},
        {"vf2", 0x02, 0x01, 0x04},
        {"doa", 0x04, 0x02, 0x01},
        {"schamp", 0x02, 0x01, 0x04},
    };
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    for (const auto& layout : punch_kick_layouts) {
        const auto selected = game(layout.root);
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_B, 0};
        poll(selected);
        check(inputs.in1 == static_cast<u8>(0xffu & ~layout.south_kick_bit),
              "South maps the title's Kick hardware action");
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_A, 0};
        poll(selected);
        check(inputs.in1 == static_cast<u8>(0xffu & ~layout.east_punch_bit),
              "East maps the title's Punch hardware action");
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_Y, 0};
        poll(selected);
        check(inputs.in1 == static_cast<u8>(0xffu & ~layout.west_other_bit),
              "West preserves the title's third action");
    }

    auto soccer = game("vstriker");
    devices[0] = RETRO_DEVICE_JOYPAD;
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_B, 0};
    poll(soccer);
    check(inputs.in1 == 0xfb, "Soccer South writes hardware bit 0x04");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_A, 0};
    poll(soccer);
    check(inputs.in1 == 0xfe, "Soccer East writes hardware bit 0x01");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_Y, 0};
    poll(soccer);
    check(inputs.in1 == 0xfd, "Soccer West writes hardware bit 0x02");
    desc = libretro::descriptors(soccer, devices);
    check(has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_B, "Short Pass")
              && has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Long Pass")
              && has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "Shoot"),
          "Excel Soccer descriptions and positions");

    auto von = game("von");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L | 1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = -0x5000;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = 0x5000;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = 0x5000;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = -0x5000;
    poll(von);
    check(inputs.in1 == 0x6d, "Virtual On left stick and Left Dash mapping");
    check(inputs.in2 == 0x9e, "Virtual On right stick and Right Shot Trigger mapping");
    desc = libretro::descriptors(von, devices);
    check(has_descriptor(desc, 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
                         RETRO_DEVICE_ID_ANALOG_X, "Left Joystick X")
              && has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Left Dash (Turbo)")
              && has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Right Dash (Turbo)")
              && has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Right Shot Trigger"),
          "Excel Virtual On descriptions and positions");

    desc = libretro::descriptors(unknown, devices);
    check(desc.back().description == nullptr, "Descriptor list terminates");
    check(!has_descriptor(desc, 0, RETRO_DEVICE_JOYPAD, 0,
                          RETRO_DEVICE_ID_JOYPAD_B, "Button 1"),
          "Fallback does not invent gameplay controls");

    const auto* daytona = database.find("daytona");
    const auto* srally = database.find("srallyc");
    const auto* stcc_metadata = database.find("stcc");
    const auto* sgt24h_metadata = database.find("sgt24h");
    const auto* overrevb = database.find("overrevb");
    check(daytona && srally && stcc_metadata && sgt24h_metadata && overrevb,
          "Updated driving metadata sets exist");
    check(daytona->start_gear == 4 && srally->start_gear == 1,
          "Daytona starts in fourth while Sega Rally retains first gear");
    check(srally->drive_board && srally->drive_protocol == rom::DriveProtocol::Rally
              && stcc_metadata->drive_board
              && stcc_metadata->drive_protocol == rom::DriveProtocol::Stcc,
          "Sega Rally and STCC retain their upstream drive-board protocols");
    check(sgt24h_metadata->drive_board
          && sgt24h_metadata->drive_protocol == rom::DriveProtocol::Indy,
          "Super GT uses the upstream shared Indy drive-board protocol");
    for (const char* name : {"indy500", "indy500d", "indy500to", "overrev", "overrevb", "overrevba"}) {
        const auto* set = database.find(name);
        check(set && set->drive_board && set->drive_protocol == rom::DriveProtocol::Indy,
              "Indy 500 and Over Rev parent/clones select the later drive-board protocol");
    }
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*daytona)))
              == "Driving: 4-Speed + VR4",
          "Daytona profile name must match the approved workbook");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*srally)))
              == "Driving: 4-Speed + VR1 + Handbrake",
          "Sega Rally profile name must match the approved workbook");

    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto driving_desc = libretro::descriptors(*daytona, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_ANALOG,
                         RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,
                         "Steering")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,
                                "H-Gate: Left / Right")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "4-Speed: Neutral")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_DOWN, "VR1 (Red)")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_LEFT, "VR2 (Blue)")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_RIGHT, "VR3 (Yellow)")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_UP, "VR4 (Green)"),
          "Daytona descriptions must match the approved workbook");
    driving_desc = libretro::descriptors(*srally, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_DOWN, "VR1")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Handbrake (Analog)"),
          "Sega Rally descriptions must match the approved workbook");

    pressed = {};
    axes = {};
    analog_buttons = {};
    runtime = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.gears == 0x10, "Daytona hands its rolling start to fourth gear");

    runtime = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {}, {}, false);
    check(inputs.gears == 0x02,
          "Disabling Automatic Start Gear starts Daytona in first gear");

    const auto poll_neutral = [&](const rom::GameSpec& target, unsigned ms) {
        libretro::poll_input(inputs, target, devices, runtime, true, state,
                             libretro::GunInputMode::Hybrid, true, {}, {}, true, false, {}, ms);
    };
    for (const auto* target : {daytona, srally}) {
        for (unsigned ms = 0; ms <= 500; ms += 10) {
            runtime = {}; runtime.gear_initialized = true; runtime.gear = 2;
            pressed = {}; axes = {}; analog_buttons = {};
            pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_L;
            const unsigned duration = static_cast<unsigned>(ms * 57.5245 / 1000.0 + 0.999);
            for (unsigned frame = 0; frame <= duration + 1; ++frame) {
                poll_neutral(*target, ms);
                check(runtime.gear == 1 && inputs.gears == (frame < duration ? 1 : 2),
                      "Sequential shift emits exactly the upstream neutral interval without repeating held steps");
            }
        }
    }
    runtime = {}; runtime.gear_initialized = true; runtime.gear = 2;
    pressed = {}; pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_R;
    poll_neutral(*daytona, 100);
    check(inputs.gears == 1 && runtime.gear == 3 && runtime.shift_neutral_frames == 5,
          "100 ms starts a six-native-frame neutral crossing with the destination gear retained");
    pressed = {}; pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_Y;
    poll_neutral(*daytona, 100);
    check(inputs.gears == 1 && runtime.gear == 0 && runtime.shift_neutral_frames == 0,
          "Explicit neutral cancels a pending crossing");
    pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_R;
    poll_neutral(*daytona, 100);
    check(inputs.gears == 2 && runtime.shift_neutral_frames == 0,
          "Leaving neutral selects first gear without a synthetic crossing");
    pressed[0] = 0; poll_neutral(*daytona, 100);
    pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_R;
    poll_neutral(*daytona, 100);
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT] = {-20000, -20000};
    poll_neutral(*daytona, 100);
    check(inputs.gears == 2 && runtime.gear == 1 && runtime.shift_neutral_frames == 0,
          "A held direct H-gate position bypasses and cancels the timed crossing");
    axes = {}; pressed = {}; poll_neutral(*daytona, 100);
    pressed[0] = 1u << RETRO_DEVICE_ID_JOYPAD_R;
    poll_neutral(*daytona, 100);
    poll_neutral(*daytona, 0);
    check(inputs.gears == 4 && runtime.shift_neutral_frames == 0,
          "Changing the neutral interval to zero restores direct shifting without reset");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_LEFT)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT), 0};
    axes = {};
    analog_buttons = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = -0x5000;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = -0x5000;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 16384;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    runtime = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.in0 == 0x1f && inputs.in1 == 0xfe,
          "Daytona VR1-VR4 use the cataloged hardware ports and bits");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0xff
              && inputs.analog[2] == 0x80,
          "Daytona steering, accelerator and brake retain analog precision");
    check(inputs.gears == 0x02, "H-Gate upper-left selects first gear");

    pressed = {};
    axes = {};
    analog_buttons = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 16384;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 16384;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::DrivingAnalogOptions progressive{
        libretro::SteeringResponse::Progressive, 100, 500, 500};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, progressive);
    check(inputs.analog[0] == 0x9f && inputs.analog[1] == 0x80
              && inputs.analog[2] == 0x40,
          "Driving tuning applies Progressive steering and independent pedal ranges");
    libretro::DrivingAnalogOptions fbneo{
        libretro::SteeringResponse::FBNeoLogarithmic, 100, 1000, 1000};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, fbneo);
    check(inputs.analog[0] == 0x93,
          "Driving tuning applies the exact Supermodel FBNeo steering curve");
    libretro::DrivingAnalogOptions saturated{
        libretro::SteeringResponse::Linear, 150, 1000, 1000};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, saturated);
    check(inputs.analog[0] == 0xde,
          "Driving steering range above 100 percent reaches full lock sooner");
    libretro::DrivingAnalogOptions sega_adc_ranges{
        libretro::SteeringResponse::Linear, 63, 753, 753};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, sega_adc_ranges);
    check(inputs.analog[0] == 0x30 && inputs.analog[1] == 0xc0
              && inputs.analog[2] == 0x60,
          "Supermodel presets produce the exact 30-80-D0 and 00-C0 ADC ranges");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 32767;
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, sega_adc_ranges);
    check(inputs.analog[0] == 0xd0 && inputs.analog[2] == 0xc0,
          "Supermodel preset endpoints retain their exact hexadecimal values");

    const std::pair<unsigned, std::pair<u8, u8>> vr4_bindings[] = {
        {RETRO_DEVICE_ID_JOYPAD_DOWN, {0xdf, 0xff}},
        {RETRO_DEVICE_ID_JOYPAD_LEFT, {0xbf, 0xff}},
        {RETRO_DEVICE_ID_JOYPAD_RIGHT, {0x7f, 0xff}},
        {RETRO_DEVICE_ID_JOYPAD_UP, {0xff, 0xfe}},
    };
    for (const auto& [button, expected] : vr4_bindings) {
        pressed = {1u << button, 0};
        axes = {};
        analog_buttons = {};
        libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
        check(inputs.in0 == expected.first && inputs.in1 == expected.second,
              "Daytona VR4 D-Pad direction selects the intended hardware button");
    }

    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.analog[1] == 0x00,
          "Digital R2 is not converted to an analog accelerator by the core");

    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_Y, 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.gears == 0x01, "West selects 4-Speed Neutral");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R, 0};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.gears == 0x02, "Shift Up advances once");
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.gears == 0x02, "Held Shift Up does not repeat");
    pressed = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R, 0};
    libretro::poll_input(inputs, *daytona, devices, runtime, true, state);
    check(inputs.gears == 0x04, "Second Shift Up advances to second gear");

    pressed = {};
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = 0x6000;
    runtime = {};
    libretro::poll_input(inputs, *daytona, devices, runtime, false, state);
    check(inputs.gears == 0x10, "Standard shifter Right selects fourth gear");

    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_DOWN) | (1u << RETRO_DEVICE_ID_JOYPAD_B), 0};
    axes = {};
    runtime = {};
    libretro::poll_input(inputs, *srally, devices, runtime, true, state);
    check(inputs.in0 == 0xdf, "Sega Rally VR1 uses IN0 bit 0x20");
    check(inputs.in2 == 0xff, "Sega Rally South applies full analog handbrake");
    pressed = {};
    libretro::poll_input(inputs, *srally, devices, runtime, true, state);
    check(inputs.in2 == 0x00, "Sega Rally handbrake releases to analog zero");

    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_UP, 0};
    libretro::poll_input(inputs, *srally, devices, runtime, true, state);
    check(inputs.in0 == 0xdf && inputs.in2 == 0x00,
          "Sega Rally Up aliases VR1 without applying Handbrake");

    const auto* indy500 = database.find("indy500");
    const auto* overrev = database.find("overrev");
    const auto* stcc = database.find("stcc");
    check(indy500 && overrev && stcc, "Sequential VR2 parent games exist");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*indy500)))
              == "Driving: Sequential + VR2"
          && libretro::recognize_profile(*overrev) == libretro::InputProfile::DrivingSequentialVR2
          && libretro::recognize_profile(*stcc) == libretro::InputProfile::DrivingSequentialVR2,
          "Sequential VR2 parents share the approved profile");
    libretro::configure_controllers(*indy500, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential + VR2 + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Sequential VR2 exposes the one game-specific controller type");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    driving_desc = libretro::descriptors(*indy500, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "View 1")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_DOWN, "View 2")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Shift Down")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Shift Up")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L2, "Brake")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Steering"),
          "Sequential VR2 descriptions match the approved workbook");
    check(!has_descriptor_id(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_LEFT)
              && !has_descriptor_id(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_RIGHT),
          "Sequential VR2 leaves unassigned D-Pad directions undescribed");

    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R), 0};
    axes = {};
    analog_buttons = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 16384;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    runtime = {};
    libretro::poll_input(inputs, *indy500, devices, runtime, true, state);
    check(inputs.in1 == 0xcc,
          "Sequential VR2 writes View 1/2 and Shift Down/Up directly");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0xff
              && inputs.analog[2] == 0x80,
          "Sequential VR2 retains steering and pedal precision");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *indy500, devices, runtime, true, state);
    check(inputs.analog[1] == 0x00,
          "Sequential VR2 does not convert digital R2 to an analog accelerator");
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::poll_input(inputs, *overrev, devices, runtime, true, state);
    check(inputs.analog[1] == 0x00,
          "Over Rev applies its declared reversed accelerator calibration");
    libretro::DrivingAnalogOptions half_accelerator{
        libretro::SteeringResponse::Linear, 100, 500, 1000};
    libretro::poll_input(inputs, *overrev, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, half_accelerator);
    check(inputs.analog[1] == 0x80,
          "Accelerator range preserves Over Rev's reversed calibration");

    const auto* sgt24h = database.find("sgt24h");
    check(sgt24h != nullptr, "Super GT 24h parent game exists");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*sgt24h)))
              == "Driving: Sequential + VR2",
          "Super GT 24h uses the Sequential VR2 profile");
    check(sgt24h->wheel_button_bits[0] == std::pair<u8, u8>{1, 0x01}
          && sgt24h->wheel_button_bits[1] == std::pair<u8, u8>{1, 0x02},
          "Super GT 24h maps both native view inputs");
    libretro::configure_controllers(*sgt24h, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential + VR2 + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Super GT exposes the VR2 controller type");
    driving_desc = libretro::descriptors(*sgt24h, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "View 1")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_DOWN, "View 2")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Shift Down")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Shift Up")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L2, "Brake")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Steering"),
          "Super GT VR2 descriptions match the common layout");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R), 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *sgt24h, devices, runtime, true, state);
    check(inputs.in1 == 0xcc,
          "Super GT writes both views and Shift Down/Up independently");
    check(inputs.analog[1] == 0xff && inputs.analog[2] == 0xff,
          "Super GT released pedals retain their declared reversed calibration");
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 32767;
    libretro::poll_input(inputs, *sgt24h, devices, runtime, true, state);
    check(inputs.analog[1] == 0x00 && inputs.analog[2] == 0x00,
          "Super GT pressed pedals retain their declared reversed calibration");

    const auto* manxtt = database.find("manxtt");
    check(manxtt != nullptr, "Manx TT parent game exists");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*manxtt)))
              == "Driving: Sequential + VR1 (Manx TT Superbike)",
          "Manx TT uses the approved profile name");
    libretro::configure_controllers(*manxtt, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential + VR1 (Manx TT Superbike) + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Manx TT exposes the one game-specific controller type");
    driving_desc = libretro::descriptors(*manxtt, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_START, "Start / VR")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_DOWN, "Start / VR")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Shift Down")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Shift Up")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L2, "Brake")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Bank"),
          "Manx TT descriptions match the approved workbook");
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "Start / VR"),
          "Manx TT exposes the supplemental Up Start/VR alias");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_START)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R), 0};
    axes = {};
    analog_buttons = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 16384;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::poll_input(inputs, *manxtt, devices, runtime, true, state);
    check(inputs.in0 == 0xbf && inputs.in1 == 0xcf,
          "Manx TT writes Start/VR and Shift Down/Up directly");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0x80
              && inputs.analog[2] == 0x00,
          "Manx TT preserves throttle, brake and reversed Bank calibration");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_DOWN, 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *manxtt, devices, runtime, true, state);
    check(inputs.in0 == 0xbf,
          "Manx TT D-Pad Down reaches the shared Start/VR hardware input");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *manxtt, devices, runtime, true, state);
    check(inputs.analog[0] == 0x00,
          "Manx TT does not convert digital R2 to analog Accelerator");
    pressed = {};
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 32767;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::DrivingAnalogOptions half_motorcycle_pedals{
        libretro::SteeringResponse::Linear, 100, 500, 500};
    libretro::poll_input(inputs, *manxtt, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, half_motorcycle_pedals);
    check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0x80,
          "Driving pedal ranges apply independently to motorcycle Throttle and Brake");

    const auto* motoraid = database.find("motoraid");
    check(motoraid != nullptr, "Motor Raid parent game exists");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*motoraid)))
              == "Driving: Sequential + VR1 (Motor Raid)",
          "Motor Raid uses the approved profile name");
    libretro::configure_controllers(*motoraid, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential + VR1 (Motor Raid) + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Motor Raid exposes the one game-specific controller type");
    driving_desc = libretro::descriptors(*motoraid, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_START, "Start / VR")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Kick")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Punch")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Kick")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Punch")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L2, "Brake")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Bank"),
          "Motor Raid descriptions match the approved workbook");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_B, 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *motoraid, devices, runtime, true, state);
    check(inputs.in1 == 0xdf, "Motor Raid South writes Kick");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_A, 0};
    libretro::poll_input(inputs, *motoraid, devices, runtime, true, state);
    check(inputs.in1 == 0xef, "Motor Raid East writes Punch");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_R)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L), 0};
    libretro::poll_input(inputs, *motoraid, devices, runtime, true, state);
    check(inputs.in1 == 0xcf,
          "Motor Raid L1/R1 are secondary bindings for Kick/Punch");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    libretro::poll_input(inputs, *motoraid, devices, runtime, true, state);
    check(inputs.analog[0] == 0x00,
          "Motor Raid does not convert digital R2 to analog Accelerator");

    // Single-View motorcycle aliases must preserve the shared native Start line.
    for (const char* set : {"manxtt", "manxttc", "manxttdx", "motoraid", "motoraiddx"}) {
        const auto* motorcycle = database.find(set);
        check(motorcycle != nullptr, "VR1 motorcycle set exists");
        const auto motorcycle_desc = libretro::descriptors(*motorcycle, devices);
        check(has_descriptor(motorcycle_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_UP, "Start / VR"),
              "Every VR1 motorcycle set exposes Up");
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_UP, 0};
        axes = {};
        analog_buttons = {};
        libretro::poll_input(inputs, *motorcycle, devices, runtime, true, state);
        check(inputs.in0 == static_cast<u8>(0xff & ~motorcycle->start1_bit)
                  && inputs.in1 == 0xff,
              "VR1 Up reaches only the shared native Start/View line");
        pressed = {};
        libretro::poll_input(inputs, *motorcycle, devices, runtime, true, state);
        check(inputs.in0 == 0xff && inputs.in1 == 0xff,
              "VR1 motorcycle alias releases cleanly");
    }

    const auto* skytargt = database.find("skytargt");
    check(skytargt != nullptr, "Sky Target parent game exists");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*skytargt)))
              == "Joystick (Analog): Sky Target + VR1",
          "Sky Target uses the approved profile name");
    libretro::configure_controllers(*skytargt, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Analog): Sky Target + VR1 + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Sky Target exposes the one game-specific controller type");
    auto sky_desc = libretro::descriptors(*skytargt, devices);
    check(has_descriptor(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_B, "Machine Gun")
              && has_descriptor(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Missile")
              && has_descriptor(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Machine Gun")
              && has_descriptor(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Missile")
              && has_descriptor(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_UP, "View Change")
              && has_descriptor(sky_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Analog Joystick X")
              && has_descriptor(sky_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_Y, "Analog Joystick Y"),
          "Sky Target descriptions match the approved workbook");
    check(!has_descriptor_id(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_X)
              && !has_descriptor_id(sky_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_Y),
          "Sky Target exposes no obsolete North or unapproved West binding");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_B)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_UP), 0};
    axes = {};
    analog_buttons = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
    libretro::poll_input(inputs, *skytargt, devices, runtime, true, state);
    check(inputs.in0 == 0xdf && inputs.in1 == 0xcf,
          "Sky Target writes Machine Gun, Missile and View Change directly");
    check(inputs.analog[0] == 0x00 && inputs.analog[2] == 0x00,
          "Sky Target preserves normal Y and reversed X calibration");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_R)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L), 0};
    libretro::poll_input(inputs, *skytargt, devices, runtime, true, state);
    check(inputs.in0 == 0xff && inputs.in1 == 0xcf,
          "Sky Target maps RB and LB as second Machine Gun and Missile bindings");

    const auto* dynabb = database.find("dynabb");
    const auto* dynabb97 = database.find("dynabb97");
    check(dynabb != nullptr && dynabb97 != nullptr,
          "Both Dynamite Baseball parents exist");
    check(libretro::recognize_profile(*dynabb) == libretro::InputProfile::BaseballDynamite
              && libretro::recognize_profile(*dynabb97)
                  == libretro::InputProfile::BaseballDynamite,
          "Both Dynamite Baseball parents use the approved shared profile");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*dynabb)))
              == "Joystick (Standard): Baseball (Dynamite Baseball)",
          "Dynamite Baseball uses the approved profile name");
    libretro::configure_controllers(*dynabb, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Standard): Baseball (Dynamite Baseball) + Test/Service slots"
          && controllers.ports[0].num_types == 1
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Baseball (Dynamite Baseball) + Test/Service slots"
          && controllers.ports[1].num_types == 1,
          "Dynamite Baseball exposes one game-specific controller type on both players");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto baseball_desc = libretro::descriptors(*dynabb, devices);
    for (unsigned p = 0; p < 2; ++p) {
        check(has_descriptor(baseball_desc, p, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_B, "Button 1")
                  && has_descriptor(baseball_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_A, "Button 2")
                  && has_descriptor(baseball_desc, p, RETRO_DEVICE_ANALOG,
                                    RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                    RETRO_DEVICE_ID_ANALOG_Y, "Bat Swing"),
              "Dynamite Baseball descriptions match the approved workbook");
        check(!has_descriptor_id(baseball_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                 RETRO_DEVICE_ID_JOYPAD_Y),
              "Dynamite Baseball exposes no unapproved third button");
    }
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_B,
               1u << RETRO_DEVICE_ID_JOYPAD_A};
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = 32767;
    axes[1][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = 16384;
    libretro::poll_input(inputs, *dynabb, devices, runtime, true, state);
    check(inputs.in1 == 0xfe && inputs.in2 == 0xfd,
          "Dynamite Baseball keeps player buttons independent");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0x80,
          "Dynamite Baseball maps each downward Right Analog Y semiaxis to Bat Swing");
    pressed = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
    axes[1][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
    libretro::poll_input(inputs, *dynabb97, devices, runtime, true, state);
    check(inputs.analog[0] == 0x00 && inputs.analog[1] == 0x00,
          "Upward Right Analog Y remains outside the Bat Swing semiaxis");
    auto changed_baseball = *dynabb;
    changed_baseball.analog[1].control = rom::AnalogControl::None;
    check(libretro::recognize_profile(changed_baseball) == libretro::InputProfile::Unsupported,
          "Changed Dynamite Baseball hardware signature requires review");

    const auto* segawski = database.find("segawski");
    check(segawski != nullptr, "Sega Water Ski parent exists");
    check(libretro::recognize_profile(*segawski) == libretro::InputProfile::SpecialWaterSki
              && std::string_view(libretro::profile_name(
                     libretro::recognize_profile(*segawski))) == "Special: Water Ski",
          "Sega Water Ski uses the approved profile");
    libretro::configure_controllers(*segawski, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Special: Water Ski + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Sega Water Ski exposes one game-specific controller type");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto water_ski_desc = libretro::descriptors(*segawski, devices);
    check(has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "Select Up")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_DOWN, "Select Down")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Pitch Left")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Pitch Right")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Set")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "Pitch Left")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Pitch Right")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_START, "Start / Select Down")
              && has_descriptor(water_ski_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Slide"),
          "Sega Water Ski descriptions match the approved workbook");
    check(!has_descriptor_id(water_ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_LEFT)
              && !has_descriptor_id(water_ski_desc, 0, RETRO_DEVICE_ANALOG,
                                    RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                    RETRO_DEVICE_ID_ANALOG_Y),
          "Sega Water Ski exposes no unapproved directions or axes");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_B)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_START), 0};
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    libretro::poll_input(inputs, *segawski, devices, runtime, true, state);
    check(inputs.in0 == 0xbf && inputs.in1 == 0xf0,
          "Sega Water Ski writes every reviewed action to the MAME hardware bits");
    check(inputs.analog[0] == 0x00,
          "Sega Water Ski maps Left X+ to the inverted Slide endpoint");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_Y)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_A), 0};
    axes = {};
    libretro::poll_input(inputs, *segawski, devices, runtime, true, state);
    check(inputs.in1 == 0xf3,
          "Sega Water Ski West/East activate the secondary Pitch Left/Right bindings");
    pressed = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *segawski, devices, runtime, true, state);
    check(inputs.analog[0] == 0xff,
          "Sega Water Ski maps Left X- to the opposite inverted Slide endpoint");
    auto changed_water_ski = *segawski;
    changed_water_ski.analog[0].control = rom::AnalogControl::Steer;
    check(libretro::recognize_profile(changed_water_ski) == libretro::InputProfile::Unsupported,
          "Changed Sega Water Ski hardware signature requires review");

    const auto* skisuprg = database.find("skisuprg");
    check(skisuprg != nullptr, "Sega Ski Super G parent exists");
    check(libretro::recognize_profile(*skisuprg) == libretro::InputProfile::SpecialSkiSuperG
              && std::string_view(libretro::profile_name(
                     libretro::recognize_profile(*skisuprg))) == "Special: Ski Super G",
          "Sega Ski Super G uses the approved profile");
    libretro::configure_controllers(*skisuprg, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Special: Ski Super G + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Sega Ski Super G exposes one game-specific controller type");
    auto ski_desc = libretro::descriptors(*skisuprg, devices);
    check(has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "Zoom In")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_DOWN, "Zoom Out")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Foot Sensor Left")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Foot Sensor Right")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Select 2")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Select 3")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "Select 1")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Swing")
              && has_descriptor(ski_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                RETRO_DEVICE_ID_ANALOG_X, "Inclining"),
          "Sega Ski Super G descriptions match the reviewed profile");
    check(!has_descriptor_id(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_X)
              && !has_descriptor_id(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_LEFT)
              && !has_descriptor_id(ski_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_RIGHT)
              && !has_descriptor_id(ski_desc, 0, RETRO_DEVICE_ANALOG,
                                    RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                    RETRO_DEVICE_ID_ANALOG_Y),
          "Sega Ski Super G exposes no obsolete or unapproved bindings");
    pressed = {};
    axes = {};
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.in2 == 0x00,
          "Sega Ski Super G active-high foot sensors rest released");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_B)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_START), 0};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.in0 == 0x0f && inputs.in1 == 0xfe && inputs.in2 == 0xff,
          "Sega Ski Super G writes selections, zoom and active-high foot sensors");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0x80,
          "Sega Ski Super G maps Right X to Inclining on Model 2 analogue channel 0");
    pressed = {};
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0x00,
          "Sega Ski Super G maps Left X+ to the inverted Swing endpoint");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0xff,
          "Sega Ski Super G maps Left X- to the opposite inverted Swing endpoint");
    axes = {};
    for (const auto button : {RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_R2}) {
        pressed = {1u << button, 0};
        libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
        check(inputs.analog[0] == (button == RETRO_DEVICE_ID_JOYPAD_L2 ? 0 : 255)
                  && inputs.analog[1] == 128,
              "Ski Super G digital trigger aliases incline independently of Swing");
    }
    pressed = {};
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 16000;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 16000;
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.analog[0] == 128, "Equal Ski Super G triggers cancel Inclining");
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 0;
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.analog[0] > 128 && inputs.analog[0] < 255,
          "Ski Super G analogue trigger travel remains proportional");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *skisuprg, devices, runtime, true, state);
    check(inputs.analog[0] == 0, "Stronger Ski Super G right stick retains priority");
    analog_buttons = {};
    axes = {};
    auto changed_ski_super_g = *skisuprg;
    changed_ski_super_g.analog[1].control = rom::AnalogControl::StickX;
    check(libretro::recognize_profile(changed_ski_super_g) == libretro::InputProfile::Unsupported,
          "Changed Sega Ski Super G hardware signature requires review");

    const auto* topskatr = database.find("topskatr");
    check(topskatr != nullptr, "Top Skater parent exists");
    check(libretro::recognize_profile(*topskatr) == libretro::InputProfile::SpecialTopSkater
              && std::string_view(libretro::profile_name(
                     libretro::recognize_profile(*topskatr))) == "Special: Top Skater",
          "Top Skater uses the approved profile");
    libretro::configure_controllers(*topskatr, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Special: Top Skater + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Top Skater exposes one game-specific controller type");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto top_skater_desc = libretro::descriptors(*topskatr, devices);
    check(has_descriptor(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_LEFT, "Select Left")
              && has_descriptor(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_RIGHT, "Select Right")
              && has_descriptor(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Jump Tail")
              && has_descriptor(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Jump Front")
              && has_descriptor(top_skater_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Curving")
              && has_descriptor(top_skater_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                RETRO_DEVICE_ID_ANALOG_X, "Slide"),
          "Top Skater descriptions match the approved workbook");
    check(!has_descriptor_id(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_UP)
              && !has_descriptor_id(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_DOWN)
              && !has_descriptor_id(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_Y),
          "Top Skater exposes no unapproved D-Pad or face-button actions");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_LEFT)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_B)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_START), 0};
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.in0 == 0x0f && inputs.in1 == 0xfe,
          "Top Skater writes selections, jumps and Start to the MAME hardware bits");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_B, 0};
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.in0 == 0xff && inputs.in1 == 0xfe,
          "Top Skater maps South to Jump Tail");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_A, 0};
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.in0 == 0xdf && inputs.in1 == 0xff,
          "Top Skater maps East to Jump Front");
    pressed = {};
    check(inputs.analog[0] == 0x00 && inputs.analog[1] == 0x00,
          "Top Skater maps Left X+ to the inverted Curving endpoint");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[0] == 0xff,
          "Top Skater maps Left X- to the opposite inverted Curving endpoint");
    check(has_descriptor(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_L2, "Slide Left")
          && has_descriptor(top_skater_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                            RETRO_DEVICE_ID_JOYPAD_R2, "Slide Right"),
          "Top Skater exposes upstream trigger aliases in frontend remapping");
    axes = {};
    analog_buttons = {};
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 32767;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0x00,
          "Top Skater left trigger slides left without curving");
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[1] == 0x80, "Top Skater equal triggers cancel at center");
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_L2] = 16384;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[1] == 0xbf, "Top Skater subtracts analog trigger travel");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[1] == 0x00, "Top Skater preserves stronger right-stick slide");
    axes = {};
    analog_buttons = {};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[1] == 0xff, "Top Skater supports digital-only trigger fallback");
    pressed = {};
    for (const char* name : {"topskatr", "topskatrj", "topskatru", "topskatruo"}) {
        const auto* revision = database.find(name);
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_LEFT, 0};
        libretro::poll_input(inputs, *revision, devices, runtime, true, state);
        check(inputs.in0 == 0x7f && inputs.analog[0] == 0xff,
              "Top Skater D-Pad left selects and steers left across all revisions");
        pressed = {1u << RETRO_DEVICE_ID_JOYPAD_RIGHT, 0};
        libretro::poll_input(inputs, *revision, devices, runtime, true, state);
        check(inputs.in0 == 0xef && inputs.analog[0] == 0x00,
              "Top Skater D-Pad right selects and steers right across all revisions");
        pressed = {};
        libretro::poll_input(inputs, *revision, devices, runtime, true, state);
        check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0x80,
              "Top Skater released controls return to rest");
    }
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 3000;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[0] == 0x80, "Top Skater Progressive centers residual stick travel");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 16384;
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state);
    check(inputs.analog[0] == 0x66,
          "Top Skater Progressive matches upstream half-stick quadratic response");
    libretro::poll_input(inputs, *topskatr, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {}, {}, true, false, {false});
    check(inputs.analog[0] == 0x40,
          "Top Skater Linear preserves the previous half-stick response");
    axes = {};
    auto changed_top_skater = *topskatr;
    changed_top_skater.analog[1].control = rom::AnalogControl::StickX;
    check(libretro::recognize_profile(changed_top_skater) == libretro::InputProfile::Unsupported,
          "Changed Top Skater hardware signature requires review");

    const auto* waverunr = database.find("waverunr");
    check(waverunr != nullptr, "Wave Runner parent exists");
    check(libretro::recognize_profile(*waverunr) == libretro::InputProfile::SpecialWaveRunner
              && std::string_view(libretro::profile_name(
                     libretro::recognize_profile(*waverunr))) == "Special: Wave Runner + VR1",
          "Wave Runner uses the approved profile");
    libretro::configure_controllers(*waverunr, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Special: Wave Runner + VR1 + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Wave Runner exposes one game-specific controller type");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto wave_runner_desc = libretro::descriptors(*waverunr, devices);
    check(has_descriptor(wave_runner_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "View")
              && has_descriptor(wave_runner_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Throttle")
              && has_descriptor(wave_runner_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Handle")
              && has_descriptor(wave_runner_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_Y, "Pitch")
              && has_descriptor(wave_runner_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                RETRO_DEVICE_ID_ANALOG_X, "Roll"),
          "Wave Runner descriptions match the approved workbook");
    check(!has_descriptor_id(wave_runner_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_DOWN)
              && !has_descriptor_id(wave_runner_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_B)
              && !has_descriptor_id(wave_runner_desc, 0, RETRO_DEVICE_ANALOG,
                                    RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                    RETRO_DEVICE_ID_ANALOG_Y),
          "Wave Runner exposes no unapproved directions, buttons or axes");
    pressed = {};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *waverunr, devices, runtime, true, state);
    check(inputs.in2 == 0xf7,
          "Wave Runner safety sensor rests inactive without a frontend binding");
    check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0x80
              && inputs.analog[2] == 0x80 && inputs.analog[3] == 0x80,
          "Wave Runner analog controls rest at their declared centers");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_START), 0};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = -32768;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::poll_input(inputs, *waverunr, devices, runtime, true, state);
    check(inputs.in0 == 0xbf && inputs.in1 == 0xfe && inputs.in2 == 0xf7,
          "Wave Runner writes Start and View while preserving the safety sensor");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0x00
              && inputs.analog[2] == 0x00 && inputs.analog[3] == 0xff,
          "Wave Runner maps Handle, Roll, Throttle and Pitch to the approved axes");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_R2), 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *waverunr, devices, runtime, true, state);
    check(inputs.analog[2] == 0x80,
          "Wave Runner leaves digital-to-analog Throttle conversion to the frontend");
    auto changed_wave_runner = *waverunr;
    changed_wave_runner.analog[3].control = rom::AnalogControl::StickY;
    check(libretro::recognize_profile(changed_wave_runner) == libretro::InputProfile::Unsupported,
          "Changed Wave Runner hardware signature requires review");

    struct SinglePlayerDigitalCase {
        const char* set;
        libretro::InputProfile profile;
        const char* name;
    };
    constexpr SinglePlayerDigitalCase single_player_digital_cases[] = {
        {"rascot2", libretro::InputProfile::HorseRacingRoyalAscotII,
         "Joystick (Standard): Horse Racing (Royal Ascot II)"},
    };
    for (const auto& single : single_player_digital_cases) {
        const auto* selected = database.find(single.set);
        check(selected != nullptr, "Single-player digital parent exists");
        check(libretro::recognize_profile(*selected) == single.profile
                  && std::string_view(libretro::profile_name(single.profile)) == single.name,
              "Single-player digital game uses the approved profile");
        libretro::configure_controllers(*selected, controllers);
        check(std::string_view(controllers.descriptions[0][0].desc)
                  == std::string(single.name) + " + Test/Service slots"
              && controllers.ports[0].num_types == 1
              && std::string_view(controllers.descriptions[1][0].desc)
                  == std::string(single.name) + " + Test/Service slots"
              && controllers.ports[1].num_types == 1,
              "Single-player digital game retains one game-specific controller type on the second port");
        devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
        auto single_desc = libretro::descriptors(*selected, devices);
        check(has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_UP, "Joystick Up")
                  && has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_DOWN, "Joystick Down")
                  && has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_LEFT, "Joystick Left")
                  && has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_RIGHT, "Joystick Right")
                  && has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_B, "Button 1")
                  && has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_A, "Button 2")
                  && has_descriptor(single_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_Y, "Button 3"),
              "Single-player digital descriptions match the approved workbook");
        check(!has_descriptor_id(single_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                 RETRO_DEVICE_ID_JOYPAD_UP)
                  && !has_descriptor_id(single_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                        RETRO_DEVICE_ID_JOYPAD_B),
              "Single-player digital profile exposes gameplay only on port one");
        pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_LEFT)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_B)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_START),
                   (1u << RETRO_DEVICE_ID_JOYPAD_UP)
                       | (1u << RETRO_DEVICE_ID_JOYPAD_B)};
        axes = {};
        analog_buttons = {};
        libretro::poll_input(inputs, *selected, devices, runtime, true, state);
        check(inputs.in0 == 0xee && inputs.in1 == 0x08 && inputs.in2 == 0xff,
              "Single-player digital profile maps P1 and ignores P2 gameplay");
        auto changed_single = *selected;
        changed_single.inputs = changed_single.inputs | rom::InputFlags::Joystick2;
        check(libretro::recognize_profile(changed_single) == libretro::InputProfile::Unsupported,
              "Changed single-player digital signature requires review");
    }

    const auto* airwlkrs = database.find("airwlkrs");
    check(airwlkrs != nullptr, "Air Walkers parent exists");
    check(libretro::recognize_profile(*airwlkrs)
              == libretro::InputProfile::BasketballAirWalkers
          && libretro::profile_players(libretro::InputProfile::BasketballAirWalkers) == 4,
          "Air Walkers exposes all four cabinet players");
    libretro::configure_controllers(*airwlkrs, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Standard): Basketball (Air Walkers) + Test/Service slots"
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Basketball (Air Walkers) + Test/Service slots"
          && std::string_view(controllers.descriptions[2][0].desc)
              == "Joystick (Standard): Basketball (Air Walkers) + Test/Service slots"
          && std::string_view(controllers.descriptions[3][0].desc)
              == "Joystick (Standard): Basketball (Air Walkers) + Test/Service slots"
          && controllers.ports[4].num_types == 0,
          "Air Walkers exposes the approved profile on four RetroPad ports");
    devices.fill(RETRO_DEVICE_JOYPAD);
    auto airwlkrs_desc = libretro::descriptors(*airwlkrs, devices);
    check(has_descriptor(airwlkrs_desc, 2, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "Joystick Up")
              && has_descriptor(airwlkrs_desc, 2, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Button 1")
              && has_descriptor(airwlkrs_desc, 3, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Button 2")
              && has_descriptor(airwlkrs_desc, 3, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "Button 3"),
          "Air Walkers publishes the approved P3/P4 directions and buttons");
    pressed = {
        (1u << RETRO_DEVICE_ID_JOYPAD_UP)
            | (1u << RETRO_DEVICE_ID_JOYPAD_B)
            | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_START),
        (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
            | (1u << RETRO_DEVICE_ID_JOYPAD_A),
        (1u << RETRO_DEVICE_ID_JOYPAD_LEFT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
            | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_START),
        (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_B)
            | (1u << RETRO_DEVICE_ID_JOYPAD_A)
            | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
            | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_START),
    };
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *airwlkrs, devices, runtime, true, state);
    check(inputs.in0 == 0x3e && inputs.player_starts == 0xf2
              && inputs.in1 == 0xde && inputs.in2 == 0xed
              && inputs.in3 == 0x7b && inputs.in4 == 0xb8,
          "Air Walkers maps four independent players, coins and starts");
    check(inputs.player_port(0) == 0xde && inputs.player_port(1) == 0xed
              && inputs.player_port(2) == 0x7b && inputs.player_port(3) == 0xb8
              && inputs.start_pair(false) == 0x20
              && inputs.start_pair(true) == 0x00,
          "Air Walkers matrix exposes the selected player and Start pair");

    const auto* hpyagu98 = database.find("hpyagu98");
    check(hpyagu98 != nullptr, "Hanguk Pro Yagu 98 parent exists");
    check(libretro::recognize_profile(*hpyagu98)
              == libretro::InputProfile::BaseballHangukProYagu98,
          "Hanguk Pro Yagu 98 uses the approved profile");
    libretro::configure_controllers(*hpyagu98, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Standard): Baseball (Hanguk Pro Yagu 98) + Test/Service slots"
          && controllers.ports[0].num_types == 1
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Baseball (Hanguk Pro Yagu 98) + Test/Service slots"
          && controllers.ports[1].num_types == 1,
          "Hanguk Pro Yagu 98 exposes one game-specific controller type on both players");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto hpyagu_desc = libretro::descriptors(*hpyagu98, devices);
    for (unsigned p = 0; p < 2; ++p) {
        check(has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_UP, "Joystick Up")
                  && has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_DOWN, "Joystick Down")
                  && has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_LEFT, "Joystick Left")
                  && has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_RIGHT, "Joystick Right")
                  && has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_B, "Button 1")
                  && has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_A, "Button 2")
                  && has_descriptor(hpyagu_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_Y, "Button 3"),
              "Hanguk Pro Yagu 98 descriptions match the approved workbook");
    }
    pressed = {
        (1u << RETRO_DEVICE_ID_JOYPAD_UP)
            | (1u << RETRO_DEVICE_ID_JOYPAD_B)
            | (1u << RETRO_DEVICE_ID_JOYPAD_A)
            | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
            | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_START),
        (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
            | (1u << RETRO_DEVICE_ID_JOYPAD_B)
            | (1u << RETRO_DEVICE_ID_JOYPAD_A)
            | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
            | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
            | (1u << RETRO_DEVICE_ID_JOYPAD_START),
    };
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *hpyagu98, devices, runtime, true, state);
    check(inputs.in0 == 0xcc && inputs.in1 == 0xd8 && inputs.in2 == 0xe8,
          "Hanguk Pro Yagu 98 maps both players independently to the VF2 layout");
    auto changed_hpyagu98 = *hpyagu98;
    changed_hpyagu98.inputs = changed_hpyagu98.inputs | rom::InputFlags::Vehicle;
    check(libretro::recognize_profile(changed_hpyagu98) == libretro::InputProfile::Unsupported,
          "Changed Hanguk Pro Yagu 98 hardware signature requires review");

    const auto* desert = database.find("desert");
    check(desert != nullptr, "Desert Tank parent exists");
    check(libretro::recognize_profile(*desert) == libretro::InputProfile::SpecialDesertTank
              && std::string_view(libretro::profile_name(
                     libretro::recognize_profile(*desert))) == "Joystick (Analog): Desert Tank + VR3",
          "Desert Tank uses the approved profile");
    libretro::configure_controllers(*desert, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Analog): Desert Tank + VR3 + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Desert Tank exposes one game-specific controller type");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto desert_desc = libretro::descriptors(*desert, devices);
    check(has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_DOWN, "VR1 (Blue)")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_LEFT, "VR2 (Green)")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_UP, "VR3 (Red)")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Machine Gun")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Machine Gun")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Cannon")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Cannon")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "Shift")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R2, "Accelerator")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_X, "Steering")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                RETRO_DEVICE_ID_ANALOG_Y, "Elevation")
              && has_descriptor(desert_desc, 0, RETRO_DEVICE_ANALOG,
                                RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                RETRO_DEVICE_ID_ANALOG_Y, "Elevation"),
          "Desert Tank descriptions match the approved workbook");
    check(!has_descriptor_id(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_RIGHT)
              && !has_descriptor_id(desert_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_L2)
              && !has_descriptor_id(desert_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_UP),
          "Desert Tank exposes no unapproved controls or second player");
    runtime = {};
    pressed = {};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.in0 == 0xff && inputs.in1 == 0xff && inputs.in2 == 0xff,
          "Desert Tank digital controls rest released");
    check(inputs.analog[0] == 0x80 && inputs.analog[1] == 0x00
              && inputs.analog[2] == 0x80,
          "Desert Tank analog controls use their documented rest values");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_UP)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_LEFT)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_B)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_START), 0};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 32767;
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = 32767;
    analog_buttons[0][RETRO_DEVICE_ID_JOYPAD_R2] = 32767;
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.in0 == 0x0e && inputs.in1 == 0xce && inputs.in2 == 0xff,
          "Desert Tank maps VR buttons, weapons, Shift, Coin and Start to MAME bits");
    check(inputs.analog[0] == 0xff && inputs.analog[1] == 0xff
              && inputs.analog[2] == 0x83,
          "Desert Tank maps Steering and Accelerator directly and advances Elevation relatively");
    pressed = {};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.in1 == 0xfe && inputs.analog[2] == 0x83,
          "Desert Tank Shift remains toggled and relative Elevation holds at rest");
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = -32768;
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.analog[2] == 0x80,
          "Desert Tank relative Elevation moves back upward proportionally");

    runtime.desert_elevation = 128.0f;
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = 32767;
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.analog[2] == 0x83,
          "Desert Tank Right Analog Y is a secondary Elevation binding");

    runtime.desert_elevation = 128.0f;
    axes = {};
    axes[0][RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = 32767;
    libretro::poll_input(inputs, *desert, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {},
                         {libretro::DesertElevationControl::Relative, 10, false});
    check(inputs.analog[2] == 0x80,
          "Desert Tank relative Elevation preserves fractional movement at 10 percent speed");
    for (unsigned frame = 0; frame < 4; ++frame)
        libretro::poll_input(inputs, *desert, devices, runtime, true, state,
                             libretro::GunInputMode::Hybrid, true, {},
                             {libretro::DesertElevationControl::Relative, 10, false});
    check(inputs.analog[2] == 0x82,
          "Desert Tank relative Elevation accumulates the 10 percent speed setting");
    runtime.desert_elevation = 128.0f;
    libretro::poll_input(inputs, *desert, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {},
                         {libretro::DesertElevationControl::Relative, 200, false});
    check(inputs.analog[2] == 0x86,
          "Desert Tank relative Elevation supports the 200 percent speed setting");
    runtime.desert_elevation = 128.0f;
    libretro::poll_input(inputs, *desert, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {},
                         {libretro::DesertElevationControl::Relative, 100, true});
    check(inputs.analog[2] == 0x7d,
          "Desert Tank relative Elevation supports axis inversion");
    libretro::poll_input(inputs, *desert, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {},
                         {libretro::DesertElevationControl::Absolute, 100, false});
    check(inputs.analog[2] == 0xff,
          "Desert Tank Absolute mode maps the full downward endpoint directly");
    libretro::poll_input(inputs, *desert, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, true, {},
                         {libretro::DesertElevationControl::Absolute, 100, true});
    check(inputs.analog[2] == 0x00,
          "Desert Tank Absolute mode supports axis inversion");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_Y), 0};
    axes = {};
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.in1 == 0xff,
          "Desert Tank second Shift press returns the toggle to its initial state");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_R)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L), 0};
    libretro::poll_input(inputs, *desert, devices, runtime, true, state);
    check(inputs.in1 == 0xcf,
          "Desert Tank secondary shoulder bindings reach Machine Gun and Cannon");
    auto changed_desert = *desert;
    changed_desert.analog[2].control = rom::AnalogControl::Brake;
    check(libretro::recognize_profile(changed_desert) == libretro::InputProfile::Unsupported,
          "Changed Desert Tank hardware signature requires review");

    const auto* bel = database.find("bel");
    check(bel != nullptr, "Behind Enemy Lines parent game exists");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*bel)))
              == "Gun: Behind Enemy Lines",
          "Behind Enemy Lines uses the approved profile name");
    libretro::configure_controllers(*bel, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Gun: Behind Enemy Lines + Test/Service slots"
          && controllers.ports[0].num_types == 1
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Gun: Behind Enemy Lines + Test/Service slots"
          && controllers.ports[1].num_types == 1,
          "Behind Enemy Lines exposes one game-specific controller type on both players");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto bel_desc = libretro::descriptors(*bel, devices);
    for (unsigned p = 0; p < 2; ++p) {
        check(has_descriptor(bel_desc, p, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_B, "Shot")
                  && has_descriptor(bel_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_A, "Missile")
                  && has_descriptor(bel_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_L, "Missile")
                  && has_descriptor(bel_desc, p, RETRO_DEVICE_ANALOG,
                                    RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                    RETRO_DEVICE_ID_ANALOG_X,
                                    "Gun Yaw (Analog Cursor)")
                  && has_descriptor(bel_desc, p, RETRO_DEVICE_ANALOG,
                                    RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                    RETRO_DEVICE_ID_ANALOG_Y,
                                    "Gun Pitch (Analog Cursor)"),
              "Behind Enemy Lines descriptions match the approved workbook");
        check(!has_descriptor_id(bel_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                 RETRO_DEVICE_ID_JOYPAD_UP)
                  && !has_descriptor_id(bel_desc, p, RETRO_DEVICE_JOYPAD, 0,
                                        RETRO_DEVICE_ID_JOYPAD_Y),
              "Behind Enemy Lines exposes no unapproved D-Pad or West binding");
    }
    pressed = {};
    axes = {};
    analog_buttons = {};
    mouse_axes = {};
    mouse_buttons = {};
    lightgun_axes = {{{-32768, 32767}, {32767, -32768}}};
    lightgun_buttons = {
        (1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) | (1u << RETRO_DEVICE_ID_LIGHTGUN_AUX_A),
        (1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) | (1u << RETRO_DEVICE_ID_LIGHTGUN_RELOAD)};
    libretro::poll_input(inputs, *bel, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun);
    check(inputs.in1 == 0xcc,
          "Behind Enemy Lines writes both players Shot and Missile directly");
    check(inputs.analog[0] == 0 && inputs.analog[1] == 255
              && inputs.analog[2] == 0 && inputs.analog[3] == 255,
          "Behind Enemy Lines upstream mapping reaches all raw endpoints for both players");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L, 0};
    lightgun_buttons = {};
    libretro::poll_input(inputs, *bel, devices, runtime, true, state,
                         libretro::GunInputMode::AnalogSticks);
    check(inputs.in1 == 0xef,
          "Behind Enemy Lines maps RetroPad LB as a second Missile binding");
    pressed = {};
    const auto bel_lightgun_desc = libretro::descriptors(
        *bel, devices, libretro::GunInputMode::Lightgun, false);
    check(has_descriptor(bel_lightgun_desc, 0, RETRO_DEVICE_LIGHTGUN, 0,
                         RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Shot")
              && has_descriptor(bel_lightgun_desc, 0, RETRO_DEVICE_LIGHTGUN, 0,
                                RETRO_DEVICE_ID_LIGHTGUN_AUX_A, "Missile")
              && has_descriptor(bel_lightgun_desc, 0, RETRO_DEVICE_LIGHTGUN, 0,
                                RETRO_DEVICE_ID_LIGHTGUN_RELOAD, "Missile"),
          "Behind Enemy Lines Lightgun path keeps Shot and Missile actions independently of reload shortcuts");

    const auto* gunblade = database.find("gunblade");
    const auto* rchase2 = database.find("rchase2");
    const auto* vcop = database.find("vcop");
    const auto* vcop2 = database.find("vcop2");
    const auto* hotd = database.find("hotd");
    check(gunblade && rchase2 && vcop && vcop2 && hotd,
          "Every reviewed Gun parent exists");
    libretro::configure_controllers(*vcop, controllers, libretro::GunInputMode::MouseAnalog);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Gun (Mouse + Analog Stick) + Test/Service slots"
          && controllers.ports[0].num_types == 1,
          "Gun profiles expose the selected Supermodel-style source name and one game-specific controller type");
    const auto vcop_hybrid = libretro::descriptors(*vcop, devices);
    check(has_descriptor(vcop_hybrid, 0, RETRO_DEVICE_LIGHTGUN, 0,
                         RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Shot")
              && has_descriptor(vcop_hybrid, 0, RETRO_DEVICE_MOUSE, 0,
                                RETRO_DEVICE_ID_MOUSE_RIGHT, "Reload Offscreen")
              && has_descriptor(vcop_hybrid, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Reload Offscreen")
              && has_descriptor(vcop_hybrid, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Shot")
              && has_descriptor(vcop_hybrid, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_L, "Reload Offscreen"),
          "Serial gun Standard mode exposes Lightgun, Mouse and RetroPad paths");
    const auto vcop_without_reload = libretro::descriptors(
        *vcop, devices, libretro::GunInputMode::Hybrid, false);
    check(!has_descriptor_id(vcop_without_reload, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_A)
              && !has_descriptor_id(vcop_without_reload, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_L)
              && !has_descriptor_id(vcop_without_reload, 0, RETRO_DEVICE_MOUSE, 0,
                                    RETRO_DEVICE_ID_MOUSE_RIGHT)
              && !has_descriptor_id(vcop_without_reload, 0, RETRO_DEVICE_LIGHTGUN, 0,
                                    RETRO_DEVICE_ID_LIGHTGUN_RELOAD)
              && has_descriptor(vcop_without_reload, 0, RETRO_DEVICE_LIGHTGUN, 0,
                                RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Shot")
              && has_descriptor(vcop_without_reload, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_R, "Shot"),
          "Disabling the shortcut hides only explicit serial off-screen reload bindings");
    const auto gunblade_hybrid = libretro::descriptors(*gunblade, devices);
    check(!has_descriptor_id(gunblade_hybrid, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_A)
              && !has_descriptor_id(gunblade_hybrid, 0, RETRO_DEVICE_MOUSE, 0,
                                    RETRO_DEVICE_ID_MOUSE_RIGHT)
              && !has_descriptor_id(gunblade_hybrid, 0, RETRO_DEVICE_LIGHTGUN, 0,
                                    RETRO_DEVICE_ID_LIGHTGUN_RELOAD),
          "Positional Gun profile exposes no unsupported reload action");

    // Standard arbitrates sources per frontend port, including mixed devices.
    for (const auto* target : {vcop, hotd}) {
        for (unsigned gun_player = 0; gun_player < 2; ++gun_player) {
            runtime = {}; pressed = {}; axes = {}; analog_buttons = {};
            mouse_axes = {}; mouse_buttons = {}; lightgun_axes = {}; lightgun_buttons = {};
            libretro::poll_input(inputs, *target, devices, runtime, true, state);
            const unsigned mouse_player = 1 - gun_player;
            const int before = runtime.gun_cursor_x[mouse_player];
            lightgun_axes[gun_player] = {12000, -9000};
            lightgun_buttons[gun_player] = 1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER;
            mouse_axes[mouse_player][0] = 7;
            libretro::poll_input(inputs, *target, devices, runtime, true, state);
            const auto shooting = [&](unsigned player) {
                return player == 0 ? !(inputs.in1 & 1)
                    : target->lightgun.p2_trigger_on_in2 ? !(inputs.in2 & 1)
                    : !(inputs.in1 & 2);
            };
            check(runtime.gun_cursor_x[mouse_player] == before + 7
                && shooting(gun_player) && !shooting(mouse_player),
                "A gun on either port neither steals the other port's Mouse nor duplicates Shot");
            const int gun_x = runtime.gun_cursor_x[gun_player];
            lightgun_buttons = {};
            mouse_buttons[mouse_player] = 1u << RETRO_DEVICE_ID_MOUSE_LEFT;
            libretro::poll_input(inputs, *target, devices, runtime, true, state);
            check(runtime.gun_cursor_x[gun_player] == gun_x
                && runtime.gun_cursor_x[mouse_player] == before + 14
                && !shooting(gun_player) && shooting(mouse_player),
                "Mixed-device trigger release and Mouse Shot remain independent on both ports");
            mouse_axes = {}; mouse_buttons = {};
            libretro::poll_input(inputs, *target, devices, runtime, true, state,
                                 libretro::GunInputMode::Lightgun);
            const auto only_gun_cursors = runtime.gun_cursor_x;
            mouse_axes[mouse_player][0] = 12;
            mouse_buttons[mouse_player] = 1u << RETRO_DEVICE_ID_MOUSE_LEFT;
            libretro::poll_input(inputs, *target, devices, runtime, true, state,
                                 libretro::GunInputMode::Lightgun);
            check(runtime.gun_cursor_x == only_gun_cursors && !shooting(mouse_player),
                  "Lightgun Only ignores Mouse on either port");
        }
    }
    // An idle off-screen Lightgun report must not monopolize Standard's cursor.
    for (const auto* target : {bel, vcop}) {
        runtime = {};
        pressed = {};
        axes = {};
        analog_buttons = {};
        mouse_axes = {};
        mouse_buttons = {};
        lightgun_axes = {};
        lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN, 0};
        libretro::poll_input(inputs, *target, devices, runtime, true, state);
        const int before_mouse = runtime.gun_cursor_x[0];
        mouse_axes[0][0] = 12;
        libretro::poll_input(inputs, *target, devices, runtime, true, state);
        check(runtime.gun_cursor_x[0] == before_mouse + 12,
              "Standard accepts Mouse movement with an idle off-screen Lightgun");
        check(!runtime.gun_aim_offscreen[0],
              "An inactive Lightgun does not mark the Mouse cursor off-screen");
        mouse_axes = {};
        const int before_stick = runtime.gun_cursor_x[0];
        axes[0][0][0] = 32767;
        libretro::poll_input(inputs, *target, devices, runtime, true, state);
        check(runtime.gun_cursor_x[0] > before_stick,
              "Standard accepts Stick movement with an idle off-screen Lightgun");
    }
    pressed = {};
    axes = {};
    mouse_axes = {};
    mouse_buttons = {};

    // Analog Stick Aim = Absolute: the stick's position is the aim, the centre
    // when released; with a mouse alongside, the mouse moves it once released.
    runtime = {};
    lightgun_axes = {};
    lightgun_buttons = {};
    const auto poll_absolute = [&](libretro::GunInputMode mode) {
        libretro::poll_input(inputs, *vcop, devices, runtime, true, state, mode, false,
                             {}, {}, true, false, {}, 0, true);
    };
    axes[0][0][0] = 32767;
    axes[0][0][1] = -32768;
    poll_absolute(libretro::GunInputMode::AnalogSticks);
    check(runtime.gun_cursor_x[0] == 495 && runtime.gun_cursor_y[0] == 0,
          "Absolute Analog Stick Aim puts the cursor where the stick points");
    axes = {};
    poll_absolute(libretro::GunInputMode::AnalogSticks);
    check(runtime.gun_cursor_x[0] == 248 && runtime.gun_cursor_y[0] == 192,
          "Absolute Analog Stick Aim centres the cursor when the stick is released");
    axes[0][0][0] = -32768;
    poll_absolute(libretro::GunInputMode::Hybrid);
    check(runtime.gun_cursor_x[0] == 0, "Absolute Analog Stick Aim applies in Standard");
    axes = {};
    poll_absolute(libretro::GunInputMode::Hybrid);
    check(runtime.gun_cursor_x[0] == 248,
          "Absolute Analog Stick Aim centres the cursor in Standard once released");
    mouse_axes[0][0] = 10;
    poll_absolute(libretro::GunInputMode::Hybrid);
    check(runtime.gun_cursor_x[0] == 258,
          "The mouse moves the cursor in Standard while the stick is at rest");
    mouse_axes = {};

    runtime = {};
    lightgun_axes = {{{-32768, 32767}, {32767, -32768}}};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER,
                        1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER};
    libretro::poll_input(inputs, *vcop, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun, false);
    check(inputs.gun_p1x == vcop->lightgun.p1x.minimum + 1
              && inputs.gun_p1y == vcop->lightgun.p1y.maximum - 1
              && inputs.gun_p2x == vcop->lightgun.p2x.maximum - 1
              && inputs.gun_p2y == vcop->lightgun.p2y.minimum + 1
              && inputs.in1 == 0xfc && inputs.gun_offscreen == 0,
          "Virtua Cop keeps extreme coordinates separate from off-screen status");
    runtime = {};
    lightgun_axes = {};
    lightgun_buttons = {
        (1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER)
            | (1u << RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN),
        0};
    libretro::poll_input(inputs, *vcop, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun, false);
    check(inputs.gun_p1x == vcop->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop->lightgun.p1y.minimum
              && inputs.in1 == 0xfe && inputs.gun_offscreen == 0x01,
          "Libretro Lightgun off-screen status reaches the serial gun board");
    runtime = {};
    lightgun_axes = {{{-32768, 32767}, {32767, -32768}}};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER,
                        1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER};
    libretro::poll_input(inputs, *hotd, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun);
    check(inputs.in1 == 0xfc && inputs.in2 == 0xff,
          "House of the Dead routes both triggers to IN1 as corrected upstream");
    for (const char* name : {"hotdo", "hotdp"}) {
        const auto* clone = database.find(name);
        libretro::poll_input(inputs, *clone, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun);
        check(inputs.in1 == 0xfc && inputs.in2 == 0xff,
              "All House of the Dead revisions use the corrected trigger route");
    }
    // Exercise both selector branches with identical frontend coordinates.
    pressed = {};
    mouse_buttons = {};
    lightgun_buttons = {};
    lightgun_axes = {};
    for (s16 edge : {static_cast<s16>(-32768), static_cast<s16>(32767)}) {
        lightgun_axes = {{{edge, edge}, {edge, edge}}};
        runtime = {};
        libretro::poll_input(inputs, *gunblade, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun);
        check(inputs.analog[0] == (edge < 0 ? 0x72 : 0xf5)
                  && inputs.analog[1] == (edge < 0 ? 0x0b : 0x8e)
                  && inputs.analog[2] == (edge < 0 ? 0x1d : 0xa4)
                  && inputs.analog[3] == inputs.analog[2],
              "Gunblade Custom uses the imported aim table and vertical range for both players");
        libretro::SpecialAnalogOptions previous;
        previous.gunblade_upstream = false;
        libretro::poll_input(inputs, *gunblade, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun, true, {}, {}, true, false, previous);
        check(inputs.analog[0] == (edge < 0 ? 0x69 : 0xff)
                  && inputs.analog[1] == (edge < 0 ? 0x00 : 0x96)
                  && inputs.analog[2] == (edge < 0 ? 0x11 : 0xae)
                  && inputs.analog[3] == inputs.analog[2],
              "Gunblade Linear restores the previous mux limits for both players");
    }
    lightgun_axes = {};
    for (const char* name : {"rchase2", "rchase2a"}) {
        const auto* target = database.find(name);
        check(target != nullptr, "Rail Chase revision exists");
        runtime = {};
        libretro::poll_input(inputs, *target, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun);
        const auto mapped = inputs.analog;
        libretro::SpecialAnalogOptions previous;
        previous.rail_chase_upstream = false;
        libretro::poll_input(inputs, *target, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun, true, {}, {}, true, false, previous);
        check(mapped[0] != inputs.analog[0] && mapped[1] != inputs.analog[1],
              "Rail Chase selector switches both player X mappings");
        for (unsigned axis = 0; axis < 4; ++axis) {
            const auto& channel = target->analog[axis];
            float fraction = (axis < 2 ? 247.0f / 495.0f : 191.0f / 383.0f);
            if (channel.reverse) fraction = 1.0f - fraction;
            const auto expected = static_cast<u8>(channel.minimum
                + fraction * (channel.maximum - channel.minimum) + 0.5f);
            check(inputs.analog[axis] == expected,
                  "Rail Chase Linear restores the previous midpoint for each mux channel");
        }
    }
    libretro::SpecialAnalogOptions wide;
    wide.widescreen_columns = 83;
    for (s16 edge : {static_cast<s16>(-32768), static_cast<s16>(32767)}) {
        lightgun_axes = {{{edge, edge}, {edge, edge}}};
        runtime = {};
        libretro::poll_input(inputs, *vcop, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun);
        const auto narrow_inputs = inputs;
        libretro::poll_input(inputs, *vcop, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun, true, {}, {}, true, false, wide);
        check((edge < 0 ? inputs.gun_p1x < narrow_inputs.gun_p1x
                        && inputs.gun_p2x < narrow_inputs.gun_p2x
                       : inputs.gun_p1x > narrow_inputs.gun_p1x
                        && inputs.gun_p2x > narrow_inputs.gun_p2x)
                  && inputs.gun_p1y == narrow_inputs.gun_p1y
                  && inputs.gun_p2y == narrow_inputs.gun_p2y
                  && inputs.gun_offscreen == 0,
              "Expanded gun travel reaches both side columns without offscreen reload or Y changes");
        libretro::poll_input(inputs, *gunblade, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun, true, {}, {}, true, false, wide);
        check(inputs.analog[0] == (edge < 0 ? 0x5d : 0xff)
                  && inputs.analog[1] == (edge < 0 ? 0x00 : 0xa3),
              "Gunblade extrapolates end segments into widescreen and clamps the 8-bit ADC");
    }
    const auto* prototype = database.find("hotdp");
    for (s16 edge : {static_cast<s16>(-32768), static_cast<s16>(32767)}) {
        lightgun_axes = {{{edge, edge}, {edge, edge}}};
        runtime = {};
        libretro::poll_input(inputs, *prototype, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun);
        check(inputs.gun_p1x == (edge < 0 ? 140 : 636)
                  && inputs.gun_p2x == inputs.gun_p1x
                  && inputs.gun_p1y == (edge < 0 ? 40 : 424)
                  && inputs.gun_p2y == inputs.gun_p1y,
              "HOTD prototype Upstream uses expanded endpoints for both players");
        libretro::SpecialAnalogOptions previous;
        previous.hotd_prototype_upstream = false;
        libretro::poll_input(inputs, *prototype, devices, runtime, true, state,
                             libretro::GunInputMode::Lightgun, true, {}, {}, true, false, previous);
        check(inputs.gun_p1x == (edge < 0 ? 174 : 595)
                  && inputs.gun_p2x == (edge < 0 ? 164 : 595)
                  && inputs.gun_p1y == (edge < 0 ? 88 : 379),
              "HOTD prototype Previous restores the published port endpoints");
    }
    lightgun_axes = {};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER,
                        1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER};
    auto legacy_hotd = *hotd;
    legacy_hotd.lightgun.p2_trigger_on_in2 = true;
    libretro::poll_input(inputs, legacy_hotd, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun);
    check(inputs.in1 == 0xfe && inputs.in2 == 0xfe,
          "Explicit legacy external metadata retains its declared P2 trigger route");

    runtime = {};
    lightgun_buttons = {};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_A, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::AnalogSticks);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum
              && inputs.in1 == 0xfe && inputs.gun_offscreen == 0x01,
          "RetroPad Reload Offscreen snaps serial aim to the corner and fires");
    runtime = {};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::AnalogSticks);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum
              && inputs.in1 == 0xfe && inputs.gun_offscreen == 0x01,
          "RetroPad LB is a second Reload Offscreen binding");
    runtime = {};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::AnalogSticks);
    check(inputs.in1 == 0xfe,
          "RetroPad RB is a second Shot binding");
    runtime = {};
    pressed = {};
    mouse_buttons = {1u << RETRO_DEVICE_ID_MOUSE_RIGHT, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::Mouse);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum
              && inputs.in1 == 0xfe && inputs.gun_offscreen == 0x01,
          "Mouse right implements the same serial offscreen reload");
    runtime = {};
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L), 0};
    mouse_buttons = {1u << RETRO_DEVICE_ID_MOUSE_RIGHT, 0};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_RELOAD, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, false);
    check(inputs.in1 == 0xff && inputs.gun_offscreen == 0,
          "Disabled reload shortcut ignores RetroPad, Mouse and Lightgun shortcut inputs");
    runtime = {};
    mouse_axes = {{{-32768, -32768}, {0, 0}}};
    mouse_buttons = {1u << RETRO_DEVICE_ID_MOUSE_LEFT, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::Mouse);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum + 1
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum + 1
              && inputs.in1 == 0xfe && inputs.gun_offscreen == 0,
          "Mouse coordinates at the screen edge remain an on-screen shot");
    runtime = {};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::Mouse, true, {}, {}, true, true);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum
              && inputs.in1 == 0xfe && inputs.gun_offscreen == 0x01
              && runtime.gun_aim_offscreen[0],
          "Enabled Mouse edge reload restores the MAME-style off-screen shot");
    runtime = {};
    pressed = {};
    mouse_axes = {};
    mouse_buttons = {1u << RETRO_DEVICE_ID_MOUSE_RIGHT, 0};
    lightgun_buttons = {};
    libretro::poll_input(inputs, *gunblade, devices, runtime, true, state,
                         libretro::GunInputMode::Mouse);
    check(inputs.in1 == 0xff && runtime.gun_aim_active[0]
              && runtime.gun_aim_active[1],
          "Gunblade ignores reload and exposes both positional crosshair aims");

    mouse_buttons = {};
    lightgun_buttons = {};
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_L3)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R3), 0};
    libretro::poll_input(inputs, *bel, devices, runtime, true, state);
    check(inputs.in0 == 0xf3,
          "Behind Enemy Lines maps Test and Service to its reversed hardware bits");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L3, 0};
    libretro::poll_input(inputs, *bel, devices, runtime, true, state);
    check(inputs.in0 == 0xfb, "L3 maps Service on Behind Enemy Lines hardware");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R3, 0};
    libretro::poll_input(inputs, *bel, devices, runtime, true, state);
    check(inputs.in0 == 0xf7, "R3 maps Test on Behind Enemy Lines hardware");
    devices = {libretro::kNoServiceDevice, libretro::kNoServiceDevice};
    libretro::poll_input(inputs, *bel, devices, runtime, true, state);
    check(inputs.in0 == 0xff,
          "Behind Enemy Lines reduced variant hides Test and Service");
    auto changed_bel = *bel;
    changed_bel.gun_missile = false;
    check(libretro::recognize_profile(changed_bel) == libretro::InputProfile::Unsupported,
          "Behind Enemy Lines changed hardware signature requires review");

    runtime = {};
    runtime.gun_aim_active = {true, true};
    runtime.gun_cursor_x = {124, 371};
    runtime.gun_cursor_y = {96, 287};
    const auto both_crosshairs = libretro::crosshair_state(*vcop, runtime, 3);
    std::vector<u32> crosshair_frame(496 * 384);
    libretro::draw_crosshairs(crosshair_frame, 496, 384, both_crosshairs);
    check(std::count(crosshair_frame.begin(), crosshair_frame.end(), 0x0000ff00u) > 0
              && std::count(crosshair_frame.begin(), crosshair_frame.end(), 0x0000c8ffu) > 0,
          "Default serial-gun crosshair draws upstream SM2-Emu colours");
    const auto supermodel_crosshairs = libretro::crosshair_state(
        *vcop, runtime, 3, libretro::CrosshairStyle::Supermodel);
    std::fill(crosshair_frame.begin(), crosshair_frame.end(), 0);
    libretro::draw_crosshairs(crosshair_frame, 496, 384, supermodel_crosshairs);
    check(std::count(crosshair_frame.begin(), crosshair_frame.end(), 0x00ff0000u) > 0
              && std::count(crosshair_frame.begin(), crosshair_frame.end(), 0x0000ff00u) > 0,
          "Optional serial-gun crosshair draws Supermodel colours");
    runtime.gun_aim_offscreen[0] = true;
    const auto hidden_offscreen = libretro::crosshair_state(*vcop, runtime, 1);
    check(!hidden_offscreen.aims[0].active,
          "Crosshair hides while player one aims or reloads off screen");
    runtime.gun_aim_offscreen = {};
    const auto hidden_positional_crosshair = libretro::crosshair_state(*gunblade, runtime, 0);
    check(!hidden_positional_crosshair.aims[0].active
              && !hidden_positional_crosshair.aims[1].active,
          "Automatic positional-gun mask retains the in-game reticle");
    const auto selected_positional_crosshair = libretro::crosshair_state(*gunblade, runtime, 1);
    check(selected_positional_crosshair.aims[0].active
              && !selected_positional_crosshair.aims[1].active,
          "Explicit selection enables an external positional-gun crosshair");
    // User artwork lookup, per-player fallback, transparency and inactivity.
    const auto artwork_dir = std::filesystem::temp_directory_path()
        / ("sm2-crosshair-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(artwork_dir);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{artwork_dir};
    constexpr unsigned char png[] = {137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 9, 0, 0, 0, 9, 8, 6, 0, 0, 0, 224, 145, 6, 16, 0, 0, 0, 20, 73, 68, 65, 84, 120, 156, 99, 248, 207, 192, 208, 64, 8, 51, 140, 42, 34, 74, 17, 0, 19, 70, 121, 48, 237, 96, 113, 203, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};
    const auto write_png = [&](const char* name) {
        std::ofstream file(artwork_dir / name, std::ios::binary);
        file.write(reinterpret_cast<const char*>(png), sizeof(png));
        check(file.good(), "Write synthetic PNG fixture");
    };
    auto clone = *vcop;
    clone.name = "testclone";
    clone.parent = "vcop";
    libretro::CrosshairPresentation presentation;
    presentation.configure(clone, artwork_dir);
    runtime = {};
    runtime.gun_aim_active = {true, true};
    runtime.gun_cursor_x = {124, 371};
    runtime.gun_cursor_y = {96, 287};
    auto shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Auto, 0, 0.1);
    check(!shown.images[0] && !shown.images[1] && shown.aims[0].active,
          "Auto without artwork retains the SM2-Emu fallback and Disabled timeout");
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Custom, 0, 0.1);
    check(!shown.images[0] && libretro::crosshair_geometry(shown, 496, 384).count == 0
          && !presentation.take_notice().empty(), "Forced Custom reports missing PNGs without substituting a reticle");
    presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Custom, 0, 0.1);
    check(presentation.take_notice().empty(), "Missing artwork notification does not repeat each frame");
    write_png("vcop-p1.png");
    write_png("default-p2.png");
    // A malformed higher-priority file must not defeat a valid parent fallback.
    { std::ofstream invalid(artwork_dir / "testclone.png"); invalid << "invalid PNG"; }
    presentation.configure(clone, artwork_dir);
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Custom, 0, 0.1);
    check(shown.images[0] && shown.images[1] && presentation.take_notice().empty(),
          "Custom lookup supports parent/player and default/player PNGs past malformed candidates");
    check(shown.images[0]->pixels[0] == 0x80ff0000,
          "PNG decoding preserves colour and partial alpha");
    std::fill(crosshair_frame.begin(), crosshair_frame.end(), 0x000000ff);
    libretro::draw_crosshairs(crosshair_frame, 496, 384, shown);
    check(std::count(crosshair_frame.begin(), crosshair_frame.end(), 0x0080007f) > 0,
          "Software custom reticle blends PNG alpha over the game");
    const auto clipped = libretro::crosshair_geometry(shown, 1984, 1536);
    check(clipped.custom && std::all_of(clipped.rectangles.begin(), clipped.rectangles.end(), [](const auto& r) {
        return r.x >= 0 && r.y >= 0 && r.x + r.width <= 1984 && r.y + r.height <= 1536;
    }), "Scaled custom geometry stays within the frame");
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Supermodel, 0, 0.1);
    check(!shown.images[0] && !shown.images[1], "Explicit built-in style ignores custom PNGs");
    presentation.reset_activity();
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Sm2, 3, 0.1);
    check(!shown.aims[0].active && !shown.aims[1].active, "Timeout hides unused players independently");
    runtime.gun_activity[0] = true;
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Sm2, 3, 0.1);
    check(shown.aims[0].active && !shown.aims[1].active, "P1 activity does not show P2's idle crosshair");
    runtime.gun_activity = {};
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Sm2, 3, 2.5);
    check(shown.aims[0].active, "Crosshair remains visible before timeout");
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Sm2, 3, 0.6);
    check(!shown.aims[0].active, "Crosshair hides after configured timeout");
    shown = presentation.update(clone, runtime, 3, libretro::CrosshairStyle::Sm2, 0, 0.1);
    check(shown.aims[0].active && shown.aims[1].active, "Disabling timeout immediately restores selected visibility");
    runtime.gun_activity = {true, true};
    runtime.gun_aim_offscreen[0] = true;
    shown = presentation.update(clone, runtime, 2, libretro::CrosshairStyle::Sm2, 3, 0.1);
    check(!shown.aims[0].active && shown.aims[1].active, "Activity never overrides the visibility mask or off-screen state");
    std::puts("Input profile checks passed");
}
