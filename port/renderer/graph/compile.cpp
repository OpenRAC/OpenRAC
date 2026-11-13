// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fixed-order compilation with partial contents tracking and all-reader dependency retention.
#include <algorithm>
#include <array>
#include <limits>
#include <utility>

#include "common/profiler.h"
#include "renderer/graph/breadcrumbs.h"
#include "renderer/graph/frame_graph.h"
#include "renderer/graph/graph_internal.h"

namespace openrac::renderer::graph {
namespace {
constexpr unsigned kLayoutCount = static_cast<unsigned>(Layout::Present) + 1;

// One nonoverlapping byte/cell interval; readers retain every pass since its last writer.
struct Span {
    std::uint64_t begin = 0;         // First included byte/cell.
    std::uint64_t end = 0;           // Exclusive end.
    State state;                     // Most recent write or union of compatible reads.
    bool defined = false;            // Whether its contents can be read.
    std::optional<unsigned> writer;  // Most recent graph writer, not an imported external writer.
    std::vector<unsigned> readers;   // Graph readers that must precede a later write/transition.
    bool accessed = false;
};

struct ResourceUses {
    std::array<Stage, kLayoutCount> reader_scopes{};
    std::vector<Span> spans;
    bool repeated = false;  // More than one normalised interval, possibly in the same pass.
};

using Producers = std::vector<std::pair<unsigned, unsigned>>;

struct CompileScratch {
    std::vector<ResourceUses> histories;
    std::vector<detail::Use> uses;
    std::vector<detail::Use> sorted;
    Producers producers;
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
    Plan& plan,
    Span& span,
    const detail::Use& use,
    unsigned pass,
    Stage reader_scope,
    Producers* producers
) {
    // A clear supplies defined attachment contents before a read/write operation.
    if (detail::reads(use.state.access) && !span.defined && use.load != Load::Clear) {
        return unexpected(
            Error{"read of unwritten contents: " + plan.resources[use.resource].name}
        );
    }

    // Liveness follows contents, so a full overwrite and ordering-only edges keep no producer.
    const bool preserves_partial =
        use.partial_write && use.load != Load::Discard && use.store == Store::Preserve;
    if (producers && span.writer && *span.writer != pass && use.load != Load::Clear
        && (detail::reads(use.state.access) || preserves_partial)) {
        producers->emplace_back(*span.writer, pass);
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
             destination,
             !span.accessed}
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
        span.defined = use.store != Store::Discard
                       && (use.load == Load::Clear || !use.partial_write
                           || (use.load != Load::Discard && span.defined));
    } else if (plan.resources[use.resource].last != pass
               && (span.readers.empty() || span.readers.back() != pass)) {
        span.readers.push_back(pass);
    }

    span.state = after;
    span.accessed = true;
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
    Plan& plan,
    std::vector<Span>& spans,
    const detail::Use& use,
    unsigned pass,
    Stage reader_scope,
    Producers* producers
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
        if (auto applied = apply(plan, spans[index], use, pass, reader_scope, producers);
            !applied) {
            return unexpected(applied.error());
        }
    }
    return {};
}

Span imported_span(const Resource& resource, std::uint64_t begin, std::uint64_t end) {
    return std::visit(
        [=](const auto& desc) { return Span{begin, end, desc.initial, desc.defined, {}, {}}; },
        resource.description
    );
}

void initialise_spans(const Resource& resource, std::vector<Span>& spans) {
    spans.push_back(imported_span(resource, 0, detail::extent(resource)));
    const auto* image = std::get_if<ImageDesc>(&resource.description);
    if (!image) {
        return;
    }
    for (const auto& entry : image->initial_ranges) {
        detail::for_each_image_interval(*image, entry.range, [&](auto begin, auto end) {
            const auto first = std::ranges::upper_bound(spans, begin, {}, &Span::end);
            auto index = static_cast<std::size_t>(first - spans.begin());
            if (spans[index].begin < begin) {
                index = split_span(spans, index, begin);
            }
            if (spans[index].end > end) {
                split_span(spans, index, end);
            }
            spans[index].state = entry.contents.state;
            spans[index].defined = entry.contents.defined;
        });
    }
}

Result<void> apply_use(
    Plan& plan, ResourceUses& history, const detail::Use& use, unsigned pass, Producers* producers
) {
    const auto& resource = plan.resources[use.resource];
    const auto scope = history.reader_scopes[static_cast<unsigned>(use.state.layout)];
    const auto* image = std::get_if<ImageDesc>(&resource.description);
    // A sole use needs only its exact range and the import's uniform entry state.
    if (!history.repeated && (!image || image->initial_ranges.empty())) {
        auto span = imported_span(resource, use.begin, use.end);
        return apply(plan, span, use, pass, scope, producers);
    }
    if (history.spans.empty()) {
        initialise_spans(resource, history.spans);
    }
    return apply_range(plan, history.spans, use, pass, scope, producers);
}

// Normalises all declarations before planning any barriers.
Result<void> pass_uses(const Graph& graph, const Plan& plan, CompileScratch& scratch) {
    const profiling::Zone timing("Normalize graph uses");
    const auto passes = graph.passes();
    auto& result = scratch.uses;
    result.clear();
    std::size_t declarations = 0;
    for (const auto& pass : passes) {
        declarations += pass.images.size() + pass.buffers.size();
    }
    result.reserve(declarations);

    // Keep normalized uses grouped in declaration order.
    for (unsigned index = 0; index < passes.size(); ++index) {
        const auto& pass = passes[index];
        if (auto owned = graph.validate_handles(pass); !owned) {
            return owned;
        }
        const auto first = result.size();
        if (auto normalised = detail::normalise(pass, plan.resources, result, scratch.sorted);
            !normalised) {
            return unexpected(Error{pass.name + ": " + normalised.error().message});
        }
        for (auto& use : std::span(result).subspan(first)) {
            use.pass = index;
        }
    }
    return {};
}

void find_lifetimes(Plan& plan, CompileScratch& scratch) {
    if (scratch.histories.size() < plan.resources.size()) {
        scratch.histories.resize(plan.resources.size());
    }
    for (std::size_t index = 0; index < plan.resources.size(); ++index) {
        plan.resources[index].first.reset();
        plan.resources[index].last.reset();
        auto& history = scratch.histories[index];
        history.reader_scopes.fill(Stage::None);
        history.spans.clear();
        history.repeated = false;
    }
    for (const auto& use : scratch.uses) {
        if (plan.passes[use.pass].culled) {
            continue;
        }
        auto& resource = plan.resources[use.resource];
        auto& history = scratch.histories[use.resource];
        if (resource.first) {
            history.repeated = true;
        } else {
            resource.first = use.pass;
        }
        resource.last = use.pass;
        if (use.state.access == Access::Read) {
            auto& stages = history.reader_scopes[static_cast<unsigned>(use.state.layout)];
            stages = stages | use.state.stages;
        }
    }
}

Result<void> plan_barriers(Plan& plan, CompileScratch& scratch, Producers* producers) {
    plan.dependencies.clear();
    for (auto& pass : plan.passes) {
        pass.barriers.clear();
    }
    find_lifetimes(plan, scratch);
    for (const auto& use : scratch.uses) {
        const auto& pass = plan.passes[use.pass];
        if (pass.culled) {
            continue;
        }
        if (auto applied =
                apply_use(plan, scratch.histories[use.resource], use, use.pass, producers);
            !applied) {
            return unexpected(Error{pass.name + ": " + applied.error().message});
        }
    }
    return {};
}

void cull_unused(Plan& plan, std::span<const Pass> declarations, const Producers& producers) {
    const profiling::Zone timing("Cull unused graph passes");
    for (unsigned index = 0; index < declarations.size(); ++index) {
        const auto& pass = declarations[index];
        plan.passes[index].culled = pass.kind == PassKind::Work && pass.cull_if_unused;
    }

    // Producers precede their consumers, so a single reverse walk propagates liveness to roots.
    auto edge = producers.rbegin();
    for (unsigned index = static_cast<unsigned>(declarations.size()); index-- > 0;) {
        const bool culled = plan.passes[index].culled;
        plan.culled_passes += culled;
        for (; edge != producers.rend() && edge->second == index; ++edge) {
            if (!culled) {
                plan.passes[edge->first].culled = false;
            }
        }
    }
}

// A validated plan is reusable across recordings with the same declarations and entry states.
bool same_pass(const Pass& cached, const Pass& current) {
    return cached.name == current.name && cached.kind == current.kind
           && cached.separate == current.separate && cached.execution == current.execution
           && cached.cull_if_unused == current.cull_if_unused
           && cached.compute_metadata_only == current.compute_metadata_only
           && cached.compute_indexed_storage == current.compute_indexed_storage
           && std::ranges::equal(
               cached.images,
               current.images,
               [](const auto& a, auto b) {
                   b.image.owner = a.image.owner;
                   return a == b;
               }
           )
           && std::ranges::equal(cached.buffers, current.buffers, [](const auto& a, auto b) {
                  b.buffer.owner = a.buffer.owner;
                  return a == b;
              });
}
}  // namespace

Result<Plan> Graph::compile(DeclarationRetention retention) const {
    Plan plan;
    const auto compiled = compile_into(plan, retention);
    if (!compiled) {
        return unexpected(compiled.error());
    }
    return plan;
}

Result<void> Graph::compile_into(Plan& plan, DeclarationRetention retention) const {
    const profiling::Zone timing("Compile render graph");
    BreadcrumbScope operation{"graph compile"};
    // Dependencies and lifetimes use the same index width as public handles.
    if (m_passes.size() > std::numeric_limits<unsigned>::max()) {
        return unexpected(Error{"frame graph has too many passes"});
    }

    const auto imports = resources();
    plan.owner = m_owner;
    plan.culled_passes = 0;
    plan.resources.assign(imports.begin(), imports.end());
    plan.passes.resize(m_passes.size());

    // Normal recording avoids copying diagnostic declarations on every subgraph compilation.
    if (retention == DeclarationRetention::Retain) {
        plan.declarations = m_passes;
    } else {
        plan.declarations.clear();
    }
    // Compilation invokes no recording callbacks. The scratch holds no bindings or plan data.
    thread_local CompileScratch scratch;
    const auto normalized = pass_uses(*this, plan, scratch);
    if (!normalized) {
        return normalized;
    }
    bool exporting = false;
    bool can_cull = false;

    // Preserve the renderer's declared order while deriving resource-specific dependencies.
    for (unsigned index = 0; index < m_passes.size(); ++index) {
        const auto& pass = m_passes[index];

        // Exports are boundary contracts; later work would invalidate their advertised states.
        if (pass.name.empty() || (pass.kind != PassKind::Work && pass.kind != PassKind::Export)
            || (exporting && pass.kind != PassKind::Export)) {
            return unexpected(Error{"unnamed pass or work declared after resource exports"});
        }

        exporting = pass.kind == PassKind::Export;
        auto& planned = plan.passes[index];
        planned.name = pass.name;
        planned.kind = pass.kind;
        planned.separate = pass.separate;
        planned.culled = false;
        can_cull |= pass.kind == PassKind::Work && pass.cull_if_unused;
    }

    // Validate the original graph even when some work will be discarded.
    scratch.producers.clear();
    const auto validated = plan_barriers(plan, scratch, can_cull ? &scratch.producers : nullptr);
    if (!validated) {
        return validated;
    }
    if (can_cull) {
        cull_unused(plan, m_passes, scratch.producers);
        if (plan.culled_passes != 0) {
            // Culled work contributes no barriers, dependencies or resource lifetimes.
            if (auto rebuilt = plan_barriers(plan, scratch, nullptr); !rebuilt) {
                return rebuilt;
            }
        }
    }

    operation.complete();
    return {};
}

PlanCache::PlanCache(std::size_t capacity) : m_entries(std::max<std::size_t>(capacity, 1)) {}

Result<std::shared_ptr<const Plan>> PlanCache::compile(const Graph& graph) {
    const auto result = find_or_compile(graph);
    if (!result) {
        return unexpected(result.error());
    }
    auto& entry = **result;
    entry.shared = true;
    return std::shared_ptr<const Plan>(entry.plan);
}

Result<const Plan*> PlanCache::compile_borrowed(const Graph& graph) {
    const auto result = find_or_compile(graph);
    if (!result) {
        return unexpected(result.error());
    }
    return (*result)->plan.get();
}

Result<PlanCache::Entry*> PlanCache::find_or_compile(const Graph& graph) {
    const profiling::Zone timing("Find validated graph plan");
    const auto matches_resource = [](const Resource& cached, const Resource& current) {
        return cached.name == current.name && cached.description == current.description;
    };
    auto* oldest = &m_entries.front();
    for (auto& entry : m_entries) {
        const auto& plan = entry.plan;
        // Entry contents and layouts can change even when the allocations and passes are stable.
        if (plan && std::ranges::equal(plan->declarations, graph.passes(), same_pass)
            && std::ranges::equal(plan->resources, graph.resources(), matches_resource)) {
            for (const auto& pass : graph.passes()) {
                if (auto owned = graph.validate_handles(pass); !owned) {
                    return unexpected(owned.error());
                }
            }
            const profiling::Zone reused("Reuse validated graph plan");
            entry.last_used = ++m_clock;
            return &entry;
        }
        if (entry.last_used < oldest->last_used) {
            oldest = &entry;
        }
    }

    if (!m_scratch) {
        m_scratch = std::make_shared<Plan>();
    }
    const auto compiled = graph.compile_into(*m_scratch, DeclarationRetention::Retain);
    if (!compiled) {
        return unexpected(compiled.error());
    }
    // Keep old entries through failed compiles. Only storage that was exclusively borrowed can
    // be recycled after eviction because published plans may still have shared or weak owners.
    oldest->plan.swap(m_scratch);
    if (oldest->shared) {
        m_scratch.reset();
    }
    oldest->shared = false;
    oldest->last_used = ++m_clock;
    return oldest;
}
}  // namespace openrac::renderer::graph
