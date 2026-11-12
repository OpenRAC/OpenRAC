// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private allocation bindings shared by graph execution and reflected compute-input capture.
#pragma once

#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// An imported image owner or stable storage-buffer region, retained for one execution.
struct GraphBinding {
    ImagePtr image;  // Pins renderer-owned image memory through all callbacks.
    // Owned by the imported GL buffer or the backend's frame-ring retirement.
    BufferRef buffer{};
};

}  // namespace openrac::vkgl::detail

namespace openrac::vkgl {
namespace graph = renderer::graph;

// Owns declarations and allocation bindings for one synchronous command recording.
struct GraphExecution::Recording {
    graph::Graph graph;  // Owns immutable declarations once execution starts.
    std::vector<detail::GraphBinding> bindings;  // Indexed by the graph's canonical resource index.
    // True only during a synchronous work callback; typed transfer commands cannot escape its pass.
    bool recording = false;
    bool executed = false;  // A compiled entry state can be consumed only once.
    // A read-only buffer import took the backend's word that every GPU write so far is visible
    // (buffer_writes_visible), so it carries no barrier: each pass checks that this still holds.
    bool trusted_reads = false;
    // Canonicalises a borrowed allocation after pending attachment work has finished.
    // `written`: the open pass drew into it, so it holds that even while it is marked let go.
    Result<graph::ImageHandle> import_image(
        std::string name, detail::ImagePtr image, bool defined, bool written = false
    );
    // Imports an allocation, preserving exact offsets for ring-backed read-only regions.
    // `read_only`: the import is only read by shaders here, so it may enter as already visible.
    Result<renderer::graph::BufferHandle> import_buffer(
        std::string name, detail::BufferRef region, bool defined, bool read_only = false
    );
    // Appends one sampled image read after checking the legacy whole-image layout constraint.
    Result<void> sampled_image(
        graph::Pass& pass,
        const std::string& name,
        const detail::ImagePtr& image,
        graph::Stage stages
    );
    // Invokes one work callback and clears its active state on every exit.
    void record_work(unsigned index, const std::function<void(unsigned)>& record);
    // Ends earlier drawing and records this pass's exact entry dependencies.
    void enter(const graph::PlannedPass& pass);
};

}  // namespace openrac::vkgl
