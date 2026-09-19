// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "input.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sm2::libretro {

enum class CrosshairStyle {
    Sm2,
    Supermodel,
};

struct CrosshairAim {
    bool active = false;
    float x = 0.5f;
    float y = 0.5f;
};

struct CrosshairState {
    std::array<CrosshairAim, 2> aims{};
    CrosshairStyle style = CrosshairStyle::Sm2;
};

struct CrosshairRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::uint32_t colour = 0;
};

struct CrosshairGeometry {
    std::array<CrosshairRect, 1024> rectangles{};
    std::size_t count = 0;
};

/// Build the visible aims selected for any recognised gun profile. The
/// Automatic core-option mask keeps positional-gun reticles hidden by default.
CrosshairState crosshair_state(const rom::GameSpec& game, const InputRuntime& runtime,
                               unsigned mask, CrosshairStyle style = CrosshairStyle::Sm2);

/// Rasterise the selected reticle into a backend-neutral list of clipped
/// rectangles.
CrosshairGeometry crosshair_geometry(const CrosshairState& state, unsigned width,
                                     unsigned height);

/// Composite the same geometry into a Libretro XRGB8888 software frame.
void draw_crosshairs(std::span<std::uint32_t> frame, unsigned width, unsigned height,
                     const CrosshairState& state);

}  // namespace sm2::libretro
