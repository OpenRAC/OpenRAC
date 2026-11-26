// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Colour transfers shared by UI, movie playback, and presentation.
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "renderer/graph/command_buffer.h"

namespace openrac::renderer {

// Replaces level zero of a 2D texture with tightly packed RGBA8 pixels on the graphics thread.
void upload_rgba8(unsigned texture, int width, int height, std::span<const std::uint8_t> rgba);

// Copies colour between framebuffers, using 0 for the window. Coordinates follow GL's bottom-up
// convention. `overwrite` declares that the rectangle covers the entire destination. A clear in
// `region` also defines the entire destination.
void blit_colour(
    std::string_view name,
    unsigned source,
    unsigned destination,
    const graph::BlitRegion& region,
    bool overwrite = false
);

// Reads tightly packed RGBA8 from a colour attachment, bottom row first, into the caller's buffer.
void read_colour(unsigned framebuffer, int width, int height, std::span<std::uint8_t> rgba);

}  // namespace openrac::renderer
