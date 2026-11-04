// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GraphExecution's borrowed allocation bindings and synchronous command recording.
#include "renderer/vulkan/graph_execution.h"

#include <algorithm>
#include <bit>
#include <exception>
#include <utility>

#include "common/breadcrumbs.h"
#include "renderer/vulkan/graph_barriers.h"
#include "renderer/vulkan/graph_execution_internal.h"
#include "renderer/vulkan/surface_batch.h"

namespace openrac::vkgl {
namespace graph = renderer::graph;

GraphExecution::GraphExecution() : m_recording(std::make_unique<Recording>()) {}

GraphExecution::~GraphExecution() = default;
GraphExecution::GraphExecution(GraphExecution&&) noexcept = default;
GraphExecution& GraphExecution::operator=(GraphExecution&&) noexcept = default;

Result<graph::ImageHandle> GraphExecution::import_image(
    std::string name, unsigned texture, InitialContents contents
) {
    using namespace detail;

    // End pending attachment work before observing its completed layout and discard state.
    if (!g || !g->open) {
        return unexpected(Error{"graph image import requires Vulkan"});
    }

    const auto* found = g->textures.find(texture);
    if (found && found->image) {
        end_rendering_for(found->image.get());
    }

    // The legacy tracker cannot represent independently transitioned mip levels yet.
    if (!found || !found->image || found->image->levels != 1) {
        return unexpected(Error{"graph image import requires a single-level texture"});
    }

    return m_recording
        ->import_image(std::move(name), found->image, contents == InitialContents::Preserve);
}

Result<graph::ImageHandle> GraphExecution::Recording::import_image(
    std::string name, detail::ImagePtr image, bool defined, bool written
) {
    using namespace detail;
    const graph::Aspect aspect =
        image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? graph::Aspect::Depth : graph::Aspect::Colour;
    const VkImageAspectFlags sampled = image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT
                                           ? VK_IMAGE_ASPECT_DEPTH_BIT
                                           : VK_IMAGE_ASPECT_COLOR_BIT;
    const bool contents =
        defined
        && (written
            || (image->layout != VK_IMAGE_LAYOUT_UNDEFINED && !(image->discarded & sampled)));
    const auto state = imported_state(image->layout);

    // Unsupported legacy layouts report an import error before reaching frame recording.
    if (!state) {
        return unexpected(state.error());
    }

    const auto handle = graph.import_image(
        std::move(name),
        {{image->serial, 0}, static_cast<unsigned>(image->levels), 1, aspect, *state, contents}
    );

    // Aliased imports must agree on allocation metadata and entry contents.
    if (!handle) {
        return unexpected(handle.error());
    }

    bindings.resize(std::max(bindings.size(), std::size_t(handle->index) + 1));
    bindings[handle->index].image = std::move(image);
    return *handle;
}

Result<graph::ImageHandle> GraphExecution::import_attachment(
    std::string name, unsigned framebuffer, graph::Aspect aspect, InitialContents contents
) {
    using namespace detail;

    // Attachment lookup is only meaningful in the context that owns these names.
    if (!g || !g->open) {
        return unexpected(Error{"graph attachment import requires Vulkan"});
    }

    // Framebuffer lookup supports one colour or depth attachment, not arbitrary aspect combinations.
    if (aspect != graph::Aspect::Colour && aspect != graph::Aspect::Depth) {
        return unexpected(Error{"graph framebuffer import requires colour or depth aspect"});
    }

    // Preserved default colour may still be a deferred window copy; materialize it only when
    // contents are needed.
    if (framebuffer == 0 && aspect == graph::Aspect::Colour
        && contents == InitialContents::Preserve) {
        make_present_copy();
    }
    const bool depth = aspect == graph::Aspect::Depth;
    ImagePtr owner;

    // The default framebuffer is owned directly by the context.
    if (framebuffer == 0) {
        owner = depth ? g->default_depth : g->default_colour;
    } else if (const auto* target = g->framebuffers.find(framebuffer)) {
        const unsigned texture = depth ? target->depth_texture : target->colour_texture;
        const unsigned renderbuffer =
            depth ? target->depth_renderbuffer : target->colour_renderbuffer;

        // Texture attachments and renderbuffer attachments have different lifetime owners.
        if (const auto* found = g->textures.find(texture)) {
            owner = found->image;
        } else if (const auto* render = g->renderbuffers.find(renderbuffer)) {
            owner = render->image;
        }
    }

    // This executor shares the legacy whole-image layout tracker.
    if (!owner || owner->levels != 1) {
        return unexpected(Error{"invalid graph framebuffer attachment"});
    }

    // An attachment the open pass has drawn into already: the graph draws on in that pass (its
    // layout is the attachment's), rather than storing and loading the whole frame for the next.
    const bool open =
        g->rendering && g->pass.deferred
        && owner.get() == (depth ? g->rendering_target.depth : g->rendering_target.colour);
    const bool written = open
                         && (depth ? g->pass.used_depth || g->pass.clear_depth
                                   : g->pass.used_colour || g->pass.clear_colour);
    if (!written) {
        end_rendering_for(owner.get());
    }
    return m_recording->import_image(
        std::move(name), std::move(owner), contents == InitialContents::Preserve, written
    );
}

Result<graph::BufferHandle> GraphExecution::import_buffer(
    std::string name, unsigned buffer, InitialContents contents
) {
    using namespace detail;
    // Storage imports must keep one GPU allocation through all graph callbacks.
    auto* found = g && g->open ? g->buffers.find(buffer) : nullptr;

    // A streaming ring region could move during a callback, so imports require stable storage.
    if (!found || !found->storage || !found->size) {
        return unexpected(Error{"graph buffer import requires nonempty storage"});
    }

    prepare_frame();
    const BufferRef region = resolve(*found);
    // Writable imports cannot move through the frame ring while callbacks use their logical ranges.
    if (region.offset != 0 || region.buffer != found->own) {
        return unexpected(Error{"graph storage import has no stable allocation"});
    }
    return m_recording
        ->import_buffer(std::move(name), region, contents == InitialContents::Preserve);
}

Result<graph::BufferHandle> GraphExecution::Recording::import_buffer(
    std::string name, detail::BufferRef region, bool defined
) {
    using namespace detail;
    VkDeviceSize allocation_size = region.size;
    // Read-only upload regions share ring allocations; canonicalise the allocation, not each view.
    for (const auto& slot : g->frames) {
        for (const auto& chunk : slot.chunks) {
            if (chunk.buffer == region.buffer) {
                allocation_size = chunk.size;
            }
        }
    }
    if (region.offset > allocation_size || region.size > allocation_size - region.offset) {
        return unexpected(Error{"graph buffer region exceeds its allocation"});
    }
    // Buffer keys use a separate generation namespace from image serials.
    const auto handle = graph.import_buffer(
        std::move(name),
        {{std::bit_cast<std::uint64_t>(region.buffer), 1},
         allocation_size,
         {graph::Stage::All, graph::Access::ReadWrite, graph::Layout::Buffer},
         defined}
    );

    // Canonical allocation identity prevents aliases from receiving conflicting histories.
    if (!handle) {
        return unexpected(handle.error());
    }

    bindings.resize(std::max(bindings.size(), std::size_t(handle->index) + 1));
    bindings[handle->index].buffer = {region.buffer, 0, allocation_size};
    return *handle;
}

void GraphExecution::add_pass(graph::Pass pass) {
    m_recording->graph.add_pass(std::move(pass));
}

Result<void> GraphExecution::run(const std::function<void(unsigned)>& record) {
    using namespace detail;

    // Replaying imported entry states after GPU writes would use stale layouts and visibility scopes.
    if (m_recording->executed) {
        return unexpected(Error{"graph recording has already executed"});
    }

    // Surface preparation freezes compute bindings now and executes only after the shared compile.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Compute);
    }

    const auto compiled = m_recording->graph.compile(detail::frame_capture_retention());

    // Validation completes before any callback or compiled barrier is recorded.
    if (!compiled) {
        return unexpected(compiled.error());
    }

    // All callbacks and barriers belong to one active renderer context.
    if (!g || !g->open || !record) {
        return unexpected(Error{"graph execution requires an active context and callback"});
    }

    // Backend errors stay inside this module and are returned to the provider as diagnostics.
    try {
        prepare_frame();
        m_recording->executed = true;

        // Barriers retain imported images even if a later callback throws before binding descriptors.
        for (auto& binding : m_recording->bindings) {
            // Buffer owners already retire through their existing stable-allocation path.
            if (binding.image) {
                binding.image->touched = g->recording;
            }
        }

        // Legacy attachment recording may remain active between declared graphics passes.
        for (unsigned index = 0; index < compiled->passes.size(); ++index) {
            const auto& pass = compiled->passes[index];
            if (pass.kind == graph::PassKind::Work) {
                crash::breadcrumb(pass.name);
                if (g->gpu_breadcrumbs) {
                    record_gpu_breadcrumb(frame().main, pass.name, false);
                }
            }
            const auto event = capture_graph_pass(*compiled, index, index == 0);
            m_recording->enter(pass);

            // Exports make graph writes visible to subsequent legacy consumers without GPU work.
            if (pass.kind == graph::PassKind::Work) {
                m_recording->record_work(index, record);
                if (g->gpu_breadcrumbs) {
                    record_gpu_breadcrumb(frame().main, pass.name, true);
                }
            }
            complete_graph_pass(*compiled, index, event);
        }
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

    return {};
}

Result<void> GraphExecution::run_prepared(const std::function<void(unsigned)>& record) {
    // Prepared callbacks explicitly retain their bindings through the enclosing execution scope.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Prepared);
    }
    return run(record);
}

Result<void> GraphExecution::run_raster(const std::function<void(unsigned)>& record) {
    // Temporal shaders reuse programs and framebuffer names between distinct prepared passes.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Raster);
    }
    return run(record);
}

Result<void> GraphExecution::run_ordered(const std::function<void(unsigned)>& record) {
    // Full bucket callbacks inherit live state only after all preceding declared work has executed.
    if (detail::collecting_surface_chain()) {
        return detail::collect_surface_graph(*this, record, detail::SurfaceWork::Ordered);
    }
    return run(record);
}

void GraphExecution::Recording::record_work(
    unsigned index, const std::function<void(unsigned)>& record
) {
    recording = true;
    // The outer execution boundary converts all callback exceptions into Result errors.
    try {
        record(index);
    } catch (...) {
        recording = false;
        throw;
    }
    recording = false;
}

void GraphExecution::Recording::enter(const graph::PlannedPass& pass) {
    using namespace detail;
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
    if (std::all_of(pass.barriers.begin(), pass.barriers.end(), continues)) {
        return;
    }
    end_rendering();
    const auto command = frame().main;

    // Exact buffer ranges allow one allocation to hold several independent depth mips.
    for (const auto& transition : pass.barriers) {
        auto& binding = bindings.at(transition.resource);

        // Images need layout transitions; buffers need only exact memory-range dependencies.
        if (binding.image) {
            record_graph_image_barrier(command, transition, *binding.image);
        } else {
            detail::record_graph_buffer_barrier(command, transition, binding.buffer);
        }
    }
}
}  // namespace openrac::vkgl
