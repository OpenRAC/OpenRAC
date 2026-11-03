// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synchronous execution of declared passes on renderer-owned Vulkan images and storage buffers.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "renderer/graph/frame_graph.h"

namespace openrac::vkgl {
namespace detail {
class SurfaceBatch;
}
// Preserves valid incoming contents, or requires each imported range to be written before its first read.
enum class InitialContents : std::uint8_t {
    Preserve,
    Discard
};

// Combined shortlist kernels can capture metadata without declaring inactive sampler branches.
enum class ComputeInputs : std::uint8_t {
    All,
    UniformBlocks
};

// One graph recording on the context's owning thread; bindings outlive all synchronous callbacks.
// Callbacks record existing GL-shaped commands, without reallocating imports or submitting frames.
class GraphExecution {
public:
    GraphExecution();
    ~GraphExecution();
    GraphExecution(const GraphExecution&) = delete;
    GraphExecution& operator=(const GraphExecution&) = delete;
    GraphExecution(GraphExecution&&) noexcept;
    GraphExecution& operator=(GraphExecution&&) noexcept;

    // Imports a renderer texture's actual allocation and entry state, with an explicit contents policy.
    [[nodiscard]] Result<renderer::graph::ImageHandle> import_image(
        std::string name, unsigned texture, InitialContents contents = InitialContents::Preserve
    );
    // Imports stable GPU storage, including its existing allocation offset and size in bytes.
    [[nodiscard]] Result<renderer::graph::BufferHandle> import_buffer(
        std::string name, unsigned buffer, InitialContents contents = InitialContents::Preserve
    );
    // Imports a read-only storage, texel or uniform-buffer view, retaining its actual allocation offset.
    [[nodiscard]] Result<renderer::graph::BufferUse> read_buffer(
        std::string name, unsigned buffer, renderer::graph::Stage stages
    );
    // Imports a complete sampled texture, preserving its actual aspect and mip range.
    [[nodiscard]] Result<renderer::graph::ImageUse> read_image(
        std::string name, unsigned texture, renderer::graph::Stage stages
    );
    // Appends descriptor-matched input reads, without changing `pass` when capture fails.
    // Storage accesses remain explicit; loose uniforms and immutable fallback descriptors stay backend-owned.
    [[nodiscard]] Result<void> capture_compute_reads(
        renderer::graph::Pass& pass, ComputeInputs inputs = ComputeInputs::All
    );
    // Captures read-only graphics descriptors for the currently bound vertex/fragment program.
    // Callers promise all reflected graphics storage is read-only; failure leaves `pass` unchanged.
    [[nodiscard]] Result<void> capture_graphics_reads(renderer::graph::Pass& pass);
    // Imports a framebuffer attachment, including renderer-owned default or multisample storage.
    [[nodiscard]] Result<renderer::graph::ImageHandle> import_attachment(
        std::string name,
        unsigned framebuffer,
        renderer::graph::Aspect aspect,
        InitialContents contents = InitialContents::Preserve
    );
    // Records a complete single-level colour copy inside a declared transfer pass.
    // Source bytes and destination TransferDestination layout must already be ordered by this graph.
    [[nodiscard]] Result<void> copy_buffer_to_image(
        const renderer::graph::BufferUse& source, renderer::graph::ImageHandle destination
    );
    // Appends one ordered declaration; exports have no callback.
    void add_pass(renderer::graph::Pass pass);
    // Compiles all accesses before recording; invokes each work pass's callback synchronously.
    // Once recording starts, this object is single-use even on failure; validation invokes no callback.
    [[nodiscard]] Result<void> run(const std::function<void(unsigned)>& record);

    // Records prepared native or raster commands; captured bindings must outlive the enclosing chain.
    [[nodiscard]] Result<void> run_prepared(const std::function<void(unsigned)>& record);
    // Freezes raster uniform values and framebuffer attachments before later setup can overwrite them.
    [[nodiscard]] Result<void> run_raster(const std::function<void(unsigned)>& record);
    // Records ordered bucket work, inheriting and publishing the previous ordered callback's GL state.
    [[nodiscard]] Result<void> run_ordered(const std::function<void(unsigned)>& record);

private:
    friend class detail::SurfaceBatch;
    // Captures real uniform-block allocation ranges into a temporary declaration.
    [[nodiscard]] Result<void> capture_uniform_reads(
        renderer::graph::Pass& reads, renderer::graph::Stage stages
    );
    // Captures sampled image and texel-buffer allocations into a temporary declaration.
    [[nodiscard]] Result<void> capture_sampled_reads(
        renderer::graph::Pass& reads, renderer::graph::Stage stages
    );
    // Captures the bound graphics program's read-only storage allocation ranges.
    [[nodiscard]] Result<void> capture_graphics_storage_reads(renderer::graph::Pass& reads);
    // Owns declarations and borrowed allocation bindings until this synchronous recording ends.
    struct Recording;
    std::unique_ptr<Recording> m_recording;
};
}  // namespace openrac::vkgl
