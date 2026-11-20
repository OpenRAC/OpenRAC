// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A depth-free raster pass for blending vertex-generated quads onto the window.
#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <span>
#include <string_view>

#include "renderer/frame/raster_pass.h"

namespace openrac::renderer {

// Compiles a pass that preserves the window colour and samples the given textures. Each entry in
// `textures` describes one quad in draw order, with zero for a quad without a texture. `setup`
// binds the program and vertex array once. `uniforms` sets the shader values for each quad. Both
// callbacks run synchronously on the graphics thread and leave attachment and texture binding
// to the executor. Textures must have uploaded contents before calling this function.
void draw_window_quads(
    std::string_view name,
    std::array<int, 2> size,
    std::span<const RasterTexture> textures,
    const std::function<void()>& setup,
    const std::function<void(std::size_t)>& uniforms
);

}  // namespace openrac::renderer
