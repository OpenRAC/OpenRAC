// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Captures compatibility GPU operations as declared graph work.
#pragma once

#include "renderer/vulkan/vkgl_internal.h"

namespace openrac::vkgl {
class GraphExecution;
}

namespace openrac::vkgl::detail {
class GraphImmediateWork {
public:
    static void draw(const std::function<void()>& record, GpuPage* page, unsigned indirect);
    static void dispatch(const std::function<void()>& record);
    static void copy_buffer(
        VkCommandBuffer command,
        BufferRef source,
        BufferRef destination,
        VkBufferCopy region,
        bool export_read = false
    );
    static void copy_image(
        VkCommandBuffer command,
        Image& source,
        Image& destination,
        std::span<const VkImageCopy> regions
    );
    static void blit_image(
        VkCommandBuffer command,
        Image& source,
        Image& destination,
        VkImageBlit region,
        VkFilter filter
    );
    static void resolve_image(
        VkCommandBuffer command, Image& source, Image& destination, VkImageResolve region
    );
    static void upload_image(
        VkCommandBuffer command, BufferRef source, Image& destination, VkBufferImageCopy region
    );
    static void read_image(
        VkCommandBuffer command, Image& source, BufferRef destination, VkBufferImageCopy region
    );
    static void generate_mipmaps(VkCommandBuffer command, const ImagePtr& image);
    static void clear_image(
        VkCommandBuffer command,
        Image& image,
        VkImageSubresourceRange range,
        VkClearColorValue colour,
        VkClearDepthStencilValue depth = {}
    );
    static void clear_attachments(const std::function<void()>& record, VkImageAspectFlags aspects);
    static void resolve_depth(
        VkCommandBuffer command,
        Image& source,
        Image& destination,
        VkRect2D area,
        VkImageAspectFlags aspects
    );

private:
    // Captures the target's existing contents and attachment role for draws and clears.
    static void capture_attachment(
        GraphExecution& execution,
        renderer::graph::Pass& pass,
        Image* image,
        renderer::graph::Aspect aspect
    );

    struct ImageAccess {
        Image* image;
        renderer::graph::ImageRange range;
        renderer::graph::State state;
        bool partial_write = false;
    };

    struct BufferAccess {
        BufferRef buffer;
        renderer::graph::BufferRange range;
        renderer::graph::State state;
    };

    static void transfer(
        std::string_view name,
        VkCommandBuffer command,
        std::span<const ImageAccess> images,
        std::span<const BufferAccess> buffers,
        const std::function<void()>& record,
        std::source_location site = std::source_location::current()
    );
};

}  // namespace openrac::vkgl::detail
