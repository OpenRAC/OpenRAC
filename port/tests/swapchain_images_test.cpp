// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The swapchain's image count (renderer/vulkan/swapchain_images.h): low-latency frames take the
// fewest images, except where the compositor holds one a refresh longer (Android), where two
// images would halve the frame rate.
#include "renderer/vulkan/swapchain_images.h"
#include "tests/check.h"

namespace {

using openrac::vkgl::swapchain_images;

void test_normal_frames() {
    CHECK(swapchain_images(1, false, false) == 3);
    CHECK(swapchain_images(2, false, false) == 3);
    CHECK(swapchain_images(3, false, false) == 4);
    CHECK(swapchain_images(2, false, true) == 3);
}

void test_low_latency_frames() {
    CHECK(swapchain_images(1, true, false) == 2);
    CHECK(swapchain_images(2, true, false) == 2);
    CHECK(swapchain_images(3, true, false) == 3);
}

// Android: two images would let each frame start only once the one before left the screen.
void test_low_latency_frames_under_a_compositor() {
    CHECK(swapchain_images(1, true, true) == 3);
    CHECK(swapchain_images(2, true, true) == 3);
    CHECK(swapchain_images(3, true, true) == 4);
}

}  // namespace

int main() {
    test_normal_frames();
    test_low_latency_frames();
    test_low_latency_frames_under_a_compositor();
    return openrac::test::result();
}
