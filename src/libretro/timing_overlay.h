// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <cstdint>
#include <span>

struct ImDrawData;

namespace sm2::libretro {

struct TimingOverlayData {
    bool enabled = false;
    bool valid = false;
    bool native_timing = true;
    float machine_ms = 0.0f;
    float video_ms = 0.0f;
    float audio_ms = 0.0f;
    float run_ms = 0.0f;
    float worst_ms = 0.0f;
    float actual_fps = 0.0f;
    float engine_cap_fps = 0.0f;
    float callback_cap_fps = 0.0f;
};

// The timing panel intentionally follows the Supermodel Libretro overlay:
// Dear ImGui fits an input-free window around the text with uniform padding.
void timing_overlay_initialize();
void timing_overlay_shutdown();
ImDrawData* build_timing_overlay(const TimingOverlayData& data,
                                 unsigned width, unsigned height,
                                 double frames_per_second);

// Software rendering uses the same ImGui draw data as the GPU backends.
void draw_timing_overlay_software(std::span<std::uint32_t> frame,
                                  unsigned width, unsigned height,
                                  const TimingOverlayData& data,
                                  double frames_per_second);

}  // namespace sm2::libretro
