// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private allocation bindings shared by graph execution and reflected compute-input capture.
#pragma once

#include <array>
#include <variant>

#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/prepared_dispatch.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {
// An imported image owner or stable storage-buffer region, retained for one execution.
struct GraphBinding {
    ImagePtr image;  // Pins renderer-owned image memory through all callbacks.
    // Owned by the imported GL buffer or the backend's frame-ring retirement.
    BufferRef buffer{};
    bool discard_image = false;  // Explicit discard at graph entry for tracked textures.
    bool transient_image = false;
};

}  // namespace openrac::vkgl::detail

namespace openrac::vkgl {
namespace graph = renderer::graph;

// Owns declarations and allocation bindings for one synchronous command recording.
struct GraphExecution::Recording {
    static constexpr std::size_t kSpareCount = 64;
    static std::array<std::unique_ptr<Recording>, kSpareCount>& spares();

    std::source_location site;
    graph::Graph graph;                     // Owns immutable declarations once execution starts.
    std::optional<graph::PlanCache> cache;  // Exact recurring plans from this builder's call site.
    graph::Plan compiled;                // Storage for plans with changing entry states or ranges.
    graph::Pass captured_reads;          // Reflected input scratch, reused across recordings.
    graph::ImageDesc image_description;  // Reuses import scratch for mixed subresource states.
    std::string attachment_label;
    std::vector<detail::GraphBinding> bindings;  // Indexed by the graph's canonical resource index.
    std::vector<unsigned> transient_images;      // Logical images without a backing allocation yet.
    using Work = std::variant<graph::CommandBuffer, detail::PreparedDispatch>;

    struct Packet {
        unsigned index;
        Work work;
    };

    std::vector<Packet> packets;       // Owned work, appended in pass order and retained for reuse.
    bool executed = false;             // A compiled entry state can be consumed only once.
    bool attachment_fallback = false;  // Match legacy draws' immutable feedback substitution.
    bool trusted_reads = false;  // A shader-only import used the current buffer visibility epoch.
    VkCommandBuffer target = VK_NULL_HANDLE;  // Independent upload/readback stream, when supplied.

    [[nodiscard]] VkCommandBuffer command() const {
        return target != VK_NULL_HANDLE ? target : detail::frame().main;
    }

    [[nodiscard]] bool needs_callback(const graph::Plan* plan = nullptr) const;
    [[nodiscard]] const Work* work_at(unsigned index) const;
    void reset() noexcept;
    [[nodiscard]] Result<void> allocate_transient_images(const graph::Plan& plan);
    // Canonicalises a borrowed allocation after pending attachment work has finished.
    // `written`: the open pass drew into it, so it holds that even while it is marked let go.
    Result<graph::ImageHandle> import_image(
        std::string_view name, detail::ImagePtr image, bool defined, VkImageAspectFlags written = 0
    );
    // Imports an allocation, preserving exact offsets for ring-backed read-only regions.
    Result<renderer::graph::BufferHandle> import_buffer(
        std::string_view name, detail::BufferRef region, bool defined, bool shader_read = false
    );
    // Imports one sampled image using the legacy whole-image layout.
    Result<graph::ImageUse> sampled_image(
        std::string_view name, const detail::Texture& texture, graph::Stage stages
    );
    // Ends earlier drawing and records this pass's exact entry dependencies.
    void enter(const graph::PlannedPass& pass);
};

}  // namespace openrac::vkgl
