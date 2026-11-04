// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements frame_context.h's frame-local identities and execution-order dependency composition.
#include "renderer/graph/frame_context.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace openrac::renderer::graph {
namespace {
// Returns the physical allocation key shared by every view in a frame.
ResourceKey key_of(const Resource& resource) {
    return std::visit([](const auto& value) { return value.key; }, resource.description);
}

// Compares immutable allocation shape while retaining the first observed entry state and contents.
bool same_shape(const Resource& first, Resource next) {
    // An image and a buffer cannot describe the same live physical allocation key.
    if (first.description.index() != next.description.index()) {
        return false;
    }

    std::visit(
        [&](const auto& original) {
            using Description = std::decay_t<decltype(original)>;
            auto& value = std::get<Description>(next.description);
            value.initial = original.initial;
            value.defined = original.defined;
        },
        first.description
    );
    return first.description == next.description;
}

// Imports a new allocation through the planner's original typed validation.
Result<unsigned> import_new(Graph& builder, const Resource& resource) {
    return std::visit(
        [&](const auto& description) -> Result<unsigned> {
            using Description = std::decay_t<decltype(description)>;

            // Typed handles share canonical indices but retain separate validation at import.
            if constexpr (std::is_same_v<Description, ImageDesc>) {
                auto result = builder.import_image(resource.name, description);

                // Typed import failures cannot become valid frame-local handles.
                // Preserve the first failure instead of publishing a partial diagnostic plan.
                if (!result) {
                    return unexpected(result.error());
                }
                return result->index;
            } else {
                auto result = builder.import_buffer(resource.name, description);

                // Typed import failures cannot become valid frame-local handles.
                // Preserve the first failure instead of publishing a partial diagnostic plan.
                if (!result) {
                    return unexpected(result.error());
                }
                return result->index;
            }
        },
        resource.description
    );
}
}  // namespace

// ---- Allocation identities ----

Result<std::vector<unsigned>> FrameContext::import(const Plan& source) {
    std::vector<unsigned> handles;
    handles.reserve(source.resources.size());

    // Imports from later subgraphs reuse the first frame-local identity, even with different entry
    // states.
    for (const auto& resource : source.resources) {
        const auto found =
            std::find_if(m_resources.begin(), m_resources.end(), [&](const auto& item) {
                return key_of(item) == key_of(resource);
            });

        // Live allocation metadata cannot change without a new identity or generation.
        if (found != m_resources.end()) {
            // A recycled key would otherwise merge incompatible physical allocation histories.
            if (!same_shape(*found, resource)) {
                return unexpected(Error{"frame allocation shape changed: " + resource.name});
            }

            handles.push_back(static_cast<unsigned>(found - m_resources.begin()));
            continue;
        }

        const auto imported = import_new(m_graph, resource);
        // Invalid imports must stop diagnostic compilation before adding dependent frame accesses.
        if (!imported) {
            return unexpected(imported.error());
        }

        handles.push_back(*imported);
        m_resources.push_back(resource);
    }

    return handles;
}

Result<Pass> FrameContext::remap(const Pass& pass, const std::vector<unsigned>& handles) const {
    Pass result = pass;

    // Source handles are graph-local and must never be interpreted as frame indices directly.
    for (auto& use : result.images) {
        // Source indices must resolve within their own subgraph before frame remapping.
        if (use.image.index >= handles.size()) {
            return unexpected(Error{"unbound frame image in " + pass.name});
        }

        use.image.index = handles[use.image.index];
    }

    // Preserve byte offsets and sizes exactly while changing only the owning handle.
    for (auto& use : result.buffers) {
        // Source indices must resolve within their own subgraph before frame remapping.
        if (use.buffer.index >= handles.size()) {
            return unexpected(Error{"unbound frame buffer in " + pass.name});
        }

        use.buffer.index = handles[use.buffer.index];
    }

    return result;
}

void FrameContext::append(Pass pass) {
    m_closed.push_back(false);
    m_kinds.push_back(pass.kind);
    // Each source validates its export contract locally; the composed stream allows later
    // subgraphs.
    pass.kind = PassKind::Work;
    m_graph.add_pass(std::move(pass));
}

void FrameContext::opaque_boundary(const Plan& source, const std::vector<unsigned>& handles) {
    Pass boundary{"opaque imported contents (untracked commands)"};

    // Native entry contents are trusted only at an explicit boundary, never silently between
    // subgraphs.
    for (unsigned index = 0; index < source.resources.size(); ++index) {
        const auto& resource = source.resources[index];

        // Image boundaries also retain the native discard contract.
        if (const auto* image = std::get_if<ImageDesc>(&resource.description)) {
            // Undefined imports require explicit diagnostic invalidation, modeled as an attachment
            // discard.
            const Layout discard_layout = image->aspects == Aspect::Colour
                                              ? Layout::ColourAttachment
                                              : Layout::DepthAttachment;
            const Layout layout = image->defined ? Layout::General : discard_layout;
            boundary.images.push_back(
                {{handles[index]},
                 {image->aspects, 0, image->mips, 0, image->layers},
                 {Stage::All, Access::Write, layout},
                 Load::None,
                 image->defined ? Store::Preserve : Store::Discard}
            );
        } else {
            const auto& buffer = std::get<BufferDesc>(resource.description);

            // Buffer imports promise defined backing bytes; uninitialized ranges remain local to
            // producers.
            if (buffer.defined) {
                boundary.buffers.push_back(
                    {{handles[index]},
                     {0, buffer.size},
                     {Stage::All, Access::Write, Layout::Buffer}}
                );
            }
        }
    }

    append(std::move(boundary));
    m_closed.back() = true;
}

// ---- Execution events ----

Result<unsigned> FrameContext::record_event(const Plan& source, unsigned pass, FrameEntry entry) {
    // Normal execution plans need not retain source accesses unless frame capture is enabled.
    if (source.declarations.size() != source.passes.size() || pass >= source.declarations.size()) {
        return unexpected(Error{"frame capture requires retained source declarations"});
    }

    auto handles = import(source);

    // A callback cannot outlive or change its canonical allocation contract.
    if (!handles) {
        return unexpected(handles.error());
    }

    auto declaration = remap(source.declarations[pass], *handles);

    // Invalid source handles cannot contribute partial accesses to the frame plan.
    if (!declaration) {
        return unexpected(declaration.error());
    }

    // Opaque imports explicitly account for legacy producers before the first pass of a subgraph.
    if (entry == FrameEntry::Opaque) {
        opaque_boundary(source, *handles);
    }

    const unsigned event = static_cast<unsigned>(m_kinds.size());
    append(std::move(*declaration));
    return event;
}

Result<void> FrameContext::complete_event(const Plan& source, unsigned pass, unsigned event) {
    // Invalid or stale event indices cannot describe a callback completion.
    if (event >= m_kinds.size() || pass >= source.declarations.size()) {
        return unexpected(Error{"unbound frame callback completion"});
    }

    // Every callback event has exactly one completion in this frame.
    if (m_closed[event]) {
        return unexpected(Error{"frame callback already completed"});
    }

    m_closed[event] = true;

    // Without nested events the entry already covers the complete synchronous callback.
    if (event + 1 == m_kinds.size() || source.declarations[pass].kind == PassKind::Export) {
        return {};
    }

    auto handles = import(source);

    // A callback cannot outlive or change its canonical allocation contract.
    if (!handles) {
        return unexpected(handles.error());
    }

    auto completion = remap(source.declarations[pass], *handles);

    // Completion must name the same valid resources as the enclosing callback.
    if (!completion) {
        return unexpected(completion.error());
    }

    completion->name += " (callback completion)";
    append(std::move(*completion));
    m_closed.back() = true;
    return {};
}

Result<unsigned> FrameContext::record(const Plan& source, unsigned pass, FrameEntry entry) {
    // Earlier failures prevent a partial capture from becoming a successful frame dump.
    if (m_failure) {
        return unexpected(*m_failure);
    }

    auto result = record_event(source, pass, entry);

    // Preserve the first failure instead of publishing a partial diagnostic plan.
    if (!result) {
        m_failure = result.error();
    }

    return result;
}

Result<void> FrameContext::complete(const Plan& source, unsigned pass, unsigned event) {
    // Earlier failures prevent a partial capture from becoming a successful frame dump.
    if (m_failure) {
        return unexpected(*m_failure);
    }

    auto result = complete_event(source, pass, event);

    // Preserve the first failure instead of publishing a partial diagnostic plan.
    if (!result) {
        m_failure = result.error();
    }

    return result;
}

Result<Plan> FrameContext::compile() const {
    // Partial or failed callback recordings do not describe a completed displayed frame.
    if (m_failure) {
        return unexpected(*m_failure);
    }

    // Open callback scopes do not describe a completed presented frame.
    if (std::find(m_closed.begin(), m_closed.end(), false) != m_closed.end()) {
        return unexpected(Error{"frame capture contains an incomplete callback"});
    }

    auto result = m_graph.compile();

    // Preserve the first failure instead of publishing a partial diagnostic plan.
    if (!result) {
        return unexpected(result.error());
    }

    // Restore diagnostic export labels without changing dependency compilation or execution order.
    for (unsigned index = 0; index < m_kinds.size(); ++index) {
        result->passes[index].kind = m_kinds[index];
    }

    return result;
}
}  // namespace openrac::renderer::graph
