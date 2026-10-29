// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Synthetic colour/depth/motion inputs shared by the blur regression scenarios.
#pragma once

#include <array>
#include <vector>

#include "enhanced/fsr3/motion_blur.h"
#include "renderer/framebuffer.h"

namespace openrac::test::motion_blur {
// Small square fixtures keep exact full-frame comparisons inexpensive.
inline constexpr int kSize = 64;
using Pixels = std::vector<std::array<unsigned char, 4>>;
// Reference frames are collected first, then checked against the direct output path.
extern std::vector<Pixels> reference_frames;
extern std::size_t compared_frames;
extern bool compare_direct;
// Records or compares the complete output, including untouched pixels and alpha.
void compare_pixels(const Pixels& result);

// Owns synthetic input targets and the production blur pipeline for one scenario.
struct Fixture {
    renderer::FrameBuffer output;
    enhanced::fsr3_detail::FrameTarget motion, depth;
    enhanced::fsr3_detail::MotionBlur blur;
    Pixels colour = Pixels(kSize * kSize, {0, 0, 255, 255});
    std::vector<std::array<float, 4>> vectors = std::vector<std::array<float, 4>>(kSize * kSize);
    std::vector<std::array<float, 4>> depths =
        std::vector<std::array<float, 4>>(kSize * kSize, {.5f, 0, 0, 0});
    renderer::Mat4 previous = renderer::identity();
    renderer::Mat4 inverse_raster = renderer::identity();
    renderer::Vec3 reference_delta{};
    bool camera_relative = false;
    renderer::MotionBlurSettings tuning;
    std::array<float, 2> jitter{};

    // Allocates colour, motion and depth targets in the test context.
    bool create(std::string& error);
    // Adds a foreground red square with eight pixels of horizontal motion.
    void moving_square(bool reject = false);
    // Uploads inputs and captures the production result for complete-frame comparison.
    Pixels render(float strength = 1.f, float ms = 16.666667f);
    // Releases blur resources before the enclosing GL context is destroyed.
    ~Fixture();
};
}  // namespace openrac::test::motion_blur
