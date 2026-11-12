// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Prepares surface compute packets and records the compiled chain without changing shader
// algorithms.
#include <exception>
#include <stdexcept>

#include "common/profiler.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/vulkan/graph/graph_pipeline.h"
#include "renderer/vulkan/graph/surface_batch_internal.h"

namespace openrac::vkgl::detail {
namespace {
namespace graph = renderer::graph;

struct SurfaceStorage {
    std::vector<SurfaceNode> nodes;
    std::vector<SurfaceExport> exports;
};

SurfaceStorage& spare_storage() {
    thread_local SurfaceStorage storage;
    return storage;
}

// Invalidates native bind caches whenever a deferred raster callback restores its GL-shaped state.
void restore_state(const State& state) {
    g->state = state;
    g->bound_pipeline = VK_NULL_HANDLE;
    g->recorded.forget();
}

bool covers_export(const graph::ImageUse& use, const graph::ImageUse& handoff) {
    const bool reads =
        use.state.access == graph::Access::Read || use.state.access == graph::Access::ReadWrite;
    const bool overwrites = use.state.access == graph::Access::Write
                            && (!use.partial_write || use.load == graph::Load::Clear);
    if (use.image != handoff.image || (!reads && !overwrites)) {
        return false;
    }
    const auto& full = use.range;
    const auto& part = handoff.range;
    const auto contains = [](unsigned first, unsigned count, unsigned start, unsigned length) {
        return start >= first && start - first <= count && length <= count - (start - first);
    };
    return (static_cast<unsigned>(part.aspects) & ~static_cast<unsigned>(full.aspects)) == 0
           && contains(full.first_mip, full.mip_count, part.first_mip, part.mip_count)
           && contains(full.first_layer, full.layer_count, part.first_layer, part.layer_count);
}
}  // namespace

SurfaceBatch::SurfaceBatch(std::source_location site)
    : m_execution(site),
      m_nodes(std::move(spare_storage().nodes)),
      m_exports(std::move(spare_storage().exports)) {}

SurfaceBatch::~SurfaceBatch() {
    if (g->surface_depth_owner == this) {
        g->surface_depth_owner = nullptr;
    }
    m_nodes.clear();
    m_exports.clear();
    auto& spare = spare_storage();
    if (m_nodes.capacity() > spare.nodes.capacity()) {
        spare.nodes.swap(m_nodes);
    }
    if (m_exports.capacity() > spare.exports.capacity()) {
        spare.exports.swap(m_exports);
    }
}

Result<SurfaceNode> SurfaceBatch::prepare_node(
    const graph::Pass& pass, unsigned index, SurfaceRecord record, SurfaceWork kind
) {
    // Raster setup includes mutable program values and framebuffer roles, not only GL bindings.
    if (kind == SurfaceWork::Raster) {
        return SurfaceNode{record.own(), PreparedRaster{}, index};
    }
    if (kind == SurfaceWork::Prepared || kind == SurfaceWork::Ordered) {
        return SurfaceNode{record.own(), g->state, index, kind == SurfaceWork::Ordered};
    }
    return unexpected(Error{pass.name + ": invalid raster work kind"});
}

Result<PreparedDispatch> SurfaceBatch::prepare_dispatch(
    const graph::Pass& pass, unsigned index, SurfaceRecord record
) {
    // A compute callback may set uniforms and prepare dispatches, but cannot recursively collect
    // graphs.
    if (m_packet) {
        return unexpected(Error{"nested surface compute preparation"});
    }
    std::optional<PreparedDispatch> packet;
    m_packet = &packet;
    const ComputeBindingScope bindings{pass, m_execution.m_recording->bindings};
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
    return *packet;
}

Result<void> SurfaceBatch::append_pass(
    const graph::Pass& source,
    const graph::CommandBuffer* commands,
    unsigned index,
    const graph::ResourceRemap& resources,
    SurfaceRecord record,
    SurfaceWork kind
) {
    auto pass = m_execution.make_pass(source.name);
    pass.images = source.images;
    pass.buffers = source.buffers;
    pass.kind = source.kind;
    pass.execution = source.execution;
    pass.cull_if_unused = source.cull_if_unused;
    pass.separate = source.separate;
    pass.compute_metadata_only = source.compute_metadata_only;
    pass.compute_indexed_storage = source.compute_indexed_storage;
    if (auto remapped = resources.apply(pass); !remapped) {
        return remapped;
    }
    // Exports describe the section boundary after all its work.
    if (pass.kind == graph::PassKind::Export) {
        if (!pass.images.empty() || !pass.buffers.empty()) {
            m_exports.push_back({std::move(pass), m_execution.m_recording->graph.passes().size()});
        }
        return {};
    }
    if (commands) {
        auto remapped = commands->remap(resources);
        if (!remapped) {
            return unexpected(remapped.error());
        }
        auto added = m_execution.add_pass(std::move(pass), std::move(*remapped));
        if (added) {
            // Keep the callback index aligned; GraphExecution runs this pass's owned commands.
            m_nodes.emplace_back();
        }
        return added;
    }
    if (kind == SurfaceWork::Compute) {
        auto packet = prepare_dispatch(pass, index, record);
        if (!packet) {
            return unexpected(packet.error());
        }
        const auto added = m_execution.add_dispatch_pass(std::move(pass), std::move(*packet));
        if (added) {
            m_nodes.emplace_back();
        }
        return added;
    }
    auto node = prepare_node(pass, index, record, kind);
    // A declared writer needs an executable packet before compilation.
    if (!node) {
        return unexpected(node.error());
    }
    m_execution.add_pass(std::move(pass));
    m_nodes.push_back(std::move(*node));
    return {};
}

Result<void> SurfaceBatch::append(GraphExecution& source, SurfaceRecord record, SurfaceWork kind) {
    const profiling::Zone timing("Collect surface graph");
    // Failed preparation cannot publish a partial executable chain.
    if (m_failure) {
        return unexpected(*m_failure);
    }
    auto& recording = *source.m_recording;
    if (!recording.transient_images.empty()) {
        return unexpected(Error{"transient images require a self-contained transfer graph"});
    }
    // Local recordings are single-use even when their execution is deferred.
    if (recording.executed || (!record && recording.needs_callback())) {
        return unexpected(Error{"invalid surface graph callback"});
    }
    recording.executed = true;
    // Keep any preparation error sticky until the owning scope ends.
    try {
        std::array<unsigned, 32> local_handles;
        std::vector<unsigned> extra_handles;
        const auto count = recording.graph.resources().size();
        std::span<unsigned> handles;
        if (count <= local_handles.size()) {
            handles = std::span(local_handles).first(count);
        } else {
            extra_handles.resize(count);
            handles = extra_handles;
        }
        const auto imported = import(recording, handles);
        // Allocation contracts must agree across every local graph.
        if (!imported) {
            m_failure = imported.error();
            return unexpected(*m_failure);
        }
        const graph::ResourceRemap resources{
            handles, recording.graph.owner(), m_execution.m_recording->graph.owner()
        };
        // Preserve the original producer order while joining declarations.
        for (unsigned index = 0; index < recording.graph.passes().size(); ++index) {
            const auto* commands = std::get_if<graph::CommandBuffer>(recording.work_at(index));
            const auto added = append_pass(
                recording.graph.passes()[index], commands, index, resources, record, kind
            );
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
    const profiling::Zone timing("Execute surface graph");
    // No GPU callbacks have executed if preparation failed.
    if (m_failure) {
        return unexpected(*m_failure);
    }
    const State final_state = g->state;
    // External consumers follow the complete section.
    const auto work_count = m_execution.m_recording->graph.passes().size();
    for (auto& [pass, after_work] : m_exports) {
        const auto later =
            m_execution.m_recording->graph.passes().first(work_count).subspan(after_work);
        // Only guaranteed work can replace an export that keeps its producer live.
        std::erase_if(pass.images, [&](const auto& use) {
            return use.continuation && std::ranges::any_of(later, [&](const auto& work) {
                       return !work.cull_if_unused
                              && std::ranges::any_of(work.images, [&](const auto& access) {
                                     return covers_export(access, use);
                                 });
                   });
        });
        m_execution.add_pass(std::move(pass));
    }
    const auto result = m_execution.run([&](unsigned pass) {
        auto& node = m_nodes.at(pass);
        const auto& name = m_execution.m_recording->graph.passes()[pass].name;
        const stats::Scope timing(name, stats::kGpu);
        const auto record = [&] {
            std::visit(
                [&](const auto& callback) { SurfaceRecord{callback}(node.source_index); },
                node.record
            );
        };
        if (auto* raster = std::get_if<PreparedRaster>(&node.snapshot)) {
            raster->record(record);
        } else {
            // Ordered buckets inherit the previous bucket's outgoing state.
            restore_state(
                node.ordered && m_ordered_state ? *m_ordered_state : std::get<State>(node.snapshot)
            );
            record();
            if (node.ordered) {
                m_ordered_state = g->state;
            }
        }
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
    m_execution.m_recording->reset();
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

Result<void> collect_surface_graph(GraphExecution& source, SurfaceRecord record, SurfaceWork kind) {
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

Result<void> run_surface_chain(const std::function<void()>& prepare, std::source_location site) {
    using namespace vkgl::detail;
    // Nested scopes would overwrite the active collector and its borrowed callbacks.
    if (g->surface_batch) {
        return unexpected(Error{"nested surface chain"});
    }
    SurfaceBatch batch(site);
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
