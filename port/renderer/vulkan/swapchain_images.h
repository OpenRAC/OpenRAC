// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// How many images the swapchain asks for (vkgl_present.cpp's make_swapchain). Arithmetic on what
// it is given only, so that tests/swapchain_images_test.cpp can pin it.

#pragma once

#include <algorithm>
#include <cstdint>

namespace openrac::vkgl {

// Whether the system's compositor keeps an image of the window's for a refresh more after it was
// presented (Android's SurfaceFlinger: it latches a picture at one refresh and shows it at the
// next, and lets the one before go only then).
#if defined(__ANDROID__)
constexpr bool kCompositorHoldsImage = true;
#else
constexpr bool kCompositorHoldsImage = false;
#endif

// The images for a swapchain whose surface needs at least `min_images`: one more than that, and
// at least three, so that a frame never waits for the display to let go of one; the player's
// low-latency frames as few as the surface allows (at least two), fewer frames queued for the
// display. Not where the compositor holds an image a refresh longer: with two there, each frame
// waits for the one before it to leave the screen, and a frame takes two refreshes (60 frames a
// second on a 120 Hz display), so low-latency frames keep the three as well. Their one frame in
// flight on the graphics card keeps the queue short by itself.
constexpr std::uint32_t swapchain_images(
    std::uint32_t min_images, bool low_latency, bool compositor_holds_image
) {
    return low_latency && !compositor_holds_image ? std::max(min_images, 2u)
                                                  : std::max(min_images + 1, 3u);
}

}  // namespace openrac::vkgl
