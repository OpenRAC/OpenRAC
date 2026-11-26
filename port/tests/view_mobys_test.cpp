// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests the split-screen views each moby is drawn in (game/common/frontend/view_mobys.h): the views
// whose own camera's distance test it passes, as the game's MobyProc runs once a view.

#include <cstdint>
#include <optional>
#include <vector>

#include "game/common/frontend/view_mobys.h"
#include "tests/check.h"

namespace {

using openrac::frontend::drawn_in_view;
using openrac::frontend::fade_in_views;
using openrac::frontend::kEveryView;

// Up Your Arsenal's two-player map: the second player stands far beyond the first player's draw
// distance. Tested from the first view's camera alone, the second view lost its own player.
void test_a_moby_only_one_camera_reaches() {
    const auto drawn = fade_in_views(0b11, [](std::uint32_t view) -> std::optional<std::uint8_t> {
        if (view == 1) {
            return std::uint8_t{0x80};
        }
        return std::nullopt;
    });
    CHECK(drawn.views == 0b10);
    CHECK(drawn.fade == std::optional<std::uint8_t>{0x80});
}

// Seen by both, at the strongest fade; by none, not drawn; untested views are not asked.
void test_both_none_and_untested_views() {
    const auto both = fade_in_views(0b11, [](std::uint32_t view) -> std::optional<std::uint8_t> {
        return view == 0 ? std::uint8_t{0x20} : std::uint8_t{0x60};
    });
    CHECK(both.views == 0b11);
    CHECK(both.fade == std::optional<std::uint8_t>{0x60});
    const auto none = fade_in_views(0b11, [](std::uint32_t) -> std::optional<std::uint8_t> {
        return std::nullopt;
    });
    CHECK(none.views == 0);
    CHECK(!none.fade);
    int asked = 0;
    const auto second = fade_in_views(0b10, [&](std::uint32_t view) -> std::optional<std::uint8_t> {
        ++asked;
        return view == 1 ? std::optional<std::uint8_t>{0x80} : std::nullopt;
    });
    CHECK(asked == 1);
    CHECK(second.views == 0b10);
}

// Each view's world: the mobys it draws, and every instance past the mobys' entries.
void test_each_view_takes_its_mobys() {
    const std::vector<int> items{10, 11, 12, 13};
    const std::vector<std::uint8_t> views{0b01, 0b10, kEveryView};
    CHECK((drawn_in_view(items, views, 0) == std::vector<int>{10, 12, 13}));
    CHECK((drawn_in_view(items, views, 1) == std::vector<int>{11, 12, 13}));
    CHECK((drawn_in_view(items, {}, 1) == items));
}

}  // namespace

int main() {
    test_a_moby_only_one_camera_reaches();
    test_both_none_and_untested_views();
    test_each_view_takes_its_mobys();
    return openrac::test::result();
}
