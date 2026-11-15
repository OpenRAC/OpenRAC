// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synchronous execution of declared passes on renderer-owned Vulkan images and storage buffers.
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <source_location>
#include <string_view>

#include "renderer/graph/command_buffer.h"

namespace openrac::vkgl {
namespace detail {
class SurfaceBatch;
struct PreparedDispatch;
struct Program;
class GraphImmediateWork;
}
// Preserves valid incoming contents, or requires each imported range to be written before its first read.
enum class InitialContents : std::uint8_t {
    Preserve,
    Discard
};

// Allows stable storage to reuse recorded shader visibility within the main command buffer.
enum class BufferVisibility : std::uint8_t {
    Conservative,
    TrackShaderReads
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
    explicit GraphExecution(std::source_location site = std::source_location::current());
    ~GraphExecution();
    GraphExecution(const GraphExecution&) = delete;
    GraphExecution& operator=(const GraphExecution&) = delete;
    GraphExecution(GraphExecution&&) noexcept;
    GraphExecution& operator=(GraphExecution&&) noexcept;

    // Imports a renderer texture's actual allocation and entry state, with an explicit contents policy.
    [[nodiscard]] Result<renderer::graph::ImageHandle> import_image(
        std::string_view name, unsigned texture, InitialContents contents = InitialContents::Preserve
    );
    // An undefined RGBA8 image for owned transfers within this recording. A compatible
    // temporary can reuse its backing image after its last declared use.
    [[nodiscard]] Result<renderer::graph::ImageHandle> create_transient_colour(
        std::string_view name, std::array<int, 2> extent
    );
    // Imports stable GPU storage, including its existing allocation offset and size in bytes.
    [[nodiscard]] Result<renderer::graph::BufferHandle> import_buffer(
        std::string_view name,
        unsigned buffer,
        InitialContents contents = InitialContents::Preserve,
        BufferVisibility visibility = BufferVisibility::Conservative
    );
    // Imports a read-only storage, texel or uniform-buffer view, retaining its actual allocation offset.
    [[nodiscard]] Result<renderer::graph::BufferUse> read_buffer(
        std::string_view name, unsigned buffer, renderer::graph::Stage stages
    );
    // Imports the sampled texture view, preserving its actual aspect and mip range.
    [[nodiscard]] Result<renderer::graph::ImageUse> read_image(
        std::string_view name, unsigned texture, renderer::graph::Stage stages
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
        std::string_view name,
        unsigned framebuffer,
        renderer::graph::Aspect aspect,
        InitialContents contents = InitialContents::Preserve
    );
    // Appends one ordered declaration; exports have no callback.
    void add_pass(renderer::graph::Pass pass);
    [[nodiscard]] renderer::graph::Pass make_pass(std::string_view name);
    // Registers an owned command buffer. Its declarations are checked before compilation.
    [[nodiscard]] Result<void> add_pass(
        renderer::graph::Pass pass, renderer::graph::CommandBuffer commands
    );
    // Compiles all accesses before recording; invokes each work pass's callback synchronously.
    // Once recording starts, this object is single-use even on failure; validation invokes no callback.
    [[nodiscard]] Result<void> run(const std::function<void(unsigned)>& record = {});

    // Records prepared native or raster commands; captured bindings must outlive the enclosing chain.
    [[nodiscard]] Result<void> run_prepared(const std::function<void(unsigned)>& record);
    [[nodiscard]] Result<void> run_prepared(const std::function<void()>& record);
    // Freezes raster uniform values and framebuffer attachments before later setup can overwrite them.
    [[nodiscard]] Result<void> run_raster(const std::function<void(unsigned)>& record);
    // Records ordered bucket work, inheriting and publishing the previous ordered callback's GL state.
    [[nodiscard]] Result<void> run_ordered(const std::function<void()>& record);

private:
    friend class detail::SurfaceBatch;
    friend class detail::GraphImmediateWork;
    [[nodiscard]] Result<void> add_dispatch_pass(
        renderer::graph::Pass pass, detail::PreparedDispatch packet
    );
    // Records one compiled pass or the complete retained plan.
    [[nodiscard]] Result<void> record_work(
        unsigned index,
        const renderer::graph::PlannedPass& pass,
        const std::function<void(unsigned)>& record
    );
    [[nodiscard]] Result<void> record_plan(
        const renderer::graph::Plan& plan, const std::function<void(unsigned)>& record,
        bool coupled_layouts
    );
    [[nodiscard]] Result<void> validate_commands(const renderer::graph::CommandBuffer& commands);
    [[nodiscard]] Result<void> execute(const renderer::graph::CommandBuffer& commands);
    [[nodiscard]] Result<void> transfer_command(
        const renderer::graph::Command& command, bool encode
    );
    [[nodiscard]] Result<void> transfer_full_image(
        const renderer::graph::FullImageTransfer& transfer, bool encode
    );
    [[nodiscard]] Result<void> transfer_image_blit(
        const renderer::graph::ImageBlit& blit, bool encode
    );
    [[nodiscard]] Result<void> transfer_buffer_to_image(
        const renderer::graph::BufferToImageCopy& copy, bool encode
    );
    [[nodiscard]] Result<void> transfer_buffer(
        const renderer::graph::BufferCopy& copy, bool encode
    );
    [[nodiscard]] Result<void> capture_shader_reads(
        renderer::graph::Pass& pass, renderer::graph::Stage stages, ComputeInputs inputs
    );
    // Captures real uniform-block allocation ranges.
    [[nodiscard]] Result<void> capture_uniform_reads(
        renderer::graph::Pass& reads, const detail::Program& program, renderer::graph::Stage stages
    );
    // Captures sampled image and texel-buffer allocations.
    [[nodiscard]] Result<void> capture_sampled_reads(
        renderer::graph::Pass& reads, const detail::Program& program, renderer::graph::Stage stages
    );
    // Captures the bound graphics program's read-only storage allocation ranges.
    [[nodiscard]] Result<void> capture_graphics_storage_reads(
        renderer::graph::Pass& reads, const detail::Program& program
    );
    // Owns declarations and borrowed allocation bindings until this synchronous recording ends.
    struct Recording;
    std::unique_ptr<Recording> m_recording;
};
}  // namespace openrac::vkgl
