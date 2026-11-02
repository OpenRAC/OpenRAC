// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic game callbacks check shared shadow selection without a renderer or guest memory.

#include "game/common/frontend/hero_shadows.h"
#include "tests/check.h"

namespace {

// Synthetic moby addresses distinguish the inactive body, controlled body and an attachment.
constexpr gaddr kInactive = 0x100;
constexpr gaddr kControlled = 0x200;
constexpr gaddr kAttachment = 0x300;

// Supplies a body different from the class-based fallback, as when control switches to Clank.
unsigned controlled_body(gaddr* addresses, unsigned capacity) {
    // Production always supplies room for at least the body and one attachment.
    if (capacity < 2) {
        return 0;
    }

    addresses[0] = kControlled;
    addresses[1] = kAttachment;
    return 2;
}

// Original volumes select the same controlled body as enhanced maps, without claiming attachments.
void test_both_shadow_paths_select_the_controlled_body() {
    openrac_front_end game{};
    game.levels = 1;
    game.hero_shadow_casters = controlled_body;

    // Each path must work independently; enabling both must not change ownership.
    for (unsigned paths = 1; paths <= 3; ++paths) {
        const auto rig = openrac::frontend::read_hero_shadow_rig(&game, 0, paths & 1, paths & 2);
        CHECK(rig.count == 2);
        CHECK(rig.anchors(kControlled, false));
        CHECK(!rig.anchors(kInactive, true));
        CHECK(!rig.anchors(kAttachment, false));
        CHECK(rig.owns(kControlled));
        CHECK(rig.owns(kAttachment));
        CHECK(!rig.owns(kInactive));
    }
}

// Missing callbacks and invalid level states preserve the old fallback without reading ownership.
void test_unavailable_rigs_keep_the_fallback() {
    openrac_front_end game{};
    game.levels = 1;
    game.hero_shadow_casters = controlled_body;
    CHECK(openrac::frontend::read_hero_shadow_rig(&game, 0, false, false).count == 0);
    CHECK(openrac::frontend::read_hero_shadow_rig(&game, -1, true, true).count == 0);
    CHECK(openrac::frontend::read_hero_shadow_rig(&game, 1, true, true).count == 0);
    CHECK(openrac::frontend::read_hero_shadow_rig(nullptr, 0, true, true).count == 0);
    game.hero_shadow_casters = nullptr;
    const auto rig = openrac::frontend::read_hero_shadow_rig(&game, 0, true, true);
    CHECK(rig.count == 0);
    CHECK(rig.anchors(kInactive, true));
    CHECK(!rig.anchors(kControlled, false));
}

}  // namespace

int main() {
    test_both_shadow_paths_select_the_controlled_body();
    test_unavailable_rigs_keep_the_fallback();
    return openrac::test::result();
}
