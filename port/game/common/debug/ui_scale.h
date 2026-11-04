// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// How large the debug overlay's text and rows are drawn, against their size on a 1080p desktop.

#pragma once

#include <algorithm>
#include <cmath>

namespace openrac::frontend::debug_overlay {

// display_scale and pixel_density as SDL gives them for the window (SDL_GetWindowDisplayScale,
// SDL_GetWindowPixelDensity); display_height the display's height in window units (its current
// mode's h), 0 when unknown. The system's own content scale (Windows at 150 %, a phone's density)
// counts everywhere; on a desktop the display's height over 1080, a quarter at a time, counts as
// well, so that a 4K display left at 100 % still gets twice the size. 1 at 1080p.
inline float ui_scale(float display_scale, float pixel_density, int display_height, bool desktop) {
    float scale =
        display_scale > 0.0f && pixel_density > 0.0f ? display_scale / pixel_density : 1.0f;
    if (desktop && display_height > 0) {
        scale =
            std::max(scale, std::floor(static_cast<float>(display_height) / 1080.0f * 4.0f) / 4.0f);
    }
    return std::clamp(scale, 1.0f, 4.0f);
}

}  // namespace openrac::frontend::debug_overlay
