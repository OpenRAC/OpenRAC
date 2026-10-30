// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Borrowed Vulkan resources for optional native compute providers.
#pragma once
#include <functional>
#include <span>
#include <string_view>
#include <volk.h>

namespace openrac::vkgl {
// Device handles borrowed until the graphics context closes; used only on its owning thread.
struct NativeDevice {
    VkDevice device = VK_NULL_HANDLE;               // Device owned by the renderer.
    VkPhysicalDevice physical = VK_NULL_HANDLE;     // Selected physical device.
    PFN_vkGetDeviceProcAddr device_proc = nullptr;  // Renderer-loaded device entry points.
    bool float16 = false;          // Enabled shaderFloat16 arithmetic; no implied 16-bit storage.
    bool int16 = false;            // Enabled shaderInt16 arithmetic (independent of float16).
    bool storage_formats = false;  // Extended storage formats and formatless writes are enabled.
    bool subgroup_extended_types = false;  // Subgroup operations on 16-bit values are enabled.
    bool fragment_stores = false;          // Fragment shaders may write storage images.
};

// An image's native identity, borrowed only for a synchronous compute callback.
struct NativeImage {
    VkImage image;              // Renderer-owned image; never destroy through this handle.
    VkFormat format;            // Allocated Vulkan format.
    VkImageAspectFlags aspect;  // Colour or depth/stencil aspects.
    unsigned width;             // Base-level width in pixels.
    unsigned height;            // Base-level height in pixels.
};

// A pool texture and the layout required at both ends of a native compute callback.
struct NativeImageUse {
    unsigned texture;    // GL-shaped renderer texture name.
    bool write = false;  // General layout for writes, shader-read layout otherwise.
};

// Returns false without an active Vulkan context; no device is opened by this call.
[[nodiscard]] bool native_device(NativeDevice& out);

// Returns whether a texture's current backing image was allocated for storage writes.
[[nodiscard]] bool storage_texture(unsigned texture);

// Allocates an uninitialised, single-level storage texture behind an existing renderer texture name.
// Existing contents are discarded; callers allocate before attaching or submitting the texture.
[[nodiscard]] bool allocate_storage_texture(
    unsigned texture, unsigned format, int width, int height
);

// Applies one immutable per-context compute colour transform in place to RGBA8 storage.
// Bindings zero/one sample/store the same image; push constants hold one float, workgroups are 8x8.
// Different shader source or unsupported targets return false without modifying their pixels.
[[nodiscard]] bool transform_colour(unsigned texture, std::string_view source, float parameter);

// Ends drawing, transitions the textures, then records the callback on the current graphics queue.
// The callback restores each image to its requested layout before returning and retains no handles.
[[nodiscard]] bool record_native_compute(
    std::span<const NativeImageUse> textures,
    const std::function<bool(VkCommandBuffer, std::span<const NativeImage>)>& record
);
}  // namespace openrac::vkgl
