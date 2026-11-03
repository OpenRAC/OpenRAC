// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements renderer/colour_graph_backend.h: colour transfers, sample-zero depth resolves and
// sampled snapshot exports.
#include <array>
#include <utility>

#include "renderer/colour_graph_backend.h"
#include "renderer/vulkan/graph_barriers.h"
#include "renderer/vulkan/graph_execution.h"

namespace openrac::renderer::backend {
namespace {
// Source and destination colour/depth images, pinned by their owning graph through both callbacks.
struct SnapshotImages {
    std::array<graph::ImageHandle, 2> source;       // Colour first, then depth.
    std::array<graph::ImageHandle, 2> destination;  // Complete, independent sampled copies.
};

// Imports each complete source and overwritten destination before either native transfer starts.
Result<SnapshotImages> snapshot_images(
    vkgl::GraphExecution& execution, unsigned source, unsigned destination
) {
    SnapshotImages result;

    // Colour and depth are independent allocations even on multisampled targets.
    for (unsigned index = 0; index < result.source.size(); ++index) {
        const auto aspect = index == 0 ? graph::Aspect::Colour : graph::Aspect::Depth;
        const auto from = execution.import_attachment("refraction source", source, aspect);
        const auto to = execution.import_attachment(
            "refraction background", destination, aspect, vkgl::InitialContents::Discard
        );

        // Refraction must never read an aliased or incomplete snapshot.
        if (!from || !to) {
            return unexpected(!from ? from.error() : to.error());
        }

        // Copying an attachment onto itself would feed back through water shading.
        if (from->index == to->index) {
            return unexpected(Error{"refraction snapshot aliases its source"});
        }

        result.source[index] = *from;
        result.destination[index] = *to;
    }

    return result;
}

// Retains main attachments for more colour draws and exposes complete background copies to water
// shaders.
void snapshot_exports(vkgl::GraphExecution& execution, const SnapshotImages& images) {
    constexpr graph::Stage kShaders =
        graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
    graph::Pass consumers{"refraction snapshot consumers", {}, {}, graph::PassKind::Export};

    // Sources return to their original attachment roles; snapshot images become shader-readable.
    for (unsigned index = 0; index < images.source.size(); ++index) {
        const bool depth = index != 0;
        const graph::ImageRange range{depth ? graph::Aspect::Depth : graph::Aspect::Colour};
        consumers.images.push_back(
            {images.source[index],
             range,
             {depth ? graph::Stage::DepthTests : graph::Stage::ColourOutput,
              graph::Access::Read,
              depth ? graph::Layout::DepthAttachment : graph::Layout::ColourAttachment}}
        );
        consumers.images.push_back(
            {images.destination[index],
             range,
             {kShaders, graph::Access::Read, graph::Layout::Sampled}}
        );
    }

    execution.add_pass(std::move(consumers));
}

}  // namespace

// Keeps native colour and depth resolve commands in their existing order, with their actual
// execution scopes.
Result<void> record_colour_snapshot(
    unsigned source, unsigned destination, int samples, const std::function<void(bool)>& record
) {
    vkgl::GraphExecution execution;
    const auto images = snapshot_images(execution, source, destination);

    // All imports must validate before callbacks record a partial snapshot.
    if (!images) {
        return unexpected(images.error());
    }

    // The two work callbacks retain separate colour and depth native resolve paths.
    for (unsigned index = 0; index < images->source.size(); ++index) {
        const bool depth = index != 0;
        const graph::ImageRange range{depth ? graph::Aspect::Depth : graph::Aspect::Colour};
        execution.add_pass(
            {depth ? "refraction depth snapshot" : "refraction colour snapshot",
             {{images->source[index],
               range,
               vkgl::detail::attachment_copy_state(depth && samples > 1, graph::Access::Read)},
              {images->destination[index],
               range,
               vkgl::detail::attachment_copy_state(depth && samples > 1, graph::Access::Write),
               depth && samples > 1 ? graph::Load::Discard : graph::Load::None}}}
        );
    }

    snapshot_exports(execution, *images);
    // Copied callbacks survive local preparation and run before the enclosing refraction consumers.
    return execution.run_prepared([record](unsigned index) { record(index != 0); });
}

}  // namespace openrac::renderer::backend
