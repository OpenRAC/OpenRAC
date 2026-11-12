// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Canonical allocation bindings and entry contents for Vulkan graph recordings.
#include <algorithm>
#include <bit>

#include "common/profiler.h"
#include "common/stats.h"
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/graph/graph_execution_internal.h"
#include "renderer/vulkan/graph/surface_batch.h"

namespace openrac::vkgl {
namespace graph = renderer::graph;

Result<graph::ImageHandle> GraphExecution::create_transient_colour(
    std::string_view name, std::array<int, 2> extent
) {
    using namespace detail;
    if (!g || !g->open || m_recording->executed || detail::collecting_surface_chain()
        || !image_extent_valid(extent[0], extent[1])) {
        return unexpected(Error{"invalid transient colour image or recording scope"});
    }
    // Owned transfer commands validate against this metadata before a backing image exists.
    auto image = std::make_shared<Image>();
    image->serial = ++g->image_serial;
    image->format = format_of(gl::GL_RGBA8).vk;
    image->usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image->width = extent[0];
    image->height = extent[1];
    const auto handle = m_recording->import_image(name, std::move(image), false);
    if (handle) {
        m_recording->transient_images.push_back(handle->index);
        m_recording->bindings[handle->index].transient_image = true;
    }
    return handle;
}

Result<void> GraphExecution::Recording::allocate_transient_images(const graph::Plan& plan) {
    using namespace detail;
    const profiling::Zone timing("Allocate transient graph images");
    for (const auto& pass : graph.passes()) {
        for (const auto& use : pass.images) {
            if (!bindings[use.image.index].transient_image) {
                continue;
            }
            const bool transfer = use.state.layout == graph::Layout::TransferSource
                                  || use.state.layout == graph::Layout::TransferDestination;
            if (pass.kind != graph::PassKind::Work || !transfer || use.partial_write) {
                return unexpected(
                    Error{pass.name + ": transient images require complete owned transfers"}
                );
            }
        }
    }

    // First-fit over inclusive lifetimes. Images in the same pass must remain independent.
    std::ranges::sort(transient_images, {}, [&](unsigned index) {
        return plan.resources[index].first;
    });

    struct Slot {
        ImagePtr image;
        unsigned last;
    };

    std::vector<Slot> slots;
    slots.reserve(transient_images.size());
    std::vector<std::pair<unsigned, std::size_t>> placements;
    placements.reserve(transient_images.size());
    for (const auto index : transient_images) {
        const auto& lifetime = plan.resources[index];
        if (!lifetime.first) {
            continue;
        }
        const auto& request = *bindings[index].image;
        auto slot = std::ranges::find_if(slots, [&](const Slot& candidate) {
            return candidate.last < *lifetime.first && candidate.image->width == request.width
                   && candidate.image->height == request.height;
        });
        if (slot == slots.end()) {
            auto image = make_image(
                format_of(gl::GL_RGBA8), request.width, request.height, 1, VK_SAMPLE_COUNT_1_BIT
            );
            if (!image) {
                return unexpected(Error{"could not allocate transient graph image"});
            }
            placements.emplace_back(index, slots.size());
            slots.push_back({std::move(image), *lifetime.last});
        } else {
            slot->last = *lifetime.last;
            placements.emplace_back(index, static_cast<std::size_t>(slot - slots.begin()));
        }
    }

    // Publish bindings only after all allocations succeeded.
    for (const auto& [index, slot] : placements) {
        bindings[index].image = slots[slot].image;
    }
    if (stats::enabled()) {
        VkDeviceSize bytes = 0;
        for (const auto& slot : slots) {
            VmaAllocationInfo info{};
            vmaGetAllocationInfo(g->allocator, slot.image->allocation, &info);
            bytes += info.size;
        }
        stats::gauge("graph transient image bytes", double(bytes));
        stats::gauge("graph transient logical images", double(placements.size()));
        stats::gauge("graph transient backing images", double(slots.size()));
    }
    return {};
}

Result<graph::ImageHandle> GraphExecution::import_image(
    std::string_view name, unsigned texture, InitialContents contents
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

    if (!found || !found->image) {
        return unexpected(Error{"graph image import requires a texture allocation"});
    }

    return m_recording->import_image(name, found->image, contents == InitialContents::Preserve);
}

Result<graph::ImageHandle> GraphExecution::Recording::import_image(
    std::string_view name, detail::ImagePtr image, bool defined, VkImageAspectFlags written
) {
    using namespace detail;
    const graph::Aspect aspect = static_cast<graph::Aspect>(image->aspect);
    const VkImageAspectFlags sampled = image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT
                                           ? VK_IMAGE_ASPECT_DEPTH_BIT
                                           : VK_IMAGE_ASPECT_COLOR_BIT;
    auto& description = image_description;
    description.key = image->serial
                          ? graph::ResourceKey{image->serial, 0}
                          : graph::ResourceKey{std::bit_cast<std::uint64_t>(image->image), 2};
    description.mips = static_cast<unsigned>(image->levels);
    description.layers = image->layers;
    description.aspects = aspect;
    description.initial_ranges.clear();

    if (tracks_subresources(*image)) {
        description.initial = image->uniform_contents.state;
        description.defined = defined && image->uniform_contents.defined;
        if (!image->subresource_contents.empty()) {
            for (auto plane :
                 {graph::Aspect::Colour, graph::Aspect::Depth, graph::Aspect::Stencil}) {
                if (!(image->aspect & static_cast<unsigned>(plane))) {
                    continue;
                }
                for (unsigned layer = 0; layer < description.layers; ++layer) {
                    for (unsigned mip = 0; mip < description.mips;) {
                        auto entry = image_contents(*image, plane, mip, layer);
                        entry.defined &= defined;
                        const unsigned first = mip++;
                        while (mip < description.mips) {
                            auto next = image_contents(*image, plane, mip, layer);
                            next.defined &= defined;
                            if (next != entry) {
                                break;
                            }
                            ++mip;
                        }
                        if (entry
                            != graph::ImageContents{description.initial, description.defined}) {
                            description.initial_ranges.push_back(
                                {{plane, first, mip - first, layer}, entry}
                            );
                        }
                    }
                }
            }
        }
    } else {
        const auto state = imported_state(image->layout);
        if (!state) {
            return unexpected(state.error());
        }
        description.initial = *state;
        const bool has_layout = image->layout != VK_IMAGE_LAYOUT_UNDEFINED;
        const auto has_contents = [&](VkImageAspectFlags plane) {
            return defined && ((written & plane) || (has_layout && !(image->discarded & plane)));
        };
        description.defined = has_contents(sampled);
        if (image->aspect & VK_IMAGE_ASPECT_STENCIL_BIT) {
            const bool stencil = has_contents(VK_IMAGE_ASPECT_STENCIL_BIT);
            if (stencil != description.defined) {
                description.initial_ranges.push_back({{graph::Aspect::Stencil}, {*state, stencil}});
            }
        }
    }

    const auto handle = graph.import_image(name, description);

    // Aliased imports must agree on allocation metadata and entry contents.
    if (!handle) {
        return unexpected(handle.error());
    }

    bindings.resize(std::max(bindings.size(), std::size_t(handle->index) + 1));
    bindings[handle->index] = {std::move(image), {}, !defined};
    return *handle;
}

Result<graph::ImageHandle> GraphExecution::import_attachment(
    std::string_view name, unsigned framebuffer, graph::Aspect aspect, InitialContents contents
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

    // The GL framebuffer path attaches one level and one layer.
    if (!owner || owner->levels != 1 || owner->layers != 1) {
        return unexpected(Error{"invalid graph framebuffer attachment"});
    }

    // An attachment the open pass has drawn into already: the graph draws on in that pass (its
    // layout is the attachment's), rather than storing and loading the whole frame for the next.
    const bool open =
        g->rendering && g->pass.deferred
        && owner.get() == (depth ? g->rendering_target.depth : g->rendering_target.colour);
    const auto& pass = g->pass;
    const bool depth_active =
        !pass.drop_depth && (pass.used_depth || pass.clear_depth || pass.clear_stencil);
    const bool colour_active = !pass.drop_colour && (pass.used_colour || pass.clear_colour);
    VkImageAspectFlags written = 0;
    if (open && (depth ? depth_active : colour_active)) {
        written = depth ? pass.written_depth | (pass.clear_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0u)
                              | (pass.clear_stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u)
                        : VK_IMAGE_ASPECT_COLOR_BIT;
    } else {
        end_rendering_for(owner.get());
    }
    return m_recording
        ->import_image(name, std::move(owner), contents == InitialContents::Preserve, written);
}

Result<graph::BufferHandle> GraphExecution::import_buffer(
    std::string_view name, unsigned buffer, InitialContents contents, BufferVisibility visibility
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
    auto handle = m_recording->import_buffer(name, region, contents == InitialContents::Preserve);
    if (handle && visibility == BufferVisibility::TrackShaderReads) {
        g->buffer_visibility.try_emplace(region.buffer);
    }
    return handle;
}

Result<graph::BufferHandle> GraphExecution::Recording::import_buffer(
    std::string_view name, detail::BufferRef region, bool defined, bool shader_read
) {
    using namespace detail;
    VkDeviceSize allocation_size = region.size;
    bool read_only = false;
    // Read-only upload regions share ring allocations; canonicalise the allocation, not each view.
    for (const auto& slot : g->frames) {
        if (slot.bulk_texture_staging.buffer == region.buffer) {
            allocation_size = slot.bulk_texture_staging.size;
            read_only = true;
        }
        for (const auto& chunk : slot.chunks) {
            if (chunk.buffer == region.buffer) {
                allocation_size = chunk.size;
                read_only = true;
            }
        }
    }
    const auto owned = g->owned_buffer_writable.find(region.buffer);
    const bool known = owned != g->owned_buffer_writable.end();
    read_only |= known && !owned->second;
    if (region.offset > allocation_size || region.size > allocation_size - region.offset) {
        return unexpected(Error{"graph buffer region exceeds its allocation"});
    }
    // GL-owned buffers are used for host upload, transfer, shader storage/uniforms, and
    // vertex/index/indirect input. Raw native allocations have no such provenance here.
    constexpr auto gl_stages = graph::Stage::Host | graph::Stage::Transfer
                               | graph::Stage::VertexInput | graph::Stage::DrawIndirect
                               | graph::Stage::VertexShader | graph::Stage::FragmentShader
                               | graph::Stage::ComputeShader;
    // Buffer keys use a separate generation namespace from image serials.
    const graph::ResourceKey key{std::bit_cast<std::uint64_t>(region.buffer), 1};
    graph::State initial{
        known ? gl_stages : graph::Stage::All,
        read_only ? graph::Access::Read : graph::Access::ReadWrite,
        graph::Layout::Buffer
    };
    if (!read_only) {
        // An earlier import owns the allocation's entry state, including when shader and
        // writable views of the same buffer appear in different passes.
        const auto resources = graph.resources();
        const auto previous = std::ranges::find_if(resources, [&](const auto& resource) {
            const auto* buffer = std::get_if<graph::BufferDesc>(&resource.description);
            return buffer && buffer->key == key;
        });
        if (previous != resources.end()) {
            initial = std::get<graph::BufferDesc>(previous->description).initial;
        } else if (known && shader_read && defined && buffer_writes_visible()) {
            initial.access = graph::Access::Read;
            trusted_reads = true;
        }
    }
    const auto handle = graph.import_buffer(name, {key, allocation_size, initial, defined});

    // Canonical allocation identity prevents aliases from receiving conflicting histories.
    if (!handle) {
        return unexpected(handle.error());
    }

    bindings.resize(std::max(bindings.size(), std::size_t(handle->index) + 1));
    bindings[handle->index].buffer = {region.buffer, 0, allocation_size};
    return *handle;
}

}  // namespace openrac::vkgl
