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
constexpr unsigned kLayoutCount = static_cast<unsigned>(Layout::Present) + 1;
using ReaderScopes = std::vector<std::array<Stage, kLayoutCount>>;

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
    if (source == destination) {
        return;
    }

    // Only edges emitted for this pass can be duplicates.
    for (auto edge = plan.dependencies.rbegin();
         edge != plan.dependencies.rend() && edge->destination == destination;
         ++edge) {
        if (edge->source == source && edge->resource == resource) {
            return;
        }
    }

    plan.dependencies.push_back({source, destination, resource});
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
Result<void> apply(
    Plan& plan, Span& span, const detail::Use& use, unsigned pass, Stage reader_scope
) {
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
        State destination = use.state;
        // Later readers can reuse this visibility. Track only actual accesses in span.state.
        if (destination.access == Access::Read) {
            destination.stages = reader_scope;
        }
        plan.passes[pass].barriers.push_back(
            {use.resource,
             detail::public_range(plan.resources[use.resource], span.begin, span.end),
             span.state,
             destination}
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
    } else if (span.readers.empty() || span.readers.back() != pass) {
        span.readers.push_back(pass);
    }

    span.state = after;
    return {};
}

// Splits a span at an interior boundary and returns the index of the right half.
std::size_t split_span(std::vector<Span>& spans, std::size_t index, std::uint64_t boundary) {
    Span right = spans[index];
    right.begin = boundary;
    spans[index].end = boundary;
    spans.insert(spans.begin() + index + 1, std::move(right));
    return index + 1;
}

// Updates touched intervals in place, copying history only when a boundary splits a span.
Result<void> apply_range(
    Plan& plan, std::vector<Span>& spans, const detail::Use& use, unsigned pass, Stage reader_scope
) {
    const auto first = std::upper_bound(
        spans.begin(), spans.end(), use.begin, [](std::uint64_t begin, const Span& span) {
            return begin < span.end;
        }
    );
    auto index = static_cast<std::size_t>(first - spans.begin());

    // Validated uses are nonempty and wholly within the span partition.
    if (spans[index].begin < use.begin) {
        index = split_span(spans, index, use.begin);
    }

    for (; index < spans.size() && spans[index].begin < use.end; ++index) {
        if (spans[index].end > use.end) {
            split_span(spans, index, use.end);
        }
        if (auto applied = apply(plan, spans[index], use, pass, reader_scope); !applied) {
            return unexpected(applied.error());
        }
    }
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
    const std::vector<Pass>& passes, const Plan& plan, ReaderScopes& readers
) {
    std::vector<std::vector<detail::Use>> result;
    result.reserve(passes.size());

    // Union read visibility by resource and layout, conservatively across ranges and later writes.
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
    plan.passes.reserve(m_passes.size());

    // Normal recording avoids copying diagnostic declarations on every subgraph compilation.
    if (retention == DeclarationRetention::Retain) {
        plan.declarations = m_passes;
    }
    ReaderScopes readers(plan.resources.size());
    const auto uses = pass_uses(m_passes, plan, readers);
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
        plan.passes.push_back({pass.name, pass.kind, {}, pass.separate});

        // Lifetimes include exports, since external consumers require the allocation to survive.
        for (const auto& use : (*uses)[index]) {
            auto& resource = plan.resources[use.resource];

            // Only the first declared use opens the inclusive lifetime.
            if (!resource.first) {
                resource.first = index;
            }

            resource.last = index;

            // No partial plan escapes when any range reads undefined contents.
            const auto scope = readers[use.resource][static_cast<unsigned>(use.state.layout)];
            if (auto applied = apply_range(plan, spans[use.resource], use, index, scope);
                !applied) {
                return unexpected(Error{pass.name + ": " + applied.error().message});
            }
        }
    }

    return plan;
}
}  // namespace openrac::renderer::graph
