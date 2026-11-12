// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Setting the device up and taking it down (vkgl_internal.h): the instance and the validation
// layer, the physical device, the logical device and its queue, the pipeline cache on disk, the
// frames' objects, the dummy resources, the window's surface, and destroy_all.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <volk.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "common/breadcrumbs.h"
#include "common/cache_folder.h"
#include "common/crash.h"
#include "common/env.h"
#include "common/log.h"
#include "common/profiler.h"
#include "renderer/render_opt.h"
#include "renderer/vulkan/pipeline_cache_file.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {

using namespace openrac::gl;

namespace {

// ---- The device, the screen and the frames ----

std::string version_text(std::uint32_t v) {
    return std::format(
        "{}.{}.{}", VK_API_VERSION_MAJOR(v), VK_API_VERSION_MINOR(v), VK_API_VERSION_PATCH(v)
    );
}

VKAPI_ATTR VkBool32 VKAPI_CALL on_message(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*
) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        if (g) {
            ++g->validation_errors;
        }
        const auto* id = data->pMessageIdName ? data->pMessageIdName : "validation error";
        crash::failure_breadcrumb(id);
        const auto message = std::format("Vulkan validation [{}]: {}", id, data->pMessage);
        crash::note_vulkan(message);
        log::error("{}", message);
    } else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        crash::note_vulkan(data->pMessage);
        log::warn("vulkan: {}", data->pMessage);
    }
    return VK_FALSE;
}

std::filesystem::path pipeline_cache_file() {
    if (g == nullptr) {
        return {};
    }
    // 0, off or none keep no cache; the empty string is as if the variable were unset.
    if (const std::optional<std::string_view> chosen = env::value(env::Var::VulkanPipelineCache)) {
        if (*chosen == "0" || *chosen == "off" || *chosen == "none") {
            return {};
        }
        if (!chosen->empty()) {
            return std::filesystem::path(*chosen);
        }
    }
    const std::filesystem::path base = cache_folder();
    return base
           / std::format(
               "vulkan_pipelines_{:04x}_{:04x}_{}.bin",
               g->properties.vendorID,
               g->properties.deviceID,
               OPENRAC_VK_PIPELINE_KEY
           );
}

// Caches of this device that other builds saved (another key, or none): no longer read.
void remove_other_pipeline_caches(const std::filesystem::path& path) {
    const std::string prefix = std::format(
        "vulkan_pipelines_{:04x}_{:04x}", g->properties.vendorID, g->properties.deviceID
    );
    if (!path.filename().string().starts_with(prefix)) {
        return;  // OPENRAC_VULKAN_PIPELINE_CACHE names a file of its own: leave its folder alone
    }
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path(), ec)) {
        const std::string name = entry.path().filename().string();
        const bool cache_file = name.ends_with(".bin") || name.ends_with(".bin.manifest")
                                || name.ends_with(".bin.spirv");
        const bool ours = entry.path() == path
                          || entry.path().string() == path.string() + ".manifest"
                          || entry.path().string() == path.string() + ".spirv";

        // Another build's cache, or the manifest or SPIR-V beside it; this build's stay.
        if (!ours && name.starts_with(prefix) && cache_file) {
            std::filesystem::remove(entry.path(), ec);
        }
    }
}

bool make_pipeline_cache() {
    std::vector<char> initial_data;
    const std::filesystem::path path = pipeline_cache_file();
    if (!path.empty()) {
        SavedCache saved = read_pipeline_cache(path);
        // A save cut short or mixed from two programs is dropped; this run's save replaces it.
        if (saved.state == SavedCacheState::Damaged) {
            log::warn(
                "vulkan: pipeline cache {} is damaged, starting a fresh cache", path.string()
            );
        }
        if (saved.data.size() >= sizeof(VkPipelineCacheHeaderVersionOne)) {
            initial_data = std::move(saved.data);
        }
    }
    bool use_cache = false;
    if (!initial_data.empty()) {
        VkPipelineCacheHeaderVersionOne header{};
        std::memcpy(&header, initial_data.data(), sizeof(header));
        if (header.headerSize == sizeof(VkPipelineCacheHeaderVersionOne)
            && header.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE
            && header.vendorID == g->properties.vendorID
            && header.deviceID == g->properties.deviceID
            && std::memcmp(header.pipelineCacheUUID, g->properties.pipelineCacheUUID, VK_UUID_SIZE)
                   == 0) {
            use_cache = true;
        } else {
            log::info("vulkan: pipeline cache UUID changed, starting a fresh cache");
        }
    }
    VkPipelineCacheCreateInfo cache{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    if (use_cache) {
        cache.initialDataSize = initial_data.size();
        cache.pInitialData = initial_data.data();
    }
    if (!check(
            vkCreatePipelineCache(g->device, &cache, nullptr, &g->pipeline_cache),
            "the pipeline cache"
        )) {
        if (use_cache) {
            log::warn("vulkan: driver refused cached pipelines, starting a fresh cache");
            cache.initialDataSize = 0;
            cache.pInitialData = nullptr;
            if (!check(
                    vkCreatePipelineCache(g->device, &cache, nullptr, &g->pipeline_cache),
                    "the pipeline cache"
                )) {
                return false;
            }
        } else {
            return false;
        }
    } else if (use_cache) {
        log::info(
            "vulkan: loaded {} bytes of cached pipelines from {}",
            initial_data.size(),
            path.string()
        );
    }
    // The keys of the pipelines earlier runs made, for the pre-warm (R-10).
    load_pipeline_manifest(path);
    // What glslang made in earlier runs (R-12), beside the cache; none with the cache off.
    std::filesystem::path spirv = path;
    spirv += path.empty() ? "" : ".spirv";

    // A damaged file, or one of another compiler, is replaced at close.
    if (g->spirv_cache.open(spirv, spirv_compiler_version()) == CacheFileState::Damaged) {
        log::info(
            "vulkan: SPIR-V cache {} is damaged or stale, starting a fresh one", spirv.string()
        );
    }

    return true;
}

// Saves the pipeline cache's data to `path`; quiet saves (the ones made while the game runs) log
// only a failure. Any thread: the cache is internally synchronised. False when nothing was saved.
bool write_cache_data(const std::filesystem::path& path, bool quiet) {
    std::size_t size = 0;
    if (vkGetPipelineCacheData(g->device, g->pipeline_cache, &size, nullptr) != VK_SUCCESS
        || size <= sizeof(VkPipelineCacheHeaderVersionOne)) {
        return false;
    }
    std::vector<char> buffer(size);
    if (vkGetPipelineCacheData(g->device, g->pipeline_cache, &size, buffer.data()) != VK_SUCCESS) {
        return false;
    }
    buffer.resize(size);
    std::string error;
    // Other programs may save the same file now: each saves whole, through a file of its own.
    if (!write_pipeline_cache(path, buffer, error)) {
        log::warn("vulkan: could not save the pipeline cache: {}", error);
        return false;
    }
    if (!quiet) {
        log::info("vulkan: saved {} bytes of pipeline cache to {}", size, path.string());
    }
    return true;
}

void save_pipeline_cache() {
    if (g == nullptr || g->device == VK_NULL_HANDLE || g->pipeline_cache == VK_NULL_HANDLE
        || !g->open) {
        return;
    }
    const std::filesystem::path path = pipeline_cache_file();
    if (path.empty()) {
        return;
    }
    if (write_cache_data(path, false)) {
        remove_other_pipeline_caches(path);
    }
}

}  // namespace

void save_pipeline_caches_soon(bool spirv) {
    // Closed, no cache kept, or nothing new since the last save.
    if (g == nullptr || !g->open || g->pipeline_cache == VK_NULL_HANDLE
        || g->manifest.added == g->saved_keys) {
        return;
    }

    const std::filesystem::path path = pipeline_cache_file();

    // No cache file, or a save still writing: the next call saves.
    if (path.empty() || g->cache_saving.load()) {
        return;
    }

    g->saved_keys = g->manifest.added;
    g->saved_at = Clock::now();

    // What glslang made is saved on this thread, where it is compiled: only at a level's load,
    // when the level's programs have just been compiled.
    if (spirv) {
        std::string error;
        if (!g->spirv_cache.save(error)) {
            log::warn("vulkan: could not save the SPIR-V cache: {}", error);
        }
    }

    // The manifest as it is now, and the driver's cache as it is when the thread reads it.
    g->cache_saver.join();
    g->cache_saving = true;
    g->cache_saver =
        JoiningThread([path, entries = manifest_entries(), manifest = g->manifest.path] {
            profiling::thread_name("Pipeline cache save");
            write_manifest(manifest, entries);
            write_cache_data(path, true);
            g->cache_saving = false;
        });
}

// ---- The device, the screen and the frames ----

bool layer_available(const char* name) {
    std::uint32_t n = 0;
    vkEnumerateInstanceLayerProperties(&n, nullptr);
    std::vector<VkLayerProperties> layers(n);
    vkEnumerateInstanceLayerProperties(&n, layers.data());
    for (const VkLayerProperties& l : layers) {
        if (std::strcmp(l.layerName, name) == 0) {
            return true;
        }
    }
    return false;
}

bool make_instance(const Config& config, bool validation, std::string& error) {
    g->allow_present_id = config.present_id;
    g->allow_gpu_breadcrumbs = env::value(env::Var::GpuBreadcrumbs)
                                   ? env::flag(env::Var::GpuBreadcrumbs)
                                   : config.gpu_breadcrumbs;
    g->allow_present_wait = config.present_wait;
    g->allow_latest_ready = config.fifo_latest_ready;
    std::vector<const char*> extensions;
#if defined(__ANDROID__)
    // Asked of the driver itself (a player's own driver is not the one SDL would load).
    if (config.window != nullptr) {
        extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    }
#else
    if (config.window != nullptr) {
        Uint32 n = 0;
        const char* const* names = SDL_Vulkan_GetInstanceExtensions(&n);
        if (names == nullptr) {
            error = std::format("SDL has no Vulkan surface here: {}", SDL_GetError());
            return false;
        }
        extensions.assign(names, names + n);
    }
#endif
    std::vector<const char*> layers;
    bool debug_utils = validation;
    if (validation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
#if defined(OPENRAC_GPU_MARKERS)
    debug_utils |= env::flag(env::Var::GpuMarkers);
#endif
    if (debug_utils) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    // A driver that carries out only part of Vulkan (MoltenVK, on Metal) is listed by the loader
    // only to an instance that asks for these.
    VkInstanceCreateFlags flags = 0;
    {
        std::uint32_t n = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr);
        std::vector<VkExtensionProperties> offered(n);
        vkEnumerateInstanceExtensionProperties(nullptr, &n, offered.data());
        for (const VkExtensionProperties& e : offered) {
            if (std::strcmp(e.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
                extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            }
            // HDR output's colour spaces, only where the player may ask for it (Config): without
            // it the surface offers what it always did.
            if (config.hdr_colour_spaces && config.window != nullptr
                && std::strcmp(e.extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0) {
                extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
                g->hdr.colour_spaces = true;
            }
            // What VK_EXT_full_screen_exclusive needs (vkgl_device below, vkgl_present).
            if (config.window != nullptr
                && std::strcmp(e.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0) {
                if (std::none_of(extensions.begin(), extensions.end(), [](const char* name) {
                        return std::strcmp(name, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0;
                    })) {
                    extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
                g->surface_capabilities2 = true;
            }
        }
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "OpenRAC";
    app.pEngineName = "OpenRAC";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.flags = flags;
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.data();
    g->setup = config.setup;
    VkResult r = VK_SUCCESS;

    // An outside owner adds what it needs to the instance (OpenXR: xrCreateVulkanInstanceKHR).
    if (g->setup.create_instance != nullptr) {
        void* made = nullptr;
        r = static_cast<VkResult>(g->setup.create_instance(
            g->setup.user, reinterpret_cast<void*>(vkGetInstanceProcAddr), &info, &made
        ));
        g->instance = static_cast<VkInstance>(made);
    } else {
        r = vkCreateInstance(&info, nullptr, &g->instance);
    }

    if (r != VK_SUCCESS || g->instance == VK_NULL_HANDLE) {
        error = std::format("no Vulkan instance ({})", static_cast<int>(r));
        return false;
    }
    volkLoadInstance(g->instance);
    if (validation) {
        VkDebugUtilsMessengerCreateInfoEXT m{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
        };
        m.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        m.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        m.pfnUserCallback = on_message;
        vkCreateDebugUtilsMessengerEXT(g->instance, &m, nullptr, &g->messenger);
    }
    return true;
}

bool pick_device(std::string& error) {
    std::uint32_t n = 0;
    vkEnumeratePhysicalDevices(g->instance, &n, nullptr);
    std::vector<VkPhysicalDevice> devices(n);
    vkEnumeratePhysicalDevices(g->instance, &n, devices.data());

    // An outside owner names the device (OpenXR's, where the headset is): only it is considered.
    if (g->setup.physical_device != nullptr) {
        if (const auto chosen = static_cast<VkPhysicalDevice>(
                g->setup.physical_device(g->setup.user, g->instance)
            )) {
            devices.assign(1, chosen);
        }
    }

    int best = -1;
    for (VkPhysicalDevice d : devices) {
        VkPhysicalDeviceProperties p{};
        vkGetPhysicalDeviceProperties(d, &p);
        if (p.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }
        VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        f2.pNext = &f13;
        vkGetPhysicalDeviceFeatures2(d, &f2);
        if (!f13.dynamicRendering) {
            continue;
        }
        std::uint32_t qn = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qn, nullptr);
        std::vector<VkQueueFamilyProperties> queues(qn);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qn, queues.data());
        for (std::uint32_t i = 0; i < qn; ++i) {
            if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                continue;
            }
            VkBool32 present = VK_TRUE;
            if (g->surface != VK_NULL_HANDLE) {
                vkGetPhysicalDeviceSurfaceSupportKHR(d, i, g->surface, &present);
            }
            if (!present) {
                continue;
            }
            const int score = p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU     ? 4
                              : p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 3
                              : p.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU            ? 1
                                                                                       : 2;
            if (score > best) {
                best = score;
                g->physical = d;
                g->family = i;
            }
            break;
        }
    }
    if (g->physical == VK_NULL_HANDLE) {
        error = "no Vulkan 1.3 device with dynamic rendering that can show the window";
        return false;
    }
    vkGetPhysicalDeviceProperties(g->physical, &g->properties);
    std::uint32_t families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g->physical, &families, nullptr);
    std::vector<VkQueueFamilyProperties> queues(families);
    vkGetPhysicalDeviceQueueFamilyProperties(g->physical, &families, queues.data());
    g->timestamp_bits = queues.at(g->family).timestampValidBits;

    // Whether the debug tools' GPU timers can work on this queue (the vk_gpu_timestamps gauge).
    if (g->timestamp_bits != 0) {
        log::info(
            "vulkan: gpu timestamps: yes ({} valid bits, {} ns a tick)",
            g->timestamp_bits,
            g->properties.limits.timestampPeriod
        );
    } else {
        log::info("vulkan: gpu timestamps: no (the queue keeps no timestamp bits)");
    }

    vkGetPhysicalDeviceFeatures(g->physical, &g->features);
    return true;
}

namespace {

// Asks for the presentation extensions (R-09) the device offers and Config allows, adding their
// names to `extensions` and setting their features: VK_KHR_present_id, VK_KHR_present_wait (only
// with present ids), and the FIFO latest-ready present mode (the KHR name or the older EXT one).
// Only with a window, which they need. Returns the head of the feature chain: those asked for,
// linked present_id to present_wait to latest_ready, then `next`; notes in `g` which.
void* present_features(
    std::vector<const char*>& extensions,
    VkPhysicalDevicePresentIdFeaturesKHR& present_id,
    VkPhysicalDevicePresentWaitFeaturesKHR& present_wait,
    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesKHR& latest_ready,
    void* next
) {
    g->present_id_enabled = false;
    g->present_wait_enabled = false;
    g->latest_ready_enabled = false;

    // Without a surface there is nothing presented.
    if (g->surface == VK_NULL_HANDLE) {
        return next;
    }

    std::uint32_t n = 0;
    vkEnumerateDeviceExtensionProperties(g->physical, nullptr, &n, nullptr);
    std::vector<VkExtensionProperties> offered(n);
    vkEnumerateDeviceExtensionProperties(g->physical, nullptr, &n, offered.data());
    const auto has = [&offered](const char* name) {
        return std::ranges::any_of(offered, [name](const VkExtensionProperties& e) {
            return std::strcmp(e.extensionName, name) == 0;
        });
    };
    const char* latest_name = has(VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME)
                                  ? VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME
                                  : VK_EXT_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME;

    // What the device says of each feature.
    present_id.pNext = &present_wait;
    present_wait.pNext = &latest_ready;
    VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    query.pNext = &present_id;
    vkGetPhysicalDeviceFeatures2(g->physical, &query);
    const bool id = g->allow_present_id && has(VK_KHR_PRESENT_ID_EXTENSION_NAME)
                    && present_id.presentId == VK_TRUE;
    const bool wait = id && g->allow_present_wait && has(VK_KHR_PRESENT_WAIT_EXTENSION_NAME)
                      && present_wait.presentWait == VK_TRUE;
    const bool latest = g->allow_latest_ready && has(latest_name)
                        && latest_ready.presentModeFifoLatestReady == VK_TRUE;
    present_id = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR};
    present_wait = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR};
    latest_ready = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_KHR};
    void* head = nullptr;
    void** tail = &head;

    // Present ids: each present numbered, for the waits.
    if (id) {
        extensions.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
        present_id.presentId = VK_TRUE;
        *tail = &present_id;
        tail = &present_id.pNext;
        g->present_id_enabled = true;
    }

    // Waits on a present id, which say when that present reached the screen.
    if (wait) {
        extensions.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
        present_wait.presentWait = VK_TRUE;
        *tail = &present_wait;
        tail = &present_wait.pNext;
        g->present_wait_enabled = true;
    }

    // The present mode that shows the latest frame at each refresh without tearing.
    if (latest) {
        extensions.push_back(latest_name);
        latest_ready.presentModeFifoLatestReady = VK_TRUE;
        *tail = &latest_ready;
        tail = &latest_ready.pNext;
        g->latest_ready_enabled = true;
    }

    *tail = next;

    log::info(
        "vulkan: present ids {}, present waits {}, latest-ready mode {}",
        id ? "yes" : "no",
        wait ? "yes" : "no",
        latest ? "yes" : "no"
    );
    return head;
}

}  // namespace

bool make_device(std::string& error) {
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue.queueFamilyIndex = g->family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures features{};
    features.fillModeNonSolid = g->features.fillModeNonSolid;
    features.samplerAnisotropy = g->features.samplerAnisotropy;
    features.depthClamp = g->features.depthClamp;  // GL_DEPTH_CLAMP (FrameInput::depth_clamp)
    features.shaderClipDistance = g->features.shaderClipDistance;  // the world's clip planes
    features.shaderCullDistance = g->features.shaderCullDistance;
    features.drawIndirectFirstInstance = g->features.drawIndirectFirstInstance;
    features.multiDrawIndirect = g->features.multiDrawIndirect;
    features.multiViewport = g->features.multiViewport;
    features.textureCompressionBC = g->features.textureCompressionBC;
    features.textureCompressionASTC_LDR = g->features.textureCompressionASTC_LDR;
#if OPENRAC_ENHANCED_RENDERING
    // Native enhanced compute providers need these optional storage-image operations.
    features.shaderStorageImageExtendedFormats = g->features.shaderStorageImageExtendedFormats;
    features.shaderStorageImageWriteWithoutFormat =
        g->features.shaderStorageImageWriteWithoutFormat;
    features.shaderStorageImageReadWithoutFormat = g->features.shaderStorageImageReadWithoutFormat;
    // Arm ASR's fragment passes also write storage images.
    features.fragmentStoresAndAtomics = g->features.fragmentStoresAndAtomics;
#endif
    g->features = features;
    VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    f13.dynamicRendering = VK_TRUE;
    VkPhysicalDeviceVulkan11Features f11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceFeatures2 available{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    available.pNext = &f11;
    VkPhysicalDeviceVulkan13Features supported13{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    f11.pNext = &f12;
    f12.pNext = &supported13;
    vkGetPhysicalDeviceFeatures2(g->physical, &available);
    g->astc_hdr = supported13.textureCompressionASTC_HDR;
    f13.textureCompressionASTC_HDR = supported13.textureCompressionASTC_HDR;
    // shaderFloat16 is core in Vulkan 1.2; storage16BitAccess is a separate feature and is not needed.
    g->shader_float16 = f12.shaderFloat16
                        && renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderFloat16);
    // The vendor FSR half path also uses native 16-bit integer indexing. No 16-bit storage is enabled.
    features.shaderInt16 = g->shader_float16 && available.features.shaderInt16;
    g->features.shaderInt16 = features.shaderInt16;
    // Arm ASR's prebuilt shaders also use subgroup operations on their 16-bit values.
    g->subgroup_extended_types = g->shader_float16 && f12.shaderSubgroupExtendedTypes;
    g->viewport_index_supported = features.multiViewport && f12.shaderOutputViewportIndex;
    // Vulkan 1.3 made extended dynamic state core, with no feature to ask for (documented: the
    // Vulkan 1.3 promotion of VK_EXT_extended_dynamic_state); the device was chosen for 1.3.
    g->extended_dynamic_state = g->properties.apiVersion >= VK_API_VERSION_1_3;
    g->bindless_supported =
        f11.shaderDrawParameters && f12.shaderSampledImageArrayNonUniformIndexing
        && f12.descriptorBindingSampledImageUpdateAfterBind && f12.descriptorBindingPartiallyBound;
    VkPhysicalDeviceDescriptorIndexingProperties indexing{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES
    };
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties.pNext = &indexing;
    vkGetPhysicalDeviceProperties2(g->physical, &properties);
    const auto limit = std::min(
        {indexing.maxPerStageDescriptorUpdateAfterBindSamplers,
         indexing.maxPerStageDescriptorUpdateAfterBindSampledImages,
         indexing.maxDescriptorSetUpdateAfterBindSamplers,
         indexing.maxDescriptorSetUpdateAfterBindSampledImages}
    );
    g->material_texture_capacity = limit > 32 ? std::min(1024u, limit - 32u) : 0;
    g->bindless_supported = g->bindless_supported && g->material_texture_capacity >= 16;
    f11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    f12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    f12.shaderFloat16 = g->shader_float16 ? VK_TRUE : VK_FALSE;
    f12.shaderSubgroupExtendedTypes = g->subgroup_extended_types ? VK_TRUE : VK_FALSE;
    f12.shaderOutputViewportIndex = g->viewport_index_supported ? VK_TRUE : VK_FALSE;
    f13.pNext = &f11;
    f11.pNext = &f12;
    if (g->bindless_supported) {
        f11.shaderDrawParameters = VK_TRUE;
        f12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        f12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        f12.descriptorBindingPartiallyBound = VK_TRUE;
        f13.pNext = &f11;
        f11.pNext = &f12;
    }
    std::vector<const char*> extensions;
    if (g->surface != VK_NULL_HANDLE) {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    VkPhysicalDeviceFaultFeaturesEXT fault{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT};
    // A driver that offers it (MoltenVK) must be told the application knows it carries out only
    // part of Vulkan. What it leaves out (point polygons, wide lines, cull distances, logic ops)
    // the renderer does not use; triangle fans it has.
    {
        constexpr const char* kPortabilitySubset = "VK_KHR_portability_subset";  // vulkan_beta.h
        std::uint32_t n = 0;
        vkEnumerateDeviceExtensionProperties(g->physical, nullptr, &n, nullptr);
        std::vector<VkExtensionProperties> offered(n);
        vkEnumerateDeviceExtensionProperties(g->physical, nullptr, &n, offered.data());
        for (const VkExtensionProperties& e : offered) {
#ifdef OPENRAC_TRACY
            if (std::strcmp(e.extensionName, VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME) == 0) {
                extensions.push_back(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
            }
#endif
            if (std::strcmp(e.extensionName, kPortabilitySubset) == 0) {
                extensions.push_back(kPortabilitySubset);
            }
            if (std::strcmp(e.extensionName, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) == 0) {
                VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
                query.pNext = &fault;
                vkGetPhysicalDeviceFeatures2(g->physical, &query);
                if (fault.deviceFault) {
                    extensions.push_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
                    fault.deviceFaultVendorBinary = VK_FALSE;
                    fault.pNext = f12.pNext;
                    f12.pNext = &fault;
                    g->device_fault_enabled = true;
                }
            }
            if (g->allow_gpu_breadcrumbs) {
                for (const char* name :
                     {VK_AMD_BUFFER_MARKER_EXTENSION_NAME,
                      VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME}) {
                    if (std::strcmp(e.extensionName, name) == 0) {
                        extensions.push_back(name);
                    }
                }
            }
            // The HDR screen's metadata (hdr_output.cpp), with HDR's colour spaces only.
            if (g->hdr.colour_spaces
                && std::strcmp(e.extensionName, VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
                extensions.push_back(VK_EXT_HDR_METADATA_EXTENSION_NAME);
                g->hdr.metadata = true;
            }
            // Windows only (vulkan_win32.h): a driver may otherwise make a window covering the
            // display exclusive full screen; the swapchain says it must not (vkgl_present.cpp).
            constexpr const char* kFullScreenExclusive = "VK_EXT_full_screen_exclusive";
            if (g->surface_capabilities2 && std::strcmp(e.extensionName, kFullScreenExclusive) == 0) {
                extensions.push_back(kFullScreenExclusive);
                g->no_exclusive_full_screen = true;
            }
        }

        // The same operations under two names: the EXT one where offered, which more tools know.
        for (const char* name :
             {VK_EXT_LOAD_STORE_OP_NONE_EXTENSION_NAME, VK_KHR_LOAD_STORE_OP_NONE_EXTENSION_NAME}) {
            const bool offers = std::any_of(offered.begin(), offered.end(), [&](const auto& e) {
                return std::strcmp(e.extensionName, name) == 0;
            });
            if (offers && !g->load_store_none) {
                extensions.push_back(name);
                g->load_store_none = true;
            }
        }

        // Shader statistics for profiling on a device (OPENRAC_VK_PIPELINE_STATS).
        g->pipeline_stats =
            env::flag(env::Var::VkPipelineStats)
            && std::ranges::any_of(offered, [](const VkExtensionProperties& e) {
                   return std::strcmp(
                              e.extensionName, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME
                          )
                          == 0;
               });
        if (g->pipeline_stats) {
            extensions.push_back(VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
        }
    }

    // Pipelines linked from parts at a draw (vk-pipeline-library), where the device links them
    // fast. Desktop drivers only for now: the phones' drivers carry the extension unevenly.
    VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT library{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT
    };
    g->libraries.enabled = false;
#if !defined(__ANDROID__)
    {
        std::uint32_t n = 0;
        vkEnumerateDeviceExtensionProperties(g->physical, nullptr, &n, nullptr);
        std::vector<VkExtensionProperties> offered(n);
        vkEnumerateDeviceExtensionProperties(g->physical, nullptr, &n, offered.data());
        const auto has = [&offered](const char* name) {
            return std::ranges::any_of(offered, [name](const VkExtensionProperties& e) {
                return std::strcmp(e.extensionName, name) == 0;
            });
        };
        VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT fast{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT
        };
        VkPhysicalDeviceProperties2 asked{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        asked.pNext = &fast;
        VkPhysicalDeviceFeatures2 offers{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        offers.pNext = &library;
        const bool named = has(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
                           && has(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);
        if (named) {
            vkGetPhysicalDeviceFeatures2(g->physical, &offers);
            vkGetPhysicalDeviceProperties2(g->physical, &asked);
        }

        // Linking that is not fast would only move the compile, so it is not taken.
        if (named && library.graphicsPipelineLibrary == VK_TRUE
            && fast.graphicsPipelineLibraryFastLinking == VK_TRUE && g->extended_dynamic_state) {
            extensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
            extensions.push_back(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);
            library = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT};
            library.graphicsPipelineLibrary = VK_TRUE;
            library.pNext = f12.pNext;
            f12.pNext = &library;
            g->libraries.enabled = true;
        }
    }
#endif
    log::info(
        "vulkan: pipelines linked from parts at a draw: {}", g->libraries.enabled ? "yes" : "no"
    );
    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR executables{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR
    };
    if (g->pipeline_stats) {
        executables.pipelineExecutableInfo = VK_TRUE;
        executables.pNext = f12.pNext;
        f12.pNext = &executables;
        log::info("vulkan: pipeline shader statistics are logged (OPENRAC_VK_PIPELINE_STATS)");
    }

    VkPhysicalDevicePresentIdFeaturesKHR present_id{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR
    };
    VkPhysicalDevicePresentWaitFeaturesKHR present_wait{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR
    };
    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesKHR latest_ready{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_KHR
    };
    void* const features_chain =
        present_features(extensions, present_id, present_wait, latest_ready, &f13);
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.pNext = features_chain;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.pEnabledFeatures = &features;
    VkResult r = VK_SUCCESS;

    // An outside owner adds what it needs to the device (OpenXR: xrCreateVulkanDeviceKHR).
    if (g->setup.create_device != nullptr) {
        void* made = nullptr;
        r = static_cast<VkResult>(g->setup.create_device(
            g->setup.user, reinterpret_cast<void*>(vkGetInstanceProcAddr), g->physical, &info, &made
        ));
        g->device = static_cast<VkDevice>(made);
    } else {
        r = vkCreateDevice(g->physical, &info, nullptr, &g->device);
    }

    if (r != VK_SUCCESS || g->device == VK_NULL_HANDLE) {
        error = std::format("no Vulkan device ({})", static_cast<int>(r));
        return false;
    }
    log::info(
        "vulkan: shaderFloat16 arithmetic {} (storage layouts unchanged)",
        g->shader_float16 ? "enabled" : "disabled"
    );
    volkLoadDevice(g->device);
    vkGetDeviceQueue(g->device, g->family, 0, &g->queue);

    VmaAllocatorCreateInfo alloc{};
    alloc.vulkanApiVersion = VK_API_VERSION_1_3;
    alloc.physicalDevice = g->physical;
    alloc.device = g->device;
    alloc.instance = g->instance;
    VmaVulkanFunctions functions{};
    vmaImportVulkanFunctionsFromVolk(&alloc, &functions);
    alloc.pVulkanFunctions = &functions;
    if (!check(vmaCreateAllocator(&alloc, &g->allocator), "the memory allocator")) {
        error = "no memory allocator";
        return false;
    }
    setup_gpu_breadcrumbs();
    g->float_depth_format = g->depth_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    for (const VkFormat f : {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT}) {
        VkFormatProperties p{};
        vkGetPhysicalDeviceFormatProperties(g->physical, f, &p);
        if ((p.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
            g->depth_format = f;
            break;
        }
    }
    {
        VkFormatProperties p{};
        vkGetPhysicalDeviceFormatProperties(g->physical, VK_FORMAT_D32_SFLOAT_S8_UINT, &p);
        if ((p.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0) {
            g->float_depth_format = g->depth_format;
        }
    }
    return make_pipeline_cache();
}

bool make_frames() {
    for (Frame& f : g->frames) {
        if (!reserve_upload_ring(f)) {
            return false;
        }
        if (g->timestamp_bits) {
            VkQueryPoolCreateInfo queries{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
            queries.queryType = VK_QUERY_TYPE_TIMESTAMP;
            queries.queryCount = kTimestamps;
            if (!check(
                    vkCreateQueryPool(g->device, &queries, nullptr, &f.timestamps), "timestamp pool"
                )) {
                return false;
            }
        }
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool.queueFamilyIndex = g->family;
        if (!check(vkCreateCommandPool(g->device, &pool, nullptr, &f.pool), "a command pool")) {
            return false;
        }
        VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc.commandPool = f.pool;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 2;
        VkCommandBuffer buffers[2];
        if (!check(vkAllocateCommandBuffers(g->device, &alloc, buffers), "command buffers")) {
            return false;
        }
        f.upload = buffers[0];
        f.main = buffers[1];
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(g->device, &fence, nullptr, &f.fence);
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(g->device, &semaphore, nullptr, &f.acquired);
    }
    return true;
}

bool make_dummies() {
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = 512;
    info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                 | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;
    alloc.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc.requiredFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VmaAllocationInfo got{};
    if (!check(
            vmaCreateBuffer(
                g->allocator, &info, &alloc, &g->dummy_buffer, &g->dummy_allocation, &got
            ),
            "the default buffer"
        )) {
        return false;
    }
    auto* bytes = static_cast<std::uint8_t*>(got.pMappedData);
    std::memset(bytes, 0, 512);
    const float one_f[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    const std::uint32_t one_u[4] = {0, 0, 0, 1};
    std::memcpy(bytes + kDefaultFloat, one_f, sizeof one_f);
    std::memcpy(bytes + kDefaultInteger, one_u, sizeof one_u);
    const VkFormat formats[] =
        {VK_FORMAT_R32_UINT,
         VK_FORMAT_R32G32B32A32_UINT,
         VK_FORMAT_R32G32B32A32_SFLOAT,
         VK_FORMAT_R32G32B32A32_SINT};
    for (std::size_t i = 0; i < std::size(formats); ++i) {
        VkBufferViewCreateInfo v{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
        v.buffer = g->dummy_buffer;
        v.format = formats[i];
        v.range = 256;
        vkCreateBufferView(g->device, &v, nullptr, &g->dummy_views[i]);
    }
    // GL's incomplete texture reads opaque black.
    g->dummy_image = make_image(
        {VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 4}, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT
    );
    if (!g->dummy_image) {
        return false;
    }
    const std::uint8_t black[4] = {0, 0, 0, 0xFF};
    const Slice s = ring_copy(black, 4);
    VkCommandBuffer cmd = upload_cmd();
    copy_to_image(cmd, *g->dummy_image, s, 0, 0, 0, 1, 1);
    barrier(cmd, *g->dummy_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return true;
}

bool make_surface(SDL_Window* window, std::string& error) {
#if defined(__ANDROID__)
    // Made with the instance's own functions, from the window SDL holds.
    auto* native = static_cast<ANativeWindow*>(SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr
    ));
    VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    info.window = native;
    if (native == nullptr
        || vkCreateAndroidSurfaceKHR(g->instance, &info, nullptr, &g->surface) != VK_SUCCESS) {
        error = "no Vulkan surface for the window";
        return false;
    }
    return true;
#else
    if (!SDL_Vulkan_CreateSurface(window, g->instance, nullptr, &g->surface)) {
        error = std::format("no Vulkan surface for the window: {}", SDL_GetError());
        return false;
    }
    return true;
#endif
}

void describe_device() {
    const VkPhysicalDeviceProperties& p = g->properties;
    g->device_name = p.deviceName;
    g->api_version = version_text(p.apiVersion);
    VkPhysicalDeviceDriverProperties driver{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
    VkPhysicalDeviceProperties2 p2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    p2.pNext = &driver;
    vkGetPhysicalDeviceProperties2(g->physical, &p2);
    g->driver_version =
        driver.driverInfo[0] != '\0'
            ? std::format(
                  "{} {}", std::string_view(driver.driverName), std::string_view(driver.driverInfo)
              )
            : std::string(driver.driverName);
    g->software = p.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
    g->gl_renderer = std::format("{} (Vulkan)", g->device_name);
    g->gl_version = std::format("4.1 OpenRAC on Vulkan {}, {}", g->api_version, g->driver_version);
}

void destroy_all() {
    if (g->device != VK_NULL_HANDLE) {
        // The pre-warm workers stop first: they use the programs' modules and the cache. A save
        // on its own thread ends before the last one below.
        stop_pipeline_prewarm();
        g->cache_saver.join();
        vkDeviceWaitIdle(g->device);
        // Teardown drains all slots directly; resource retirement must not begin another recording.
        g->frame_pending = false;
        release_depth_combine();
        release_colour_transform();
        release_hdr();
        tracy_gpu_close();
        gpu_release();
        material_release();
        save_pipeline_manifest();
        save_pipeline_cache();
        std::string spirv_error;

        // The SPIR-V compiled this run, kept for the next.
        if (!g->spirv_cache.save(spirv_error)) {
            log::warn("vulkan: could not save the SPIR-V cache: {}", spirv_error);
        }

        for (Frame& f : g->frames) {
            // Window teardown can run after the statistics singleton's
            // destructor. Drain measurements only at live frame-slot reuse,
            // never call stats from static shutdown.
            run_deferred(f);
        }
        for (auto& [name, t] : g->textures.objects) {
            t.image.reset();
        }
        for (auto& [name, r] : g->renderbuffers.objects) {
            r.image.reset();
        }
        for (auto& [name, b] : g->buffers.objects) {
            destroy_buffer(b);
        }
        clear_test_programs();
        for (auto& [name, p] : g->programs.objects) {
            if (p.serial != 0) {
                release_program(p);
            }
        }
        for (Frame& f : g->frames) {
            run_deferred(f);
        }
        for (auto& [key, pipeline] : g->pipelines) {
            vkDestroyPipeline(g->device, pipeline, nullptr);
        }
        destroy_pipeline_libraries();
        for (auto& [key, sampler] : g->vk_samplers) {
            vkDestroySampler(g->device, sampler, nullptr);
        }
        g->default_colour.reset();
        g->default_depth.reset();
        g->dummy_image.reset();
        for (VkBufferView v : g->dummy_views) {
            vkDestroyBufferView(g->device, v, nullptr);
        }
        if (g->dummy_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(g->allocator, g->dummy_buffer, g->dummy_allocation);
        }
        // The descriptor sets kept across frames go with their pools (R-14).
        for (VkDescriptorPool pool : g->persistent_sets.pools) {
            vkDestroyDescriptorPool(g->device, pool, nullptr);
        }

        for (Frame& f : g->frames) {
            release_bulk_texture_staging(f);
            for (const Chunk& c : f.chunks) {
                vmaDestroyBuffer(g->allocator, c.buffer, c.allocation);
            }
            for (VkDescriptorPool pool : f.pools) {
                vkDestroyDescriptorPool(g->device, pool, nullptr);
            }
            vkDestroyCommandPool(g->device, f.pool, nullptr);
            vkDestroyQueryPool(g->device, f.timestamps, nullptr);
            vkDestroyFence(g->device, f.fence, nullptr);
            vkDestroySemaphore(g->device, f.acquired, nullptr);
        }
        for (VkSemaphore s : g->rendered) {
            vkDestroySemaphore(g->device, s, nullptr);
        }
        if (g->swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(g->device, g->swapchain, nullptr);
        }
        if (g->pipeline_cache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(g->device, g->pipeline_cache, nullptr);
            g->pipeline_cache = VK_NULL_HANDLE;
        }
        if (g->allocator != nullptr) {
            if (g->gpu_breadcrumbs && g->gpu_breadcrumbs->buffer) {
                vmaDestroyBuffer(
                    g->allocator, g->gpu_breadcrumbs->buffer, g->gpu_breadcrumbs->allocation
                );
            }
            vmaDestroyAllocator(g->allocator);
        }
        vkDestroyDevice(g->device, nullptr);
        g->device = VK_NULL_HANDLE;
    }
    if (g->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(g->instance, g->surface, nullptr);
    }
    if (g->messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(g->instance, g->messenger, nullptr);
    }
    if (g->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(g->instance, nullptr);
    }
}

}  // namespace openrac::vkgl::detail
