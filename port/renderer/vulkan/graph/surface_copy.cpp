// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements surface_chain.h's complete surface copies and multisample resolves on Vulkan.
#include "common/env.h"
#include "common/stats.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/gl.h"
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/surface_batch_internal.h"

namespace openrac::renderer::backend {
namespace {
// Pinned graph handles and resolve intent, validated before any commands are lowered.
struct CopyImages {
    graph::ImageHandle source;                    // Complete source attachment.
    graph::ImageHandle destination;               // Independent, completely overwritten attachment.
    graph::ImageTransferKind kind;
    graph::Aspect aspects;
};

// Rejects partial coverage, aliases and unsupported sample conversion before importing the copy.
Result<CopyImages> copy_images(
    vkgl::GraphExecution& execution,
    unsigned source,
    unsigned destination,
    std::array<int, 2> size,
    SurfaceCopy aspect
) {
    using namespace vkgl::detail;
    const auto from_target = target_of(source);
    const auto to_target = target_of(destination);
    const bool depth = aspect == SurfaceCopy::DepthStencil;
    auto* from = depth ? from_target.depth : from_target.colour;
    auto* to = depth ? to_target.depth : to_target.colour;

    // Full overwrites must cover both allocations and retain their exact format.
    if (!from || !to || from == to || from->format != to->format || size[0] < 1 || size[1] < 1
        || from->width != size[0] || from->height != size[1] || to->width != size[0]
        || to->height != size[1]) {
        return unexpected(Error{"invalid complete surface copy attachments"});
    }

    const bool resolve = from->samples != to->samples;

    // Only same-sample copying or multisample-to-single-sample resolution belongs to this contract.
    if (resolve
        && (from->samples == VK_SAMPLE_COUNT_1_BIT || to->samples != VK_SAMPLE_COUNT_1_BIT)) {
        return unexpected(Error{"unsupported surface copy sample conversion"});
    }

    const auto range = depth ? graph::Aspect::Depth : graph::Aspect::Colour;
    const auto input = execution.import_attachment("surface copy source", source, range);
    const auto output = execution.import_attachment(
        "surface copy destination", destination, range, vkgl::InitialContents::Discard
    );

    // Both allocations must enter the shared plan before a native command can be queued.
    if (!input || !output) {
        return unexpected(!input ? input.error() : output.error());
    }

    using Kind = graph::ImageTransferKind;
    return CopyImages{
        *input,
        *output,
        !resolve ? Kind::Copy
        : depth  ? Kind::ResolveDepth
                 : Kind::ResolveColour,
        static_cast<graph::Aspect>(from->aspect)
    };
}

// Returns whether the image was reused. Replaced images retire with the frame that used them.
Result<bool> prepare_scratch_texture(
    unsigned& name, const vkgl::detail::Format& format, std::array<int, 2> size, gl::GLenum filter
) {
    using namespace vkgl::detail;
    const auto* existing = g->textures.find(name);
    if (existing && existing->image->format == format.vk && existing->image->width == size[0]
        && existing->image->height == size[1]) {
        return true;
    }
    auto image = make_image(format, size[0], size[1], 1, VK_SAMPLE_COUNT_1_BIT);
    if (!image) {
        return unexpected(Error{"surface scratch image allocation failed"});
    }
    gl::glDeleteTextures(1, &name);
    name = g->textures.add();
    auto& stored = *g->textures.find(name);
    stored.target = gl::GL_TEXTURE_2D;
    stored.image = std::move(image);
    stored.params = SamplerParams{filter, filter, gl::GL_CLAMP_TO_EDGE, gl::GL_CLAMP_TO_EDGE};
    return false;
}

void scratch_stats(unsigned texture, bool reused, const char* bytes, const char* allocations) {
    using namespace vkgl::detail;
    if (stats::enabled()) {
        VmaAllocationInfo allocation{};
        vmaGetAllocationInfo(
            g->allocator, g->textures.find(texture)->image->allocation, &allocation
        );
        stats::gauge(bytes, double(allocation.size));
        stats::gauge(allocations, reused ? 0 : 1);
    }
}
}  // namespace

static SurfaceDepthScratch lease_depth(unsigned source, std::array<int, 2> size, bool refraction) {
    using namespace vkgl::detail;
    static const bool off = env::flag(env::Var::VkSurfaceScratchOff);
    if (off || !vkgl::detail::collecting_surface_chain()
        || (source != 0 && source == g->surface_depth_framebuffer)) {
        return {};
    }
    const auto* depth = target_of(source).depth;
    if (!depth || depth->width != size[0] || depth->height != size[1]
        || !has_stencil(depth->format)) {
        return {};
    }
    if (!g->surface_batch->claim_depth_scratch()) {
        ++g->measures.scratch_overlaps;
        return {};
    }

    auto& framebuffer = g->surface_depth_framebuffer;
    auto& texture = g->surface_depth_texture;
    const auto reused =
        prepare_scratch_texture(texture, {depth->format, depth->aspect, 4}, size, gl::GL_NEAREST);
    if (!reused) {
        return {};
    }
    if (!*reused) {
        gl::glDeleteFramebuffers(1, &framebuffer);
        framebuffer = g->framebuffers.add();
        g->framebuffers.find(framebuffer)->depth_texture = texture;
    }
    scratch_stats(
        texture, *reused, "surface depth scratch bytes", "surface depth scratch allocations"
    );
    g->measures.scratch_cross_reuses += *reused && g->surface_depth_last_refraction != refraction;
    g->measures.scratch_allocations += !*reused;
    g->surface_depth_last_refraction = refraction;
    return {framebuffer, texture};
}

SurfaceDepthScratch lease_surface_depth(unsigned source, std::array<int, 2> size) {
    const auto depth = lease_depth(source, size, false);
    if (depth) {
        ++vkgl::detail::g->measures.scratch_depth_leases;
    }
    return depth;
}

SurfaceRefractionScratch lease_surface_refraction(
    unsigned source, std::array<int, 2> size, bool with_colour
) {
    using namespace vkgl::detail;
    const auto depth = lease_depth(source, size, true);
    if (!depth) {
        return {};
    }
    if (!with_colour) {
        ++g->measures.scratch_refraction_leases;
        return {depth.framebuffer, {0, depth.texture}};
    }
    auto& colour = g->surface_refraction_colour;
    // Water samples the same RGBA8 snapshot even when the source has HDR colour.
    const auto reused =
        prepare_scratch_texture(colour, format_of(gl::GL_RGBA8), size, gl::GL_LINEAR);
    if (!reused) {
        return {};
    }
    g->framebuffers.find(depth.framebuffer)->colour_texture = colour;
    scratch_stats(
        colour, *reused, "refraction colour scratch bytes", "refraction colour scratch allocations"
    );
    ++g->measures.scratch_refraction_leases;
    g->measures.scratch_allocations += !*reused;
    return {depth.framebuffer, {colour, depth.texture}};
}

Result<bool> queue_surface_copy(
    unsigned source, unsigned destination, std::array<int, 2> size, SurfaceCopy aspect
) {
    // Immediate callers keep the original copy/resolve commands and implicit-resolve policy.
    if (!vkgl::detail::collecting_surface_chain()) {
        return false;
    }

    vkgl::GraphExecution execution;
    const auto images = copy_images(execution, source, destination, size, aspect);

    // Invalid attachments reject the section before any producer or consumer executes.
    if (!images) {
        return unexpected(images.error());
    }

    const bool depth = aspect == SurfaceCopy::DepthStencil;
    const graph::ImageRange range{images->aspects};
    // Names distinguish per-sample reuse from the sampled depth and normal inputs.
    const char* name = "surface normal resolve";

    // Depth copies distinguish per-sample reuse from sample-zero resolution.
    const bool depth_resolve = images->kind == graph::ImageTransferKind::ResolveDepth;
    if (depth) {
        name = depth_resolve ? "surface depth resolve" : "surface depth reuse copy";
    }

    graph::CommandBuffer commands;
    commands.transfer_image(images->source, images->destination, range.aspects, images->kind);
    auto pass = execution.make_pass(name);
    pass.images.push_back(
        {images->source, range,
         vkgl::detail::attachment_copy_state(depth_resolve, graph::Access::Read)}
    );
    pass.images.push_back(
        {images->destination, range,
         vkgl::detail::attachment_copy_state(depth_resolve, graph::Access::Write),
         depth_resolve ? graph::Load::Discard : graph::Load::None}
    );
    const auto added = execution.add_pass(std::move(pass), std::move(commands));
    if (!added) {
        return unexpected(added.error());
    }
    const auto result = execution.run();

    // Successful queuing promises a complete copy, not an already-recorded native command.
    if (!result) {
        return unexpected(result.error());
    }

    return true;
}
}  // namespace openrac::renderer::backend
