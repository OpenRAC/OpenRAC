// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fixed-order frame dependencies. This CPU-only planner owns declarations, not GPU allocations
// or callbacks; executors apply its barriers around existing pass recording.
#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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
    Host = 256,
    AccelerationBuild = 512,
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

// Typed handles are valid only for the recording that imported them.
struct ImageHandle {
    unsigned index = std::numeric_limits<unsigned>::max();  // Unbound until imported.
    std::uint64_t owner = 0;
    bool operator==(const ImageHandle&) const = default;
};

struct BufferHandle {
    unsigned index = std::numeric_limits<unsigned>::max();  // Unbound until imported.
    std::uint64_t owner = 0;
    bool operator==(const BufferHandle&) const = default;
};

// Exact image subresources. Empty ranges are rejected.
struct ImageRange {
    Aspect aspects = Aspect::Colour;
    unsigned first_mip = 0;
    unsigned mip_count = 1;
    unsigned first_layer = 0;
    unsigned layer_count = 1;
    bool operator==(const ImageRange&) const = default;
};

struct ImageContents {
    State state{};
    bool defined = false;
    bool operator==(const ImageContents&) const = default;
};

struct ImageInitialRange {
    ImageRange range;
    ImageContents contents;
    bool operator==(const ImageInitialRange&) const = default;
};

// Import descriptions borrow allocation identity and state; their owners retain the GPU lifetime.
struct ImageDesc {
    ResourceKey key{};                // Shared by aliases of this allocation.
    unsigned mips = 1;                // Number of mip levels, not dimensions in pixels.
    unsigned layers = 1;              // Number of array layers.
    Aspect aspects = Aspect::Colour;  // Aspects present in this allocation.
    State initial{};                  // Access completed or pending before graph recording.
    bool defined = false;             // Default contents for subresources outside initial_ranges.
    std::vector<ImageInitialRange> initial_ranges{};  // Nonoverlapping overrides of the defaults.
    bool operator==(const ImageDesc&) const = default;
};

// Acceleration structures and build scratch use different access types from ordinary shader data.
enum class BufferKind : std::uint8_t {
    Data,
    AccelerationStructure,
    AccelerationScratch
};

struct BufferDesc {
    ResourceKey key;         // Shared by aliases of this allocation.
    std::uint64_t size = 0;  // Allocation size in bytes.
    State initial{Stage::None, Access::None, Layout::Buffer};  // Prior GPU access.
    bool defined = false;  // Whether every byte contains valid data.
    BufferKind kind = BufferKind::Data;  // Fixed allocation role, independent of execution stage.
    bool operator==(const BufferDesc&) const = default;
};

// Exact buffer byte intervals; empty ranges are rejected.
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
    bool partial_write = false;    // A write to only part of each selected subresource.
    bool continuation = false;     // Later declared work may replace this export in a joined plan.
    bool operator==(const ImageUse&) const = default;
};

struct BufferUse {
    BufferHandle buffer;  // Imported allocation.
    BufferRange range;    // Byte interval accessed by this pass.
    State state;          // Required execution and access with layout Buffer.
    bool operator==(const BufferUse&) const = default;
};

// Owned pass declarations, executed in insertion order; no draw packets or resource owners are retained.
struct Pass {
    std::string name;                  // Stable diagnostic label, not a per-draw label.
    std::vector<ImageUse> images{};    // All image accesses, including attachments.
    std::vector<BufferUse> buffers{};  // All buffer accesses.
    PassKind kind = PassKind::Work;    // Exports follow all work passes.
    Stage execution = Stage::None;     // Work with no resources still declares its pipeline class.
    // Only opt in for work whose outputs and effects are fully visible to this graph.
    bool cull_if_unused = false;
    // Metadata kernels bind dummy samplers. Indexed kernels address explicit storage intervals.
    bool compute_metadata_only = false;
    bool compute_indexed_storage = false;
    bool separate = false;             // Never joins the open render pass on its attachments.
    bool operator==(const Pass&) const = default;
};

// Changes ownership as well as indices when composing graph declarations.
struct ResourceRemap {
    std::span<const unsigned> indices;
    std::uint64_t source_owner;
    std::uint64_t destination_owner;

    template <typename Handle>
    [[nodiscard]] bool rebind(Handle& handle) const {
        if (!source_owner || handle.owner != source_owner || handle.index >= indices.size()) {
            return false;
        }
        handle = {indices[handle.index], destination_owner};
        return true;
    }

    [[nodiscard]] Result<void> apply(Pass& pass) const;
};

// Copies a pass's ranges into a read-only shader export, preserving its sampled input and output coverage.
// Images become sampled with no attachment operations; buffers retain their exact byte ranges.
[[nodiscard]] Pass shader_read_export(std::string name, const Pass& source, Stage stages);
// Fills a pass with the export contract, reusing its declaration storage.
void shader_read_export(Pass& destination, const Pass& source, Stage stages);

// A barrier covers one aspect/layer's mip interval, or one contiguous buffer interval.
using Range = std::variant<ImageRange, BufferRange>;

struct Barrier {
    unsigned resource = 0;  // Canonical resource index in Plan::resources.
    Range range;            // Exact overlapping subresources or bytes.
    State before;           // Includes all prior overlapping readers.
    State after;            // State required by the next use.
    bool from_import = false;  // No earlier use in this plan touched the range.
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
    [[nodiscard]] bool same_shape(const Resource& other) const;
};

// Entry barriers and kind for one pass; recording remains the executor's responsibility.
struct PlannedPass {
    std::string name;                // Copied from its declaration.
    PassKind kind = PassKind::Work;  // Exports execute barriers only.
    std::vector<Barrier> barriers;   // Applied before this pass.
    bool separate = false;           // Copied from its declaration.
    bool culled = false;             // Keeps the declaration index for surviving callbacks.
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
    std::uint64_t owner = 0;         // Owner of the retained declarations' handles.
    unsigned culled_passes = 0;
    // Returns a deterministic text dump; does no logging or file I/O.
    [[nodiscard]] std::string describe() const;
};

// One frame's declarations. No sorting, allocation aliasing, queue submission or Vulkan dependency.
class Graph {
public:
    Graph() = default;
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph(Graph&& other) noexcept;
    Graph& operator=(Graph&& other) noexcept;

    // Imports an image, coalescing compatible aliases of the same key; rejects inconsistent imports.
    [[nodiscard]] Result<ImageHandle> import_image(std::string_view name, const ImageDesc& description);
    // Imports a buffer, coalescing compatible aliases of the same key; rejects inconsistent imports.
    [[nodiscard]] Result<BufferHandle> import_buffer(std::string_view name, BufferDesc description);
    // Appends one owned pass; invalid accesses are reported together with its label by compile().
    void add_pass(Pass pass);

    // Takes storage retained from a previous recording when available.
    [[nodiscard]] Pass make_pass(std::string_view name);

    // Releases all declarations while keeping capacity for the next recording.
    void clear() noexcept;

    // Borrows immutable declarations while a scoped executor composes fixed-order subgraphs.
    [[nodiscard]] std::span<const Resource> resources() const {
        return std::span(m_resources).first(m_resource_count);
    }

    [[nodiscard]] std::span<const Pass> passes() const { return m_passes; }
    [[nodiscard]] std::uint64_t owner() const { return m_owner; }
    [[nodiscard]] Result<void> validate_handles(const Pass& pass) const;

    // Validates access, tracks partial initialisation and plans barriers without modifying imports.
    [[nodiscard]] Result<Plan> compile(
        DeclarationRetention retention = DeclarationRetention::Discard
    ) const;
    // Reuses a plan's backing storage. The result must succeed before the plan is read.
    [[nodiscard]] Result<void> compile_into(Plan& plan, DeclarationRetention retention) const;

private:
    std::vector<Resource> m_resources;  // Canonical image and buffer declarations.
    std::size_t m_resource_count = 0;
    std::vector<Pass> m_passes;         // Ordered work and export declarations.
    std::vector<Pass> m_spare_passes;
    std::uint64_t m_owner = 0;  // Assigned on the first import and reset when the graph is cleared.
};

// Per recording thread. Retaining a plan keeps it valid across nested recordings and eviction.
class PlanCache {
public:
    explicit PlanCache(std::size_t capacity = 32);
    PlanCache(const PlanCache&) = delete;
    PlanCache& operator=(const PlanCache&) = delete;
    [[nodiscard]] Result<std::shared_ptr<const Plan>> compile(const Graph& graph);
    // The caller must finish recording before this cache is used again.
    [[nodiscard]] Result<const Plan*> compile_borrowed(const Graph& graph);

private:
    struct Entry {
        std::shared_ptr<Plan> plan;
        std::uint64_t last_used = 0;
        bool shared = false;
    };
    [[nodiscard]] Result<Entry*> find_or_compile(const Graph& graph);

    std::vector<Entry> m_entries;
    std::shared_ptr<Plan> m_scratch;
    std::uint64_t m_clock = 0;
};
}  // namespace openrac::renderer::graph
