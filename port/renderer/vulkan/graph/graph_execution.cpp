// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GraphExecution's borrowed allocation bindings and synchronous command recording.
#include "renderer/vulkan/graph/graph_execution.h"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

#include "common/breadcrumbs.h"
#include "common/profiler.h"
#include "renderer/graph/breadcrumbs.h"
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution_internal.h"
#include "renderer/vulkan/graph/graph_pipeline.h"
#include "renderer/vulkan/graph/surface_batch.h"

namespace openrac::vkgl {
namespace graph = renderer::graph;

namespace {
bool small_graph(const graph::Graph& builder) {
    return builder.resources().size() <= 64 && builder.passes().size() <= 32;
}

Result<void> validate_coupled_layouts(
    std::span<const graph::Pass> passes, std::span<const detail::GraphBinding> bindings
) {
    for (const auto& pass : passes) {
        for (std::size_t index = 0; index < pass.images.size(); ++index) {
            const auto& use = pass.images[index];
            const auto& image = *bindings[use.image.index].image;
            if (!detail::has_stencil(image.format)) {
                continue;
            }
            for (const auto& previous : std::span(pass.images).first(index)) {
                const auto& a = previous.range;
                const auto& b = use.range;
                if (previous.image == use.image && previous.state.layout != use.state.layout
                    && a.first_mip < b.first_mip + b.mip_count
                    && b.first_mip < a.first_mip + a.mip_count
                    && a.first_layer < b.first_layer + b.layer_count
                    && b.first_layer < a.first_layer + a.layer_count) {
                    return unexpected(Error{pass.name + ": depth and stencil require one layout"});
                }
            }
        }
    }
    return {};
}
}  // namespace

std::array<std::unique_ptr<GraphExecution::Recording>, GraphExecution::Recording::kSpareCount>&
GraphExecution::Recording::spares() {
    thread_local std::array<std::unique_ptr<Recording>, kSpareCount> recordings;
    return recordings;
}

GraphExecution::GraphExecution(std::source_location site) {
    auto& spares = Recording::spares();
    // Builders from different call sites often have very different pass and binding capacities.
    const auto spare = std::ranges::find_if(spares, [site](const auto& recording) {
        return recording && recording->site.line() == site.line()
               && recording->site.column() == site.column()
               && std::string_view(recording->site.file_name()) == site.file_name();
    });
    if (spare != spares.end()) {
        m_recording = std::move(*spare);
        return;
    }
    m_recording = std::make_unique<Recording>();
    m_recording->site = site;
}

GraphExecution::~GraphExecution() {
    if (!m_recording || !small_graph(m_recording->graph)) {
        return;
    }
    auto& spares = Recording::spares();
    const auto spare =
        std::ranges::find_if(spares, [](const auto& recording) { return !recording; });
    m_recording->reset();
    if (spare == spares.end()) {
        std::rotate(spares.begin(), spares.begin() + 1, spares.end());
        spares.back() = std::move(m_recording);
    } else {
        *spare = std::move(m_recording);
    }
}

void GraphExecution::Recording::reset() noexcept {
    // Recycle only empty builder storage. Bindings and packets retire at the usual scope boundary.
    packets.clear();
    bindings.clear();
    transient_images.clear();
    graph.clear();
    executed = false;
    attachment_fallback = false;
    trusted_reads = false;
    target = VK_NULL_HANDLE;
}

GraphExecution::GraphExecution(GraphExecution&&) noexcept = default;
GraphExecution& GraphExecution::operator=(GraphExecution&&) noexcept = default;

graph::Pass GraphExecution::make_pass(std::string_view name) {
    return m_recording->graph.make_pass(name);
}

void GraphExecution::add_pass(graph::Pass pass) {
    if (m_recording->executed) {
        throw std::logic_error("cannot append to an executed graph");
    }
    m_recording->graph.add_pass(std::move(pass));
}

Result<void> GraphExecution::add_pass(graph::Pass pass, graph::CommandBuffer commands) {
    if (m_recording->executed || commands.commands().empty()) {
        return unexpected(Error{pass.name + ": cannot attach an empty or late command buffer"});
    }
    if (auto owned = m_recording->graph.validate_handles(pass); !owned) {
        return owned;
    }
    auto valid = commands.validate(pass);
    if (!valid) {
        return valid;
    }
    auto bindings = validate_commands(commands);
    if (!bindings) {
        return bindings;
    }
    const auto index = static_cast<unsigned>(m_recording->graph.passes().size());
    m_recording->packets.push_back({index, std::move(commands)});
    try {
        add_pass(std::move(pass));
    } catch (...) {
        m_recording->packets.pop_back();
        throw;
    }
    return {};
}

Result<void> GraphExecution::add_dispatch_pass(graph::Pass pass, detail::PreparedDispatch packet) {
    if (auto owned = m_recording->graph.validate_handles(pass); !owned) {
        return owned;
    }
    auto valid = graph::validate_pipeline_command(pass, packet.command);
    if (!valid) {
        return valid;
    }
    // Surface preparation validated the immutable descriptors against this same pass.
    const auto index = static_cast<unsigned>(m_recording->graph.passes().size());
    m_recording->packets.push_back({index, std::move(packet)});
    try {
        add_pass(std::move(pass));
    } catch (...) {
        m_recording->packets.pop_back();
        throw;
    }
    return {};
}

// Records one work packet under its compiled resource contract.
Result<void> GraphExecution::record_work(
    unsigned index, const graph::PlannedPass& pass, const std::function<void(unsigned)>& record
) {
    using namespace detail;
    const bool main_stream =
        m_recording->target == VK_NULL_HANDLE || m_recording->target == frame().main;
    const GraphPipelineScope pipeline_pass{
        m_recording->graph.passes()[index],
        m_recording->bindings,
        pass,
        main_stream ? VK_NULL_HANDLE : m_recording->target
    };
    const auto* work = m_recording->work_at(index);
    if (const auto* commands = std::get_if<graph::CommandBuffer>(work)) {
        const auto recorded = execute(*commands);
        if (!recorded) {
            return recorded;
        }
    } else {
        const auto* dispatch = std::get_if<detail::PreparedDispatch>(work);
        if (!dispatch) {
            try {
                record(index);
            } catch (...) {
                // Work deferred by a failed callback must not escape its graph pass.
                g->gpu.pending_count = 0;
                throw;
            }
        } else {
            const stats::Scope timing(pass.name, stats::kGpu);
            record_prepared_dispatch(*dispatch);
        }
        // A deferred indirect batch belongs to this pass, even if the next pass
        // retains the same attachments and does not end rendering.
        if (main_stream) {
            gpu_flush_draws();
        }
    }
    return {};
}

// Retains allocations and records capture events around the ordered pass plan.
Result<void> GraphExecution::record_plan(
    const graph::Plan& plan, const std::function<void(unsigned)>& record, bool coupled_layouts
) {
    using namespace detail;
    const profiling::Zone timing("Record graph plan");
    const bool main_stream = m_recording->command() == frame().main;
    // Finish any batch queued by the enclosing draw before entering a nested pass.
    // Its commands belong to the enclosing scope and must precede the new plan's barriers.
    if (!plan.passes.empty() && main_stream) {
        gpu_flush_draws();
    }
    // Barriers retain imported images even if a later callback throws before binding descriptors.
    for (std::size_t index = 0; index < m_recording->bindings.size(); ++index) {
        auto& binding = m_recording->bindings[index];
        if ((plan.culled_passes != 0 || binding.transient_image) && !plan.resources[index].first) {
            continue;
        }
        // Buffer owners already retire through their existing stable-allocation path.
        if (binding.image) {
            // Upload-only images can stay on the upload stream until the main stream uses them.
            if (main_stream) {
                binding.image->touched = g->recording;
            }
            if (binding.discard_image) {
                discard_image_contents(*binding.image, whole_image(*binding.image));
            }
            frame().lifetime.retain(binding.image);
        }
    }

    // Legacy attachment recording may remain active between declared graphics passes.
    bool entry = true;
    for (unsigned index = 0; index < plan.passes.size(); ++index) {
        const auto& pass = plan.passes[index];
        if (pass.culled) {
            continue;
        }
        const profiling::Zone pass_timing(pass.name);
        graph::BreadcrumbScope operation{
            pass.kind == graph::PassKind::Work ? "graph pass" : "graph export", pass.name
        };
        if (pass.kind == graph::PassKind::Work && g->gpu_breadcrumbs) {
            record_gpu_breadcrumb(frame().main, pass.name, false);
        }
        const auto event = capture_graph_pass(plan, index, entry);
        entry = false;
        m_recording->enter(pass);
        const auto& declared = m_recording->graph.passes()[index];
        const auto& images = declared.images;
        const auto track_accesses = [&](bool completed) {
            for (const auto& use : images) {
                record_graph_image_access(
                    *m_recording->bindings[use.image.index].image, use, completed
                );
            }
        };
        if (coupled_layouts) {
            for (const auto& use : images) {
                auto& image = *m_recording->bindings[use.image.index].image;
                if (has_stencil(image.format)
                    && !image_has_layout(image, use.range, layout_of(use.state.layout))) {
                    if (main_stream) {
                        end_rendering();
                    }
                    record_coupled_image_layout(m_recording->command(), image, use);
                }
            }
        }

        // Exports make graph writes visible to subsequent legacy consumers without GPU work.
        if (pass.kind == graph::PassKind::Work) {
            if (std::ranges::any_of(declared.buffers, [](const auto& use) {
                    return use.state.access == graph::Access::Write
                           || use.state.access == graph::Access::ReadWrite;
                })) {
                note_buffer_write();
            }
            if (!g->buffer_visibility.empty()) {
                for (const auto& use : declared.buffers) {
                    if (use.state.access != graph::Access::Read) {
                        const auto buffer = m_recording->bindings[use.buffer.index].buffer.buffer;
                        invalidate_graph_buffer_reads(buffer, use.range);
                    }
                }
            }
            Result<void> recorded;
            const auto image_work = NativeCommandEncoder::image_work_count();
            const auto invalidate_failed_writes = [&] {
                // Rejected operations can emit barriers before validation fails. Only work
                // that could have written an image invalidates the declared outputs.
                if (NativeCommandEncoder::image_work_count() != image_work) {
                    track_accesses(false);
                }
            };
            try {
                recorded = record_work(index, pass, record);
            } catch (...) {
                invalidate_failed_writes();
                throw;
            }
            if (!recorded) {
                invalidate_failed_writes();
                abort_frame_capture(recorded.error().message);
                return recorded;
            }
            if (g->gpu_breadcrumbs) {
                record_gpu_breadcrumb(frame().main, pass.name, true);
            }
        }
        // A compatible read may have no barrier, but still participates in the next scope's hazards.
        track_accesses(true);
        complete_graph_pass(plan, index, event);
        operation.complete();
    }
    return {};
}

Result<void> GraphExecution::run(const std::function<void(unsigned)>& record) {
    using namespace detail;
    graph::BreadcrumbScope operation{"graph recording"};

    // Replaying imported entry states after GPU writes would use stale layouts and visibility scopes.
    if (m_recording->executed) {
        return unexpected(Error{"graph recording has already executed"});
    }

    // Surface preparation freezes compute bindings now and executes only after the shared compile.
    if (m_recording->target == VK_NULL_HANDLE && detail::collecting_surface_chain()) {
        auto collected = detail::collect_surface_graph(*this, record, detail::SurfaceWork::Compute);
        if (collected) {
            operation.complete();
        }
        return collected;
    }

    const auto execute_plan = [&](const graph::Plan& plan) -> Result<void> {
        // All callbacks and barriers belong to one active renderer context.
        const bool backend_recording =
            g && g->device != VK_NULL_HANDLE && m_recording->target != VK_NULL_HANDLE;
        if (!g || (!g->open && !backend_recording)
            || (!record && m_recording->needs_callback(&plan))) {
            return unexpected(
                Error{"graph execution requires an active context and work for every pass"}
            );
        }
        if (!m_recording->transient_images.empty()
            && (record || m_recording->target != VK_NULL_HANDLE)) {
            return unexpected(
                Error{"transient images require an owned main-stream transfer graph"}
            );
        }
        const bool coupled_layouts =
            std::ranges::any_of(m_recording->bindings, [](const auto& binding) {
                return binding.image && has_stencil(binding.image->format);
            });
        if (coupled_layouts) {
            const auto layouts =
                validate_coupled_layouts(m_recording->graph.passes(), m_recording->bindings);
            if (!layouts) {
                return layouts;
            }
        }
        // Backend errors stay inside this module and are returned to the provider as diagnostics.
        try {
            if (!m_recording->transient_images.empty()) {
                if (auto allocated = m_recording->allocate_transient_images(plan); !allocated) {
                    return allocated;
                }
            }
            if (plan.culled_passes != 0 && plan.culled_passes == plan.passes.size()) {
                m_recording->executed = true;
                operation.complete();
                return {};
            }
            prepare_frame();
            m_recording->executed = true;

            auto recorded = record_plan(plan, record, coupled_layouts);
            if (recorded) {
                operation.complete();
            }
            return recorded;
        } catch (const RecordingFailure& failure) {
            abort_frame_capture(failure.what());
            throw;
        } catch (const std::exception& failure) {
            abort_frame_capture(failure.what());
            return unexpected(Error{failure.what()});
        } catch (...) {
            abort_frame_capture("graph recording callback threw an unknown exception");
            return unexpected(Error{"graph recording callback threw an unknown exception"});
        }
    };

    const auto& builder = m_recording->graph;
    const auto retention = frame_capture_retention();
    // Leave transient upload ranges and diagnostic captures outside the recurring draw cache.
    if (g && g->open && retention == graph::DeclarationRetention::Discard
        && (m_recording->target == VK_NULL_HANDLE || m_recording->target == frame().main)
        && m_recording->transient_images.empty() && small_graph(builder)) {
        if (!m_recording->cache) {
            m_recording->cache.emplace(8);
        }
        // This recording owns the cache throughout execution, including nested callbacks.
        const auto compiled = m_recording->cache->compile_borrowed(builder);
        if (!compiled) {
            return unexpected(compiled.error());
        }
        return execute_plan(**compiled);
    }

    const auto compiled = builder.compile_into(m_recording->compiled, retention);
    if (!compiled) {
        return unexpected(compiled.error());
    }
    return execute_plan(m_recording->compiled);
}

const GraphExecution::Recording::Work* GraphExecution::Recording::work_at(unsigned index) const {
    const auto found = std::ranges::lower_bound(packets, index, {}, &Packet::index);
    return found != packets.end() && found->index == index ? &found->work : nullptr;
}

bool GraphExecution::Recording::needs_callback(const graph::Plan* plan) const {
    const auto passes = graph.passes();
    for (unsigned index = 0; index < passes.size(); ++index) {
        if (passes[index].kind == graph::PassKind::Work && (!plan || !plan->passes[index].culled)
            && !work_at(index)) {
            return true;
        }
    }
    return false;
}

Result<void> GraphExecution::run_prepared(const std::function<void(unsigned)>& record) {
    // Prepared callbacks explicitly retain their bindings through the enclosing execution scope.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Prepared);
    }
    return run(record);
}

Result<void> GraphExecution::run_prepared(const std::function<void()>& record) {
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Prepared);
    }
    if (!record) {
        return run();
    }
    return run([&record](unsigned) { record(); });
}

Result<void> GraphExecution::run_raster(const std::function<void(unsigned)>& record) {
    // Temporal shaders reuse programs and framebuffer names between distinct prepared passes.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Raster);
    }
    return run(record);
}

Result<void> GraphExecution::run_ordered(const std::function<void()>& record) {
    // Full bucket callbacks inherit live state only after all preceding declared work has executed.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Ordered);
    }
    if (!record) {
        return run();
    }
    return run([&record](unsigned) { record(); });
}

void GraphExecution::Recording::enter(const graph::PlannedPass& pass) {
    using namespace detail;
    const bool main_stream = target == VK_NULL_HANDLE || target == frame().main;
    const auto already_visible = [&](const graph::Barrier& barrier) {
        return main_stream && !g->buffer_visibility.empty()
               && std::holds_alternative<graph::BufferRange>(barrier.range)
               && graph_buffer_reads_visible(bindings.at(barrier.resource).buffer.buffer, barrier);
    };
    // A pass that only draws on into the open pass's own attachments, in the layout they are in,
    // goes on in that pass, as before the graph: draws in one pass are kept in order already, and
    // each new pass stores and loads the whole frame (MSAA too), which a tiled phone GPU pays dearly.
    const auto continues = [&](const graph::Barrier& barrier) {
        const auto& image = bindings.at(barrier.resource).image;
        const bool attachment = barrier.after.layout == graph::Layout::ColourAttachment
                                || barrier.after.layout == graph::Layout::DepthAttachment;
        return g->rendering && image && attachment && barrier.before.layout == barrier.after.layout
               && (image.get() == g->rendering_target.colour
                   || image.get() == g->rendering_target.depth);
    };
    const bool stale = main_stream && trusted_reads && !buffer_writes_visible();
    if (!stale && !(main_stream && pass.separate)
        && std::all_of(pass.barriers.begin(), pass.barriers.end(), [&](const auto& barrier) {
               return continues(barrier) || already_visible(barrier);
           })) {
        return;
    }
    const profiling::Zone timing("Record graph barriers");
    // Independent uploads/readbacks do not interrupt an open main-stream pass.
    if (main_stream) {
        end_rendering(PassEnd::Graph);
    }
    const auto command = this->command();
    GraphBarrierBatch batch(command);
    if (main_stream
        && (stale
            || (!buffer_writes_visible()
                && std::ranges::any_of(pass.barriers, [](const auto& transition) {
                       return std::holds_alternative<graph::BufferRange>(transition.range)
                              && transition.after.access == graph::Access::Read;
                   })))) {
        batch.add(buffer_writes_barrier());
        note_buffer_writes_visible();
    }

    // Exact buffer ranges allow one allocation to hold several independent depth mips.
    for (const auto& transition : pass.barriers) {
        auto& binding = bindings.at(transition.resource);

        // Images need layout transitions; buffers need only exact memory-range dependencies.
        if (binding.image) {
            if (binding.transient_image && transition.from_import
                && binding.image->layout != VK_IMAGE_LAYOUT_UNDEFINED) {
                // The graph sees undefined contents at each logical first use. The backing image
                // must finish its previous transfer accesses before the next lifetime writes it.
                auto handoff = transition;
                handoff.before = imported_state(binding.image->layout).value();
                handoff.before.access = graph::Access::ReadWrite;
                record_graph_image_barrier(command, handoff, *binding.image, 0, &batch);
            } else {
                record_graph_image_barrier(command, transition, *binding.image, 0, &batch);
            }
        } else if (!already_visible(transition)) {
            const auto& buffer =
                std::get<graph::BufferDesc>(graph.resources()[transition.resource].description);
            detail::record_graph_buffer_barrier(batch, transition, binding.buffer, buffer.kind);
        }
    }
}
}  // namespace openrac::vkgl
