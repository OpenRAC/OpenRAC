// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements frame_context.h's frame-local identities and execution-order dependency composition.
#include "renderer/graph/frame_context.h"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace openrac::renderer::graph {
namespace {
// Returns the physical allocation key shared by every view in a frame.
ResourceKey key_of(const Resource& resource) {
    return std::visit([](const auto& value) { return value.key; }, resource.description);
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

void append_opaque_image(Pass& pass, ImageHandle handle, const ImageDesc& image) {
    const auto append = [&](ImageRange range, bool defined) {
        const auto discard =
            image.aspects == Aspect::Colour ? Layout::ColourAttachment : Layout::DepthAttachment;
        pass.images.push_back(
            {handle,
             range,
             {Stage::All, Access::Write, defined ? Layout::General : discard},
             Load::None,
             defined ? Store::Preserve : Store::Discard}
        );
    };
    if (image.initial_ranges.empty()) {
        append({image.aspects, 0, image.mips, 0, image.layers}, image.defined);
        return;
    }
    for (const auto aspect : {Aspect::Colour, Aspect::Depth, Aspect::Stencil}) {
        const auto bit = static_cast<unsigned>(aspect);
        if (!(static_cast<unsigned>(image.aspects) & bit)) {
            continue;
        }
        for (unsigned layer = 0; layer < image.layers; ++layer) {
            for (unsigned mip = 0; mip < image.mips; ++mip) {
                const auto initial =
                    std::ranges::find_if(image.initial_ranges, [&](const auto& entry) {
                        const auto& range = entry.range;
                        return (static_cast<unsigned>(range.aspects) & bit)
                               && mip >= range.first_mip && mip - range.first_mip < range.mip_count
                               && layer >= range.first_layer
                               && layer - range.first_layer < range.layer_count;
                    });
                append(
                    {aspect, mip, 1, layer},
                    initial == image.initial_ranges.end() ? image.defined
                                                          : initial->contents.defined
                );
            }
        }
    }
}
}  // namespace

// ---- Allocation identities ----

Result<std::vector<unsigned>> FrameContext::import(const Plan& source) {
    std::vector<unsigned> handles;
    handles.reserve(source.resources.size());

    // Imports from later subgraphs reuse the first frame-local identity, even with different entry
    // states.
    for (const auto& resource : source.resources) {
        if (source.culled_passes != 0 && !resource.first) {
            handles.push_back(std::numeric_limits<unsigned>::max());
            continue;
        }
        const auto found =
            std::find_if(m_resources.begin(), m_resources.end(), [&](const auto& item) {
                return key_of(item) == key_of(resource);
            });

        // Live allocation metadata cannot change without a new identity or generation.
        if (found != m_resources.end()) {
            // A recycled key would otherwise merge incompatible physical allocation histories.
            if (!found->same_shape(resource)) {
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

void FrameContext::append(Pass pass) {
    m_closed.push_back(false);
    m_kinds.push_back(pass.kind);
    // Each source validates its export contract locally; the composed stream allows later
    // subgraphs.
    pass.kind = PassKind::Work;
    pass.cull_if_unused = false;
    m_graph.add_pass(std::move(pass));
}

void FrameContext::opaque_boundary(const Plan& source, const std::vector<unsigned>& handles) {
    Pass boundary{"opaque imported contents (untracked commands)"};

    // Native entry contents are trusted only at an explicit boundary, never silently between
    // subgraphs.
    for (unsigned index = 0; index < source.resources.size(); ++index) {
        const auto& resource = source.resources[index];
        if (source.culled_passes != 0 && !resource.first) {
            continue;
        }

        // Image boundaries also retain the native discard contract.
        if (const auto* image = std::get_if<ImageDesc>(&resource.description)) {
            append_opaque_image(boundary, {handles[index], m_graph.owner()}, *image);
        } else {
            const auto& buffer = std::get<BufferDesc>(resource.description);

            // Buffer imports promise defined backing bytes; uninitialized ranges remain local to
            // producers.
            if (buffer.defined) {
                boundary.buffers.push_back(
                    {{handles[index], m_graph.owner()},
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
    if (source.passes[pass].culled) {
        return unexpected(Error{"cannot capture a culled pass"});
    }

    auto handles = import(source);

    // A callback cannot outlive or change its canonical allocation contract.
    if (!handles) {
        return unexpected(handles.error());
    }

    auto declaration = source.declarations[pass];
    const ResourceRemap remap{*handles, source.owner, m_graph.owner()};
    if (auto remapped = remap.apply(declaration); !remapped) {
        return unexpected(remapped.error());
    }

    // Opaque imports explicitly account for legacy producers before the first pass of a subgraph.
    if (entry == FrameEntry::Opaque) {
        opaque_boundary(source, *handles);
    }

    const unsigned event = static_cast<unsigned>(m_kinds.size());
    append(std::move(declaration));
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

    auto completion = source.declarations[pass];
    const ResourceRemap remap{*handles, source.owner, m_graph.owner()};
    if (auto remapped = remap.apply(completion); !remapped) {
        return unexpected(remapped.error());
    }

    completion.name += " (callback completion)";
    append(std::move(completion));
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
