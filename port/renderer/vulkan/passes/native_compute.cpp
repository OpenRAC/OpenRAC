// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native compute access to the renderer's device, images and existing command recording.
#include "renderer/vulkan/passes/native_compute.h"

#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
using namespace detail;

bool native_device(NativeDevice& out) {
    // Optional providers cannot create or outlive the renderer's graphics context.
    if (!g || !g->open) {
        return false;
    }

    out =
        {g->device,
         g->physical,
         vkGetDeviceProcAddr,
         g->shader_float16,
         bool(g->features.shaderInt16),
         bool(
             g->features.shaderStorageImageExtendedFormats
             && g->features.shaderStorageImageWriteWithoutFormat
         ),
         g->subgroup_extended_types,
         bool(g->features.fragmentStoresAndAtomics)};
    return true;
}

bool storage_texture(unsigned texture) {
    const auto* found = g ? g->textures.find(texture) : nullptr;
    return found && found->image && (found->image->usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
}

bool allocate_storage_texture(unsigned texture, unsigned format, int width, int height) {
    // Storage allocations require a live name and positive dimensions.
    if (!g || width <= 0 || height <= 0) {
        return false;
    }

    auto* destination = g->textures.find(texture);
    const auto description = format_of(format);

    // Invalid texture names and unknown formats cannot be queried or allocated.
    if (!destination || description.vk == VK_FORMAT_UNDEFINED) {
        return false;
    }

    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(g->physical, description.vk, &properties);

    // Unsupported storage formats must fall back before any commands use the image.
    if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
        return false;
    }

    auto image = make_image(
        description, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_STORAGE_BIT
    );

    // Allocation failures leave the previous texture intact.
    if (!image) {
        return false;
    }

    retire(std::move(destination->image));
    destination->image = std::move(image);
    destination->internal = format;
    destination->target = gl::GL_TEXTURE_2D;
    return true;
}

}  // namespace openrac::vkgl
