// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Stack-owned preparation scope: shared allocation bindings, immutable compute packets and deferred
// native or raster work.
#pragma once
#include "renderer/vulkan/graph/graph_execution_internal.h"
#include "renderer/vulkan/graph/prepared_dispatch.h"
#include "renderer/vulkan/graph/prepared_raster.h"
#include "renderer/vulkan/graph/surface_batch.h"

namespace openrac::vkgl::detail {
// One callback executed after a successful shared compile, with no borrowed preparation locals.
struct SurfaceNode {
    SurfaceCallback record;
    std::variant<State, PreparedRaster> snapshot;
    unsigned source_index = 0;
    bool ordered = false;
};

struct SurfaceExport {
    renderer::graph::Pass pass;
    std::size_t after_work;
};

// Only the graphics owner uses this object, during one run_surface_chain call.
class SurfaceBatch {
public:
    explicit SurfaceBatch(std::source_location site);
    ~SurfaceBatch();

    // A lease spans all sections of this scope, including synchronous flushes.
    bool claim_depth_scratch() {
        if (g->surface_depth_owner) {
            return false;
        }
        g->surface_depth_owner = this;
        return true;
    }

    // Imports source allocations and prepares compute packets, or retains an explicitly scoped
    // prepared callback.
    [[nodiscard]] Result<void> append(
        GraphExecution& source, SurfaceRecord record, SurfaceWork kind
    );
    // Receives one immutable dispatch from the currently preparing declared compute pass.
    void dispatch(PreparedDispatch& packet);
    // Compiles once and publishes ordered bucket state, or preparation's final state without ordered work.
    [[nodiscard]] Result<void> execute();
    // Executes one complete section and starts a fresh declaration set after a successful SDK boundary.
    [[nodiscard]] Result<void> flush();

private:
    // Appends one remapped work node or defers its shader boundary until all producers are
    // declared.
    [[nodiscard]] Result<void> append_pass(
        const renderer::graph::Pass& source,
        const renderer::graph::CommandBuffer* commands,
        unsigned index,
        const renderer::graph::ResourceRemap& resources,
        SurfaceRecord record,
        SurfaceWork kind
    );
    // Shares physical identities while preserving the chain's first entry state and immutable
    // allocation shape.
    [[nodiscard]] Result<void> import(
        const GraphExecution::Recording& source, std::span<unsigned> handles
    );
    // Freezes native dispatch state during the original synchronous setup callback.
    [[nodiscard]] Result<SurfaceNode> prepare_node(
        const renderer::graph::Pass& pass,
        unsigned index,
        SurfaceRecord record,
        SurfaceWork kind
    );
    // Runs compute setup once and retains its single immutable dispatch for later execution.
    [[nodiscard]] Result<PreparedDispatch> prepare_dispatch(
        const renderer::graph::Pass& pass,
        unsigned index,
        SurfaceRecord record
    );
    GraphExecution m_execution;  // Owns the complete shared plan and its borrowed image bindings.
    std::vector<SurfaceNode>
        m_nodes;  // Work callbacks in the same order as the compiled work passes.
    std::vector<SurfaceExport> m_exports;  // Shader boundaries and their original positions.
    std::optional<PreparedDispatch>* m_packet =
        nullptr;                     // Borrows the current preparation stack only.
    std::optional<Error> m_failure;  // Prevents executing a partial chain after preparation fails.
    std::optional<State> m_ordered_state;  // Last executed bucket's outgoing graphics state.
};
}  // namespace openrac::vkgl::detail
