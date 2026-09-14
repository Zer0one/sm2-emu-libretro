// SPDX-License-Identifier: BSD-3-Clause
#include "crosshair.h"

#include <algorithm>
#include <cmath>

namespace sm2::libretro {
namespace {

constexpr std::array<std::uint32_t, 2> kColours = {
    0x00ff0000u, // Player 1: red, matching Supermodel's vector crosshair.
    0x0000ff00u, // Player 2: green.
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

}  // namespace

CrosshairState crosshair_state(const rom::GameSpec& game, const InputRuntime& runtime,
                               unsigned mask)
{
    CrosshairState state{};
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
    constexpr int layers = 8;
    constexpr int base_half_width = 3;
    const int gap = 2 * scale;

    for (unsigned player = 0; player < state.aims.size(); ++player) {
        const auto& aim = state.aims[player];
        if (!aim.active) continue;
        const int cx = static_cast<int>(std::lround(
            std::clamp(aim.x, 0.0f, 1.0f) * static_cast<float>(width - 1)));
        const int cy = static_cast<int>(std::lround(
            std::clamp(aim.y, 0.0f, 1.0f) * static_cast<float>(height - 1)));
        for (int layer = 0; layer < layers; ++layer) {
            const int distance = gap + layer * scale;
            const int half_width = std::max(scale,
                ((layer + 1) * base_half_width * scale + layers - 1) / layers);
            const int span = 2 * half_width + 1;
            add_clipped(geometry, cx - half_width, cy - distance - scale + 1,
                        span, scale, width, height, kColours[player]);
            add_clipped(geometry, cx - half_width, cy + distance,
                        span, scale, width, height, kColours[player]);
            add_clipped(geometry, cx - distance - scale + 1, cy - half_width,
                        scale, span, width, height, kColours[player]);
            add_clipped(geometry, cx + distance, cy - half_width,
                        scale, span, width, height, kColours[player]);
        }
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
