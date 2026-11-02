// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native compute access to the renderer's device, images and existing command recording.
#include "renderer/vulkan/native_compute.h"

#include <array>

#include "renderer/render_opt.h"
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

bool record_native_compute(
    std::span<const NativeImageUse> textures,
    const std::function<bool(VkCommandBuffer, std::span<const NativeImage>)>& record
) {
    // A native callback borrows a single active command recording.
    if (!g || !g->open || !record) {
        return false;
    }

    std::vector<ImagePtr> owners;
    std::vector<NativeImage> images;
    // FSR imports eight images. Keep synchronous handles pinned without growing
    // temporary vectors on every dispatch; larger native providers retain the dynamic path.
    const bool streamlined =
        renderer::render_opt::enabled(renderer::render_opt::Opt::NativeComputeSetup);
    std::array<ImagePtr, 8> local_owners{};
    std::array<NativeImage, 8> local_images{};
    const bool local = streamlined && textures.size() <= local_owners.size();

    // Validate all inputs before changing layouts or ending the current render pass.
    for (std::size_t i = 0; i < textures.size(); ++i) {
        const auto& use = textures[i];
        const auto* texture = g->textures.find(use.texture);

        // Multisampled images must be resolved before a compute provider reads them.
        if (!texture || !texture->image || texture->image->samples != VK_SAMPLE_COUNT_1_BIT) {
            return false;
        }

        // An ordinary colour attachment cannot become a storage image through a layout change.
        if (use.write && !(texture->image->usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
            return false;
        }

        // One image cannot simultaneously have sampled-read and storage-write layouts.
        for (std::size_t previous = 0; previous < i; ++previous) {
            const auto& earlier = textures[previous];
            const auto& alias = local ? local_owners[previous] : owners[previous];
            if ((use.write || earlier.write) && alias == texture->image) {
                return false;
            }
        }

        const auto& image = *texture->image;
        const NativeImage native{
            image.image,
            image.format,
            image.aspect,
            static_cast<unsigned>(image.width),
            static_cast<unsigned>(image.height)
        };
        if (local) {
            local_owners[i] = texture->image;
            local_images[i] = native;
        } else {
            owners.push_back(texture->image);
            images.push_back(native);
        }
    }
    const auto pinned = local ? std::span(local_owners).first(textures.size()) : std::span(owners);
    const auto imported =
        local ? std::span(local_images).first(textures.size()) : std::span(images);

    end_rendering(PassEnd::Compute);
    const auto command = frame().main;

    // Queue order and the renderer's image tracker remain authoritative across native dispatches.
    for (std::size_t i = 0; i < pinned.size(); ++i) {
        const auto layout =
            textures[i].write ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // A completed transition to shader-read already made the last writer visible.
        // Read/read reuse needs no dependency; writable images retain every barrier,
        // including GENERAL->GENERAL ordering between frames.
        if (!streamlined || textures[i].write || pinned[i]->layout != layout) {
            barrier(command, *pinned[i], layout);
        }
        pinned[i]->touched = g->recording;
    }

    return record(command, imported);
}
}  // namespace openrac::vkgl
