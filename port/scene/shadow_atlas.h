// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The shadow atlas's layout: one depth target holding the four sun cascades and the hero's map,
// each in a rectangle of its own (shadow_atlas_renderer.cpp, make_shadow_atlas).

#pragma once
#include <array>
#include <cstddef>

#include "renderer/settings/shadow_settings.h"

namespace openrac::scene {
inline constexpr int kShadowAtlasSize = 3072;
// x, y, width, height; four sun cascades followed by the hero view. The nearest cascade and the
// hero's map, drawn every frame, share the top; cascades 1 to 3 sit in a row below, so that a frame
// that keeps some of them (shadow-far-alternate) draws the others in a pass that covers only their
// own rectangles and neither loads nor stores the kept ones.
inline constexpr std::array<std::array<int, 4>, 5> kShadowAtlasRects{
    {{0, 0, 2048, 2048},
     {0, 2048, 1024, 1024},
     {1024, 2048, 1024, 1024},
     {2048, 2048, 1024, 1024},
     {2048, 0, 512, 512}}
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

// Each group is cleared through its bounding rectangle. Far cascades may share a pass only
// while they are adjacent, so the clear never crosses a retained cascade between them.
constexpr std::array<unsigned, 4> shadow_atlas_redraw_groups(unsigned kept, bool hero) {
    std::array<unsigned, 4> groups{1u | (hero ? 1u << 4 : 0u)};
    std::size_t next = 1;
    unsigned run = 0;

    for (unsigned i = 1; i < 4; ++i) {
        if (!(kept & (1u << i))) {
            run |= 1u << i;
        } else if (run) {
            groups[next++] = run;
            run = 0;
        }
    }

    if (run) {
        groups[next] = run;
    }

    return groups;
}
}  // namespace openrac::scene
