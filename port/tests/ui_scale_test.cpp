// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's scale (game/common/debug/ui_scale.h): 1 on a 1080p desktop, larger on a 4K
// one or where the system scales its content, and a phone's by its density alone.

#include "check.h"
#include "game/common/debug/ui_scale.h"

int main() {
    using openrac::frontend::debug_overlay::ui_scale;
    // 1080p at 100 %: as it always was.
    CHECK(ui_scale(1.0f, 1.0f, 1080, true) == 1.0f);
    // Smaller than 1080p never shrinks it.
    CHECK(ui_scale(1.0f, 1.0f, 720, true) == 1.0f);
    // 4K at 100 % and at 150 % (Windows: window units are pixels).
    CHECK(ui_scale(1.0f, 1.0f, 2160, true) == 2.0f);
    CHECK(ui_scale(1.5f, 1.0f, 2160, true) == 2.0f);
    // 1440p, a quarter step at a time.
    CHECK(ui_scale(1.0f, 1.0f, 1440, true) == 1.25f);
    // 1080p at 200 % (a small 4K laptop).
    CHECK(ui_scale(2.0f, 1.0f, 1080, true) == 2.0f);
    // macOS Retina: window units are points, the pixels already doubled.
    CHECK(ui_scale(2.0f, 2.0f, 1117, true) == 1.0f);
    // A phone: its density only, however tall its screen.
    CHECK(ui_scale(3.0f, 1.0f, 2400, false) == 3.0f);
    CHECK(ui_scale(1.0f, 1.0f, 2400, false) == 1.0f);
    // Unknown values fall back to 1.
    CHECK(ui_scale(0.0f, 0.0f, 0, true) == 1.0f);
    return openrac::test::result();
}
