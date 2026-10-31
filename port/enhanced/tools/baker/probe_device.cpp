// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Uses local Vulkan dispatch so detection cannot replace a renderer's live function pointers.
#include "enhanced/tools/baker/probe_device.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <vector>

#include "common/env.h"
#include "platform/gpu_driver.h"
#if OPENRAC_VULKAN
#include <SDL3/SDL_loadso.h>
#if defined(__APPLE__)
#include <SDL3/SDL_filesystem.h>
#endif

#include <vulkan/vulkan.h>
#endif

namespace openrac::lighting {
#if OPENRAC_VULKAN
namespace {
// Owns loader and instance in destruction order; no graphics device or window is created.
struct DeviceQuery {
    SDL_SharedObject* library =
        nullptr;  // Owned system loader, absent for a custom Android driver.
    PFN_vkGetInstanceProcAddr entry = nullptr;  // Loader dispatch, valid while library is owned.
    VkInstance instance = VK_NULL_HANDLE;       // Owned headless query instance.
    PFN_vkDestroyInstance destroy = nullptr;    // Instance-local destructor.

    DeviceQuery() = default;
    DeviceQuery(const DeviceQuery&) = delete;
    DeviceQuery& operator=(const DeviceQuery&) = delete;

    // The instance must be destroyed before its loader is unloaded.
    ~DeviceQuery() {
        if (instance && destroy) {
            destroy(instance, nullptr);
        }

        if (library) {
            SDL_UnloadObject(library);
        }
    }

    // Resolves instance functions without modifying volk's renderer dispatch.
    template <class Function>
    Function function(const char* name) const {
        return reinterpret_cast<Function>(entry(instance, name));
    }
};

// Keep loader search independent of window creation, including bundled MoltenVK on macOS.
std::vector<std::string> loader_paths() {
    const auto chosen = env::value(env::Var::VulkanLibrary);

    // An explicit loader path must select the same driver for the baker and runtime.
    if (chosen && !chosen->empty()) {
        return {std::string(*chosen)};
    }
#if defined(_WIN32)
    return {"vulkan-1.dll"};
#elif defined(__ANDROID__)
    return {"libvulkan.so"};
#elif defined(__APPLE__)
    std::vector<std::string> paths;

    // SDL's application base is adjacent to Contents/Frameworks for a bundled executable.
    if (const auto* base = SDL_GetBasePath()) {
        paths.push_back((std::filesystem::path(base) / "../Frameworks/libMoltenVK.dylib").string());
    }

    const auto sdk = env::system_variable("VULKAN_SDK");

    // SDK loaders carry their configured driver discovery paths.
    if (sdk && !sdk->empty()) {
        paths.push_back(*sdk + "/lib/libvulkan.1.dylib");
    }

    for (const auto* folder : {"/opt/homebrew/lib/", "/usr/local/lib/", ""}) {
        paths.push_back(std::string(folder) + "libvulkan.1.dylib");
        paths.push_back(std::string(folder) + "libMoltenVK.dylib");
    }

    return paths;
#else
    return {"libvulkan.so.1"};
#endif
}

// Custom Android drivers use their existing owned entry point; system loaders remain locally owned.
bool open_loader(DeviceQuery& query) {
    platform::prepare_gpu_driver();
    query.entry = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        platform::custom_vulkan_get_instance_proc_addr()
    );

    // The platform already owns a configured custom driver.
    if (query.entry) {
        return true;
    }

    for (const auto& path : loader_paths()) {
        query.library = SDL_LoadObject(path.c_str());

        // Try the next platform loader when a candidate is not installed.
        if (!query.library) {
            continue;
        }

        query.entry = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            SDL_LoadFunction(query.library, "vkGetInstanceProcAddr")
        );

        // Only a Vulkan loader can create the headless query instance.
        if (query.entry) {
            return true;
        }

        SDL_UnloadObject(query.library);
        query.library = nullptr;
    }

    return false;
}

// Match the runtime's discrete, integrated, other and software adapter preference.
int adapter_priority(VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return 4;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 3;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 1;
        default:
            return 2;
    }
}

// Headless queries cannot test presentation, but require the runtime's Vulkan version and graphics support.
int device_score(const DeviceQuery& query, VkPhysicalDevice device) {
    const auto properties =
        query.function<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties");
    const auto features =
        query.function<PFN_vkGetPhysicalDeviceFeatures2>("vkGetPhysicalDeviceFeatures2");
    const auto queues = query.function<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        "vkGetPhysicalDeviceQueueFamilyProperties"
    );

    // Incomplete loaders cannot make a useful format choice.
    if (!properties || !features || !queues) {
        return 0;
    }

    VkPhysicalDeviceProperties info{};
    properties(device, &info);
    VkPhysicalDeviceVulkan13Features supported{};
    supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceFeatures2 capabilities{};
    capabilities.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    capabilities.pNext = &supported;
    features(device, &capabilities);

    // Reject adapters that the enhanced renderer cannot use.
    if (info.apiVersion < VK_API_VERSION_1_3 || !supported.dynamicRendering) {
        return 0;
    }

    std::uint32_t families = 0;
    queues(device, &families, nullptr);
    std::vector<VkQueueFamilyProperties> queue_info(families);
    queues(device, &families, queue_info.data());
    const bool graphics = std::any_of(queue_info.begin(), queue_info.end(), [](const auto& family) {
        return (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    });
    return graphics ? adapter_priority(info.deviceType) : 0;
}

// Discovery uses only instance-local dispatch and never changes a live renderer's Vulkan pointers.
VkPhysicalDevice choose_device(const DeviceQuery& query) {
    const auto enumerate =
        query.function<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices");
    std::uint32_t count = 0;

    // A missing loader entry or failed enumeration leaves the portable raw format selected.
    if (!enumerate || enumerate(query.instance, &count, nullptr) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> devices(count);

    // VK_INCOMPLETE is also a safe fallback when the device inventory changes during discovery.
    if (enumerate(query.instance, &count, devices.data()) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkPhysicalDevice selected = VK_NULL_HANDLE;
    int best = 0;

    for (auto device : devices) {
        const int score = device_score(query, device);

        // Stable enumeration order resolves equal-priority adapters, like the runtime.
        if (score > best) {
            best = score;
            selected = device;
        }
    }

    return selected;
}

// MoltenVK adapters are enumerated only when the application explicitly accepts portability.
bool portability_supported(const DeviceQuery& query) {
    const auto enumerate = query.function<PFN_vkEnumerateInstanceExtensionProperties>(
        "vkEnumerateInstanceExtensionProperties"
    );
    std::uint32_t count = 0;

    // Ordinary loaders need no portability flag when this query is unavailable.
    if (!enumerate || enumerate(nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> offered(count);

    // A changing extension list is treated as unavailable for this best-effort query.
    if (enumerate(nullptr, &count, offered.data()) != VK_SUCCESS) {
        return false;
    }

    return std::any_of(offered.begin(), offered.end(), [](const auto& extension) {
        return std::strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
               == 0;
    });
}

// Filters compression features by sampled-image and bilinear filtering support.
ProbeTextureFormat device_format(const DeviceQuery& query, VkPhysicalDevice device) {
    const auto features =
        query.function<PFN_vkGetPhysicalDeviceFeatures2>("vkGetPhysicalDeviceFeatures2");
    const auto formats = query.function<PFN_vkGetPhysicalDeviceFormatProperties>(
        "vkGetPhysicalDeviceFormatProperties"
    );

    // Missing dispatch must fall back to the uncompressed format.
    if (!features || !formats) {
        return ProbeTextureFormat::R11G11B10;
    }

    VkPhysicalDeviceVulkan13Features hdr{};
    hdr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceFeatures2 supported{};
    supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supported.pNext = &hdr;
    features(device, &supported);
    const auto usable = [&](VkFormat format) {
        VkFormatProperties properties{};
        formats(device, format, &properties);
        constexpr auto kRequired =
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        return (properties.optimalTilingFeatures & kRequired) == kRequired;
    };

    if (supported.features.textureCompressionBC && usable(VK_FORMAT_BC6H_UFLOAT_BLOCK)) {
        return ProbeTextureFormat::Bc6h;
    }

    if (hdr.textureCompressionASTC_HDR && usable(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK)) {
        return ProbeTextureFormat::Astc8x8;
    }

    return ProbeTextureFormat::R11G11B10;
}
}  // namespace
#endif

ProbeTextureFormat detect_probe_texture_format() {
#if OPENRAC_VULKAN
    DeviceQuery query;

    if (!open_loader(query)) {
        return ProbeTextureFormat::R11G11B10;
    }

    const auto create = query.function<PFN_vkCreateInstance>("vkCreateInstance");
    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &application;
    const char* portability = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

    // Portable drivers must be visible to the same device selector as native Vulkan drivers.
    if (portability_supported(query)) {
        info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        info.enabledExtensionCount = 1;
        info.ppEnabledExtensionNames = &portability;
    }

    if (!create || create(&info, nullptr, &query.instance) != VK_SUCCESS) {
        return ProbeTextureFormat::R11G11B10;
    }

    query.destroy = query.function<PFN_vkDestroyInstance>("vkDestroyInstance");
    const auto device = choose_device(query);

    if (device) {
        return device_format(query, device);
    }
#endif
    return ProbeTextureFormat::R11G11B10;
}
}  // namespace openrac::lighting
