// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fixed-order frame dependencies. This CPU-only planner owns declarations, not GPU allocations
// or callbacks; executors apply its barriers around existing pass recording.
#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "common/result.h"

namespace openrac::renderer::graph {

// Execution stages; combined shader reads retain all stages until a later writer.
enum class Stage : std::uint32_t {
    None = 0,
    Transfer = 1,
    VertexInput = 2,
    VertexShader = 4,
    FragmentShader = 8,
    ComputeShader = 16,
    ColourOutput = 32,
    DepthTests = 64,
    DrawIndirect = 128,
    All = 0xFFFFFFFFu
};

// Whether a use consumes existing contents, produces new contents, or does both.
enum class Access : std::uint8_t {
    None,
    Read,
    Write,
    ReadWrite
};

// Image layouts are separate from access, so GENERAL-to-GENERAL hazards remain visible.
enum class Layout : std::uint8_t {
    Undefined,
    Buffer,
    General,
    Sampled,
    ColourAttachment,
    DepthAttachment,
    DepthReadOnly,
    TransferSource,
    TransferDestination,
    Present
};

// Independently tracked image aspects; colour cannot share an image with depth/stencil.
enum class Aspect : std::uint8_t {
    Colour = 1,
    Depth = 2,
    Stencil = 4
};

// Attachment contents at entry and exit; non-attachment uses leave load as None.
enum class Load : std::uint8_t {
    None,
    Preserve,
    Clear,
    Discard
};
enum class Store : std::uint8_t {
    Preserve,
    Discard
};

// Exports establish the state borrowed by subsequent legacy recording, without running GPU work.
enum class PassKind : std::uint8_t {
    Work,
    Export
};

// Returns the union of execution stages or aspects.
constexpr Stage operator|(Stage a, Stage b) {
    return static_cast<Stage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr Aspect operator|(Aspect a, Aspect b) {
    return static_cast<Aspect>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

// Actual prior or required access, independent of whether the contents are defined.
struct State {
    Stage stages = Stage::None;         // Queue execution scopes, combined when reads overlap.
    Access access = Access::None;       // Memory access performed by those stages.
    Layout layout = Layout::Undefined;  // Buffer resources use Buffer throughout.
    bool operator==(const State&) const = default;
};

// Canonical allocation identity and generation; multiple views import the same key.
struct ResourceKey {
    std::uint64_t identity = 0;    // Nonzero allocation identity supplied by its owner.
    std::uint64_t generation = 0;  // Changes when the allocation is replaced or recycled.
    bool operator==(const ResourceKey&) const = default;
};

// Typed graph-local handles, valid only in the graph that returned them.
struct ImageHandle {
    unsigned index = std::numeric_limits<unsigned>::max();  // Unbound until imported.
};

struct BufferHandle {
    unsigned index = std::numeric_limits<unsigned>::max();  // Unbound until imported.
};

// Import descriptions borrow allocation identity and state; their owners retain the GPU lifetime.
struct ImageDesc {
    ResourceKey key{};                // Shared by aliases of this allocation.
    unsigned mips = 1;                // Number of mip levels, not dimensions in pixels.
    unsigned layers = 1;              // Number of array layers.
    Aspect aspects = Aspect::Colour;  // Aspects present in this allocation.
    State initial{};                  // Access completed or pending before graph recording.
    bool defined = false;             // Whether every imported subresource contains valid data.
    bool operator==(const ImageDesc&) const = default;
};

struct BufferDesc {
    ResourceKey key;         // Shared by aliases of this allocation.
    std::uint64_t size = 0;  // Allocation size in bytes.
    State initial{Stage::None, Access::None, Layout::Buffer};  // Prior GPU access.
    bool defined = false;  // Whether every byte contains valid data.
    bool operator==(const BufferDesc&) const = default;
};

// Exact image subresources and buffer byte intervals; empty ranges are rejected.
struct ImageRange {
    Aspect aspects = Aspect::Colour;  // A subset of the image's aspects.
    unsigned first_mip = 0;           // First included mip level.
    unsigned mip_count = 1;           // Number of consecutive mip levels.
    unsigned first_layer = 0;         // First included array layer.
    unsigned layer_count = 1;         // Number of consecutive layers.
    bool operator==(const ImageRange&) const = default;
};

struct BufferRange {
    std::uint64_t offset = 0;  // First included byte.
    std::uint64_t size = 0;    // Number of included bytes.
    bool operator==(const BufferRange&) const = default;
};

// A write-only declaration promises to initialise the entire declared range unless stored as Discard.
struct ImageUse {
    ImageHandle image;              // Imported allocation, independent of the selected view.
    ImageRange range;               // Subresources accessed by this pass.
    State state;                    // Required execution, access and layout.
    Load load = Load::None;         // Attachment entry operation, otherwise None.
    Store store = Store::Preserve;  // Discard makes subsequent reads invalid.
};

struct BufferUse {
    BufferHandle buffer;  // Imported allocation.
    BufferRange range;    // Byte interval accessed by this pass.
    State state;          // Required execution and access with layout Buffer.
};

// Owned pass declarations, executed in insertion order; no draw packets or resource owners are retained.
struct Pass {
    std::string name;                  // Stable diagnostic label, not a per-draw label.
    std::vector<ImageUse> images{};    // All image accesses, including attachments.
    std::vector<BufferUse> buffers{};  // All buffer accesses.
    PassKind kind = PassKind::Work;    // Exports follow all work passes.
    bool separate = false;             // Never joins the open render pass on its attachments.
};

// Copies a pass's ranges into a read-only shader export, preserving its sampled input and output coverage.
// Images become sampled with no attachment operations; buffers retain their exact byte ranges.
[[nodiscard]] Pass shader_read_export(std::string name, const Pass& source, Stage stages);

// A barrier covers one aspect/layer's mip interval, or one contiguous buffer interval.
using Range = std::variant<ImageRange, BufferRange>;

struct Barrier {
    unsigned resource = 0;  // Canonical resource index in Plan::resources.
    Range range;            // Exact overlapping subresources or bytes.
    State before;           // Includes all prior overlapping readers.
    State after;            // State required by the next use.
};

// Ordering edges retain all readers before a writer, not only the most recent reader.
struct Dependency {
    unsigned source = 0;       // Earlier pass index.
    unsigned destination = 0;  // Later pass index.
    unsigned resource = 0;     // Allocation responsible for this edge.
    bool operator==(const Dependency&) const = default;
};

// A resource's declaration and inclusive first/last use, including export contracts.
struct Resource {
    std::string name;                                 // First import's diagnostic label.
    std::variant<ImageDesc, BufferDesc> description;  // Allocation metadata, never a GPU owner.
    std::optional<unsigned> first;                    // Empty for an unused import.
    std::optional<unsigned> last;                     // Final consuming or exporting pass.
};

// Entry barriers and kind for one pass; recording remains the executor's responsibility.
struct PlannedPass {
    std::string name;                // Copied from its declaration.
    PassKind kind = PassKind::Work;  // Exports execute barriers only.
    std::vector<Barrier> barriers;   // Applied before this pass.
    bool separate = false;           // Copied from its declaration.
};

// Optional source declarations are retained only for frame-wide diagnostic capture.
enum class DeclarationRetention : std::uint8_t {
    Discard,
    Retain
};

// Compiled plan independent of the builder, device and frame slot.
struct Plan {
    std::vector<Resource> resources;       // Canonical imports and lifetimes.
    std::vector<PlannedPass> passes;       // In the original insertion order.
    std::vector<Dependency> dependencies;  // Unique allocation-specific ordering edges.
    std::vector<Pass> declarations;  // Empty unless diagnostic composition requested retention.
    // Returns a deterministic text dump; does no logging or file I/O.
    [[nodiscard]] std::string describe() const;
};

// One frame's declarations. No sorting, allocation aliasing, queue submission or Vulkan dependency.
class Graph {
public:
    // Imports an image, coalescing compatible aliases of the same key; rejects inconsistent imports.
    [[nodiscard]] Result<ImageHandle> import_image(std::string name, ImageDesc description);
    // Imports a buffer, coalescing compatible aliases of the same key; rejects inconsistent imports.
    [[nodiscard]] Result<BufferHandle> import_buffer(std::string name, BufferDesc description);
    // Appends one owned pass; invalid accesses are reported together with its label by compile().
    void add_pass(Pass pass);

    // Borrows immutable declarations while a scoped executor composes fixed-order subgraphs.
    [[nodiscard]] std::span<const Resource> resources() const { return m_resources; }

    [[nodiscard]] std::span<const Pass> passes() const { return m_passes; }

    // Validates access, tracks partial initialisation and plans barriers without modifying imports.
    [[nodiscard]] Result<Plan> compile(
        DeclarationRetention retention = DeclarationRetention::Discard
    ) const;

private:
    std::vector<Resource> m_resources;  // Canonical image and buffer declarations.
    std::vector<Pass> m_passes;         // Ordered work and export declarations.
};
}  // namespace openrac::renderer::graph
