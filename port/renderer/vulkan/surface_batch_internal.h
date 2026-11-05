// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Stack-owned preparation scope: shared allocation bindings, immutable compute packets and deferred
// native or raster work.
#pragma once
#include "renderer/vulkan/graph_execution_internal.h"
#include "renderer/vulkan/prepared_dispatch.h"
#include "renderer/vulkan/surface_batch.h"

namespace openrac::vkgl::detail {
// One callback executed after a successful shared compile, with no borrowed preparation locals.
struct SurfaceNode {
    std::string name;              // Stable pass label for GPU timing.
    std::function<void()> record;  // Owns frozen packets or a scoped prepared callback.
};

// Only the graphics owner uses this object, during one run_surface_chain call.
class SurfaceBatch {
public:
    ~SurfaceBatch() {
        if (g->surface_depth_owner == this) {
            g->surface_depth_owner = nullptr;
        }
    }

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
        GraphExecution& source, const std::function<void(unsigned)>& record, SurfaceWork kind
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
        unsigned index,
        const std::vector<unsigned>& handles,
        const std::function<void(unsigned)>& record,
        SurfaceWork kind
    );
    // Shares physical identities while preserving the chain's first entry state and immutable
    // allocation shape.
    [[nodiscard]] Result<std::vector<unsigned>> import(const GraphExecution::Recording& source);
    // Changes local handles without changing any byte or subresource ranges.
    [[nodiscard]] Result<renderer::graph::Pass> remap(
        const renderer::graph::Pass& pass, const std::vector<unsigned>& handles
    );
    // Freezes native dispatch state during the original synchronous setup callback.
    [[nodiscard]] Result<SurfaceNode> prepare_node(
        const renderer::graph::Pass& pass,
        unsigned index,
        const std::function<void(unsigned)>& record,
        SurfaceWork kind
    );
    // Runs compute setup once and retains its single immutable dispatch for later execution.
    [[nodiscard]] Result<SurfaceNode> prepare_compute_node(
        const renderer::graph::Pass& pass,
        unsigned index,
        const std::function<void(unsigned)>& record
    );
    GraphExecution m_execution;  // Owns the complete shared plan and its borrowed image bindings.
    std::vector<SurfaceNode>
        m_nodes;  // Work callbacks in the same order as the compiled work passes.
    std::vector<renderer::graph::Pass> m_exports;  // Shader-only consumers emitted after all work.
    std::optional<PreparedDispatch>* m_packet =
        nullptr;                     // Borrows the current preparation stack only.
    std::optional<Error> m_failure;  // Prevents executing a partial chain after preparation fails.
    std::optional<State> m_ordered_state;  // Last executed bucket's outgoing graphics state.
};
}  // namespace openrac::vkgl::detail
