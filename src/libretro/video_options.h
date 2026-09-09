// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"
namespace sm2::libretro {
inline void register_video_options(retro_environment_t env)
{
    static retro_core_option_v2_category categories[] = {
        {"video", "Video", "Renderer and internal resolution."}, {nullptr, nullptr, nullptr}};
    static retro_core_option_v2_definition definitions[] = {
        {"sm2_renderer", "Renderer (Restart Required)", nullptr,
         "Auto uses Vulkan when preferred by the frontend; otherwise software. Vulkan requires Vulkan 1.3. Reload content after changing this option.",
         nullptr, "video", {{"auto", "Auto"}, {"vulkan", "Vulkan"}, {"software", "Software"}, {nullptr, nullptr}}, "auto"},
        {"sm2_internal_resolution", "Internal Resolution (Restart Required)", nullptr,
         "Vulkan rendering resolution. Tilemaps keep their native detail. Software always uses 496 x 384. Reload content after changing this option.",
         nullptr, "video", {{"1", "1x (496 x 384)"}, {"2", "2x (992 x 768)"},
         {"3", "3x (1488 x 1152)"}, {"4", "4x (1984 x 1536)"}, {nullptr, nullptr}}, "1"},
        {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{nullptr, nullptr}}, nullptr}};
    unsigned version = 0;
    if (env(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
        static retro_core_options_v2 options{categories, definitions};
        env(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
    } else {
        static retro_variable options[] = {
            {"sm2_renderer", "Renderer (Restart Required); auto|vulkan|software"},
            {"sm2_internal_resolution", "Internal Resolution (Restart Required); 1|2|3|4"},
            {nullptr, nullptr}};
        env(RETRO_ENVIRONMENT_SET_VARIABLES, options);
    }
}
}
