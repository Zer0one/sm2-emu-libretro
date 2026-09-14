// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "input.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sm2::libretro {

struct CrosshairAim {
    bool active = false;
    float x = 0.5f;
    float y = 0.5f;
};

struct CrosshairState {
    std::array<CrosshairAim, 2> aims{};
};

struct CrosshairRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::uint32_t colour = 0;
};

struct CrosshairGeometry {
    std::array<CrosshairRect, 64> rectangles{};
    std::size_t count = 0;
};

/// Build the visible aims for every recognised gun profile.
CrosshairState crosshair_state(const rom::GameSpec& game, const InputRuntime& runtime,
                               unsigned mask);

/// Rasterise the native Supermodel-style four-wedge vector reticle into a
/// backend-neutral list of clipped rectangles.
CrosshairGeometry crosshair_geometry(const CrosshairState& state, unsigned width,
                                     unsigned height);

/// Composite the same geometry into a Libretro XRGB8888 software frame.
void draw_crosshairs(std::span<std::uint32_t> frame, unsigned width, unsigned height,
                     const CrosshairState& state);

}  // namespace sm2::libretro
