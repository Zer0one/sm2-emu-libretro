// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"

#include <string>
#include <utility>

namespace sm2::libretro {
inline retro_environment_t option_environment = nullptr;
inline std::string option_game;
inline bool nvram_master_visible = false;
inline bool vf2_options_visible = false;

inline bool update_option_visibility()
{
    if (!option_environment) return false;
    const bool master_visible = option_game == "vf2";
    retro_variable enabled_option{"sm2_nvram_settings", nullptr};
    const bool enabled = option_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &enabled_option) &&
                         enabled_option.value && std::string(enabled_option.value) == "enabled";
    const bool visible = master_visible && enabled;
    retro_core_option_display master{"sm2_nvram_settings", master_visible};
    retro_core_option_display country{"sm2_vf2_country", visible};
    retro_core_option_display drink{"sm2_vf2_drink", visible};
    retro_core_option_display difficulty{"sm2_vf2_difficulty", visible};
    retro_core_option_display display_type{"sm2_vf2_display_type", visible};
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &master);
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &country);
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &drink);
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &difficulty);
    option_environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display_type);
    const bool changed = master_visible != nvram_master_visible ||
                         visible != vf2_options_visible;
    nvram_master_visible = master_visible;
    vf2_options_visible = visible;
    return changed;
}

inline void set_option_game(std::string game)
{
    option_game = std::move(game);
    update_option_visibility();
}

inline void register_core_options(retro_environment_t env)
{
    option_environment = env;
    static retro_core_option_v2_category categories[] = {
        {"system", "System", "Machine settings stored in battery-backed memory."},
#ifdef SM2_LIBRETRO_VULKAN
        {"video", "Video", "Renderer and internal resolution."},
#endif
        {nullptr, nullptr, nullptr}};
    static retro_core_option_v2_definition definitions[] = {
        {"sm2_nvram_settings", "NVRAM Settings (Restart Required)", nullptr,
         "Let RetroArch manage the supported operator settings for the loaded game. When enabled, every displayed value is applied at startup and overrides later Service Menu changes. When disabled, the core does not modify these fields.",
         nullptr, "system", {{"disabled", "Disabled"}, {"enabled", "Enabled"},
         {nullptr, nullptr}}, "disabled"},
        {"sm2_vf2_country", "VF2 Country (Restart Required)", nullptr,
         "Country stored in Virtua Fighter 2's operator settings. In the original service menu, selecting USA or Export also sets Drink to NG. Core options keep the two settings independent; use VF2 Drink to reproduce that behavior. Only applies to vf2.",
         nullptr, "system", {{"japan", "Japan"}, {"usa", "USA"},
         {"export", "Export"}, {nullptr, nullptr}}, "japan"},
        {"sm2_vf2_difficulty", "VF2 Difficulty (Restart Required)", nullptr,
         "Difficulty stored in Virtua Fighter 2's operator settings. Also applies the game's matching Energy Max and Stage Width values.",
         nullptr, "system", {{"normal", "Normal"}, {"hard", "Hard"},
         {"hardest", "Hardest"}, {"easy", "Easy"}, {nullptr, nullptr}}, "normal"},
        {"sm2_vf2_display_type", "VF2 Display Type (Restart Required)", nullptr,
         "Display calibration profile stored in Virtua Fighter 2's operator settings.",
         nullptr, "system", {{"projector", "Projector"}, {"crt", "C.R.T."},
         {nullptr, nullptr}}, "projector"},
        {"sm2_vf2_drink", "VF2 Drink (Restart Required)", nullptr,
         "Drink setting stored in Virtua Fighter 2's operator settings. Independent from VF2 Country.",
         nullptr, "system", {{"ok", "OK"}, {"ng", "NG"},
         {nullptr, nullptr}}, "ok"},
#ifdef SM2_LIBRETRO_VULKAN
        {"sm2_renderer", "Renderer (Restart Required)", nullptr,
         "Auto uses Vulkan when preferred by the frontend; otherwise software. Vulkan requires Vulkan 1.3. Reload content after changing this option.",
         nullptr, "video", {{"auto", "Auto"}, {"vulkan", "Vulkan"}, {"software", "Software"}, {nullptr, nullptr}}, "auto"},
        {"sm2_internal_resolution", "Internal Resolution (Restart Required)", nullptr,
         "Vulkan rendering resolution. Tilemaps keep their native detail. Software always uses 496 x 384. Reload content after changing this option.",
         nullptr, "video", {{"1", "1x (496 x 384)"}, {"2", "2x (992 x 768)"},
         {"3", "3x (1488 x 1152)"}, {"4", "4x (1984 x 1536)"}, {nullptr, nullptr}}, "1"},
#endif
        {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{nullptr, nullptr}}, nullptr}};
    unsigned version = 0;
    if (env(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
        static retro_core_options_v2 options{categories, definitions};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
        retro_core_options_update_display_callback callback{update_option_visibility};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &callback);
        update_option_visibility();
    } else {
        static retro_variable options[] = {
            {"sm2_nvram_settings", "NVRAM Settings (Restart Required); disabled|enabled"},
            {"sm2_vf2_country", "VF2 Country (Restart Required); japan|usa|export"},
            {"sm2_vf2_difficulty", "VF2 Difficulty (Restart Required); normal|hard|hardest|easy"},
            {"sm2_vf2_display_type", "VF2 Display Type (Restart Required); projector|crt"},
            {"sm2_vf2_drink", "VF2 Drink (Restart Required); ok|ng"},
#ifdef SM2_LIBRETRO_VULKAN
            {"sm2_renderer", "Renderer (Restart Required); auto|vulkan|software"},
            {"sm2_internal_resolution", "Internal Resolution (Restart Required); 1|2|3|4"},
#endif
            {nullptr, nullptr}};
        env(RETRO_ENVIRONMENT_SET_VARIABLES, options);
    }
}
}
