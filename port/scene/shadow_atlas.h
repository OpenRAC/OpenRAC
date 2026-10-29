// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The shadow atlas's layout: one depth target holding the four sun cascades and the hero's map,
// each in a rectangle of its own (level_renderer.cpp, make_shadow_atlas). Restructure package
// P2-39a moved it from enhanced/viewer/.

#pragma once
#include <array>

#include "renderer/shadow_settings.h"

namespace openrac::scene {
inline constexpr int kShadowAtlasSize = 3072;
// x, y, width, height; four sun cascades followed by the hero view.
inline constexpr std::array<std::array<int, 4>, 5> kShadowAtlasRects{
    {{0, 0, 2048, 2048},
     {2048, 0, 1024, 1024},
     {2048, 1024, 1024, 1024},
     {0, 2048, 1024, 1024},
     {1024, 2048, 512, 512}}
};

struct ShadowAtlasLayout {
    int size = kShadowAtlasSize;
    std::array<std::array<int, 4>, 5> rects = kShadowAtlasRects;
};

constexpr ShadowAtlasLayout shadow_atlas_layout(renderer::ShadowQuality quality) {
    ShadowAtlasLayout layout;
    const int divisor = renderer::shadow_resolution_divisor(quality);
    layout.size /= divisor;
    for (auto& rect : layout.rects) {
        for (auto& coordinate : rect) {
            coordinate /= divisor;
        }
    }
    layout.rects[4][2] = layout.rects[4][3] = renderer::hero_shadow_resolution(quality);
    return layout;
}
}  // namespace openrac::scene
