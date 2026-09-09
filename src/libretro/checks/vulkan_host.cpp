// SPDX-License-Identifier: BSD-3-Clause
// Minimal Vulkan frontend for ABI/lifetime and native-image comparisons.
#include "libretro_vulkan.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdio>
namespace {
retro_hw_render_callback hw{};
retro_hw_render_context_negotiation_interface_vulkan negotiation{};
retro_hw_render_interface_vulkan iface{};
retro_vulkan_image image{};
VkCommandPool pool = VK_NULL_HANDLE;
VkCommandBuffer command = VK_NULL_HANDLE;
std::vector<unsigned char> pixels;
std::string error;
unsigned frame = 0, mask = 7;
void check(VkResult result) { if (result != VK_SUCCESS) throw std::runtime_error("Vulkan result " + std::to_string(result)); }
void idle(void*) { check(vkDeviceWaitIdle(iface.device)); }
void set_image(void*,const retro_vulkan_image* value,uint32_t count,const VkSemaphore*,uint32_t family) {
    if (count || family != iface.queue_index) throw std::runtime_error("Unexpected queue/semaphore contract");
    image = *value;
}
uint32_t sync_index(void*) { return frame % (mask == 7 ? 3 : 4); }
uint32_t sync_mask(void*) { return mask; }
void lock(void*) {} // This harness has no other submitting threads.
void unlock(void*) {}
VkDevice create_device(VkPhysicalDevice gpu,void*,const VkDeviceCreateInfo* info) {
    VkDevice device = VK_NULL_HANDLE;
    return vkCreateDevice(gpu,info,nullptr,&device)==VK_SUCCESS ? device : VK_NULL_HANDLE;
}
void stop() {
    if (iface.device) {
        vkDeviceWaitIdle(iface.device);
        hw.context_destroy();
        if (pool) vkDestroyCommandPool(iface.device,pool,nullptr);
        vkDestroyDevice(iface.device,nullptr);
        if (negotiation.destroy_device) negotiation.destroy_device();
    }
    if (iface.instance) vkDestroyInstance(iface.instance,nullptr);
    iface={};pool=VK_NULL_HANDLE;command=VK_NULL_HANDLE;image={};pixels.clear();frame=0;mask=7;
}
}
extern "C" {
bool sm2_gpu_environment(unsigned cmd,void* data) {
    switch(cmd) {
    case RETRO_ENVIRONMENT_GET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_SUPPORT:
        static_cast<retro_hw_render_context_negotiation_interface*>(data)->interface_version=2;return true;
    case RETRO_ENVIRONMENT_SET_HW_RENDER:
        hw=*static_cast<retro_hw_render_callback*>(data);return hw.context_type==RETRO_HW_CONTEXT_VULKAN;
    case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE:
        negotiation=*static_cast<retro_hw_render_context_negotiation_interface_vulkan*>(data);return negotiation.interface_version==2;
    case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
        *static_cast<const retro_hw_render_interface_vulkan**>(data)=&iface;return iface.device!=VK_NULL_HANDLE;
    default:return false;
    }
}
const char* sm2_gpu_error() {return error.c_str();}
bool sm2_gpu_start() {
    try {
        uint32_t count=0;check(vkEnumerateInstanceExtensionProperties(nullptr,&count,nullptr));
        std::vector<VkExtensionProperties> available(count);check(vkEnumerateInstanceExtensionProperties(nullptr,&count,available.data()));
        std::vector<const char*> extensions;
        for(const auto& e:available) if(std::strcmp(e.extensionName,VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)==0)
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        VkInstanceCreateInfo info{};info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo=negotiation.get_application_info();
        info.enabledExtensionCount=static_cast<uint32_t>(extensions.size());info.ppEnabledExtensionNames=extensions.data();
        if(!extensions.empty())info.flags=VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        check(vkCreateInstance(&info,nullptr,&iface.instance));
        retro_vulkan_context ctx{};
        if(!negotiation.create_device2(&ctx,iface.instance,VK_NULL_HANDLE,VK_NULL_HANDLE,vkGetInstanceProcAddr,create_device,nullptr))
            throw std::runtime_error("Core rejected device negotiation");
        iface.interface_type=RETRO_HW_RENDER_INTERFACE_VULKAN;iface.interface_version=5;
        iface.gpu=ctx.gpu;iface.device=ctx.device;iface.queue=ctx.queue;iface.queue_index=ctx.queue_family_index;
        iface.get_device_proc_addr=vkGetDeviceProcAddr;iface.get_instance_proc_addr=vkGetInstanceProcAddr;
        iface.set_image=set_image;iface.get_sync_index=sync_index;iface.get_sync_index_mask=sync_mask;
        iface.wait_sync_index=idle;iface.lock_queue=lock;iface.unlock_queue=unlock;
        VkCommandPoolCreateInfo cp{};cp.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cp.queueFamilyIndex=iface.queue_index;cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        check(vkCreateCommandPool(iface.device,&cp,nullptr,&pool));
        VkCommandBufferAllocateInfo ca{};ca.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
        check(vkAllocateCommandBuffers(iface.device,&ca,&command));
        hw.context_reset();return true;
    } catch(const std::exception& e){error=e.what();return false;}
}
void sm2_gpu_stop(){stop();}
void sm2_gpu_set_mask(unsigned value){if(value==7||value==15){idle(nullptr);mask=value;}}
// Consume one image. Read back only when requested; all resource lifetimes are
// still retired each frame. Real RetroArch separately tests asynchronous use.
bool sm2_gpu_frame(unsigned width,unsigned height,bool capture) {
    VkBuffer buffer=VK_NULL_HANDLE;VkDeviceMemory memory=VK_NULL_HANDLE;
    try {
        if(!image.image_view||!image.create_info.image||image.image_layout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            throw std::runtime_error("No valid GPU output image");
        idle(nullptr);
        if(capture) {
            const VkDeviceSize bytes=static_cast<VkDeviceSize>(width)*height*4;
            VkBufferCreateInfo bc{};bc.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bc.size=bytes;bc.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            check(vkCreateBuffer(iface.device,&bc,nullptr,&buffer));
            VkMemoryRequirements req{};vkGetBufferMemoryRequirements(iface.device,buffer,&req);
            VkPhysicalDeviceMemoryProperties props{};vkGetPhysicalDeviceMemoryProperties(iface.gpu,&props);
            uint32_t type=UINT32_MAX;
            for(uint32_t i=0;i<props.memoryTypeCount;++i)
                if((req.memoryTypeBits&(1u<<i))&&(props.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)){type=i;break;}
            if(type==UINT32_MAX)throw std::runtime_error("No readback memory");
            VkMemoryAllocateInfo ma{};ma.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ma.allocationSize=req.size;ma.memoryTypeIndex=type;
            check(vkAllocateMemory(iface.device,&ma,nullptr,&memory));check(vkBindBufferMemory(iface.device,buffer,memory,0));
            check(vkResetCommandBuffer(command,0));
            VkCommandBufferBeginInfo begin{};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            check(vkBeginCommandBuffer(command,&begin));
            VkImageMemoryBarrier barrier{};barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image=image.create_info.image;barrier.subresourceRange=image.create_info.subresourceRange;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.oldLayout=image.image_layout;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy region{};region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.imageExtent={width,height,1};
            vkCmdCopyImageToBuffer(command,barrier.image,barrier.newLayout,buffer,1,&region);
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=image.image_layout;
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            check(vkEndCommandBuffer(command));
            VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&command;
            check(vkQueueSubmit(iface.queue,1,&submit,VK_NULL_HANDLE));idle(nullptr);
            void* mapped=nullptr;check(vkMapMemory(iface.device,memory,0,VK_WHOLE_SIZE,0,&mapped));
            VkMappedMemoryRange range{};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;range.memory=memory;range.size=VK_WHOLE_SIZE;
            check(vkInvalidateMappedMemoryRanges(iface.device,1,&range));
            pixels.assign(static_cast<unsigned char*>(mapped),static_cast<unsigned char*>(mapped)+bytes);
            vkUnmapMemory(iface.device,memory);
        }
        if(buffer)vkDestroyBuffer(iface.device,buffer,nullptr);if(memory)vkFreeMemory(iface.device,memory,nullptr);
        ++frame;return true;
    }catch(const std::exception& e){error=e.what();if(buffer)vkDestroyBuffer(iface.device,buffer,nullptr);if(memory)vkFreeMemory(iface.device,memory,nullptr);return false;}
}
const void* sm2_gpu_pixels(){return pixels.data();}
}
