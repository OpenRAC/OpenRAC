// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements temporal_graph.h's opaque native upscaler boundary; SDK-owned histories retain their
// own barriers.
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/profiler.h"
#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/surface_batch.h"
#include "renderer/vulkan/temporal_graph.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
namespace {
namespace graph = renderer::graph;

// Imported native handles are borrowed through GraphExecution's pinned image owners for one
// dispatch.
struct Imports {
    graph::Pass pass;                 // Read-only inputs and complete storage outputs.
    std::vector<NativeImage> images;  // Same order as the provider's immutable import list.
};

// Validates the complete provider list before ending any pending rendering or importing entry
// states.
bool valid_compute_images(std::span<const NativeImageUse> uses) {
    // Validate the entire list before an earlier valid import can flush pending rendering.
    for (std::size_t index = 0; index < uses.size(); ++index) {
        const auto& use = uses[index];
        const auto* texture = detail::g->textures.find(use.texture);

        // Native providers require single-sample images and real storage usage for every writer.
        if (!texture || !texture->image || texture->image->samples != VK_SAMPLE_COUNT_1_BIT
            || texture->image->levels != 1
            || (use.write && !(texture->image->usage & VK_IMAGE_USAGE_STORAGE_BIT))) {
            return false;
        }

        // Aliased writes cannot have the SDK's independent sampled and storage image states.
        for (std::size_t previous = 0; previous < index; ++previous) {
            const auto* earlier = detail::g->textures.find(uses[previous].texture);

            // A sampled layout cannot coexist with a storage writer to the same physical image.
            if ((use.write || uses[previous].write) && earlier->image == texture->image) {
                return false;
            }
        }
    }

    return true;
}

// Validates all native compute image contracts before any callback or transition can be recorded.
Result<Imports> compute_images(
    GraphExecution& execution, std::string_view name, std::span<const NativeImageUse> uses
) {
    Imports result{{std::string(name)}, {}};
    result.images.reserve(uses.size());

    // Each writable image occupies a unique allocation; duplicate sampled reads remain legal.
    for (const auto& use : uses) {
        const auto* texture = detail::g->textures.find(use.texture);
        const auto image = execution.import_image(
            "temporal compute image",
            use.texture,
            use.write ? InitialContents::Discard : InitialContents::Preserve
        );

        // Undefined sampled contents or inconsistent views cannot enter a native callback.
        if (!image) {
            return unexpected(image.error());
        }

        const auto& native = *texture->image;
        const auto aspect = native.aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? graph::Aspect::Depth
                                                                      : graph::Aspect::Colour;
        result.pass.images.push_back(
            {*image,
             {aspect},
             {graph::Stage::ComputeShader,
              use.write ? graph::Access::Write : graph::Access::Read,
              use.write ? graph::Layout::General : graph::Layout::Sampled}}
        );
        result.images.push_back(
            {native.image,
             native.format,
             native.aspect,
             static_cast<unsigned>(native.width),
             static_cast<unsigned>(native.height)}
        );
    }

    return result;
}

// Successful native writes bypass raster stores but still define every aspect promised by the
// provider.
void set_output_defined(std::span<const NativeImageUse> uses, bool defined) {
    // Only complete storage outputs change their defined-content status after native recording.
    for (const auto& use : uses) {
        // Read-only inputs retain their original contents even if the provider fails.
        if (use.write) {
            auto& image = *detail::g->textures.find(use.texture)->image;

            // A rejected native writer may have left partial pixels and cannot preserve the
            // previous image.
            if (defined) {
                image.discarded &= ~image.aspect;
            } else {
                image.discarded |= image.aspect;
            }
        }
    }
}
}  // namespace

bool temporal_compute(
    std::string_view name,
    std::span<const NativeImageUse> inputs,
    const std::function<bool(VkCommandBuffer, std::span<const NativeImage>)>& record
) {
    // A provider cannot retain or record without an active owning graphics context.
    if (!detail::g || !detail::g->open || !record) {
        return false;
    }

    // SDK exceptions and incomplete output recordings must not publish a successful temporal frame.
    try {
        // Transactional validation must not end drawing or change any earlier image state.
        GraphExecution execution;
        auto imports = [&]() -> Result<Imports> {
            const profiling::Zone timing("Temporal compute imports");
            if (!valid_compute_images(inputs)) {
                return unexpected(Error{"invalid temporal compute images"});
            }
            return compute_images(execution, name, inputs);
        }();

        // Import validation completes before any native command is recorded.
        if (!imports) {
            return false;
        }

        execution.add_pass(std::move(imports->pass));
        bool started = false;
        bool provider_ok = false;
        // The provider owns internal barriers and restores external input/output layouts before
        // returning.
        const auto result = [&] {
            const profiling::Zone timing("Temporal compute execution");
            return execution.run_prepared([&](unsigned) {
                const profiling::Zone provider("Temporal compute provider");
                started = true;
                // Native rejection selects the existing fallback.
                try {
                    provider_ok = record(detail::frame().main, imports->images);
                } catch (...) {
                    provider_ok = false;
                }
            });
        }();
        // SDK return values and stack-owned dispatch descriptions must be consumed before this call returns.
        const auto completed = [] {
            const profiling::Zone timing("Temporal compute flush");
            return detail::flush_surface_chain();
        }();
        const bool success = bool(result) && bool(completed) && provider_ok;

        // Validation rejection leaves previous contents alone; a started writer either completes or
        // invalidates them.
        if (started) {
            set_output_defined(inputs, success);
        }

        return success;
    } catch (...) {
        return false;
    }
}
}  // namespace openrac::vkgl
