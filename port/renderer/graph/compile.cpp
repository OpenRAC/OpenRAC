// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fixed-order compilation with partial contents tracking and all-reader dependency retention.
#include <algorithm>
#include <array>
#include <limits>
#include <utility>

#include "renderer/graph/frame_graph.h"
#include "renderer/graph/graph_internal.h"

namespace openrac::renderer::graph {
namespace {
// One nonoverlapping byte/cell interval; readers retain every pass since its last writer.
struct Span {
    std::uint64_t begin = 0;         // First included byte/cell.
    std::uint64_t end = 0;           // Exclusive end.
    State state;                     // Most recent write or union of compatible reads.
    bool defined = false;            // Whether its contents can be read.
    std::optional<unsigned> writer;  // Most recent graph writer, not an imported external writer.
    std::vector<unsigned> readers;   // Graph readers that must precede a later write/transition.
};

// Adds an ordering edge once, avoiding self-dependencies from duplicate read-only views.
void dependency(Plan& plan, unsigned source, unsigned destination, unsigned resource) {
    const Dependency edge{source, destination, resource};

    // Multiple overlapping ranges can establish the same edge.
    if (source != destination
        && std::none_of(
            plan.dependencies.begin(), plan.dependencies.end(), [edge](const Dependency& previous) {
                return previous == edge;
            }
        )) {
        plan.dependencies.push_back(edge);
    }
}

// Orders all accesses that can conflict, including layout changes between read-only passes.
void order(Plan& plan, const Span& span, const detail::Use& use, unsigned pass) {
    // Every consumer follows the producer of its current contents.
    if (span.writer) {
        dependency(plan, *span.writer, pass, use.resource);
    }

    // Writers and layout changes must wait for every overlapping reader.
    if (detail::writes(use.state.access) || span.state.layout != use.state.layout) {
        // Compatible reads accumulate until an access needs exclusive ownership.
        for (unsigned reader : span.readers) {
            dependency(plan, reader, pass, use.resource);
        }
    }
}

// Checks defined contents before emitting a barrier or changing the interval's history.
Result<void> apply(Plan& plan, Span& span, const detail::Use& use, unsigned pass) {
    // A clear supplies defined attachment contents before a read/write operation.
    if (detail::reads(use.state.access) && !span.defined && use.load != Load::Clear) {
        return unexpected(
            Error{"read of unwritten contents: " + plan.resources[use.resource].name}
        );
    }

    order(plan, span, use, pass);
    const bool layout_change = span.state.layout != use.state.layout;
    const bool hazard = span.state.access != Access::None
                        && (detail::writes(span.state.access) || detail::writes(use.state.access));

    // Memory hazards require barriers even when the image layout stays unchanged.
    if (layout_change || hazard) {
        plan.passes[pass].barriers.push_back(
            {use.resource,
             detail::public_range(plan.resources[use.resource], span.begin, span.end),
             span.state,
             use.state}
        );
    }

    State after = use.state;

    // A subsequent writer must wait for every compatible reader stage, not only the last one.
    if (!layout_change && span.state.access == Access::Read && after.access == Access::Read) {
        after.stages = after.stages | span.state.stages;
    }

    // A writer replaces the contents and completes the previous reader history.
    if (detail::writes(use.state.access)) {
        span.writer = pass;
        span.readers.clear();
        span.defined = use.store != Store::Discard;
    } else if (std::find(span.readers.begin(), span.readers.end(), pass) == span.readers.end()) {
        span.readers.push_back(pass);
    }

    span.state = after;
    return {};
}

// Splits only touched intervals, so partial buffer and image writes never initialise their neighbours.
Result<void> apply_range(
    Plan& plan, std::vector<Span>& spans, const detail::Use& use, unsigned pass
) {
    std::vector<Span> updated;
    updated.reserve(spans.size() + 2);

    // Preserve untouched intervals and split only at the requested boundaries.
    for (const auto& original : spans) {
        // Disjoint contents retain their earlier state and dependency history.
        if (original.end <= use.begin || original.begin >= use.end) {
            updated.push_back(original);
            continue;
        }

        Span middle = original;
        middle.begin = std::max(original.begin, use.begin);
        middle.end = std::min(original.end, use.end);

        // The prefix lies outside this access and must not become initialised by it.
        if (original.begin < middle.begin) {
            Span prefix = original;
            prefix.end = middle.begin;
            updated.push_back(std::move(prefix));
        }

        // Failure abandons the local compiled plan before any GPU recording.
        if (auto applied = apply(plan, middle, use, pass); !applied) {
            return unexpected(applied.error());
        }

        updated.push_back(std::move(middle));

        // A partial write must leave its trailing neighbour unchanged.
        if (original.end > use.end) {
            Span suffix = original;
            suffix.begin = use.end;
            updated.push_back(std::move(suffix));
        }
    }

    spans = std::move(updated);
    return {};
}

// Builds one initial interval per import without expanding large buffers or image arrays.
std::vector<std::vector<Span>> initial_spans(const Plan& plan) {
    std::vector<std::vector<Span>> result;
    result.reserve(plan.resources.size());

    // Large buffers need one initial interval, independently of their byte size.
    for (const auto& resource : plan.resources) {
        const auto state =
            std::visit([](const auto& value) { return value.initial; }, resource.description);
        const bool defined =
            std::visit([](const auto& value) { return value.defined; }, resource.description);
        result.push_back({Span{0, detail::extent(resource), state, defined, {}, {}}});
    }

    return result;
}

// Collects compatible reader scopes before recording, so a producer is visible to every later consumer.
Result<std::vector<std::vector<detail::Use>>> pass_uses(
    const std::vector<Pass>& passes, const Plan& plan
) {
    constexpr unsigned kLayoutCount = static_cast<unsigned>(Layout::Present) + 1;
    std::vector<std::array<Stage, kLayoutCount>> readers(plan.resources.size());
    std::vector<std::vector<detail::Use>> result;
    // A visibility barrier can cover several shader stages even though the first consumer uses one.
    for (const auto& pass : passes) {
        auto uses = detail::normalise(pass, plan.resources);
        if (!uses) {
            return unexpected(Error{pass.name + ": " + uses.error().message});
        }
        for (const auto& use : *uses) {
            if (use.state.access == Access::Read) {
                auto& stages = readers[use.resource][static_cast<unsigned>(use.state.layout)];
                stages = stages | use.state.stages;
            }
        }
        result.push_back(std::move(*uses));
    }
    // Unioning across disjoint intervals or later writes is conservative and preserves declared ordering.
    for (auto& uses : result) {
        for (auto& use : uses) {
            if (use.state.access == Access::Read) {
                use.state.stages = readers[use.resource][static_cast<unsigned>(use.state.layout)];
            }
        }
    }
    return result;
}
}  // namespace

Result<Plan> Graph::compile(DeclarationRetention retention) const {
    // Dependencies and lifetimes use the same index width as public handles.
    if (m_passes.size() > std::numeric_limits<unsigned>::max()) {
        return unexpected(Error{"frame graph has too many passes"});
    }

    Plan plan;
    plan.resources = m_resources;
    // Normal recording avoids copying diagnostic declarations on every subgraph compilation.
    if (retention == DeclarationRetention::Retain) {
        plan.declarations = m_passes;
    }
    const auto uses = pass_uses(m_passes, plan);
    if (!uses) {
        return unexpected(uses.error());
    }
    auto spans = initial_spans(plan);
    bool exporting = false;

    // Preserve the renderer's declared order while deriving resource-specific dependencies.
    for (unsigned index = 0; index < m_passes.size(); ++index) {
        const auto& pass = m_passes[index];

        // Exports are boundary contracts; later work would invalidate their advertised states.
        if (pass.name.empty() || (pass.kind != PassKind::Work && pass.kind != PassKind::Export)
            || (exporting && pass.kind != PassKind::Export)) {
            return unexpected(Error{"unnamed pass or work declared after resource exports"});
        }

        exporting = pass.kind == PassKind::Export;
        plan.passes.push_back({pass.name, pass.kind, {}});

        // Lifetimes include exports, since external consumers require the allocation to survive.
        for (const auto& use : (*uses)[index]) {
            auto& resource = plan.resources[use.resource];

            // Only the first declared use opens the inclusive lifetime.
            if (!resource.first) {
                resource.first = index;
            }

            resource.last = index;

            // No partial plan escapes when any range reads undefined contents.
            if (auto applied = apply_range(plan, spans[use.resource], use, index); !applied) {
                return unexpected(Error{pass.name + ": " + applied.error().message});
            }
        }
    }

    return plan;
}
}  // namespace openrac::renderer::graph
