// SPDX-License-Identifier: BSD-3-Clause
#include "timing_overlay.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

int main()
{
    constexpr unsigned width = 496;
    constexpr unsigned height = 384;
    constexpr std::uint32_t background = 0x00204080u;
    std::vector<std::uint32_t> frame(width * height, background);
    sm2::libretro::TimingOverlayData data{};
    data.enabled = data.valid = data.native_timing = true;
    data.machine_ms = 5.7f;
    data.video_ms = 5.5f;
    data.audio_ms = 0.2f;
    data.run_ms = 11.4f;
    data.worst_ms = 13.8f;
    data.actual_fps = 57.5f;
    data.engine_cap_fps = 89.3f;
    data.callback_cap_fps = 87.7f;

    sm2::libretro::timing_overlay_initialize();
    sm2::libretro::draw_timing_overlay_software(frame, width, height, data, 57.524160);
    sm2::libretro::draw_timing_overlay_software(frame, width, height, data, 57.524160);
    constexpr unsigned scaled_width = width * 4;
    constexpr unsigned scaled_height = height * 4;
    std::vector<std::uint32_t> scaled_frame(
        static_cast<std::size_t>(scaled_width) * scaled_height, background);
    sm2::libretro::draw_timing_overlay_software(
        scaled_frame, scaled_width, scaled_height, data, 57.524160);
    sm2::libretro::timing_overlay_shutdown();

    unsigned min_x = width, min_y = height, max_x = 0, max_y = 0;
    std::size_t changed = 0;
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            if (frame[static_cast<std::size_t>(y) * width + x] == background) continue;
            ++changed;
            min_x = std::min(min_x, x); min_y = std::min(min_y, y);
            max_x = std::max(max_x, x); max_y = std::max(max_y, y);
        }
    }
    if (changed <= 1000) {
        std::fprintf(stderr, "overlay changed only %zu pixels\n", changed);
        return 1;
    }
    if (min_x < 8 || min_y < 8) {
        std::fprintf(stderr, "overlay begins outside panel: %u,%u\n", min_x, min_y);
        return 2;
    }
    if (max_x >= width || max_y >= height) {
        std::fprintf(stderr, "overlay exceeds panel: %u,%u\n", max_x, max_y);
        return 3;
    }

    // Renderer scaling must not change the auto-fitted panel dimensions.
    unsigned scaled_max_x = 0;
    unsigned scaled_min_x = scaled_width;
    for (unsigned y = 0; y < scaled_height; ++y) {
        for (unsigned x = 0; x < scaled_width; ++x) {
            if (scaled_frame[static_cast<std::size_t>(y) * scaled_width + x]
                != background) {
                scaled_min_x = std::min(scaled_min_x, x);
                scaled_max_x = std::max(scaled_max_x, x);
            }
        }
    }
    if (scaled_min_x != min_x || scaled_max_x != max_x) {
        std::fprintf(stderr, "overlay dimensions change with renderer: %u-%u vs %u-%u\n",
                     min_x, max_x, scaled_min_x, scaled_max_x);
        return 5;
    }
    return 0;
}
