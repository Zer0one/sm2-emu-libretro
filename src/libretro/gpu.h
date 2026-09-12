// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "libretro.h"
#include <memory>
namespace sm2::hw { class Model2MachineBase; }
namespace sm2::libretro {
// Requests a frontend-owned Vulkan 1.3 context with the upstream shader features.
bool request_vulkan(retro_environment_t env, retro_hw_context_reset_t reset,
                    retro_hw_context_reset_t destroy, retro_log_printf_t log);
class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    void init(retro_environment_t env, unsigned scale, retro_log_printf_t log);
    void render(hw::Model2MachineBase& machine, retro_video_refresh_t video);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
// Requests either OpenGL 4.3 core or OpenGL ES 3.1 from the frontend.
bool request_opengl(retro_environment_t env, retro_hw_context_reset_t reset,
                    retro_hw_context_reset_t destroy, bool es,
                    retro_log_printf_t log);
class OpenGlRenderer {
public:
    OpenGlRenderer();
    ~OpenGlRenderer();
    void init(unsigned scale, bool es, retro_log_printf_t log);
    void abandon_context();
    void render(hw::Model2MachineBase& machine, retro_video_refresh_t video);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
