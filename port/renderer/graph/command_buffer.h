// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned transfer packets and pipeline command contracts for graph executors.
#pragma once

#include <array>

#include "renderer/graph/frame_graph.h"

namespace openrac::renderer::graph {

struct BufferCopy {
    BufferHandle source;
    BufferRange bytes;
    BufferHandle destination;
    std::uint64_t destination_offset;
};

struct BufferToImageCopy {
    BufferHandle source;
    BufferRange bytes;
    ImageHandle destination;
};

struct BlitRegion {
    std::array<int, 2> source_begin{};
    std::array<int, 2> source_end{};
    std::array<int, 2> destination_begin{};
    std::array<int, 2> destination_end{};
    bool linear = false;
    // A clear covers the destination when the blit only writes a rectangle within it.
    std::optional<std::array<float, 4>> clear{};
};

struct ImageBlit {
    ImageHandle source;
    ImageHandle destination;
    BlitRegion region;
};

enum class ImageTransferKind : std::uint8_t {
    Copy,
    ResolveColour,
    ResolveDepth
};

struct FullImageTransfer {
    ImageHandle source;
    ImageHandle destination;
    Aspect aspect;
    ImageTransferKind kind;
};

using Command = std::variant<BufferCopy, BufferToImageCopy, ImageBlit, FullImageTransfer>;

struct Draw {
    unsigned vertices;
    unsigned instances;
    unsigned first_vertex;
    unsigned first_instance;
};

struct DrawIndexed {
    unsigned indices;
    unsigned instances;
    unsigned first_index;
    int vertex_offset;
    unsigned first_instance;
};

struct DrawIndexedIndirect {
    std::uint64_t offset;
    unsigned count;
    unsigned stride;
};

struct Dispatch {
    std::array<unsigned, 3> groups;
};

using PipelineCommand = std::variant<Draw, DrawIndexed, DrawIndexedIndirect, Dispatch>;

// Validates the pipeline class against the pass. Bindings are checked by the executor.
[[nodiscard]] Result<void> validate_pipeline_command(
    const Pass& pass, const PipelineCommand& command
);

// CPU-side commands for one pass. The backend resolves graph handles to native allocations.
class CommandBuffer {
public:
    void copy_buffer(
        BufferHandle source,
        BufferRange bytes,
        BufferHandle destination,
        std::uint64_t destination_offset
    );
    void copy_buffer_to_image(BufferHandle source, BufferRange bytes, ImageHandle destination);
    void blit(ImageHandle source, ImageHandle destination, BlitRegion region);
    void transfer_image(
        ImageHandle source, ImageHandle destination, Aspect aspect, ImageTransferKind kind
    );
    [[nodiscard]] Result<void> validate(const Pass& pass) const;
    // Derives the exact transfer declarations without inventing extra writes.
    [[nodiscard]] Pass declarations(std::string name) const;
    [[nodiscard]] Result<CommandBuffer> remap(const ResourceRemap& resources) const;

    [[nodiscard]] std::span<const Command> commands() const;

private:
    void append(Command command);
    // Most transfer passes contain a single command.
    std::variant<std::monostate, Command, std::vector<Command>> m_commands;
};

}  // namespace openrac::renderer::graph
