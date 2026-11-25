// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe shortlist and tile-mask dispatches, with exact prefix/tail graph dependencies and OpenGL compatibility barriers.
#include <utility>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/viewer/probe_selection.h"
#include "renderer/gl/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph/graph_execution.h"
#endif

namespace openrac::lighting {
using namespace gl;

#if defined(OPENRAC_VULKAN)
namespace {
namespace graph = renderer::graph;
// The mask header contains one count followed by 128 selected region indices.
constexpr std::uint64_t kPrefix = (ProbeSelection::selected_capacity + 1) * sizeof(unsigned);
// Each opaque or translucent tile stores one bit per selected region.
constexpr unsigned kWords = ProbeSelection::selected_capacity / 32;
// All shader stages may consume the completed mask allocation and its input metadata.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// Declares the global shortlist prefix without the inactive combined shader's depth sampler.
Result<graph::Pass> shortlist_pass(vkgl::GraphExecution& execution, graph::BufferHandle masks) {
    auto shortlist = execution.make_pass("probe region shortlist");
    shortlist.buffers.push_back(
        {masks,
         {0, kPrefix},
         {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}
    );
    const auto metadata =
        execution.capture_compute_reads(shortlist, vkgl::ComputeInputs::UniformBlocks);

    // An unavailable metadata allocation invalidates both dependent kernels.
    if (!metadata) {
        return unexpected(metadata.error());
    }

    return shortlist;
}

// Declares the shortlist read and exact tile tail written by the depth-aware kernel.
Result<graph::Pass> tile_pass(
    vkgl::GraphExecution& execution,
    graph::BufferHandle masks,
    std::uint64_t tile_bytes,
    bool translucent
) {
    auto tiles = execution.make_pass("probe tile masks");
    // One shader block reads the shortlist prefix and writes the tile mask tail.
    tiles.compute_indexed_storage = true;
    tiles.buffers.push_back(
        {masks,
         {0, kPrefix},
         {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer}}
    );
    tiles.buffers.push_back(
        {masks,
         {kPrefix, tile_bytes * (translucent ? 2 : 1)},
         {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}
    );
    const auto inputs = execution.capture_compute_reads(tiles);

    // Depth and metadata must describe the same allocations as dispatch descriptors.
    if (!inputs) {
        return unexpected(inputs.error());
    }

    return tiles;
}

// Exports both mask regions and descriptor inputs for later graphics and compute consumers.
graph::Pass mask_consumers(
    vkgl::GraphExecution& execution, const graph::Pass& shortlist, const graph::Pass& tiles
) {
    auto consumers = execution.make_pass("probe mask consumers");
    consumers.buffers.reserve(tiles.buffers.size() + shortlist.buffers.size());
    graph::shader_read_export(consumers, tiles, kShaders);
    // The shortlist declares only buffer accesses, including reflected uniform blocks.
    for (auto use : shortlist.buffers) {
        use.state = {kShaders, graph::Access::Read, graph::Layout::Buffer};
        consumers.buffers.push_back(use);
    }
    for (auto& image : consumers.images) {
        image.continuation = true;
    }
    return consumers;
}
}  // namespace
#endif

void ProbeSelection::dispatch_shortlist(
    const renderer::Mat4& clip, const renderer::Vec3& position
) const {
    const auto& select = m_select.id() ? m_select : m_cull;
    select.use();
    glUniform1i(select.uniform("region_count"), static_cast<int>(m_count));
    glUniform1i(select.uniform("scene_depth"), 0);
    glUniformMatrix4fv(select.uniform("world_to_clip"), 1, GL_FALSE, clip.data());
    glUniform3f(select.uniform("camera_position"), position[0], position[1], position[2]);
    glUniform1i(select.uniform("select_regions"), 1);
    const stats::Scope shortlist("probe region shortlist", stats::kGpu);
    glDispatchCompute(1, 1, 1);
}

void ProbeSelection::dispatch_tiles(unsigned width, unsigned height) const {
    m_cull.use();
    glUniform1i(m_cull.uniform("scene_depth"), 0);
    glUniformMatrix4fv(m_cull.uniform("world_to_clip"), 1, GL_FALSE, m_clip.data());
    glUniform1i(m_cull.uniform("select_regions"), 0);
    glUniform1i(m_cull.uniform("translucent_tiles"), m_translucent_tiles ? 1 : 0);
    stats::gauge("probe split kernels", m_select.id() ? 1 : 0);
    const stats::Scope tiles("probe tile masks", stats::kGpu);
    glDispatchCompute(width, height, 1);
}

#if defined(OPENRAC_VULKAN)
bool ProbeSelection::dispatch_mask_graph(
    const renderer::Camera& camera, unsigned width, unsigned height
) {
    namespace graph = renderer::graph;
    const std::uint64_t tile_bytes =
        static_cast<std::uint64_t>(width) * height * kWords * sizeof(unsigned);
    vkgl::GraphExecution execution;
    // Resize uploads initialise unused translucent masks as well as the active ranges.
    const auto masks = execution.import_buffer("probe tile masks", m_buffers[1]);

    // Missing storage leaves the existing CPU probe fallback active.
    if (!masks) {
        log::warn("probe graph unavailable: {}", masks.error().message);
        return false;
    }

    const auto& select = m_select.id() ? m_select : m_cull;
    select.use();
    auto shortlist = shortlist_pass(execution, *masks);

    // Do not execute the tile kernel without a valid shortlist declaration.
    if (!shortlist) {
        log::warn("probe graph unavailable: {}", shortlist.error().message);
        return false;
    }

    m_cull.use();
    glUniform1i(m_cull.uniform("scene_depth"), 0);
    auto tiles = tile_pass(execution, *masks, tile_bytes, m_translucent_tiles);

    // Invalid depth or metadata cannot produce reusable tile masks.
    if (!tiles) {
        log::warn("probe graph unavailable: {}", tiles.error().message);
        return false;
    }

    auto consumers = mask_consumers(execution, *shortlist, *tiles);
    execution.add_pass(std::move(*shortlist));
    execution.add_pass(std::move(*tiles));
    execution.add_pass(std::move(consumers));
    // The executor invokes each callback synchronously on the render thread.
    const auto result = execution.run([&](unsigned pass) {
        // The shortlist must finish before tiles read its prefix.
        if (pass == 0) {
            dispatch_shortlist(m_clip, camera.position);
        } else {
            dispatch_tiles(width, height);
        }
    });

    // Keep selection disabled if graph validation or recording fails.
    if (!result) {
        log::warn("probe graph unavailable: {}", result.error().message);
        return false;
    }

    return true;
}
#endif

bool ProbeSelection::dispatch_masks(
    const renderer::Camera& camera, unsigned width, unsigned height
) {
#if defined(OPENRAC_VULKAN)
    // The logical mask layout remains identical for split and combined kernels.
    if (is_vulkan()) {
        return dispatch_mask_graph(camera, width, height);
    }
#endif
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    dispatch_shortlist(m_clip, camera.position);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    dispatch_tiles(width, height);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    return true;
}
}  // namespace openrac::lighting
