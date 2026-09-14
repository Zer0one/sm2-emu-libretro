// SPDX-License-Identifier: BSD-3-Clause
#include "gpu.h"
#include "libretro_vulkan.h"
#include "render/vk/pass_context.h"
#include "render/vk/tilemap_pass.h"
#include "render/vk/poly3d_pass.h"
#include "hw/model2_machine_base.h"
#include "hw/model2_video.h"
#include "vk_dispatch.h"
#include <vk_mem_alloc.h>
#include <array>
#include <stdexcept>
#include <string>

namespace sm2::libretro {
bool validate_vulkan_device(VkDevice device);
namespace {
void check(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": " + render::vk::result_string(result));
}
struct QueueLock {
    const retro_hw_render_interface_vulkan& vk;
    explicit QueueLock(const retro_hw_render_interface_vulkan& v) : vk(v) { vk.lock_queue(vk.handle); }
    ~QueueLock() { vk.unlock_queue(vk.handle); }
};
}
struct VulkanRenderer::Impl final : render::vk::PassContext {
    const retro_hw_render_interface_vulkan* vk = nullptr;
    VmaAllocator alloc = nullptr;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, kFramesInFlight> commands{};
    std::array<VkFence, kFramesInFlight> fences{};
    unsigned slot = 0, scale = 1;
    uint32_t sync_mask = 0;
    VkFormat stencil = VK_FORMAT_UNDEFINED;
    render::vk::TilemapPass tilemaps;
    render::vk::Poly3DPass polygons;
    struct Output {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        retro_vulkan_image frontend{};
    };
    // The frontend sync index is independent of the upstream three-frame ring.
    std::array<Output, 32> outputs{};
    VkDevice device() const override { return vk->device; }
    VmaAllocator allocator() const override { return alloc; }
    VkCommandBuffer cmd() const override { return commands[slot]; }
    unsigned frame_index() const override { return slot; }
    VkFormat stencil_format() const override { return stencil; }
    bool stencil_format_has_depth() const override { return stencil != VK_FORMAT_S8_UINT; }
    void idle() {
        if (vk) { QueueLock lock(*vk); check(vkDeviceWaitIdle(device()), "Wait for GPU"); }
    }
    void destroy_outputs() {
        for (auto& out : outputs) {
            if (out.frontend.image_view) vkDestroyImageView(device(), out.frontend.image_view, nullptr);
            if (out.image) vmaDestroyImage(alloc, out.image, out.allocation);
            out = {};
        }
    }
    ~Impl() override {
        if (!vk) return;
        // The frontend still owns a live device in context_destroy/unload.
        // On device loss, continue releasing our resources without throwing.
        { QueueLock lock(*vk); (void)vkDeviceWaitIdle(device()); }
        polygons.shutdown(); tilemaps.shutdown();
        destroy_outputs();
        for (auto fence : fences) if (fence) vkDestroyFence(device(), fence, nullptr);
        if (pool) vkDestroyCommandPool(device(), pool, nullptr);
        if (alloc) vmaDestroyAllocator(alloc);
        // Never destroy the frontend's device, instance, queue or swapchain.
    }
    void init(retro_environment_t env, unsigned requested_scale, retro_log_printf_t log) {
        const retro_hw_render_interface_vulkan* iface = nullptr;
        if (!env(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &iface) || !iface ||
            iface->interface_type != RETRO_HW_RENDER_INTERFACE_VULKAN ||
            iface->interface_version != RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION ||
            !iface->get_device_proc_addr || !iface->get_instance_proc_addr ||
            !iface->get_sync_index || !iface->get_sync_index_mask ||
            !iface->wait_sync_index || !iface->set_image || !iface->lock_queue || !iface->unlock_queue)
            throw std::runtime_error("Frontend Vulkan interface v5 is unavailable");
        if (!validate_vulkan_device(iface->device))
            throw std::runtime_error("Frontend did not enable the required Vulkan shader features");
        // Resolve from this frontend's loader, never a second system Vulkan loader.
        load_vk_dispatch(*iface);
        vk = iface;
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(vk->gpu, &props);
        if (requested_scale < 1 || requested_scale > 4 ||
            496 * requested_scale > props.limits.maxImageDimension2D ||
            384 * requested_scale > props.limits.maxImageDimension2D)
            throw std::runtime_error("Requested GPU resolution exceeds device limits");
        scale = requested_scale;
        for (auto format : {VK_FORMAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT}) {
            VkFormatProperties supported{};
            vkGetPhysicalDeviceFormatProperties(vk->gpu, format, &supported);
            if (supported.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                stencil = format; break;
            }
        }
        if (stencil == VK_FORMAT_UNDEFINED) throw std::runtime_error("No stencil fill-mask format");
        VmaVulkanFunctions functions{};
        functions.vkGetInstanceProcAddr = vk->get_instance_proc_addr;
        functions.vkGetDeviceProcAddr = vk->get_device_proc_addr;
        VmaAllocatorCreateInfo info{};
        info.vulkanApiVersion = VK_API_VERSION_1_3;
        info.instance = vk->instance; info.physicalDevice = vk->gpu; info.device = device();
        info.pVulkanFunctions = &functions;
        check(vmaCreateAllocator(&info, &alloc), "Create GPU allocator");
        VkCommandPoolCreateInfo cp{};
        cp.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cp.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cp.queueFamilyIndex = vk->queue_index;
        check(vkCreateCommandPool(device(), &cp, nullptr, &pool), "Create command pool");
        VkCommandBufferAllocateInfo ca{};
        ca.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ca.commandPool = pool; ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ca.commandBufferCount = kFramesInFlight;
        check(vkAllocateCommandBuffers(device(), &ca, commands.data()), "Allocate commands");
        VkFenceCreateInfo fi{};
        fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (auto& fence : fences) check(vkCreateFence(device(), &fi, nullptr, &fence), "Create frame fence");
        if (!tilemaps.init(*this, scale) || !polygons.init(*this, scale))
            throw std::runtime_error("Cannot initialize upstream Vulkan passes");
        if (log) log(RETRO_LOG_INFO, "[SM2 GPU] Ready: %s, Vulkan %u.%u.%u, %ux%u, upstream 2D compute + 3D\n",
            props.deviceName, VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
            VK_VERSION_PATCH(props.apiVersion), 496 * scale, 384 * scale);
    }
    Output& output(unsigned index) {
        auto& out = outputs.at(index);
        if (out.image) return out;
        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = render::vk::kNativeColourFormat;
        image.extent = {496 * scale, 384 * scale, 1};
        image.mipLevels = image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo allocation{}; allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        check(vmaCreateImage(alloc, &image, &allocation, &out.image, &out.allocation, nullptr), "Create output image");
        auto& view = out.frontend.create_info;
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = out.image; view.viewType = VK_IMAGE_VIEW_TYPE_2D; view.format = image.format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        check(vkCreateImageView(device(), &view, nullptr, &out.frontend.image_view), "Create output view");
        out.frontend.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return out;
    }
    void render(hw::Model2MachineBase& machine, retro_video_refresh_t video,
                const CrosshairState& crosshairs, unsigned texture_quality,
                unsigned upscale_2d) {
        const uint32_t mask = vk->get_sync_index_mask(vk->handle);
        const unsigned index = vk->get_sync_index(vk->handle);
        if (index >= outputs.size() || !(mask & (1u << index)))
            throw std::runtime_error("Invalid frontend GPU synchronization index");
        if (mask != sync_mask) { idle(); destroy_outputs(); sync_mask = mask; }
        vk->wait_sync_index(vk->handle);
        check(vkWaitForFences(device(), 1, &fences[slot], VK_TRUE, UINT64_MAX), "Wait for frame resources");
        check(vkResetCommandBuffer(cmd(), 0), "Reset commands");
        auto& out = output(index);
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(cmd(), &begin), "Begin GPU frame");
        auto& frame = machine.video();
        polygons.set_texture_quality(texture_quality);
        tilemaps.set_upscale_2d(upscale_2d);
        const bool render_test = machine.render_test_mode();
        if (render_test) {
            machine.compose_video();
            tilemaps.upload(frame.below(), frame.above());
        } else {
            if (machine.palette_dirty()) { frame.refresh_pens(); machine.clear_palette_dirty(); }
            tilemaps.compute(machine, frame);
        }
        polygons.build(&machine, frame);
        polygons.prepare_stencil();
        render::vk::record_image_barrier(cmd(), out.image, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        const auto attachment = polygons.stencil_attachment();
        tilemaps.record_below(out.frontend.image_view, frame.background(), &attachment, polygons.stencil_has_depth());
        if (!render_test) polygons.draw_polygons();
        tilemaps.record_above();
        const CrosshairGeometry geometry = crosshair_geometry(
            crosshairs, 496 * scale, 384 * scale);
        if (geometry.count) {
            VkRenderingAttachmentInfo colour{};
            colour.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colour.imageView = out.frontend.image_view;
            colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colour.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            VkRenderingInfo rendering{};
            rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            rendering.renderArea.extent = {496 * scale, 384 * scale};
            rendering.layerCount = 1;
            rendering.colorAttachmentCount = 1;
            rendering.pColorAttachments = &colour;
            vkCmdBeginRendering(cmd(), &rendering);
            for (std::size_t i = 0; i < geometry.count; ++i) {
                const auto& rect = geometry.rectangles[i];
                VkClearAttachment crosshair_attachment{};
                crosshair_attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                crosshair_attachment.colorAttachment = 0;
                crosshair_attachment.clearValue.color.float32[0] =
                    static_cast<float>((rect.colour >> 16) & 0xffu) / 255.0f;
                crosshair_attachment.clearValue.color.float32[1] =
                    static_cast<float>((rect.colour >> 8) & 0xffu) / 255.0f;
                crosshair_attachment.clearValue.color.float32[2] =
                    static_cast<float>(rect.colour & 0xffu) / 255.0f;
                crosshair_attachment.clearValue.color.float32[3] = 1.0f;
                VkClearRect clear{};
                clear.rect.offset = {rect.x, rect.y};
                clear.rect.extent = {static_cast<u32>(rect.width),
                                     static_cast<u32>(rect.height)};
                clear.layerCount = 1;
                vkCmdClearAttachments(cmd(), 1, &crosshair_attachment, 1, &clear);
            }
            vkCmdEndRendering(cmd());
        }
        render::vk::record_image_barrier(cmd(), out.image, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        check(vkEndCommandBuffer(cmd()), "End GPU frame");
        check(vkResetFences(device(), 1, &fences[slot]), "Reset frame fence");
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1; submit.pCommandBuffers = &commands[slot];
        { QueueLock lock(*vk); check(vkQueueSubmit(vk->queue, 1, &submit, fences[slot]), "Submit GPU frame"); }
        vk->set_image(vk->handle, &out.frontend, 0, nullptr, vk->queue_index);
        if (video) video(RETRO_HW_FRAME_BUFFER_VALID, 496 * scale, 384 * scale, 0);
        slot = (slot + 1) % kFramesInFlight;
    }
};
VulkanRenderer::VulkanRenderer() : impl(std::make_unique<Impl>()) {}
VulkanRenderer::~VulkanRenderer() = default;
void VulkanRenderer::init(retro_environment_t env, unsigned scale, retro_log_printf_t log) { impl->init(env, scale, log); }
void VulkanRenderer::render(hw::Model2MachineBase& machine, retro_video_refresh_t video,
                            const CrosshairState& crosshairs, unsigned texture_quality,
                            unsigned upscale_2d)
{
    impl->render(machine, video, crosshairs, texture_quality, upscale_2d);
}
}
