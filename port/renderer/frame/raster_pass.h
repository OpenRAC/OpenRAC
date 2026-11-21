// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synchronous raster recording on borrowed, already prepared resources.
#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <string_view>

#include "renderer/frame/colour_graph.h"

namespace openrac::renderer {

// A vertex or index allocation kept stable through recording.
struct RasterBuffer {
    unsigned name;           // Graphics context's buffer name.
    std::size_t size;        // Number of initialized bytes.
    bool instanced = false;  // Instance attributes use the frame stream.
};

// A sampled 2D texture kept stable through recording.
struct RasterTexture {
    unsigned name = 0;        // Zero leaves the texture unbound.
    unsigned mip_levels = 1;  // Defined levels, beginning with base level.
};

// Resources borrowed by a raster callback on the graphics thread.
struct RasterInputs {
    std::span<const RasterBuffer> geometry{};                   // Vertex and index inputs.
    std::span<const RasterTexture> textures{};                  // Sampled images, each listed once.
    ColourAttachments attachments = ColourAttachments::Colour;  // Preserved output aspects.
    bool bound_shader_reads = false;  // For Vulkan-only overlays with prepared shader descriptors.
};

// Compiles a raster pass and invokes `record` before returning. Preparation, uploads and target
// changes belong outside `record`. Failure propagates to the graphics error boundary.
void raster_pass(
    std::string_view name,
    const RenderTarget& target,
    RasterInputs inputs,
    const std::function<void()>& record
);

// Orders explicit geometry and texture reads before drawing inside the current Vulkan raster pass.
[[nodiscard]] Result<void> raster_reads(std::string_view name, RasterInputs inputs);

}  // namespace openrac::renderer
