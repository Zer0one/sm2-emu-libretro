// SPDX-License-Identifier: BSD-3-Clause
// Frontend-owned OpenGL 4.3 / OpenGL ES 3.1 renderer using upstream passes.
#include "gpu.h"
#include "hw/model2_machine_base.h"
#include "hw/model2_video.h"
#include "render/gl/gl_common.h"
#include "render/gl/gl_poly3d_pass.h"
#include "render/gl/gl_present_pass.h"
#include "render/gl/gl_tilemap_pass.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace sm2::libretro {
namespace {
retro_hw_render_callback gl_callback{};
retro_log_printf_t gl_logger = nullptr;

SDL_FunctionPointer get_gl_proc(const char* name)
{
    return gl_callback.get_proc_address
        ? reinterpret_cast<SDL_FunctionPointer>(gl_callback.get_proc_address(name))
        : nullptr;
}

}

bool request_opengl(retro_environment_t env, retro_hw_context_reset_t reset,
                    retro_hw_context_reset_t destroy, bool es,
                    retro_log_printf_t log)
{
    gl_logger = log;
    gl_callback = {};
    gl_callback.context_type = es ? RETRO_HW_CONTEXT_OPENGLES_VERSION
                                  : RETRO_HW_CONTEXT_OPENGL_CORE;
    gl_callback.context_reset = reset;
    gl_callback.context_destroy = destroy;
    gl_callback.version_major = es ? 3 : 4;
    gl_callback.version_minor = es ? 1 : 3;
    gl_callback.depth = false;
    gl_callback.stencil = false;
    gl_callback.bottom_left_origin = true;
    gl_callback.cache_context = false;
    if (log) log(RETRO_LOG_INFO, "[SM2 GPU] Requesting %s context\n",
                 es ? "OpenGL ES 3.1" : "OpenGL 4.3 core");
    if (env && env(RETRO_ENVIRONMENT_SET_HW_RENDER, &gl_callback)) return true;
    if (!es || !env) return false;

    // Older frontends may support GLES 3 but not the later versioned-context
    // enum.  Request their legacy GLES3 context and validate the actual context
    // as >= 3.1 in init(), so this compatibility path does not lower our floor.
    gl_callback.context_type = RETRO_HW_CONTEXT_OPENGLES3;
    gl_callback.version_major = 0;
    gl_callback.version_minor = 0;
    if (log) log(RETRO_LOG_INFO, "[SM2 GPU] Retrying legacy OpenGL ES 3 context request\n");
    return env(RETRO_ENVIRONMENT_SET_HW_RENDER, &gl_callback);
}

struct OpenGlRenderer::Impl {
    bool es = false;
    unsigned scale = 1;
    render::gl::TilemapPass tilemaps;
    render::gl::Poly3DPass polygons;
    render::gl::PresentPass present;

    void init(unsigned requested_scale, bool requested_es, retro_log_printf_t log)
    {
        if (!gl_callback.get_proc_address || !gl_callback.get_current_framebuffer)
            throw std::runtime_error("Frontend OpenGL callbacks are unavailable");
        std::string error;
        if (!render::gl::load_gl_functions(get_gl_proc, &error))
            throw std::runtime_error("OpenGL function loading failed: " + error);
        es = requested_es;
        render::gl::resolve_buffer_storage(get_gl_proc, es);
        render::gl::set_version_directive(es ? render::gl::kEsVersionDirective
                                             : render::gl::kDesktopVersionDirective);

        s32 major = 0, minor = 0;
        render::gl::GetIntegerv(GL_MAJOR_VERSION, &major);
        render::gl::GetIntegerv(GL_MINOR_VERSION, &minor);
        if (es) {
            if (major < 3 || (major == 3 && minor < 1))
                throw std::runtime_error("OpenGL ES 3.1 or newer is required");
        } else {
            s32 profile = 0;
            render::gl::GetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
            if (major < 4 || (major == 4 && minor < 3) ||
                !(profile & GL_CONTEXT_CORE_PROFILE_BIT))
                throw std::runtime_error("OpenGL 4.3 core or newer is required");
        }

        s32 max_texture = 0, max_renderbuffer = 0, max_attribs = 0;
        render::gl::GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture);
        render::gl::GetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer);
        render::gl::GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
        if (max_attribs < 4)
            throw std::runtime_error("OpenGL context exposes fewer than four vertex attributes");
        const auto maximum = static_cast<unsigned>(std::max(0, std::min(max_texture, max_renderbuffer)));
        if (requested_scale < 1 || requested_scale > 4 ||
            496u * requested_scale > maximum || 384u * requested_scale > maximum)
            throw std::runtime_error("Requested OpenGL resolution exceeds device limits");
        scale = requested_scale;

        if (!tilemaps.init()) throw std::runtime_error("Cannot initialize upstream OpenGL 2D pass");
        if (!polygons.init(scale)) throw std::runtime_error("Cannot initialize upstream OpenGL 3D pass");
        if (!present.init(scale)) throw std::runtime_error("Cannot initialize OpenGL presentation pass");
        if (log) log(RETRO_LOG_INFO,
            "[SM2 GPU] Ready: %s %d.%d, %ux%u, upstream 2D compute + 3D\n",
            es ? "OpenGL ES" : "OpenGL core", major, minor, 496 * scale, 384 * scale);
    }

    void abandon_context()
    {
        present.abandon_context();
        polygons.abandon_context();
        tilemaps.abandon_context();
    }

    void render(hw::Model2MachineBase& machine, retro_video_refresh_t video,
                const CrosshairState& crosshairs, unsigned texture_quality,
                unsigned upscale_2d)
    {
        auto& frame = machine.video();
        polygons.set_texture_quality(texture_quality);
        tilemaps.set_upscale_2d(upscale_2d);
        const bool render_test = machine.render_test_mode();
        if (render_test) {
            machine.compose_video();
            tilemaps.upload(frame.below(), frame.above());
        } else {
            if (machine.palette_dirty()) {
                frame.refresh_pens();
                machine.clear_palette_dirty();
            }
            tilemaps.compute(machine, frame);
        }
        polygons.build(&machine, frame);
        present.begin_frame();
        tilemaps.draw_below(frame.background());
        if (!render_test) polygons.draw_polygons();
        tilemaps.draw_above();

        const uintptr_t target = gl_callback.get_current_framebuffer();
        if (target > std::numeric_limits<u32>::max())
            throw std::runtime_error("Frontend OpenGL framebuffer name is out of range");
        // Libretro advertises the 4:3 display aspect separately; its frontend
        // owns the final fit.  Copy the complete 496x384 raster here instead of
        // applying the standalone window letterbox inside the core as well.
        present.present(496 * scale, 384 * scale, static_cast<u32>(target), false);
        const CrosshairGeometry geometry = crosshair_geometry(
            crosshairs, 496 * scale, 384 * scale);
        if (geometry.count) {
            render::gl::Enable(GL_SCISSOR_TEST);
            render::gl::ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            for (std::size_t i = 0; i < geometry.count; ++i) {
                const auto& rect = geometry.rectangles[i];
                const float red = static_cast<float>((rect.colour >> 16) & 0xffu) / 255.0f;
                const float green = static_cast<float>((rect.colour >> 8) & 0xffu) / 255.0f;
                const float blue = static_cast<float>(rect.colour & 0xffu) / 255.0f;
                render::gl::Scissor(rect.x, static_cast<int>(384 * scale) - rect.y - rect.height,
                                    rect.width, rect.height);
                render::gl::ClearColor(red, green, blue, 1.0f);
                render::gl::Clear(GL_COLOR_BUFFER_BIT);
            }
            render::gl::Disable(GL_SCISSOR_TEST);
        }
        if (video) video(RETRO_HW_FRAME_BUFFER_VALID, 496 * scale, 384 * scale, 0);
    }
};

OpenGlRenderer::OpenGlRenderer() : impl(std::make_unique<Impl>()) {}
OpenGlRenderer::~OpenGlRenderer() = default;
void OpenGlRenderer::init(unsigned scale, bool es, retro_log_printf_t log)
{
    impl->init(scale, es, log ? log : gl_logger);
}
void OpenGlRenderer::abandon_context() { impl->abandon_context(); }
void OpenGlRenderer::render(hw::Model2MachineBase& machine, retro_video_refresh_t video,
                            const CrosshairState& crosshairs, unsigned texture_quality,
                            unsigned upscale_2d)
{
    impl->render(machine, video, crosshairs, texture_quality, upscale_2d);
}
}
