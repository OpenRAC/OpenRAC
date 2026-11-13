// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame's work for the rest of the backend (vkgl_internal.h): what waits for the frame's end,
// the ring that carries vertices, indices, uniforms and texels to the GPU, the two command buffers
// and the order between them, GPU timing, submission, and the render target drawing goes to.

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <volk.h>

#include "common/breadcrumbs.h"
#include "common/crash.h"
#include "common/env.h"
#include "common/profiler.h"
#include "common/stats.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#include "renderer/vulkan/commands/graph_queue.h"
#include "renderer/vulkan/graph/graph_barriers.h"
#include "renderer/vulkan/graph/graph_immediate_work.h"
#include "renderer/vulkan/graph/graph_pipeline.h"
#include "renderer/vulkan/graph/surface_batch.h"
#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl::detail {

using namespace openrac::gl;

// Reports a terminal recording failure at the renderer's existing exception boundary.
void require_success(VkResult result, const char* operation) {
    // Continuing could submit invalid handles or wait for work that was never queued.
    if (!check(result, operation)) {
        if (g) {
            const auto state = std::format(
                "Vulkan state: frame {}, slot {}, recording {}, queue family {}, swapchain {}x{}, "
                "submissions {}, validation errors {}",
                g->frame_count,
                g->frame,
                g->recording,
                g->family,
                g->swapchain_extent.width,
                g->swapchain_extent.height,
                g->submission.submits,
                g->validation_errors.load()
            );
            crash::note_vulkan(state);
            log::error("{}", state);
        }
        throw RecordingFailure(std::format("vulkan: {} failed ({})", operation, int(result)));
    }
}

namespace {

namespace render_opt = renderer::render_opt;

// The stages and accesses of what an image's layout is used for: what a barrier out of the layout
// waits for, and what a barrier into it makes wait (vk-tight-sync; docs/port/RENDER_PERF.md, "R-08",
// the table of barriers). A layout the backend does not use waits for everything, as before.
struct LayoutScope {
    VkPipelineStageFlags stages;
    VkAccessFlags accesses;
};

// The scope of `layout`. The shader stages are the vertex, fragment and compute ones (a texture is
// sampled by each); a depth image is loaded in the early tests, stored in the late ones and
// resolved where colour is written.
LayoutScope scope_of(VkImageLayout layout) {
    constexpr VkPipelineStageFlags kShaders = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                              | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                              | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    constexpr VkPipelineStageFlags kDepthStages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                                  | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                                                  | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    constexpr VkAccessFlags kDepthAccess =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    constexpr VkAccessFlags kColourAccess =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    switch (layout) {
        // Nothing to wait for: the contents are not kept.
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0};
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return {kShaders, VK_ACCESS_SHADER_READ_BIT};
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, kColourAccess};
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return {kDepthStages, kDepthAccess | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
        // Any other: everything, the reference path's scope.
        default:
            return {
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT
            };
    }
}

// ---- The frame's ring: vertices, indices, uniforms, texels on their way ----

VkDeviceSize ring_alignment() {
    const VkPhysicalDeviceLimits& l = g->properties.limits;
    return std::max<VkDeviceSize>(
        {256,
         l.minUniformBufferOffsetAlignment,
         l.minTexelBufferOffsetAlignment,
         l.optimalBufferCopyOffsetAlignment,
         16}
    );
}

bool allocate_chunk(Chunk& c, VkDeviceSize size) {
    c.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = c.size;
    info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                 | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
                 | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;
    alloc.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc.requiredFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VmaAllocationInfo got{};
    if (!check(
            vmaCreateBuffer(g->allocator, &info, &alloc, &c.buffer, &c.allocation, &got),
            "a ring buffer"
        )) {
        return false;
    }
    c.mapped = static_cast<std::uint8_t*>(got.pMappedData);
    return true;
}

bool new_chunk(Frame& f, VkDeviceSize size) {
    Frame::UploadChunk c;
    if (!allocate_chunk(c, std::max(size, kChunk))) {
        return false;
    }
    f.chunks.push_back(c);
    ++g->measures.ring_allocations;
    return true;
}

// ---- Where drawing goes ----

Image* image_of(GLuint texture, GLuint renderbuffer) {
    if (Texture* t = g->textures.find(texture); t != nullptr && t->image) {
        return t->image.get();
    }
    if (Renderbuffer* r = g->renderbuffers.find(renderbuffer); r != nullptr && r->image) {
        return r->image.get();
    }
    return nullptr;
}

struct Clear {
    bool colour = false, depth = false, stencil = false;
};

VkImageAspectFlags depth_aspects(bool depth, bool stencil) {
    return (depth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0u) | (stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u);
}

void note_colour_write(const Target& target) {
    g->pass.used_colour = true;
    g->pass.drop_colour = 0;
    if (target.colour && !g->pass.deferred) {
        target.colour->discarded &= ~VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

void note_depth_write(const Target& target, VkImageAspectFlags aspects) {
    if (target.depth && aspects) {
        auto& pass = g->pass;
        pass.written_depth |= aspects;
        pass.drop_depth &= ~aspects;
        if (!pass.deferred) {
            target.depth->discarded &= ~aspects;
        }
    }
}

// The garbage test_poison_discards writes over discarded data: every colour channel at its top,
// depth at the near plane and a stencil pattern, none of them what a pass of the port clears to.
constexpr VkClearColorValue kPoisonColour = {{1.0f, 1.0f, 1.0f, 1.0f}};
constexpr VkClearDepthStencilValue kPoisonDepth = {0.0f, 0xA5};

// Overwrites the `aspects` of `image` with the poison, outside any pass, in `main` (the order of
// the reads a later pass makes). Only for test_poison_discards.
void poison(Image& image, VkImageAspectFlags aspects) {
    VkCommandBuffer cmd = cmd_for(&image, true);
    if (!tracks_subresources(image)) {
        barrier(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    }
    const VkImageSubresourceRange range = {aspects, 0, 1, 0, 1};

    // A colour image, or the depth and stencil one.
    if ((aspects & VK_IMAGE_ASPECT_COLOR_BIT) != 0) {
        GraphImmediateWork::clear_image(cmd, image, range, kPoisonColour);
    } else {
        GraphImmediateWork::clear_image(cmd, image, range, {}, kPoisonDepth);
    }

    count_command(Command::Copy);
}

// Notes that `aspects` of `image` hold nothing worth keeping, and with the poison on, makes them
// garbage at once. Called once the pass that drew into them has ended, or outside any pass.
void mark_discarded(Image& image, VkImageAspectFlags aspects) {
    // Nothing let go: the image keeps what it holds.
    if (aspects == 0) {
        return;
    }

    // The poison's clear moves the image to a transfer layout, which barrier counts as a write.
    if (g->poison_discards) {
        poison(image, aspects);
    }

    image.discarded |= aspects;
    discard_image_contents(image, {static_cast<renderer::graph::Aspect>(aspects)});
}

// The load operation of one aspect of an attachment: a clear the pass began with, nothing for
// contents that were let go (the poison's clear under test_poison_discards), else a load.
VkAttachmentLoadOp load_op(const Image& image, VkImageAspectFlags aspect, bool cleared) {
    // The clear the pass began with comes first: it overwrites whatever was there.
    if (cleared) {
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }

    // Let go: no load, or under test the poison, so that a pass that reads it differs.
    if ((image.discarded & aspect) != 0) {
        return g->poison_discards ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    return VK_ATTACHMENT_LOAD_OP_LOAD;
}

// The secondary command buffer for this recording's next deferred pass into `t`, begun to
// continue a pass of `t`'s formats and samples. Allocated from the frame's pool the first time.
VkCommandBuffer secondary_for(const Target& t) {
    Frame& f = frame();

    // A buffer of an earlier recording of this slot, reset with the pool, or a new one.
    if (f.secondary_at == f.secondaries.size()) {
        VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc.commandPool = f.pool;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        alloc.commandBufferCount = 1;
        VkCommandBuffer made = VK_NULL_HANDLE;

        // No secondary buffer: the caller records the pass directly.
        if (!check(vkAllocateCommandBuffers(g->device, &alloc, &made), "a secondary buffer")) {
            return VK_NULL_HANDLE;
        }

        f.secondaries.push_back(made);
    }

    VkCommandBuffer cmd = f.secondaries[f.secondary_at++];
    const VkFormat colour = t.colour != nullptr ? t.colour->format : VK_FORMAT_UNDEFINED;
    VkCommandBufferInheritanceRenderingInfo rendering{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO
    };
    rendering.colorAttachmentCount = t.colour != nullptr ? 1 : 0;
    rendering.pColorAttachmentFormats = &colour;
    rendering.depthAttachmentFormat = t.depth != nullptr ? t.depth->format : VK_FORMAT_UNDEFINED;
    rendering.stencilAttachmentFormat =
        t.depth != nullptr && has_stencil(t.depth->format) ? t.depth->format : VK_FORMAT_UNDEFINED;
    rendering.rasterizationSamples = t.samples;
    VkCommandBufferInheritanceInfo inheritance{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    inheritance.pNext = &rendering;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
                  | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin.pInheritanceInfo = &inheritance;
    vkBeginCommandBuffer(cmd, &begin);
    return cmd;
}

// Moves `t`'s attachments into their layouts for drawing, in `cmd` before the pass.
void attachment_barriers(VkCommandBuffer cmd, const Target& t) {
    // The colour image, also when it is in that layout already: an earlier pass's writes are done.
    if (t.colour != nullptr) {
        t.colour->touched = g->recording;
        barrier(cmd, *t.colour, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    // The depth and stencil image.
    if (t.depth != nullptr) {
        t.depth->touched = g->recording;
        barrier(cmd, *t.depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
}

// Begins rendering into `t` as recording goes, the reference path: every attachment stored, and
// loaded unless `clear` clears it.
void begin_direct(const Target& t, const Clear& clear) {
    VkCommandBuffer cmd = frame().main;
    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo stencil{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingInfo info{VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.renderArea =
        {{0, 0}, {static_cast<std::uint32_t>(t.width), static_cast<std::uint32_t>(t.height)}};
    info.layerCount = 1;
    const State& s = g->state;
    const VkExtent2D area = info.renderArea.extent;
    attachment_barriers(cmd, t);
    if (t.colour != nullptr) {
        colour.imageView = attachment_view(*t.colour);
        colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colour.loadOp = clear.colour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        std::memcpy(colour.clearValue.color.float32, s.clear_colour, sizeof s.clear_colour);
        info.colorAttachmentCount = 1;
        info.pColorAttachments = &colour;
        count_attachment(*t.colour, area, !clear.colour, true);
    }
    if (t.depth != nullptr) {
        depth.imageView = attachment_view(*t.depth);
        depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.loadOp = clear.depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth.clearValue
            .depthStencil = {s.clear_depth, static_cast<std::uint32_t>(s.clear_stencil)};
        info.pDepthAttachment = &depth;
        if (has_stencil(t.depth->format)) {
            stencil = depth;
            stencil.loadOp =
                clear.stencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            info.pStencilAttachment = &stencil;
        }
        // One image holds both aspects: read in when either is kept.
        const bool kept = !clear.depth || (has_stencil(t.depth->format) && !clear.stencil);
        count_attachment(*t.depth, area, kept, true);
    }
    NativeCommandEncoder{cmd}.begin_rendering(&info);
    count_render_pass();
    g->pass = OpenPass{};
    if (clear.colour) {
        note_colour_write(t);
    }
    note_depth_write(t, depth_aspects(clear.depth, clear.stencil));
}

// Begins a pass into `t` whose load and store operations wait for its end (attachment-discard):
// the barriers go into `main` now, and `main` points at a secondary buffer until end_rendering.
// False when no secondary buffer can be had, and nothing is begun.
bool begin_deferred(const Target& t, const Clear& clear) {
    const VkCommandBuffer secondary = secondary_for(t);

    // No buffer to defer into: the pass is recorded directly.
    if (secondary == VK_NULL_HANDLE) {
        return false;
    }

    Frame& f = frame();
    const State& s = g->state;
    attachment_barriers(f.main, t);
    OpenPass& pass = g->pass;
    pass = OpenPass{};
    pass.deferred = true;
    pass.primary = f.main;
    pass.clear_colour = clear.colour;
    pass.clear_depth = clear.depth;
    pass.clear_stencil = clear.stencil;
    std::memcpy(pass.colour_value.float32, s.clear_colour, sizeof s.clear_colour);
    pass.depth_value = {s.clear_depth, static_cast<std::uint32_t>(s.clear_stencil)};
    pass.commands_at_begin = commands_recorded(g->submission);

    // A clear the pass begins with clears all of the target: its render area is all of it.
    if (clear.colour || clear.depth || clear.stencil) {
        touch_area(
            t, {{0, 0}, {static_cast<std::uint32_t>(t.width), static_cast<std::uint32_t>(t.height)}}
        );
    }

    f.main = secondary;
    return true;
}

// The store operation of an aspect the open pass may have let go (`dropped`).
VkAttachmentStoreOp store_op(VkImageAspectFlags dropped, VkImageAspectFlags aspect) {
    return (dropped & aspect) != 0 ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                                   : VK_ATTACHMENT_STORE_OP_STORE;
}

// Ends the deferred pass into `t`: begins it in the frame's `main` with its operations chosen
// now (a let-go aspect not stored, a discarded one not loaded), runs its secondary buffer and
// ends it. The images then know which of their aspects hold nothing.
void end_deferred(const Target& t) {
    Frame& f = frame();
    OpenPass& pass = g->pass;
    const VkCommandBuffer secondary = f.main;
    vkEndCommandBuffer(secondary);
    f.main = pass.primary;
    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo stencil{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingInfo info{VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
    info.renderArea =
        {{0, 0}, {static_cast<std::uint32_t>(t.width), static_cast<std::uint32_t>(t.height)}};
    info.layerCount = 1;
    bool clear_colour = pass.clear_colour;
    bool clear_depth = pass.clear_depth;
    bool clear_stencil = pass.clear_stencil;

    // Only the rectangle the draws and clears touched is loaded and stored (pass-render-area); a
    // clear that came first and covers all of it makes the pass begin cleared instead of loading.
    // That clear is still in the pass's buffer, so the value the pass begins with is not seen.
    if (pass.touched && render_opt::enabled(render_opt::Opt::PassRenderArea)) {
        info.renderArea =
            {{pass.x0, pass.y0},
             {static_cast<std::uint32_t>(pass.x1 - pass.x0),
              static_cast<std::uint32_t>(pass.y1 - pass.y0)}};
        const VkRect2D& r = pass.first_rect;
        const bool covers = pass.first_clear && r.offset.x == pass.x0 && r.offset.y == pass.y0
                            && r.extent.width == info.renderArea.extent.width
                            && r.extent.height == info.renderArea.extent.height;

        // The aspects that first clear cleared.
        if (covers) {
            clear_colour = clear_colour || pass.first_colour_cleared;
            clear_depth = clear_depth || pass.first_depth_cleared;
            clear_stencil = clear_stencil || pass.first_stencil_cleared;
        }
    }

    const VkExtent2D area = info.renderArea.extent;

    // An attachment no draw or clear of the pass touched keeps what it held, unread and unwritten
    // (attachment-untouched; it stays as let go as it was).
    const bool untouched_ops =
        g->load_store_none && render_opt::enabled(render_opt::Opt::AttachmentUntouched);
    const bool colour_untouched =
        untouched_ops && !clear_colour && !pass.used_colour && pass.resolve_colour == nullptr;
    const bool depth_untouched =
        untouched_ops && !clear_depth && !clear_stencil && !pass.used_depth;

    // The colour attachment: cleared, not loaded or loaded; stored unless let go.
    if (t.colour != nullptr && colour_untouched) {
        colour.imageView = attachment_view(*t.colour);
        colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colour.loadOp = VK_ATTACHMENT_LOAD_OP_NONE_EXT;
        colour.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
        info.colorAttachmentCount = 1;
        info.pColorAttachments = &colour;
    } else if (t.colour != nullptr) {
        colour.imageView = attachment_view(*t.colour);
        colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colour.loadOp = load_op(*t.colour, VK_IMAGE_ASPECT_COLOR_BIT, clear_colour);
        colour.storeOp = store_op(pass.drop_colour, VK_IMAGE_ASPECT_COLOR_BIT);
        // A pass's own clear keeps its value; a load of let-go data under test is the poison.
        colour.clearValue.color =
            clear_colour || !g->poison_discards ? pass.colour_value : kPoisonColour;
        info.colorAttachmentCount = 1;
        info.pColorAttachments = &colour;
        count_attachment(
            *t.colour,
            area,
            colour.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD,
            colour.storeOp == VK_ATTACHMENT_STORE_OP_STORE
        );

        // The samples averaged into the image a blit asked for, as each tile is written out.
        if (Image* resolved = pass.resolve_colour; resolved != nullptr) {
            barrier(f.main, *resolved, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            colour.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            colour.resolveImageView = attachment_view(*resolved);
            colour.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            count_attachment(*resolved, area, false, true);
            resolved->discarded = 0;
            resolved->touched = g->recording;
        }
    }

    // The depth and stencil image: each aspect's operations of its own, counted once.
    if (t.depth != nullptr && depth_untouched) {
        depth.imageView = attachment_view(*t.depth);
        depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_NONE_EXT;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
        info.pDepthAttachment = &depth;

        // The stencil aspect of the same image, alike.
        if (has_stencil(t.depth->format)) {
            stencil = depth;
            info.pStencilAttachment = &stencil;
        }
    } else if (t.depth != nullptr) {
        depth.imageView = attachment_view(*t.depth);
        depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.loadOp = load_op(*t.depth, VK_IMAGE_ASPECT_DEPTH_BIT, clear_depth);
        depth.storeOp = store_op(pass.drop_depth, VK_IMAGE_ASPECT_DEPTH_BIT);
        depth.clearValue.depthStencil = pass.depth_value;

        // A load of let-go depth under test is cleared to the poison rather than to the value.
        if (g->poison_discards && !clear_depth) {
            depth.clearValue.depthStencil.depth = kPoisonDepth.depth;
        }

        info.pDepthAttachment = &depth;
        bool loaded = depth.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD;
        bool stored = depth.storeOp == VK_ATTACHMENT_STORE_OP_STORE;

        // The stencil aspect of the same image.
        if (has_stencil(t.depth->format)) {
            stencil = depth;
            stencil.loadOp = load_op(*t.depth, VK_IMAGE_ASPECT_STENCIL_BIT, clear_stencil);
            stencil.storeOp = store_op(pass.drop_depth, VK_IMAGE_ASPECT_STENCIL_BIT);

            // The same for let-go stencil.
            if (g->poison_discards && !clear_stencil) {
                stencil.clearValue.depthStencil.stencil = kPoisonDepth.stencil;
            }

            info.pStencilAttachment = &stencil;
            loaded = loaded || stencil.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD;
            stored = stored || stencil.storeOp == VK_ATTACHMENT_STORE_OP_STORE;
        }

        // One image holds both aspects: read in when either is loaded, written when either is.
        count_attachment(*t.depth, area, loaded, stored);
    }

    NativeCommandEncoder{f.main}.begin_rendering(&info);
    count_render_pass();
    NativeCommandEncoder{f.main}.execute_commands(1, &secondary);
    count_command(Command::Other);
    NativeCommandEncoder{f.main}.end_rendering();
    count_command(Command::RenderPass);
    const OpenPass ended = pass;
    pass = OpenPass{};

    // What the pass wrote is whole again; what it let go holds nothing (or the poison).
    if (t.colour != nullptr) {
        if (clear_colour || ended.used_colour) {
            t.colour->discarded &= ~VK_IMAGE_ASPECT_COLOR_BIT;
        }
        mark_discarded(*t.colour, ended.drop_colour);
    }

    // The same for the depth and stencil image.
    if (t.depth != nullptr) {
        const auto written = ended.written_depth | depth_aspects(clear_depth, clear_stencil);
        t.depth->discarded &= ~written;
        mark_discarded(*t.depth, ended.drop_depth);
    }
}

void begin_rendering(const Target& t, const Clear& clear) {
    end_rendering(PassEnd::Target);

    // Choose operations at the pass's end; record directly if deferred setup fails.
    if (!begin_deferred(t, clear)) {
        begin_direct(t, clear);
    }

    g->rendering = true;
    g->rendering_target = t;
    g->bound_pipeline = VK_NULL_HANDLE;

    // A new pass trusts nothing recorded before it (RecordedState): more than Vulkan asks.
    g->recorded.forget();
}

}  // namespace

// ---- The render counters ----

void count_attachment(const Image& image, VkExtent2D area, bool load, bool store) {
    const std::uint64_t bytes = std::uint64_t{area.width} * area.height
                                * static_cast<std::uint64_t>(texel_bytes(image.format))
                                * static_cast<std::uint64_t>(image.samples);

    // Kept from before the pass: read from memory into the GPU's attachment storage.
    if (load) {
        g->submission.attachment_load_bytes += bytes;
    }

    // Kept after the pass: written back to memory.
    if (store) {
        g->submission.attachment_store_bytes += bytes;
    }
}

void gpu_flush_draws() {
    auto& gpu = g->gpu;
    if (!gpu.pending_count) {
        return;
    }
    record_pipeline_command(
        frame().main,
        renderer::graph::DrawIndexedIndirect{gpu.pending_offset, gpu.pending_count, 20},
        gpu.pending_arguments,
        gpu.pending_argument_range
    );
    count_command(Command::Draw);
    ++g->measures.native_draws;
    ++gpu.indirect_calls;
    gpu.pending_count = 0;
}

void defer(std::function<void()> f) {
    frame().lifetime.retire(std::move(f));
}

void destroy_image_now(Image& image) {
    for (auto& [key, view] : image.views) {
        vkDestroyImageView(g->device, view, nullptr);
    }
    if (image.attachment != VK_NULL_HANDLE) {
        vkDestroyImageView(g->device, image.attachment, nullptr);
    }
    if (image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(g->allocator, image.image, image.allocation);
    }
    image = Image{};
}

void retire(ImagePtr image) {
    if (image) {
        defer([image]() mutable { image.reset(); });
    }
}

// ---- The frame's ring: vertices, indices, uniforms, texels on their way ----

Slice ring(VkDeviceSize size) {
    Frame& f = frame();
    const VkDeviceSize align = ring_alignment();
    // Leave room for alignment without wrapping either the request or its end offset.
    if (size > std::numeric_limits<VkDeviceSize>::max() - align) {
        throw std::length_error("Vulkan upload request exceeds addressable size");
    }
    size = std::max<VkDeviceSize>(size, 4);
    while (true) {
        if (f.chunk_at < f.chunks.size()) {
            auto& c = f.chunks[f.chunk_at];
            const VkDeviceSize at = (f.chunk_used + align - 1) / align * align;
            if (at <= c.size && size <= c.size - at) {
                f.chunk_used = at + size;
                c.high_water = f.chunk_used;
                g->measures.ring_bytes += size;
                g->submission.ring_bytes += size;
                return {c.buffer, at, c.mapped + at, size};
            }
            ++f.chunk_at;
            f.chunk_used = 0;
            continue;
        }
        if (!new_chunk(f, size)) {
            throw std::runtime_error("cannot allocate Vulkan upload ring");
        }
    }
}

// Bulk texture copies reuse a dedicated buffer only after their submission has completed.
Slice texture_staging(VkDeviceSize size) {
    if (!g->bulk_texture_upload || size > kBulkTextureBatchBytes) {
        return ring(size);
    }
    prepare_frame();
    Frame& f = frame();
    Chunk& staging = f.bulk_texture_staging;
    if (!staging.buffer && !allocate_chunk(staging, kBulkTextureBatchBytes)) {
        staging = {};
        throw std::runtime_error("cannot allocate Vulkan bulk texture staging");
    }
    const auto alignment = ring_alignment();
    auto offset = (f.bulk_texture_used + alignment - 1) / alignment * alignment;
    if (offset > staging.size || size > staging.size - offset) {
        flush_and_wait();
        offset = 0;
    }
    f.bulk_texture_used = offset + size;
    return {staging.buffer, offset, staging.mapped + offset, size};
}

// Compressed texture blocks are already in the GPU's format.
Slice stage_compressed_pixels(const void* pixels, VkDeviceSize size) {
    const Slice staged = texture_staging(size);
    std::memcpy(staged.data, pixels, static_cast<std::size_t>(size));
    return staged;
}

// Called only after the frame fence or at device teardown.
void release_bulk_texture_staging(Frame& f) {
    Chunk& staging = f.bulk_texture_staging;
    if (staging.buffer) {
        vmaDestroyBuffer(g->allocator, staging.buffer, staging.allocation);
        staging = {};
        f.bulk_texture_used = 0;
    }
}

bool ring_trim_enabled() {
    static const bool off = env::flag(env::Var::VkRingTrimOff);
    return !off;
}

bool reserve_upload_ring(Frame& f) {
    // Keep normal frame uploads off the allocator, even after a load first needs overflow.
    return new_chunk(f, kChunk);
}

void reset_upload_ring(Frame& f) {
    release_bulk_texture_staging(f);
    constexpr unsigned kIdleSlotReuses = 60;
    const bool trim = ring_trim_enabled();
    std::size_t retained = 0;
    for (std::size_t index = 0; index < f.chunks.size(); ++index) {
        auto chunk = f.chunks[index];
        const bool pinned = index == 0 && chunk.size == kChunk;
        const auto used = chunk.high_water;
        // A large startup allocation can remain first in the ring and service tiny steady uploads.
        const bool cold = used == 0 || (chunk.size > kChunk && used <= kChunk);
        chunk.idle_reuses = cold ? std::min(chunk.idle_reuses + 1, kIdleSlotReuses) : 0;
        if (trim && !pinned && chunk.idle_reuses == kIdleSlotReuses) {
            vmaDestroyBuffer(g->allocator, chunk.buffer, chunk.allocation);
            ++g->measures.ring_retirements;
            g->measures.ring_retired_bytes += chunk.size;
        } else {
            chunk.high_water = 0;
            f.chunks[retained++] = chunk;
        }
    }
    f.chunks.resize(retained);
    f.chunk_at = 0;
    f.chunk_used = 0;
}

Slice ring_copy(const void* data, VkDeviceSize size) {
    Slice s = ring(size);
    if (s.data != nullptr && data != nullptr && size > 0) {
        std::memcpy(s.data, data, static_cast<std::size_t>(size));
    }
    return s;
}

// ---- Command buffers and the order of things ----

VkCommandBuffer upload_cmd() {
    Frame& f = frame();
    g->upload_used = true;
    return f.upload;
}

void count_where(std::string_view event) {
    const auto section = stats::open_section();
    auto& key = g->where_key;
    key.assign(event);
    key.append(" in ").append(section.empty() ? "frame" : section);
    g->where[key] += 1.0;
}

void end_rendering(PassEnd why) {
    gpu_flush_draws();
    if (g->rendering) {
        if (stats::enabled()) {
            constexpr std::array<std::string_view, 12> kWhy = {
                "vk_pass_end target",
                "vk_pass_end copy",
                "vk_pass_end sample",
                "vk_pass_end compute",
                "vk_pass_end barrier",
                "vk_pass_end submit",
                "vk_pass_end present",
                "vk_pass_end timing",
                "vk_pass_end graph",
                "vk_pass_end import",
                "vk_pass_end replay",
                "vk_pass_end other",
            };
            count_where(kWhy[static_cast<std::size_t>(why)]);
        }
        // Not rendering from here on: the poison of a deferred pass's end records outside it.
        g->rendering = false;
        g->bound_pipeline = VK_NULL_HANDLE;

        // A deferred pass ends with its operations chosen; a direct one as it was begun.
        if (g->pass.deferred) {
            end_deferred(g->rendering_target);

            // A secondary buffer leaves the state of the buffer that ran it undefined.
            g->recorded.forget();
        } else {
            NativeCommandEncoder{frame().main}.end_rendering();
            count_command(Command::RenderPass);
        }
    }
}

BufferWritesBarrier buffer_writes_barrier() {
    constexpr VkPipelineStageFlags kReaders =
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
        | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    // Buffers are written by compute shaders and copies alone (the R-08 table). The reference
    // path: every stage.
    const VkPipelineStageFlags writers =
        render_opt::enabled(render_opt::Opt::VkTightSync)
            ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT
            : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkMemoryBarrier writes{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    writes.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    writes.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT
                           | VK_ACCESS_INDIRECT_COMMAND_READ_BIT
                           | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT
                           | VK_ACCESS_TRANSFER_READ_BIT;
    return {writes, writers, kReaders};
}

void end_rendering_for(const Image* image) {
    if (g->rendering
        && (g->rendering_target.colour == image || g->rendering_target.depth == image)) {
        end_rendering(PassEnd::Import);
    }
}

VkCommandBuffer cmd_for(Image* image, bool force_main) {
    Frame& f = frame();
    if (!force_main && image != nullptr && image->touched != g->recording) {
        return upload_cmd();
    }
    end_rendering(PassEnd::Copy);
    if (image != nullptr) {
        image->touched = g->recording;
    }
    return f.main;
}

void barrier(VkCommandBuffer cmd, Image& image, VkImageLayout to) {
    const PendingPresent& waiting = g->present_copy;
    const bool reads_only = to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                            || to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // The default framebuffer used, or the picture written: the copy that waits comes first.
    if (waiting.pending
        && (&image == g->default_colour.get() || (&image == waiting.source.get() && !reads_only))) {
        make_present_copy();
    }

    // The old layout's use waits; the new one's waits for it. The reference path: everything.
    const bool tight = render_opt::enabled(render_opt::Opt::VkTightSync);
    const LayoutScope all = scope_of(VK_IMAGE_LAYOUT_MAX_ENUM);
    const LayoutScope into = tight ? scope_of(to) : all;
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.dstAccessMask = into.accesses;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image.image;
    const auto transition = [&](VkImageLayout old, unsigned mip, unsigned count, unsigned layer,
                                unsigned layers) {
        const LayoutScope from = tight ? scope_of(old) : all;
        b.srcAccessMask = from.accesses;
        b.oldLayout = old;
        b.subresourceRange = {image.aspect, mip, count, layer, layers};
        NativeCommandEncoder{cmd}
            .pipeline_barrier(from.stages, into.stages, 0, 0, nullptr, 0, nullptr, 1, &b);
        count_barrier(from.stages, into.stages, "image layout");
    };
    if (image.layout != VK_IMAGE_LAYOUT_MAX_ENUM) {
        transition(image.layout, 0, VK_REMAINING_MIP_LEVELS, 0, image.layers);
    } else {
        namespace graph = renderer::graph;
        const auto aspect = image.aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? graph::Aspect::Depth
                                                                     : graph::Aspect::Colour;
        const auto levels = static_cast<unsigned>(image.levels);
        for (unsigned layer = 0; layer < image.layers; ++layer) {
            for (unsigned mip = 0; mip < levels;) {
                const auto old = image_contents(image, aspect, mip, layer).state.layout;
                const unsigned first = mip++;
                while (mip < levels && image_contents(image, aspect, mip, layer).state.layout == old) {
                    ++mip;
                }
                transition(layout_of(old), first, mip - first, layer, 1);
            }
        }
    }
    if (tracks_subresources(image)) {
        const auto state = imported_state(to);
        if (!state) {
            throw std::runtime_error(state.error().message);
        }
        set_whole_image_state(image, *state);
    } else {
        image.layout = to;
    }
}

void begin_buffers() {
    Frame& f = frame();
    gpu_reset(f);
    material_reset(f);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    require_success(vkBeginCommandBuffer(f.upload, &begin), "begin upload commands");
    require_success(vkBeginCommandBuffer(f.main, &begin), "begin frame commands");
    ++g->recording;
    if (g->gpu_breadcrumbs) {
        record_gpu_breadcrumb(f.upload, "upload commands", false);
        record_gpu_breadcrumb(f.main, "frame commands", false);
    }
    tracy_gpu_collect();
    const bool reset = f.timestamps != VK_NULL_HANDLE && f.timestamp_used == 0;
    if (reset) {
        NativeCommandEncoder{f.upload}.reset_query_pool(f.timestamps, 0, kTimestamps);
        count_command(Command::Query);
        f.frame_timing = Frame::FrameTiming::None;  // a start the pool held is gone with it
    }
    // The whole frame's GPU time from here to the end of `main` (stats::gpu_frame_begin, and the
    // low-latency frames' sleep, which measures it with or without the debug tools). Only
    // into a pool reset just now: buffers begun again after a submit within the frame (its
    // sections' queries still in use) would write the frame's queries a second time unreset.
    if (reset && g->timestamp_bits != 0 && (stats::enabled() || g->low_latency)) {
        NativeCommandEncoder{f.upload}
            .write_timestamp(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, f.timestamps, kFrameTimestamp);
        count_command(Command::Query);
        f.frame_timing = Frame::FrameTiming::Started;
    }
    g->upload_used = false;
    f.secondary_at = 0;

    // A command buffer begun afresh holds no dynamic state and no binds.
    g->recorded.forget();
}

// Whether that frame's timed sections end their render pass: always while split timing is set,
// else one frame with timed sections in stats::pass_sampling(), decided at its first.
bool split_for(std::uint64_t serial) {
    if (g->split_pass_timing) {
        return true;
    }
    const int every = stats::pass_sampling();
    if (every <= 0) {
        return false;
    }
    if (serial != g->split_serial) {
        g->split_serial = serial;
        g->split_now = ++g->timed_frames % static_cast<std::uint64_t>(every) == 0;
    }
    return g->split_now;
}

bool timing_begin(std::uint64_t serial, int section) {
    // Deferred passes receive GPU timestamps when the shared plan executes.
    if (collecting_surface_chain()) {
        return false;
    }
    if (!g || !g->timestamp_bits) {
        return false;
    }
    gpu_flush_draws();
    auto& f = frame();
    if (f.active_timing >= 0 || f.timestamp_used + 2 > kFrameTimestamp) {
        return false;
    }
    // Split, the section's first draw begins a pass of its own (which loads what the last one
    // stored): the timestamp lies between passes, where a tile-based GPU measures it.
    const bool split = split_for(serial);
    if (split) {
        end_rendering(PassEnd::Timing);
    }
    f.active_timing = static_cast<int>(f.timings.size());
    f.timings.push_back({serial, section, f.timestamp_used, split});
#if defined(OPENRAC_GPU_MARKERS)
    if (env::flag(env::Var::GpuMarkers)
        && NativeCommandEncoder::supports_begin_debug_utils_label()) {
        const auto name = stats::gpu_section_name(serial, section);
        VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name.c_str();
        label.color[0] = 0.25f;
        label.color[1] = 0.6f;
        label.color[2] = 0.9f;
        label.color[3] = 1;
        NativeCommandEncoder{g->gpu.timing_upload ? f.upload : f.main}.begin_debug_utils_label(
            &label
        );
        count_command(Command::Other);
    }
#endif
    // Bound both ends at command completion. A TOP start followed by a
    // BOTTOM end includes earlier in-flight work, so adjacent scopes can
    // overlap and their sum inflates when a pass is split into subpasses.
    // This measures completion intervals, without serialising the passes.
    NativeCommandEncoder{g->gpu.timing_upload ? f.upload : f.main}
        .write_timestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, f.timestamps, f.timestamp_used);
    count_command(Command::Query);
    f.timestamp_used += 2;
    return true;
}

void timing_end() {
    if (!g) {
        return;
    }
    gpu_flush_draws();
    auto& f = frame();
    if (f.active_timing < 0) {
        return;
    }
    auto& timing = f.timings[static_cast<std::size_t>(f.active_timing)];
    if (timing.split) {
        end_rendering(PassEnd::Timing);
    }
    NativeCommandEncoder{g->gpu.timing_upload ? f.upload : f.main}
        .write_timestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, f.timestamps, timing.first + 1);
    count_command(Command::Query);
#if defined(OPENRAC_GPU_MARKERS)
    if (env::flag(env::Var::GpuMarkers) && NativeCommandEncoder::supports_end_debug_utils_label()) {
        NativeCommandEncoder{g->gpu.timing_upload ? f.upload : f.main}.end_debug_utils_label();
        count_command(Command::Other);
    }
#endif
    timing.complete = true;
    f.active_timing = -1;
}

void timing_results(Frame& f) {
    gpu_results(f);
    const auto mask =
        g->timestamp_bits == 64 ? UINT64_MAX : (std::uint64_t{1} << g->timestamp_bits) - 1;
    if (f.frame_timing == Frame::FrameTiming::Ended) {
        std::uint64_t values[2]{};
        if (vkGetQueryPoolResults(
                g->device,
                f.timestamps,
                kFrameTimestamp,
                2,
                sizeof(values),
                values,
                sizeof(std::uint64_t),
                VK_QUERY_RESULT_64_BIT
            )
            == VK_SUCCESS) {
            const double ticks = static_cast<double>((values[1] - values[0]) & mask);
            const double ms = ticks * g->properties.limits.timestampPeriod / 1e6;
            if (f.frame_serial != stats::kNoFrame) {
                stats::gpu_frame_result(f.frame_serial, ms);
            }
            // The low-latency frames' sleep is worked out from it (latency_sleep.h).
            if (g->low_latency) {
                g->latency.sleep.gpu_frame(ms / 1000.0);
            }
        }
    }
    f.frame_timing = Frame::FrameTiming::None;
    for (auto& timing : f.timings) {
        if (!timing.complete || timing.reported) {
            continue;
        }
        std::uint64_t values[2]{};
        if (vkGetQueryPoolResults(
                g->device,
                f.timestamps,
                timing.first,
                2,
                sizeof(values),
                values,
                sizeof(std::uint64_t),
                VK_QUERY_RESULT_64_BIT
            )
            == VK_SUCCESS) {
            const double ticks = static_cast<double>((values[1] - values[0]) & mask);
            if (timing.split) {
                stats::gpu_passes_split(timing.serial);
            }
            stats::gpu_result(
                timing.serial, timing.section, ticks * g->properties.limits.timestampPeriod / 1e6
            );
            timing.reported = true;
        }
    }
}

void submit(
    VkSemaphore wait_semaphore,
    VkSemaphore signal,
    bool wait,
    VkCommandBuffer present,
    const std::optional<renderer::graph::PresentImage>& presentation
) {
    crash::breadcrumb("Vulkan prepare frame submission");
    require_surface_execution();
    const profiling::Zone trace("Vulkan submit");

    end_rendering(PassEnd::Submit);
    Frame& f = frame();
    gpu_record();
    tracy_gpu_end_commands();
    const bool tight = render_opt::enabled(render_opt::Opt::VkTightSync);
    // Order the upload copies before main. GPU culling exports its own indirect and vertex ranges.
    VkMemoryBarrier m{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    VkPipelineStageFlags source = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destination = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    m.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    m.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    // The tight path only needs transfer writes after the culling graph exports its outputs.
    if (tight) {
        source = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
                      | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                      | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
        m.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        m.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT
                          | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT
                          | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
                          | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    NativeCommandEncoder{f.upload}
        .pipeline_barrier(source, destination, 0, 1, &m, 0, nullptr, 0, nullptr);
    count_barrier(source, destination, "upload order");
    // The frame's work ends here, outside any render pass (end_rendering above).
    if (f.frame_timing == Frame::FrameTiming::Started) {
        NativeCommandEncoder{f.main}.write_timestamp(
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, f.timestamps, kFrameTimestamp + 1
        );
        count_command(Command::Query);
        f.frame_timing = Frame::FrameTiming::Ended;
        f.frame_serial = stats::gpu_frame_begin();
    }
    if (g->gpu_breadcrumbs) {
        record_gpu_breadcrumb(f.upload, "upload commands", true);
        record_gpu_breadcrumb(f.main, "frame commands", true);
    }
    require_success(vkEndCommandBuffer(f.upload), "end upload commands");
    require_success(vkEndCommandBuffer(f.main), "end frame commands");
    if (present != VK_NULL_HANDLE) {
        if (g->gpu_breadcrumbs) {
            record_gpu_breadcrumb(present, "present commands", true);
        }
        require_success(vkEndCommandBuffer(present), "end present commands");
    }
    const auto schedule = [&] {
        const profiling::Zone schedule_trace("Compile frame submission");
        return renderer::graph::FrameSubmissionPlan::compile(
            std::bit_cast<std::uint64_t>(f.upload),
            std::bit_cast<std::uint64_t>(f.main),
            std::bit_cast<std::uint64_t>(present),
            std::bit_cast<std::uint64_t>(wait_semaphore),
            std::bit_cast<std::uint64_t>(signal),
            presentation
        );
    }();
    if (!schedule) {
        throw RecordingFailure(schedule.error().message);
    }
    require_success(vkResetFences(g->device, 1, &f.fence), "reset frame fence");
    {
        const profiling::Zone queue_trace("Queue submit");
        crash::breadcrumb("Vulkan submit frame");
        require_success(submit_graph_queue(g->queue, *schedule, f.fence), "submit frame");
        crash::breadcrumb("Vulkan frame submission accepted");
    }
    f.submitted = true;
    f.lifetime.submitted();
    g->last_submitted_recording = g->recording;
    ++g->submission.submits;
    if (wait) {
        crash::breadcrumb("Vulkan wait for submitted frame");
        require_success(
            vkWaitForFences(g->device, 1, &f.fence, VK_TRUE, UINT64_MAX), "wait for submitted frame"
        );
        // Counter collection still needs the completed submission's marker and signalled fence.
        timing_results(f);
        f.submitted = false;
        require_success(vkResetCommandPool(g->device, f.pool, 0), "reset completed frame pool");
        begin_buffers();
    }
}

VkCommandBuffer present_cmd() {
    Frame& f = frame();

    // The present's buffer is the frame slot's, made the first time, reset with its pool.
    if (f.present == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc.commandPool = f.pool;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 1;

        // No buffer: the copy to the screen goes into `main`, as before.
        if (!check(vkAllocateCommandBuffers(g->device, &alloc, &f.present), "a present buffer")) {
            f.present = VK_NULL_HANDLE;
            return VK_NULL_HANDLE;
        }
    }

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    require_success(vkBeginCommandBuffer(f.present, &begin), "begin present commands");
    if (g->gpu_breadcrumbs) {
        record_gpu_breadcrumb(f.present, "present commands", false);
    }
    return f.present;
}

void flush_and_wait() {
    ++g->measures.flushes;
    submit(VK_NULL_HANDLE, VK_NULL_HANDLE, true);
}

// ---- Where drawing goes ----

Target target_of(GLuint name) {
    Target t;
    if (name == 0) {
        t.colour = g->default_colour.get();
        t.depth = g->default_depth_detached ? nullptr : g->default_depth.get();
    } else if (Framebuffer* fb = g->framebuffers.find(name)) {
        t.colour = image_of(fb->colour_texture, fb->colour_renderbuffer);
        t.depth = image_of(fb->depth_texture, fb->depth_renderbuffer);
    }
    if (Image* any = t.colour != nullptr ? t.colour : t.depth) {
        t.width = any->width;
        t.height = any->height;
        t.samples = any->samples;
    }
    return t;
}

void touch_area(const Target& t, const VkRect2D& rect) {
    const int x0 = std::clamp(rect.offset.x, 0, t.width);
    const int y0 = std::clamp(rect.offset.y, 0, t.height);
    const int x1 = std::clamp(rect.offset.x + static_cast<int>(rect.extent.width), x0, t.width);
    const int y1 = std::clamp(rect.offset.y + static_cast<int>(rect.extent.height), y0, t.height);

    // An empty rectangle touches nothing.
    if (x0 == x1 || y0 == y1) {
        return;
    }

    OpenPass& pass = g->pass;

    // The first rectangle, or the one that holds it and the rest.
    if (!pass.touched) {
        pass.touched = true;
        pass.x0 = x0;
        pass.y0 = y0;
        pass.x1 = x1;
        pass.y1 = y1;
    } else {
        pass.x0 = std::min(pass.x0, x0);
        pass.y0 = std::min(pass.y0, y0);
        pass.x1 = std::max(pass.x1, x1);
        pass.y1 = std::max(pass.y1, y1);
    }
}

void ensure_rendering(const Target& t, bool draw) {
    if (!g->rendering || !(g->rendering_target == t)) {
        begin_rendering(t, Clear{});
    }

    // Where a draw may write: each indexed viewport, its own scissor, or else the scissor. A
    // clear names its own rectangle.
    if (draw && g->indexed_viewports.empty()) {
        touch_area(t, scissor_rect(t));
    } else if (draw) {
        for (const auto& r : g->indexed_viewports) {
            touch_area(
                t,
                {{r[0], r[1]},
                 {static_cast<std::uint32_t>(std::max(r[2], 0)),
                  static_cast<std::uint32_t>(std::max(r[3], 0))}}
            );
        }
    }

    if (draw) {
        // A draw after glInvalidateFramebuffer keeps the aspects it can write.
        const State& s = g->state;
        if (s.colour_mask[0] || s.colour_mask[1] || s.colour_mask[2] || s.colour_mask[3]) {
            note_colour_write(t);
        }
        g->pass.used_depth |= s.depth_test || s.stencil_test;
        const auto changes_stencil = [](const StencilFace& face) {
            return face.sfail != GL_KEEP || face.dpfail != GL_KEEP || face.dppass != GL_KEEP;
        };
        const bool writes_stencil =
            s.stencil_test && (s.stencil_write_mask & 0xff)
            && (changes_stencil(s.stencil_front) || changes_stencil(s.stencil_back));
        note_depth_write(t, depth_aspects(s.depth_test && s.depth_mask, writes_stencil));
    }
}

bool drop_clear_pass(VkClearColorValue& clear) {
    const OpenPass& pass = g->pass;
    const bool clear_only = pass.clear_colour && !pass.clear_depth && !pass.clear_stencil;

    // Only a deferred pass into the default framebuffer that cleared its colour and holds nothing.
    if (!g->rendering || !pass.deferred || g->rendering_target.colour != g->default_colour.get()
        || !clear_only || commands_recorded(g->submission) != pass.commands_at_begin) {
        return false;
    }

    Frame& f = frame();
    vkEndCommandBuffer(f.main);
    f.main = pass.primary;
    clear = pass.colour_value;
    g->pass = OpenPass{};
    g->rendering = false;
    g->bound_pipeline = VK_NULL_HANDLE;
    g->recorded.forget();
    return true;
}

void discard_attachments(const Target& t, bool colour, VkImageAspectFlags depth) {
    const bool open = g->rendering && g->pass.deferred;

    // The colour image: not stored by the open pass on it, or not loaded by the next one.
    if (t.colour != nullptr && colour) {
        // The pass drawing into it lets it go at its end; any other is done with it already.
        if (open && g->rendering_target.colour == t.colour) {
            g->pass.drop_colour |= VK_IMAGE_ASPECT_COLOR_BIT;
        } else {
            mark_discarded(*t.colour, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    const VkImageAspectFlags aspects =
        t.depth != nullptr ? depth & t.depth->aspect : VkImageAspectFlags{0};

    // The depth and stencil image, the aspects it has of those asked.
    if (aspects != 0) {
        // The same choice as for the colour image.
        if (open && g->rendering_target.depth == t.depth) {
            g->pass.drop_depth |= aspects;
        } else {
            mark_discarded(*t.depth, aspects);
        }
    }
}

VkRect2D scissor_rect(const Target& t) {
    const State& s = g->state;
    int x0 = 0, y0 = 0, x1 = t.width, y1 = t.height;
    if (s.scissor_test) {
        x0 = std::clamp(s.scissor[0], 0, t.width);
        y0 = std::clamp(s.scissor[1], 0, t.height);
        x1 = std::clamp(s.scissor[0] + std::max(s.scissor[2], 0), x0, t.width);
        y1 = std::clamp(s.scissor[1] + std::max(s.scissor[3], 0), y0, t.height);
    }
    return {{x0, y0}, {static_cast<std::uint32_t>(x1 - x0), static_cast<std::uint32_t>(y1 - y0)}};
}

void clear(GLbitfield mask) {
    require_surface_execution();
    gpu_flush_draws();
    const Target t = target_of(g->state.draw_framebuffer);
    const State& s = g->state;
    Clear c;
    c.colour = t.colour != nullptr && (mask & GL_COLOR_BUFFER_BIT) != 0
               && (s.colour_mask[0] || s.colour_mask[1] || s.colour_mask[2] || s.colour_mask[3]);
    c.depth = t.depth != nullptr && (mask & GL_DEPTH_BUFFER_BIT) != 0 && s.depth_mask;
    c.stencil = t.depth != nullptr && has_stencil(t.depth->format)
                && (mask & GL_STENCIL_BUFFER_BIT) != 0 && (s.stencil_write_mask & 0xFF) != 0;
    if (!c.colour && !c.depth && !c.stencil) {
        return;
    }
    const VkImageAspectFlags aspects = (c.colour ? VK_IMAGE_ASPECT_COLOR_BIT : 0u)
                                       | (c.depth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0u)
                                       | (c.stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u);
    if (!graph_pipeline_active()) {
        GraphImmediateWork::clear_attachments([=] { clear(mask); }, aspects);
        return;
    }
    const VkRect2D rect = scissor_rect(t);
    if (rect.extent.width == 0 || rect.extent.height == 0) {
        return;
    }
    validate_pipeline_clear(t, aspects);
    const bool whole = rect.offset.x == 0 && rect.offset.y == 0
                       && static_cast<int>(rect.extent.width) == t.width
                       && static_cast<int>(rect.extent.height) == t.height;
    if (whole && (!g->rendering || !(g->rendering_target == t))) {
        NativeCommandEncoder::note_image_work();
        begin_rendering(t, c);  // cleared as the drawing begins: no work of its own on a tiler
        return;
    }
    // Nothing drawn or cleared in the pass yet: this clear may stand for loading the area.
    const bool first = !g->rendering || !(g->rendering_target == t) || !g->pass.touched;
    ensure_rendering(t, false);

    // Kept for end_deferred, which knows the pass's area only at its end.
    if (first && g->pass.deferred) {
        OpenPass& pass = g->pass;
        pass.first_clear = true;
        pass.first_colour_cleared = c.colour;
        pass.first_depth_cleared = c.depth;
        pass.first_stencil_cleared = c.stencil;
        pass.first_rect = rect;
    }

    touch_area(t, rect);
    if (c.colour) {
        note_colour_write(t);
    }
    g->pass.used_depth |= c.depth || c.stencil;
    note_depth_write(t, depth_aspects(c.depth, c.stencil));
    VkClearAttachment attachments[2]{};
    std::uint32_t n = 0;
    if (c.colour) {
        attachments[n].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        attachments[n].colorAttachment = 0;
        std::memcpy(attachments[n].clearValue.color.float32, s.clear_colour, sizeof s.clear_colour);
        ++n;
    }
    if (c.depth || c.stencil) {
        attachments[n].aspectMask = (c.depth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0u)
                                    | (c.stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u);
        attachments[n]
            .clearValue.depthStencil = {s.clear_depth, static_cast<std::uint32_t>(s.clear_stencil)};
        ++n;
    }
    VkClearRect r{rect, 0, 1};
    NativeCommandEncoder{frame().main}.clear_attachments(n, attachments, 1, &r);
    count_command(Command::Other);
}

}  // namespace openrac::vkgl::detail

namespace openrac::vkgl {

bool test_poison_discards(bool on) {
    // No context: nothing to poison.
    if (detail::g == nullptr) {
        return false;
    }

    detail::g->poison_discards = on;
    return true;
}

void default_framebuffer_depth(bool attached) {
    if (detail::g != nullptr) {
        detail::g->default_depth_detached = !attached;
    }
}

}  // namespace openrac::vkgl
