// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"
#include "nvram_settings.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace sm2::libretro {
enum class AVTimingMode { Native, Compatibility60Hz };

inline retro_environment_t option_environment = nullptr;
inline std::string option_game;
inline bool nvram_master_visible = false;
inline bool nvram_game_visible = false;
inline std::vector<retro_core_option_v2_definition> registered_definitions;
inline std::deque<std::string> option_key_storage;
inline std::deque<std::string> default_label_storage;
inline std::deque<std::string> legacy_value_storage;
inline std::vector<retro_variable> legacy_definitions;

inline void build_option_definitions()
{
    if (!registered_definitions.empty()) return;
    const auto all = nvram::all_options();
    registered_definitions.reserve(all.size() + 7);

    retro_core_option_v2_definition master{};
    master.key = "sm2_nvram_settings";
    master.desc = "NVRAM Settings (Restart Required)";
    master.info = "Let RetroArch manage supported operator settings for the loaded game. When enabled, every displayed value is applied at startup and overrides later Service Menu changes. When disabled, the core does not modify these fields.";
    master.category_key = "system";
    master.values[0] = {"disabled", "Disabled"};
    master.values[1] = {"enabled", "Enabled"};
    master.default_value = "disabled";
    registered_definitions.push_back(master);

    for (const auto& option : all) {
        option_key_storage.emplace_back("sm2_nvram_" + std::string(option.game) + "_" + option.suffix);
        retro_core_option_v2_definition definition{};
        definition.key = option_key_storage.back().c_str();
        definition.desc = option.label;
        definition.info = option.description;
        definition.category_key = "system";
        const size_t count = std::min(option.value_count, std::size(definition.values) - 1);
        for (size_t value = 0; value < count; ++value) {
            const char* label = option.values[value].label;
            if (std::strcmp(option.values[value].key, option.default_value) == 0) {
                default_label_storage.emplace_back(std::string(label) + " (Default)");
                label = default_label_storage.back().c_str();
            }
            definition.values[value] = {option.values[value].key, label};
        }
        definition.default_value = option.default_value;
        registered_definitions.push_back(definition);
    }
#if defined(SM2_LIBRETRO_VULKAN) || defined(SM2_LIBRETRO_OPENGL)
    retro_core_option_v2_definition renderer{};
    renderer.key = "sm2_renderer";
    renderer.desc = "Renderer (Restart Required)";
    renderer.info = "Auto follows a supported hardware context preferred by the frontend, otherwise software. Vulkan requires Vulkan 1.3; OpenGL requires OpenGL 4.3 core or OpenGL ES 3.1. Reload content after changing this option.";
    renderer.category_key = "video";
    size_t renderer_value = 0;
    renderer.values[renderer_value++] = {"auto", "Auto"};
#ifdef SM2_LIBRETRO_VULKAN
    renderer.values[renderer_value++] = {"vulkan", "Vulkan"};
#endif
#ifdef SM2_LIBRETRO_OPENGL
    renderer.values[renderer_value++] = {"opengl", "OpenGL"};
#endif
    renderer.values[renderer_value++] = {"software", "Software"};
    renderer.default_value = "auto";
    registered_definitions.push_back(renderer);

    retro_core_option_v2_definition resolution{};
    resolution.key = "sm2_internal_resolution";
    resolution.desc = "Internal Resolution (Restart Required)";
    resolution.info = "Hardware rendering resolution for Vulkan and OpenGL. Tilemaps keep their native detail. Software always uses 496 x 384. Reload content after changing this option.";
    resolution.category_key = "video";
    resolution.values[0] = {"1", "1x (496 x 384)"};
    resolution.values[1] = {"2", "2x (992 x 768)"};
    resolution.values[2] = {"3", "3x (1488 x 1152)"};
    resolution.values[3] = {"4", "4x (1984 x 1536)"};
    resolution.default_value = "1";
    registered_definitions.push_back(resolution);
#endif

    retro_core_option_v2_definition timing{};
    timing.key = "sm2_av_timing";
    timing.desc = "A/V Timing (Restart Required)";
    timing.info = "Native reports the Model 2 hardware cadence of 57.524160 Hz. 60 Hz Compatibility preserves machine speed while fitting output to a 60 Hz frontend cadence, occasionally duplicating a video frame and packetizing audio at 60 Hz. Reload content after changing this option.";
    timing.category_key = "video";
    timing.values[0] = {"native", "Native (57.524160 Hz)"};
    timing.values[1] = {"60hz", "60 Hz Compatibility"};
    timing.default_value = "native";
    registered_definitions.push_back(timing);

    retro_core_option_v2_definition overlay{};
    overlay.key = "sm2_timing_overlay";
    overlay.desc = "Timing / FPS Overlay";
    overlay.info = "Show 61-frame averages for machine, video and audio work, total retro_run time, worst frame, actual frontend cadence and estimated processing capacity. The frontend renders the status overlay. Takes effect immediately.";
    overlay.category_key = "video";
    overlay.values[0] = {"disabled", "Disabled"};
    overlay.values[1] = {"enabled", "Enabled"};
    overlay.default_value = "disabled";
    registered_definitions.push_back(overlay);

    registered_definitions.push_back({});
}

inline AVTimingMode av_timing_mode()
{
    retro_variable option{"sm2_av_timing", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option) &&
                   option.value && std::strcmp(option.value, "60hz") == 0
               ? AVTimingMode::Compatibility60Hz
               : AVTimingMode::Native;
}

inline bool timing_overlay_enabled()
{
    retro_variable option{"sm2_timing_overlay", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option) &&
           option.value && std::strcmp(option.value, "enabled") == 0;
}

inline bool update_option_visibility()
{
    if (!option_environment) return false;
    const bool master_visible = !nvram::options_for_game(option_game).empty();
    retro_variable enabled_option{"sm2_nvram_settings", nullptr};
    const bool enabled = option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &enabled_option) &&
                         enabled_option.value && std::string(enabled_option.value) == "enabled";
    const bool game_visible = master_visible && enabled;
    retro_core_option_display master{"sm2_nvram_settings", master_visible};
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &master);
    for (const auto& definition : registered_definitions) {
        if (!definition.key || std::strncmp(definition.key, "sm2_nvram_", 11) != 0 ||
            std::strcmp(definition.key, "sm2_nvram_settings") == 0) continue;
        const std::string prefix = "sm2_nvram_" + option_game + "_";
        retro_core_option_display display{definition.key,
            game_visible && std::strncmp(definition.key, prefix.c_str(), prefix.size()) == 0};
        option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
    }
    const bool changed = master_visible != nvram_master_visible || game_visible != nvram_game_visible;
    nvram_master_visible = master_visible;
    nvram_game_visible = game_visible;
    return changed;
}

inline void set_option_game(std::string game)
{
    option_game = std::move(game);
    update_option_visibility();
}

inline std::vector<std::string> selected_nvram_values(std::string_view game)
{
    std::vector<std::string> selected;
    for (const auto& option : nvram::options_for_game(game)) {
        const std::string key = "sm2_nvram_" + std::string(option.game) + "_" + option.suffix;
        retro_variable variable{key.c_str(), nullptr};
        if (option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) &&
            variable.value)
            selected.emplace_back(variable.value);
        else
            selected.emplace_back(option.default_value);
    }
    return selected;
}

inline bool nvram_settings_enabled()
{
    retro_variable option{"sm2_nvram_settings", nullptr};
    return option_environment && option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &option) &&
           option.value && std::string(option.value) == "enabled";
}

inline void register_core_options(retro_environment_t env)
{
    option_environment = env;
    build_option_definitions();
    static retro_core_option_v2_category categories[] = {
        {"system", "System", "Machine settings stored in battery-backed memory."},
        {"video", "Video", "Renderer, resolution, A/V cadence and diagnostics."},
        {nullptr, nullptr, nullptr}};
    unsigned version = 0;
    if (env(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
        retro_core_options_v2 options{categories, registered_definitions.data()};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        retro_core_options_update_display_callback callback{update_option_visibility};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &callback);
        update_option_visibility();
        return;
    }

    legacy_definitions.clear();
    legacy_value_storage.clear();
    legacy_definitions.reserve(registered_definitions.size());
    for (const auto& definition : registered_definitions) {
        if (!definition.key) break;
        std::string line = std::string(definition.desc) + "; ";
        bool first = true;
        for (const auto& value : definition.values) {
            if (!value.value) break;
            if (!first) line += '|';
            line += value.value;
            first = false;
        }
        legacy_value_storage.push_back(std::move(line));
        legacy_definitions.push_back({definition.key, legacy_value_storage.back().c_str()});
    }
    legacy_definitions.push_back({nullptr, nullptr});
    env(RETRO_ENVIRONMENT_SET_VARIABLES, legacy_definitions.data());
}
}  // namespace sm2::libretro
