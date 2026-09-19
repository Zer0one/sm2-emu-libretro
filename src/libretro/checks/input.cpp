// SPDX-License-Identifier: BSD-3-Clause
#include "input.h"
#include "crosshair.h"
#include "rom/game_db.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace sm2;
namespace {
std::array<unsigned, 2> pressed{};
std::array<std::array<std::array<s16, 2>, 2>, 2> axes{};
std::array<std::array<s16, 16>, 2> analog_buttons{};
std::array<std::array<s16, 2>, 2> mouse_axes{};
std::array<unsigned, 2> mouse_buttons{};
std::array<std::array<s16, 2>, 2> lightgun_axes{};
std::array<unsigned, 2> lightgun_buttons{};

int16_t state(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port >= 2) throw std::runtime_error("Invalid frontend input port");
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
        {"sgt24h", libretro::InputProfile::DrivingSequentialVR1},
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
    };
    rom::GameDatabase database;
    check(database.load(SM2_GAMES_XML_PATH), "Load catalog games.xml");
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
    std::array<unsigned, 2> full_devices{RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
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
              && controllers.descriptions[0][1].id == libretro::kNoServiceDevice,
          "Full profile is default and reduced profile is subclass");
    check(std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Fighting + Test/Service slots"
              && std::string_view(controllers.descriptions[1][1].desc)
                  == "Joystick (Standard): Fighting"
              && controllers.ports[1].num_types == 2,
          "Player 2 exposes full and reduced profile variants");

    std::array<unsigned, 2> devices{RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
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
    check(!sgt24h_metadata->drive_board && !overrevb->drive_board,
          "Unsupported Super GT and Over Rev drive boards remain disabled");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Driving: Sequential + VR2",
          "Sequential VR2 exposes the approved service variants");
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
              == "Driving: Sequential + VR1",
          "Super GT 24h uses the approved Sequential VR1 profile");
    libretro::configure_controllers(*sgt24h, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential + VR1 + Test/Service slots"
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Driving: Sequential + VR1",
          "Sequential VR1 exposes the approved service variants");
    driving_desc = libretro::descriptors(*sgt24h, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_DOWN, "VR1")
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
          "Sequential VR1 descriptions match the approved workbook");
    check(!has_descriptor_id(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_UP),
          "Sequential VR1 does not expose an unapproved second View command");
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_DOWN)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_R), 0};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *sgt24h, devices, runtime, true, state);
    check(inputs.in1 == 0xce,
          "Sequential VR1 writes only VR1 and Shift Down/Up");
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
              == "Driving: Sequential (Manx TT Superbike)",
          "Manx TT uses the approved profile name");
    libretro::configure_controllers(*manxtt, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential (Manx TT Superbike) + Test/Service slots"
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Driving: Sequential (Manx TT Superbike)",
          "Manx TT exposes the approved service variants");
    driving_desc = libretro::descriptors(*manxtt, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_START, "Start / VR")
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
    check(!has_descriptor_id(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                             RETRO_DEVICE_ID_JOYPAD_UP)
              && !has_descriptor_id(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                    RETRO_DEVICE_ID_JOYPAD_DOWN),
          "Manx TT does not expose unapproved D-Pad commands");
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
              == "Driving: Sequential (Motor Raid)",
          "Motor Raid uses the approved profile name");
    libretro::configure_controllers(*motoraid, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Driving: Sequential (Motor Raid) + Test/Service slots"
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Driving: Sequential (Motor Raid)",
          "Motor Raid exposes the approved service variants");
    driving_desc = libretro::descriptors(*motoraid, devices);
    check(has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_START, "Start / VR")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Kick")
              && has_descriptor(driving_desc, 0, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Punch")
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
          "Motor Raid Shift Down/Up reach the same approved arcade commands");
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_R2, 0};
    libretro::poll_input(inputs, *motoraid, devices, runtime, true, state);
    check(inputs.analog[0] == 0x00,
          "Motor Raid does not convert digital R2 to analog Accelerator");

    const auto* skytargt = database.find("skytargt");
    check(skytargt != nullptr, "Sky Target parent game exists");
    check(std::string_view(libretro::profile_name(libretro::recognize_profile(*skytargt)))
              == "Joystick (Analog): Sky Target",
          "Sky Target uses the approved profile name");
    libretro::configure_controllers(*skytargt, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Analog): Sky Target + Test/Service slots"
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Joystick (Analog): Sky Target",
          "Sky Target exposes the approved service variants");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Joystick (Standard): Baseball (Dynamite Baseball)"
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Baseball (Dynamite Baseball) + Test/Service slots"
          && controllers.ports[1].num_types == 2,
          "Dynamite Baseball exposes both service variants on both players");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Special: Water Ski",
          "Sega Water Ski exposes both service variants");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Special: Ski Super G",
          "Sega Ski Super G exposes both service variants");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Special: Top Skater",
          "Top Skater exposes both service variants");
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
    auto changed_top_skater = *topskatr;
    changed_top_skater.analog[1].control = rom::AnalogControl::StickX;
    check(libretro::recognize_profile(changed_top_skater) == libretro::InputProfile::Unsupported,
          "Changed Top Skater hardware signature requires review");

    const auto* waverunr = database.find("waverunr");
    check(waverunr != nullptr, "Wave Runner parent exists");
    check(libretro::recognize_profile(*waverunr) == libretro::InputProfile::SpecialWaveRunner
              && std::string_view(libretro::profile_name(
                     libretro::recognize_profile(*waverunr))) == "Special: Wave Runner",
          "Wave Runner uses the approved profile");
    libretro::configure_controllers(*waverunr, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Special: Wave Runner + Test/Service slots"
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Special: Wave Runner",
          "Wave Runner exposes both service variants");
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
              && std::string_view(controllers.descriptions[0][1].desc) == single.name
              && std::string_view(controllers.descriptions[1][0].desc)
                  == std::string(single.name) + " + Test/Service slots"
              && std::string_view(controllers.descriptions[1][1].desc) == single.name,
              "Single-player digital game retains both service variants on the second port");
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
          && libretro::profile_players(libretro::InputProfile::BasketballAirWalkers) == 2,
          "Air Walkers exposes the two-player cabinet path supported by SM2");
    libretro::configure_controllers(*airwlkrs, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Standard): Basketball (Air Walkers) + Test/Service slots"
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Basketball (Air Walkers) + Test/Service slots",
          "Air Walkers exposes gameplay controls on both RetroPad ports");
    devices = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
    auto airwlkrs_desc = libretro::descriptors(*airwlkrs, devices);
    check(has_descriptor(airwlkrs_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                         RETRO_DEVICE_ID_JOYPAD_UP, "Joystick Up")
              && has_descriptor(airwlkrs_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_B, "Button 1")
              && has_descriptor(airwlkrs_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_A, "Button 2")
              && has_descriptor(airwlkrs_desc, 1, RETRO_DEVICE_JOYPAD, 0,
                                RETRO_DEVICE_ID_JOYPAD_Y, "Button 3"),
          "Air Walkers publishes the approved P2 directions and buttons");
    pressed = {0, (1u << RETRO_DEVICE_ID_JOYPAD_UP)
                      | (1u << RETRO_DEVICE_ID_JOYPAD_B)
                      | (1u << RETRO_DEVICE_ID_JOYPAD_A)
                      | (1u << RETRO_DEVICE_ID_JOYPAD_Y)
                      | (1u << RETRO_DEVICE_ID_JOYPAD_SELECT)
                      | (1u << RETRO_DEVICE_ID_JOYPAD_START)};
    axes = {};
    analog_buttons = {};
    libretro::poll_input(inputs, *airwlkrs, devices, runtime, true, state);
    check(inputs.in0 == 0xdd && inputs.in1 == 0xff && inputs.in2 == 0xd8,
          "Air Walkers maps P2 Coin, Start, joystick and all three buttons");

    const auto* hpyagu98 = database.find("hpyagu98");
    check(hpyagu98 != nullptr, "Hanguk Pro Yagu 98 parent exists");
    check(libretro::recognize_profile(*hpyagu98)
              == libretro::InputProfile::BaseballHangukProYagu98,
          "Hanguk Pro Yagu 98 uses the approved profile");
    libretro::configure_controllers(*hpyagu98, controllers);
    check(std::string_view(controllers.descriptions[0][0].desc)
              == "Joystick (Standard): Baseball (Hanguk Pro Yagu 98) + Test/Service slots"
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Joystick (Standard): Baseball (Hanguk Pro Yagu 98)"
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Joystick (Standard): Baseball (Hanguk Pro Yagu 98) + Test/Service slots"
          && controllers.ports[1].num_types == 2,
          "Hanguk Pro Yagu 98 exposes both service variants on both players");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Joystick (Analog): Desert Tank + VR3",
          "Desert Tank exposes both service variants");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Gun: Behind Enemy Lines"
          && std::string_view(controllers.descriptions[1][0].desc)
              == "Gun: Behind Enemy Lines + Test/Service slots"
          && controllers.ports[1].num_types == 2,
          "Behind Enemy Lines exposes both service variants on both players");
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
    check(inputs.analog[0] == 0x69 && inputs.analog[1] == 0x96
              && inputs.analog[2] == 0xae && inputs.analog[3] == 0x11,
          "Behind Enemy Lines preserves every declared gun-axis calibration");
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
          && std::string_view(controllers.descriptions[0][1].desc)
              == "Gun (Mouse + Analog Stick)",
          "Gun profiles expose the selected Supermodel-style source name and both service variants");
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

    runtime = {};
    lightgun_axes = {{{-32768, 32767}, {32767, -32768}}};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER,
                        1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER};
    libretro::poll_input(inputs, *vcop, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun, false);
    check(inputs.gun_p1x == vcop->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop->lightgun.p1y.maximum
              && inputs.gun_p2x == vcop->lightgun.p2x.maximum
              && inputs.gun_p2y == vcop->lightgun.p2y.minimum
              && inputs.in1 == 0xfc,
          "Virtua Cop writes calibrated 10-bit coordinates and both trigger bits");
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
              && inputs.in1 == 0xfe,
          "A physical Lightgun offscreen trigger automatically performs reload");
    runtime = {};
    lightgun_axes = {{{-32768, 32767}, {32767, -32768}}};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER,
                        1u << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER};
    libretro::poll_input(inputs, *hotd, devices, runtime, true, state,
                         libretro::GunInputMode::Lightgun);
    check(inputs.in1 == 0xfe && inputs.in2 == 0xfe,
          "House of the Dead routes player two trigger to its declared IN2 bit");

    runtime = {};
    lightgun_buttons = {};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_A, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::AnalogSticks);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum
              && inputs.in1 == 0xfe,
          "RetroPad Reload Offscreen snaps serial aim to the corner and fires");
    runtime = {};
    pressed = {1u << RETRO_DEVICE_ID_JOYPAD_L, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::AnalogSticks);
    check(inputs.gun_p1x == vcop2->lightgun.p1x.minimum
              && inputs.gun_p1y == vcop2->lightgun.p1y.minimum
              && inputs.in1 == 0xfe,
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
              && inputs.in1 == 0xfe,
          "Mouse right implements the same serial offscreen reload");
    runtime = {};
    pressed = {(1u << RETRO_DEVICE_ID_JOYPAD_A)
                   | (1u << RETRO_DEVICE_ID_JOYPAD_L), 0};
    mouse_buttons = {1u << RETRO_DEVICE_ID_MOUSE_RIGHT, 0};
    lightgun_buttons = {1u << RETRO_DEVICE_ID_LIGHTGUN_RELOAD, 0};
    libretro::poll_input(inputs, *vcop2, devices, runtime, true, state,
                         libretro::GunInputMode::Hybrid, false);
    check(inputs.in1 == 0xff,
          "Disabled reload shortcut ignores RetroPad, Mouse and Lightgun shortcut inputs");
    runtime = {};
    pressed = {};
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
    std::puts("Input profile checks passed");
}
