// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// How many images the swapchain asks for (vkgl_present.cpp's make_swapchain). Arithmetic on what
// it is given only, so that tests/swapchain_images_test.cpp can pin it.

#pragma once

#include <algorithm>
#include <cstdint>

namespace openrac::vkgl {

// Whether the system's compositor holds a presented image one refresh longer (Android's
// SurfaceFlinger takes a picture at one refresh, shows it at the next, and only then releases the
// one before).
#if defined(__ANDROID__)
constexpr bool kCompositorHoldsImage = true;
#else
constexpr bool kCompositorHoldsImage = false;
#endif

// The swapchain's image count for a surface that needs at least `min_images`. Normally one more
// than that and at least three, so a frame never waits for the display to release an image.
// Low-latency frames use as few as the surface allows (at least two), so fewer frames queue for
// the display. Where the compositor holds an image longer, two images make every frame wait a
// refresh (60 fps on a 120 Hz display), so low-latency frames keep three there; their single
// frame in flight on the GPU still keeps the queue short.
constexpr std::uint32_t swapchain_images(
    std::uint32_t min_images, bool low_latency, bool compositor_holds_image
) {
    return low_latency && !compositor_holds_image ? std::max(min_images, 2u)
                                                  : std::max(min_images + 1, 3u);
}

}  // namespace openrac::vkgl
