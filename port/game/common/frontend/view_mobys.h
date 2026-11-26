// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Split screen drawn in one capture (a picture that spans its sends, program/picture_lists.h): the
// game's MobyProc runs once a view, with that view's camera, so each view draws the mobys whose
// distance test passes from its own camera (host::moby_proc_distance_fade), not those of the first
// view's camera. The tick keeps, for each instance, the views that draw it (Kept::live_views), and
// each view's world takes those alone (frontend/split_views.cpp).

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace openrac::frontend {

// Every view: an instance no view tests (not a moby, or one drawn with a camera of its own).
inline constexpr std::uint8_t kEveryView = 0xFF;

// The views (bit n: view n) whose camera's test a moby passes, and the strongest fade of those;
// no view and no fade when none draws it.
struct ViewFade {
    std::uint8_t views = 0;
    std::optional<std::uint8_t> fade;
};

// `fade_in(n)`: the test from the camera of view n (none: undrawn there), for each view of `tested`
// (bit n: view n).
template <class FadeIn>
ViewFade fade_in_views(std::uint8_t tested, FadeIn&& fade_in) {
    ViewFade drawn;
    for (std::uint32_t n = 0; n < 8; ++n) {
        if (((tested >> n) & 1u) == 0) {
            continue;
        }
        const std::optional<std::uint8_t> fade = fade_in(n);
        if (!fade) {
            continue;
        }
        drawn.views = static_cast<std::uint8_t>(drawn.views | (1u << n));
        if (!drawn.fade || *fade > *drawn.fade) {
            drawn.fade = fade;
        }
    }
    return drawn;
}

// The `items` view `view` draws: one with an entry in `views` (bit n: view n) when the entry has
// that view's bit, every one past the entries' end.
template <class T>
std::vector<T> drawn_in_view(
    const std::vector<T>& items, std::span<const std::uint8_t> views, std::uint32_t view
) {
    std::vector<T> drawn;
    drawn.reserve(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i < views.size() && (view >= 8 || ((views[i] >> view) & 1u) == 0)) {
            continue;
        }
        drawn.push_back(items[i]);
    }
    return drawn;
}

}  // namespace openrac::frontend
