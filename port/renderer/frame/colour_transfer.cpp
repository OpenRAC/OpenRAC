// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Executes colour transfers on the active backend, preserving Vulkan's swapchain copy path.
#include "renderer/frame/colour_transfer.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

#include "renderer/gl/gl.h"
#include "renderer/graph/recording_scope.h"

namespace openrac::renderer {
namespace {
using namespace gl;

// Reserve identities above the range of 32-bit texture names for framebuffer colour attachments.
constexpr std::uint64_t kFramebufferNamespace = std::uint64_t{1} << 32;
// The framebuffer read and write states of the portable transfers.
constexpr graph::State kRead{
    graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource
};
constexpr graph::State kWrite{
    graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination
};

// Borrows a single colour level with its contents supplied by the caller's earlier work.
graph::ImageHandle borrow_image(
    graph::Graph& graph, std::string_view label, std::uint64_t identity, bool defined
) {
    const auto image = graph.import_image(
        label, {.key = {identity}, .initial = defined ? kRead : graph::State{}, .defined = defined}
    );
    // An invalid import cannot be used by a transfer.
    if (!image) {
        throw std::runtime_error(image.error().message);
    }
    return *image;
}

// Compiles the declared GL transfer before touching the context. GL orders these image operations.
void record_gl_transfer(graph::Graph& graph, graph::Pass pass, const std::function<void()>& emit) {
    graph.add_pass(std::move(pass));
    const auto plan = graph.compile();
    // An invalid plan must leave the original contents intact.
    if (!plan) {
        throw std::runtime_error(plan.error().message);
    }
    const graph::RecordingScope scope{plan->passes.front()};
    emit();
}

// Checks the CPU buffer before allowing a driver read or write.
void check_pixels(int width, int height, std::size_t bytes) {
    constexpr std::size_t kChannels = 4;
    if (width <= 0 || height <= 0
        || bytes / kChannels / static_cast<std::size_t>(width) < static_cast<std::size_t>(height)) {
        throw std::invalid_argument("invalid RGBA8 transfer extent or pixel buffer");
    }
}
}  // namespace

void upload_rgba8(unsigned texture, int width, int height, std::span<const std::uint8_t> rgba) {
    check_pixels(width, height, rgba.size());
    // This upload replaces the whole base level. Vulkan lowers it to its native graph transfer.
    const auto emit = [=] {
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data()
        );
    };
    if (gl::is_vulkan()) {
        emit();
        return;
    }
    graph::Graph graph;
    const auto target = borrow_image(graph, "RGBA8 texture", texture, false);
    record_gl_transfer(graph, {"RGBA8 upload", {{target, {}, kWrite}}}, emit);
}

void blit_colour(
    std::string_view name,
    unsigned source,
    unsigned destination,
    const graph::BlitRegion& region,
    bool overwrite
) {
    // Binding 0 on exit keeps later rendering and screenshot callers independent of this copy.
    const auto emit = [&] {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        // Letterboxing clears the whole output, including areas outside the copied rectangle.
        if (region.clear) {
            const auto& colour = *region.clear;
            glClearColor(colour[0], colour[1], colour[2], colour[3]);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source);
        glBlitFramebuffer(
            region.source_begin[0],
            region.source_begin[1],
            region.source_end[0],
            region.source_end[1],
            region.destination_begin[0],
            region.destination_begin[1],
            region.destination_end[0],
            region.destination_end[1],
            GL_COLOR_BUFFER_BIT,
            region.linear ? GL_LINEAR : GL_NEAREST
        );
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    };
    // The Vulkan blit declares real attachment ranges and can defer the window copy to acquisition.
    if (gl::is_vulkan()) {
        emit();
        return;
    }
    graph::Graph graph;
    const auto from = borrow_image(graph, "colour source", kFramebufferNamespace + source, true);
    const bool partial = !overwrite && !region.clear;
    const auto to =
        borrow_image(graph, "colour target", kFramebufferNamespace + destination, partial);
    graph::CommandBuffer commands;
    commands.blit(from, to, region);
    auto pass = commands.declarations(std::string(name));
    // A copy without a preceding clear keeps pixels outside its destination rectangle.
    pass.images.back().partial_write = partial;
    record_gl_transfer(graph, std::move(pass), emit);
}

void read_colour(unsigned framebuffer, int width, int height, std::span<std::uint8_t> rgba) {
    check_pixels(width, height, rgba.size());
    // The backend performs any required staging and completes the read before returning.
    const auto emit = [=] {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    };
    if (gl::is_vulkan()) {
        emit();
        return;
    }
    graph::Graph graph;
    const auto from =
        borrow_image(graph, "readback colour", kFramebufferNamespace + framebuffer, true);
    record_gl_transfer(graph, {"RGBA8 readback", {{from, {}, kRead}}}, emit);
}

}  // namespace openrac::renderer
