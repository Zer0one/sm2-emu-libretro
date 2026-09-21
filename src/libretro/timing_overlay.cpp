// SPDX-License-Identifier: BSD-3-Clause
#include "timing_overlay.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace sm2::libretro {
namespace {

bool initialized = false;
constexpr float kBaseHeight = 384.0f;
constexpr float kPanelMargin = 8.0f;

void coloured_value(const char* label, float value, float warning, float critical,
                    const char* suffix)
{
    const ImVec4 colour = value >= critical ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
        : value >= warning ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
        : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, colour);
    ImGui::Text("%-12s: %5.1f %s", label, static_cast<double>(value), suffix);
    ImGui::PopStyleColor();
}

std::uint32_t blend_xrgb(std::uint32_t destination, float red, float green,
                         float blue, float alpha)
{
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    const float inverse = 1.0f - alpha;
    const auto channel = [inverse, alpha](unsigned current, float source) {
        return static_cast<unsigned>(std::clamp(
            std::lround(current * inverse + source * 255.0f * alpha), 0l, 255l));
    };
    const unsigned r = channel((destination >> 16) & 0xffu, red);
    const unsigned g = channel((destination >> 8) & 0xffu, green);
    const unsigned b = channel(destination & 0xffu, blue);
    return (r << 16) | (g << 8) | b;
}

void rasterize_triangle(std::span<std::uint32_t> frame, unsigned width, unsigned height,
                        const ImDrawVert& a, const ImDrawVert& b, const ImDrawVert& c,
                        const ImVec4& clip, const unsigned char* texture,
                        int texture_width, int texture_height)
{
    const float area = (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y)
                     - (b.pos.y - a.pos.y) * (c.pos.x - a.pos.x);
    if (std::abs(area) < 0.0001f) return;
    const float inverse_area = 1.0f / area;
    const int min_x = std::max(0, static_cast<int>(std::floor(std::max(
        clip.x, std::min({a.pos.x, b.pos.x, c.pos.x})))));
    const int min_y = std::max(0, static_cast<int>(std::floor(std::max(
        clip.y, std::min({a.pos.y, b.pos.y, c.pos.y})))));
    const int max_x = std::min(static_cast<int>(width), static_cast<int>(std::ceil(std::min(
        clip.z, std::max({a.pos.x, b.pos.x, c.pos.x})))));
    const int max_y = std::min(static_cast<int>(height), static_cast<int>(std::ceil(std::min(
        clip.w, std::max({a.pos.y, b.pos.y, c.pos.y})))));

    for (int y = min_y; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float wa = ((b.pos.x - px) * (c.pos.y - py)
                            - (b.pos.y - py) * (c.pos.x - px)) * inverse_area;
            const float wb = ((c.pos.x - px) * (a.pos.y - py)
                            - (c.pos.y - py) * (a.pos.x - px)) * inverse_area;
            const float wc = 1.0f - wa - wb;
            if (wa < 0.0f || wb < 0.0f || wc < 0.0f) continue;

            const float u = wa * a.uv.x + wb * b.uv.x + wc * c.uv.x;
            const float v = wa * a.uv.y + wb * b.uv.y + wc * c.uv.y;
            const int tx = std::clamp(static_cast<int>(u * texture_width), 0,
                                      texture_width - 1);
            const int ty = std::clamp(static_cast<int>(v * texture_height), 0,
                                      texture_height - 1);
            const unsigned char* texel = texture + (ty * texture_width + tx) * 4;
            const auto component = [wa, wb, wc](ImU32 ca, ImU32 cb, ImU32 cc,
                                                unsigned shift) {
                return (wa * static_cast<float>((ca >> shift) & 0xffu)
                      + wb * static_cast<float>((cb >> shift) & 0xffu)
                      + wc * static_cast<float>((cc >> shift) & 0xffu)) / 255.0f;
            };
            const float red = component(a.col, b.col, c.col, IM_COL32_R_SHIFT)
                            * static_cast<float>(texel[0]) / 255.0f;
            const float green = component(a.col, b.col, c.col, IM_COL32_G_SHIFT)
                              * static_cast<float>(texel[1]) / 255.0f;
            const float blue = component(a.col, b.col, c.col, IM_COL32_B_SHIFT)
                             * static_cast<float>(texel[2]) / 255.0f;
            const float alpha = component(a.col, b.col, c.col, IM_COL32_A_SHIFT)
                              * static_cast<float>(texel[3]) / 255.0f;
            auto& pixel = frame[static_cast<std::size_t>(y) * width + x];
            pixel = blend_xrgb(pixel, red, green, blue, alpha);
        }
    }
}

}  // namespace

void timing_overlay_initialize()
{
    if (initialized) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    initialized = true;
}

void timing_overlay_shutdown()
{
    if (!initialized) return;
    ImGui::DestroyContext();
    initialized = false;
}

ImDrawData* build_timing_overlay(const TimingOverlayData& data,
                                 unsigned width, unsigned height,
                                 double frames_per_second)
{
    if (!initialized || !data.enabled || !data.valid || !width || !height)
        return nullptr;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.DeltaTime = static_cast<float>(1.0 / std::max(1.0, frames_per_second));
    // The overlay is composited into the internal framebuffer and is scaled by
    // the frontend together with the game. Scale its complete layout with the
    // internal height so it retains the same readable screen size at 1x-4x.
    const float layout_scale = std::max(1.0f, static_cast<float>(height) / kBaseHeight);
    io.FontGlobalScale = 1.0f;

    ImGui::NewFrame();
    const float font_pixels = static_cast<float>(std::clamp(data.font_pixels, 11u, 14u));
    ImGui::PushFont(nullptr, font_pixels * layout_scale);
    ImGui::SetNextWindowPos(
        ImVec2(kPanelMargin * layout_scale, kPanelMargin * layout_scale),
        ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(8.0f * layout_scale, 8.0f * layout_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(8.0f * layout_scale, 4.0f * layout_scale));
    ImGui::Begin("##timings", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("SM2 timing (%s)", data.native_timing ? "57.524 Hz" : "60 Hz");
    ImGui::Separator();
    ImGui::Text("61-frame averages");
    coloured_value("Machine", data.machine_ms, 12.0f, 17.0f, "ms");
    coloured_value("Video", data.video_ms, 12.0f, 17.0f, "ms");
    coloured_value("Audio/pacing", data.audio_ms, 4.0f, 8.0f, "ms");
    coloured_value("retro_run", data.run_ms, 16.0f, 20.0f, "ms");
    coloured_value("Worst", data.worst_ms, 20.0f, 34.0f, "ms");
    ImGui::Text("Actual      : %5.1f FPS", static_cast<double>(data.actual_fps));
    ImGui::Text("Engine cap  : %5.1f FPS", static_cast<double>(data.engine_cap_fps));
    ImGui::Text("Callback cap: %5.1f FPS", static_cast<double>(data.callback_cap_fps));
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopFont();
    ImGui::Render();
    return ImGui::GetDrawData();
}

void draw_timing_overlay_software(std::span<std::uint32_t> frame,
                                  unsigned width, unsigned height,
                                  const TimingOverlayData& data,
                                  double frames_per_second)
{
    if (!initialized || !data.enabled || !data.valid
        || frame.size() < static_cast<std::size_t>(width) * height) return;
    unsigned char* pixels = nullptr;
    int texture_width = 0, texture_height = 0;
    // The software path uses ImGui's legacy atlas API. Build the default font
    // before NewFrame(), then refresh the pixels after the selected size has
    // been baked while constructing the overlay.
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &texture_width, &texture_height);
    ImGui::GetIO().Fonts->SetTexID(static_cast<ImTextureID>(1));
    ImDrawData* draw_data = build_timing_overlay(data, width, height, frames_per_second);
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &texture_width, &texture_height);
    if (!draw_data || !pixels || texture_width <= 0 || texture_height <= 0) return;

    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
        const ImDrawList* list = draw_data->CmdLists[list_index];
        for (const ImDrawCmd& command : list->CmdBuffer) {
            if (command.UserCallback || command.GetTexID() != static_cast<ImTextureID>(1))
                continue;
            for (unsigned index = 0; index + 2 < command.ElemCount; index += 3) {
                const auto vertex = [&](unsigned offset) -> const ImDrawVert& {
                    const ImDrawIdx i = list->IdxBuffer[command.IdxOffset + index + offset];
                    return list->VtxBuffer[command.VtxOffset + i];
                };
                rasterize_triangle(frame, width, height, vertex(0), vertex(1), vertex(2),
                                   command.ClipRect, pixels, texture_width, texture_height);
            }
        }
    }
}

}  // namespace sm2::libretro
