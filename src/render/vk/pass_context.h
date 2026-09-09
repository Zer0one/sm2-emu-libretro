// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "render/vk/vk_common.h"
using VmaAllocator = struct VmaAllocator_T*;
namespace sm2::render::vk {
// Resources required by the shared rendering passes. The host owns scheduling,
// device lifetime and presentation; neither pass needs a window or swapchain.
class PassContext {
public:
    static constexpr u32 kFramesInFlight = 3;
    virtual ~PassContext() = default;
    virtual VkDevice device() const = 0;
    virtual VmaAllocator allocator() const = 0;
    virtual VkCommandBuffer cmd() const = 0;
    virtual u32 frame_index() const = 0;
    virtual VkFormat stencil_format() const = 0;
    virtual bool stencil_format_has_depth() const = 0;
    virtual void write_timestamp(VkPipelineStageFlags2, GpuStage, bool) {}
};
}
