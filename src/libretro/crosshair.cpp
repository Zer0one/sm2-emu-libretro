// SPDX-License-Identifier: BSD-3-Clause
#include "crosshair.h"

#include <algorithm>
#include <cmath>

namespace sm2::libretro {
namespace {

constexpr std::array<std::uint32_t, 2> kSupermodelColours = {
    0x00ff0000u, // Player 1: red, matching Supermodel's vector crosshair.
    0x0000ff00u, // Player 2: green.
};

constexpr std::array<std::uint32_t, 2> kSm2Colours = {
    0x0000ff00u, // Player 1: green, matching upstream SM2-Emu.
    0x0000c8ffu, // Player 2: cyan.
};

void add_clipped(CrosshairGeometry& geometry, int x, int y, int width, int height,
                 unsigned frame_width, unsigned frame_height, std::uint32_t colour)
{
    const int x0 = std::clamp(x, 0, static_cast<int>(frame_width));
    const int y0 = std::clamp(y, 0, static_cast<int>(frame_height));
    const int x1 = std::clamp(x + width, 0, static_cast<int>(frame_width));
    const int y1 = std::clamp(y + height, 0, static_cast<int>(frame_height));
    if (x1 <= x0 || y1 <= y0 || geometry.count == geometry.rectangles.size()) return;
    geometry.rectangles[geometry.count++] = {x0, y0, x1 - x0, y1 - y0, colour};
}

void add_sm2_crosshair(CrosshairGeometry& geometry, int cx, int cy, int scale,
                       unsigned frame_width, unsigned frame_height,
                       std::uint32_t colour)
{
    // Upstream draws an eight-pixel radius circle with four cardinal lines.
    // Scale it with the core's internal raster so it retains the same apparent
    // size after the frontend presents an upscaled frame.
    const int radius = 8 * scale;
    const int thickness = 2 * scale;
    const int outer = radius + thickness / 2;
    const int inner = std::max(0, radius - (thickness + 1) / 2);
    for (int dy = -outer; dy <= outer; ++dy) {
        const int outer_x = static_cast<int>(std::floor(std::sqrt(
            static_cast<double>(outer * outer - dy * dy))));
        int inner_x = -1;
        if (std::abs(dy) < inner) {
            inner_x = static_cast<int>(std::floor(std::sqrt(
                static_cast<double>(inner * inner - dy * dy))));
        }
        if (inner_x < 0) {
            add_clipped(geometry, cx - outer_x, cy + dy, 2 * outer_x + 1, 1,
                        frame_width, frame_height, colour);
        } else {
            add_clipped(geometry, cx - outer_x, cy + dy, outer_x - inner_x, 1,
                        frame_width, frame_height, colour);
            add_clipped(geometry, cx + inner_x + 1, cy + dy, outer_x - inner_x, 1,
                        frame_width, frame_height, colour);
        }
    }

    const int near = (radius * 2) / 5;
    const int far = (radius * 8) / 5;
    const int half = thickness / 2;
    add_clipped(geometry, cx - far, cy - half, far - near, thickness,
                frame_width, frame_height, colour);
    add_clipped(geometry, cx + near, cy - half, far - near, thickness,
                frame_width, frame_height, colour);
    add_clipped(geometry, cx - half, cy - far, thickness, far - near,
                frame_width, frame_height, colour);
    add_clipped(geometry, cx - half, cy + near, thickness, far - near,
                frame_width, frame_height, colour);
}

void add_supermodel_crosshair(CrosshairGeometry& geometry, int cx, int cy, int scale,
                              unsigned frame_width, unsigned frame_height,
                              std::uint32_t colour)
{
    constexpr int layers = 8;
    constexpr int base_half_width = 3;
    const int gap = 2 * scale;
    for (int layer = 0; layer < layers; ++layer) {
        const int distance = gap + layer * scale;
        const int half_width = std::max(scale,
            ((layer + 1) * base_half_width * scale + layers - 1) / layers);
        const int span = 2 * half_width + 1;
        add_clipped(geometry, cx - half_width, cy - distance - scale + 1,
                    span, scale, frame_width, frame_height, colour);
        add_clipped(geometry, cx - half_width, cy + distance,
                    span, scale, frame_width, frame_height, colour);
        add_clipped(geometry, cx - distance - scale + 1, cy - half_width,
                    scale, span, frame_width, frame_height, colour);
        add_clipped(geometry, cx + distance, cy - half_width,
                    scale, span, frame_width, frame_height, colour);
    }
}

}  // namespace

CrosshairState crosshair_state(const rom::GameSpec& game, const InputRuntime& runtime,
                               unsigned mask, CrosshairStyle style)
{
    CrosshairState state{};
    state.style = style;
    const InputProfile profile = recognize_profile(game);
    if (profile != InputProfile::Gun && profile != InputProfile::GunBehindEnemyLines)
        return state;
    mask &= 3u;
    for (unsigned player = 0; player < state.aims.size(); ++player) {
        auto& aim = state.aims[player];
        aim.active = (mask & (1u << player)) && runtime.gun_aim_active[player]
            && !runtime.gun_aim_offscreen[player];
        aim.x = static_cast<float>(runtime.gun_cursor_x[player]) / 495.0f;
        aim.y = static_cast<float>(runtime.gun_cursor_y[player]) / 383.0f;
    }
    return state;
}

CrosshairGeometry crosshair_geometry(const CrosshairState& state, unsigned width,
                                     unsigned height)
{
    CrosshairGeometry geometry{};
    if (!width || !height) return geometry;
    const int scale = std::max(1, std::min(static_cast<int>(width / 496u),
                                           static_cast<int>(height / 384u)));
    for (unsigned player = 0; player < state.aims.size(); ++player) {
        const auto& aim = state.aims[player];
        if (!aim.active) continue;
        const int cx = static_cast<int>(std::lround(
            std::clamp(aim.x, 0.0f, 1.0f) * static_cast<float>(width - 1)));
        const int cy = static_cast<int>(std::lround(
            std::clamp(aim.y, 0.0f, 1.0f) * static_cast<float>(height - 1)));
        if (state.style == CrosshairStyle::Sm2)
            add_sm2_crosshair(geometry, cx, cy, scale, width, height,
                              kSm2Colours[player]);
        else
            add_supermodel_crosshair(geometry, cx, cy, scale, width, height,
                                     kSupermodelColours[player]);
    }
    return geometry;
}

void draw_crosshairs(std::span<std::uint32_t> frame, unsigned width, unsigned height,
                     const CrosshairState& state)
{
    if (frame.size() < static_cast<std::size_t>(width) * height) return;
    const CrosshairGeometry geometry = crosshair_geometry(state, width, height);
    for (std::size_t i = 0; i < geometry.count; ++i) {
        const auto& rect = geometry.rectangles[i];
        for (int y = rect.y; y < rect.y + rect.height; ++y) {
            auto row = frame.begin() + static_cast<std::ptrdiff_t>(y) * width + rect.x;
            std::fill_n(row, rect.width, rect.colour);
        }
    }
}

}  // namespace sm2::libretro
