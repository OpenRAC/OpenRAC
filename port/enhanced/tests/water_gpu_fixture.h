// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Shared readback state for water solver, optical and scrolling-window regressions.
#pragma once
#include <array>
#include <string>
#include <vector>

#include "renderer/shader.h"

namespace openrac::test::water {
using Cell = std::array<float, 4>;
inline constexpr unsigned kWidth = 16, kCount = kWidth * kWidth;

struct Context {
    renderer::Shader& compute;
    renderer::Shader& read;
    std::string& error;
    std::vector<unsigned char>& pixels;
    std::array<unsigned, 5>& buffers;
    const std::array<Cell, 5>& bodies;
    unsigned texture, image_buffer;
    const char* vertex;
};

// Compare swell, optical shading and smooth/fine normals with independent analytic values.
void check_optics(Context& fixture);
// Check breaking-crest foam and the shared height/velocity differential samples.
void check_foam(Context& fixture);
// Check screen-space depth reconstruction, refraction rejection and single composition.
void check_transmission(Context& fixture);
// Check the actual packed reflection atlas and box-projected spatial blend.
void check_environment(Context& fixture);
// Check toroidal history, adjacent body metadata, seam sampling and lighting receivers.
void check_window(Context& fixture);
}  // namespace openrac::test::water
