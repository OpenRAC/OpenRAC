// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <algorithm>
#include <array>

#include "scene/shadow_atlas.h"
#include "tests/check.h"

using namespace openrac;

// Test the actual clear rectangles against every possible set of retained far cascades.
void check_clears(renderer::ShadowQuality quality, unsigned kept, bool hero) {
    const auto layout = scene::shadow_atlas_layout(quality);
    unsigned drawn = 0;

    for (const unsigned views : scene::shadow_atlas_redraw_groups(kept, hero)) {
        if (!views) {
            continue;
        }

        CHECK((drawn & views) == 0);
        drawn |= views;
        int low_x = layout.size, low_y = layout.size, high_x = 0, high_y = 0;

        for (unsigned i = 0; i < layout.rects.size(); ++i) {
            if (views & (1u << i)) {
                const auto& r = layout.rects[i];
                low_x = std::min(low_x, r[0]);
                low_y = std::min(low_y, r[1]);
                high_x = std::max(high_x, r[0] + r[2]);
                high_y = std::max(high_y, r[1] + r[3]);
            }
        }

        for (unsigned i = 1; i < 4; ++i) {
            if (kept & (1u << i)) {
                const auto& r = layout.rects[i];
                CHECK(
                    high_x <= r[0] || low_x >= r[0] + r[2] || high_y <= r[1] || low_y >= r[1] + r[3]
                );
            }
        }
    }

    CHECK(drawn == ((15u & ~kept) | (hero ? 16u : 0u)));
}

int main() {
    // Redrawing cascades 1 and 3 must leave the retained middle cascade untouched.
    constexpr std::array<unsigned, 4> separated{17u, 2u, 8u, 0u};
    CHECK(scene::shadow_atlas_redraw_groups(4u, true) == separated);
    // Adjacent far cascades still share a pass.
    constexpr std::array<unsigned, 4> adjacent{17u, 12u, 0u, 0u};
    CHECK(scene::shadow_atlas_redraw_groups(2u, true) == adjacent);

    for (const auto quality :
         {renderer::ShadowQuality::Low,
          renderer::ShadowQuality::Medium,
          renderer::ShadowQuality::High}) {
        for (unsigned kept = 0; kept < 16; kept += 2) {
            for (const bool hero : {false, true}) {
                check_clears(quality, kept, hero);
            }
        }
    }

    return test::result();
}
