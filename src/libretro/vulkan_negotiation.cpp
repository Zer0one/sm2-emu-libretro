// SPDX-License-Identifier: BSD-3-Clause
#include "gpu.h"
#include "libretro_vulkan.h"
#include <vector>
#include <cstring>
#include <exception>
namespace sm2::libretro {
namespace {
retro_log_printf_t logger = nullptr;
VkDevice negotiated_device = VK_NULL_HANDLE;
const VkApplicationInfo* application_info()
{
    static const VkApplicationInfo info{VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr,
        "SM2-Emu Libretro", 1, nullptr, 0, VK_API_VERSION_1_3};
    return &info;
}
bool create_device(retro_vulkan_context* context, VkInstance instance,
                   VkPhysicalDevice gpu, VkSurfaceKHR surface,
                   PFN_vkGetInstanceProcAddr proc,
                   retro_vulkan_create_device_wrapper_t create, void* opaque)
{
    try {
#define INSTANCE(name) const auto name = reinterpret_cast<PFN_##name>(proc(instance, #name)); if (!name) return false
        INSTANCE(vkEnumeratePhysicalDevices);
        INSTANCE(vkGetPhysicalDeviceProperties);
        INSTANCE(vkGetPhysicalDeviceFeatures2);
        INSTANCE(vkGetPhysicalDeviceQueueFamilyProperties);
        INSTANCE(vkEnumerateDeviceExtensionProperties);
        INSTANCE(vkGetDeviceProcAddr);
#undef INSTANCE
        const auto present = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            proc(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
        if (surface && !present) return false;
        std::vector<VkPhysicalDevice> candidates;
        if (gpu) candidates.push_back(gpu);
        else {
            uint32_t count = 0;
            if (vkEnumeratePhysicalDevices(instance, &count, nullptr) != VK_SUCCESS) return false;
            candidates.resize(count);
            if (vkEnumeratePhysicalDevices(instance, &count, candidates.data()) != VK_SUCCESS) return false;
        }
        for (auto candidate : candidates) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
            if (properties.apiVersion < VK_API_VERSION_1_3) continue;
            VkPhysicalDeviceVulkan13Features available{};
            available.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            VkPhysicalDeviceFeatures2 features{};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &available;
            vkGetPhysicalDeviceFeatures2(candidate, &features);
            if (!available.dynamicRendering || !available.synchronization2 ||
                !available.shaderDemoteToHelperInvocation) continue;
            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
            std::vector<VkQueueFamilyProperties> queues(count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, queues.data());
            uint32_t graphics = UINT32_MAX, presentation = UINT32_MAX;
            for (uint32_t i = 0; i < count; ++i) {
                if (!queues[i].queueCount) continue;
                VkBool32 supported = VK_TRUE;
                if (surface && present(candidate, i, surface, &supported) != VK_SUCCESS) continue;
                if (supported && presentation == UINT32_MAX) presentation = i;
                if ((queues[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
                    (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
                    if (graphics == UINT32_MAX) graphics = i;
                    if (supported) { graphics = presentation = i; break; }
                }
            }
            if (graphics == UINT32_MAX || presentation == UINT32_MAX) continue;
            if (!surface) presentation = graphics;
            const float priority = 1.f;
            std::vector<VkDeviceQueueCreateInfo> queue_infos;
            for (auto index : {graphics, presentation}) {
                if (!queue_infos.empty() && index == graphics) continue;
                VkDeviceQueueCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                info.queueFamilyIndex = index; info.queueCount = 1; info.pQueuePriorities = &priority;
                queue_infos.push_back(info);
            }
            VkPhysicalDeviceVulkan13Features enabled{};
            enabled.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            enabled.dynamicRendering = VK_TRUE;
            enabled.synchronization2 = VK_TRUE;
            enabled.shaderDemoteToHelperInvocation = VK_TRUE;
            std::vector<const char*> extensions;
            if (vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count, nullptr) != VK_SUCCESS) continue;
            std::vector<VkExtensionProperties> props(count);
            if (vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count, props.data()) != VK_SUCCESS) continue;
            for (const auto& prop : props)
                if (std::strcmp(prop.extensionName, "VK_KHR_portability_subset") == 0)
                    extensions.push_back("VK_KHR_portability_subset");
            VkDeviceCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            info.pNext = &enabled;
            info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
            info.pQueueCreateInfos = queue_infos.data();
            info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            info.ppEnabledExtensionNames = extensions.data();
            const VkDevice device = create(candidate, opaque, &info);
            if (!device) continue;
            const auto get_queue = reinterpret_cast<PFN_vkGetDeviceQueue>(vkGetDeviceProcAddr(device, "vkGetDeviceQueue"));
            // vkGetDeviceQueue is mandatory on every successfully created device.
            negotiated_device = device;
            context->gpu = candidate; context->device = device;
            context->queue_family_index = graphics;
            context->presentation_queue_family_index = presentation;
            get_queue(device, graphics, 0, &context->queue);
            get_queue(device, presentation, 0, &context->presentation_queue);
            if (logger) logger(RETRO_LOG_INFO, "[SM2 GPU] Negotiated Vulkan 1.3: %s\n", properties.deviceName);
            return true;
        }
    } catch (const std::exception& e) {
        if (logger) logger(RETRO_LOG_ERROR, "[SM2 GPU] Device negotiation: %s\n", e.what());
    }
    if (logger) logger(RETRO_LOG_ERROR, "[SM2 GPU] Vulkan 1.3 with dynamic rendering, synchronization2 and shader demote is required.\n");
    return false;
}
}
bool validate_vulkan_device(VkDevice device) { return device && device == negotiated_device; }
bool request_vulkan(retro_environment_t env, retro_hw_context_reset_t reset,
                    retro_hw_context_reset_t destroy, retro_log_printf_t log)
{
    logger = log;
    negotiated_device = VK_NULL_HANDLE;
    retro_hw_render_context_negotiation_interface support{
        RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN, 2};
    if (!env(RETRO_ENVIRONMENT_GET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_SUPPORT, &support)
        || support.interface_version < 2) return false;
    static retro_hw_render_callback hw{};
    hw = {}; hw.context_type = RETRO_HW_CONTEXT_VULKAN;
    hw.context_reset = reset; hw.context_destroy = destroy;
    hw.version_major = 1; hw.version_minor = 3; hw.cache_context = false;
    if (!env(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw)) return false;
    static retro_hw_render_context_negotiation_interface_vulkan negotiation{};
    negotiation.interface_type = RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN;
    negotiation.interface_version = 2;
    negotiation.get_application_info = application_info;
    // RetroArch 1.21 gates even the v2 callback on a non-null v1 callback.
    // We require negotiation v2 above; reject an unexpected legacy invocation.
    negotiation.create_device = [](retro_vulkan_context*, VkInstance, VkPhysicalDevice,
        VkSurfaceKHR, PFN_vkGetInstanceProcAddr, const char**, unsigned, const char**,
        unsigned, const VkPhysicalDeviceFeatures*) { return false; };
    negotiation.destroy_device = [] { negotiated_device = VK_NULL_HANDLE; };
    negotiation.create_device2 = create_device;
    return env(RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE, &negotiation);
}
}
