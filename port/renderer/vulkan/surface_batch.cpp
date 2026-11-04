// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Prepares surface compute packets and records the compiled chain without changing shader
// algorithms.
#include <exception>
#include <stdexcept>

#include "renderer/surface_chain.h"
#include "renderer/vulkan/prepared_raster.h"
#include "renderer/vulkan/surface_batch_internal.h"

namespace openrac::vkgl::detail {
namespace {
namespace graph = renderer::graph;

// Invalidates native bind caches whenever a deferred raster callback restores its GL-shaped state.
void restore_state(const State& state) {
    g->state = state;
    g->bound_pipeline = VK_NULL_HANDLE;
    g->recorded.forget();
}

// Retains only shader exports; transfer boundaries inside the chain are derived from their actual
// consumers.
void shader_exports(graph::Pass& pass) {
    std::erase_if(pass.images, [](const auto& use) {
        return use.state.layout != graph::Layout::Sampled;
    });
}
}  // namespace

Result<SurfaceNode> SurfaceBatch::prepare_node(
    const graph::Pass& pass,
    unsigned index,
    const std::function<void(unsigned)>& record,
    SurfaceWork kind
) {
    // Raster setup includes mutable program values and framebuffer roles, not only GL bindings.
    if (kind == SurfaceWork::Raster) {
        return SurfaceNode{pass.name, [raster = PreparedRaster{}, record, index]() mutable {
                               raster.record([record, index] { record(index); });
                           }};
    }
    // Full bucket callbacks inherit prior bucket state rather than preparation-time intermediate bindings.
    if (kind == SurfaceWork::Ordered) {
        const State state = g->state;
        return SurfaceNode{pass.name, [this, state, record, index] {
                               restore_state(m_ordered_state.value_or(state));
                               record(index);
                               m_ordered_state = g->state;
                           }};
    }
    // Prepared callbacks retain stable bindings and their original fixed state through execution.
    if (kind == SurfaceWork::Prepared) {
        const State state = g->state;
        return SurfaceNode{pass.name, [state, record, index] {
                               restore_state(state);
                               record(index);
                           }};
    }
    return prepare_compute_node(pass, index, record);
}

Result<SurfaceNode> SurfaceBatch::prepare_compute_node(
    const graph::Pass& pass, unsigned index, const std::function<void(unsigned)>& record
) {
    // A compute callback may set uniforms and prepare dispatches, but cannot recursively collect
    // graphs.
    if (m_packet) {
        return unexpected(Error{"nested surface compute preparation"});
    }
    std::optional<PreparedDispatch> packet;
    m_packet = &packet;
    // A throwing setup must not leave a borrowed packet slot installed.
    try {
        record(index);
    } catch (...) {
        m_packet = nullptr;
        throw;
    }
    m_packet = nullptr;
    // A declared full writer cannot silently leave its output undefined.
    if (!packet) {
        return unexpected(Error{"surface compute pass did not prepare a dispatch: " + pass.name});
    }
    return SurfaceNode{pass.name, [packet = *packet] { record_prepared_dispatch(packet); }};
}

Result<void> SurfaceBatch::append_pass(
    const graph::Pass& source,
    unsigned index,
    const std::vector<unsigned>& handles,
    const std::function<void(unsigned)>& record,
    SurfaceWork kind
) {
    auto pass = remap(source, handles);
    // Invalid local handles cannot enter the combined plan.
    if (!pass) {
        return unexpected(pass.error());
    }
    // Shader consumers need only the final export after all chain producers.
    if (pass->kind == graph::PassKind::Export) {
        shader_exports(*pass);
        // Dropping transfer-only boundaries can leave an empty export.
        if (!pass->images.empty() || !pass->buffers.empty()) {
            m_exports.push_back(std::move(*pass));
        }
        return {};
    }
    auto node = prepare_node(*pass, index, record, kind);
    // A declared writer needs an executable packet before compilation.
    if (!node) {
        return unexpected(node.error());
    }
    m_execution.add_pass(std::move(*pass));
    m_nodes.push_back(std::move(*node));
    return {};
}

Result<void> SurfaceBatch::append(
    GraphExecution& source, const std::function<void(unsigned)>& record, SurfaceWork kind
) {
    // Failed preparation cannot publish a partial executable chain.
    if (m_failure) {
        return unexpected(*m_failure);
    }
    auto& recording = *source.m_recording;
    // Local recordings are single-use even when their execution is deferred.
    if (recording.executed || !record) {
        return unexpected(Error{"invalid surface graph callback"});
    }
    recording.executed = true;
    // Keep any preparation error sticky until the owning scope ends.
    try {
        const auto handles = import(recording);
        // Allocation contracts must agree across every local graph.
        if (!handles) {
            m_failure = handles.error();
            return unexpected(*m_failure);
        }
        // Preserve the original producer order while joining declarations.
        for (unsigned index = 0; index < recording.graph.passes().size(); ++index) {
            const auto added =
                append_pass(recording.graph.passes()[index], index, *handles, record, kind);
            // A partial chain cannot execute after one rejected pass.
            if (!added) {
                m_failure = added.error();
                return unexpected(*m_failure);
            }
        }
        return {};
    } catch (const std::exception& error) {
        m_failure = Error{error.what()};
    } catch (...) {
        m_failure = Error{"unknown surface graph preparation failure"};
    }
    return unexpected(*m_failure);
}

void SurfaceBatch::dispatch(PreparedDispatch& packet) {
    // Undeclared dispatches cannot bypass a deferred depth or AO producer.
    if (!m_packet) {
        throw std::runtime_error("surface dispatch has no declared compute pass");
    }
    // Inter-dispatch hazards require separate declared passes, not an implicit callback-local
    // barrier.
    if (*m_packet) {
        throw std::runtime_error("surface compute pass has multiple dispatches");
    }
    *m_packet = packet;
}

Result<void> SurfaceBatch::execute() {
    // A synchronous native decision may already have flushed the final section.
    if (m_nodes.empty() && m_exports.empty() && !m_failure) {
        return {};
    }
    // No GPU callbacks have executed if preparation failed.
    if (m_failure) {
        return unexpected(*m_failure);
    }
    const State final_state = g->state;
    // All shader consumers follow the complete chain, without intermediate broad export boundaries.
    for (auto& pass : m_exports) {
        m_execution.add_pass(std::move(pass));
    }
    const auto result = m_execution.run([&](unsigned pass) {
        const stats::Scope timing(m_nodes.at(pass).name, stats::kGpu);
        m_nodes[pass].record();
    });
    restore_state(m_ordered_state.value_or(final_state));
    // Count only chains that compiled and recorded successfully.
    if (result) {
        ++g->submission.surface_chains;
    }
    return result;
}

Result<void> SurfaceBatch::flush() {
    const auto result = execute();
    // Structural failures remain sticky; partially recorded sections cannot be retried.
    if (!result) {
        m_failure = result.error();
        return result;
    }
    m_execution = GraphExecution{};
    m_nodes.clear();
    m_exports.clear();
    m_ordered_state.reset();
    return {};
}

Result<void> flush_surface_chain() {
    // Immediate providers have no pending collected section.
    if (!collecting_surface_chain()) {
        return {};
    }
    auto* batch = g->surface_batch;
    g->surface_batch = nullptr;
    // Executing callbacks cannot recursively collect themselves, including on exceptions.
    try {
        const auto result = batch->flush();
        g->surface_batch = batch;
        return result;
    } catch (...) {
        g->surface_batch = batch;
        throw;
    }
}

bool collecting_surface_chain() {
    return g && g->surface_batch;
}

Result<void> collect_surface_graph(
    GraphExecution& source, const std::function<void(unsigned)>& record, SurfaceWork kind
) {
    return g->surface_batch->append(source, record, kind);
}

bool collect_surface_dispatch(PreparedDispatch& packet) {
    // Immediate callers retain their native dispatch or copy path.
    if (!collecting_surface_chain()) {
        return false;
    }
    g->surface_batch->dispatch(packet);
    return true;
}

void require_surface_execution() {
    // Preparation may upload independent inputs, but cannot draw, copy, clear or submit deferred
    // outputs.
    if (collecting_surface_chain()) {
        throw std::runtime_error("unrepresented GPU work during surface preparation");
    }
}
}  // namespace openrac::vkgl::detail

namespace openrac::renderer::backend {
bool collecting_surface_chain() {
    return vkgl::detail::collecting_surface_chain();
}

Result<void> run_surface_setup(const std::function<void()>& prepare) {
    using namespace vkgl::detail;
    const auto completed = flush_surface_chain();

    // Setup cannot inspect or modify a failed section's outputs.
    if (!completed) {
        return unexpected(completed.error());
    }

    auto* batch = g->surface_batch;
    g->surface_batch = nullptr;

    // Borrowed provider callbacks finish before collection resumes, including on exceptions.
    try {
        prepare();
    } catch (...) {
        g->surface_batch = batch;
        throw;
    }

    g->surface_batch = batch;
    return {};
}

Result<void> run_surface_chain(const std::function<void()>& prepare) {
    using namespace vkgl::detail;
    // Nested scopes would overwrite the active collector and its borrowed callbacks.
    if (g->surface_batch) {
        return unexpected(Error{"nested surface chain"});
    }
    SurfaceBatch batch;
    g->surface_batch = &batch;
    // Reset the borrowed scope on every error before any callbacks can escape the owning stack.
    try {
        prepare();
    } catch (...) {
        g->surface_batch = nullptr;
        throw;
    }
    g->surface_batch = nullptr;
    return batch.execute();
}

}  // namespace openrac::renderer::backend
