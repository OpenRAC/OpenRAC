// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Start+Select opens the menu without the game seeing it (platform/menu_chord.h): the pair never
// reaches the game, a lone Start or Select reaches it after the wait or as a tap when let go
// sooner, and one held past the wait stays the game's.
#include <cstdint>

#include "platform/menu_chord.h"
#include "platform/pad.h"
#include "tests/check.h"

namespace {

using openrac::platform::Button;
using openrac::platform::MenuChord;

constexpr auto kStart = static_cast<std::uint16_t>(Button::Start);
constexpr auto kSelect = static_cast<std::uint16_t>(Button::Select);
constexpr auto kCross = static_cast<std::uint16_t>(Button::Cross);
constexpr std::uint64_t kFrame = 16'666'667;

// The pad's active-low word with `held` down.
std::uint16_t pad(std::uint16_t held) {
    return static_cast<std::uint16_t>(0xFFFF & ~held);
}

bool down(std::uint16_t buttons, std::uint16_t bit) {
    return (buttons & bit) == 0;
}

void test_pair() {
    MenuChord c;
    std::uint64_t t = 0;
    // Start first, Select two frames later: neither reaches the game, the menu opens once.
    CHECK(!down(c.filter(pad(kStart), t), kStart));
    t += kFrame;
    CHECK(!down(c.filter(pad(kStart), t), kStart));
    t += kFrame;
    std::uint16_t out = c.filter(pad(kStart | kSelect | kCross), t);
    CHECK(!down(out, kStart) && !down(out, kSelect));
    CHECK(down(out, kCross));  // the other buttons are the game's as ever
    CHECK(c.take_fired());
    CHECK(!c.take_fired());
    // Held long, then let go one after the other: still nothing for the game, no second menu.
    for (int i = 0; i < 20; ++i) {
        t += kFrame;
        out = c.filter(pad(kStart | kSelect), t);
        CHECK(!down(out, kStart) && !down(out, kSelect));
    }
    t += kFrame;
    CHECK(!down(c.filter(pad(kSelect), t), kSelect));
    t += kFrame;
    c.filter(pad(0), t);
    CHECK(!c.take_fired());
    // Both in one frame.
    t += kFrame;
    out = c.filter(pad(kStart | kSelect), t);
    CHECK(!down(out, kStart) && !down(out, kSelect));
    CHECK(c.take_fired());
}

void test_lone_held() {
    MenuChord c;
    std::uint64_t t = 0;
    std::uint16_t out = 0;
    int waited = 0;
    for (; waited < 20; ++waited) {
        out = c.filter(pad(kStart), t);
        if (down(out, kStart)) {
            break;
        }
        t += kFrame;
    }
    CHECK(waited == 6);  // MenuChord::kWindowNs, 100 ms
    // Held past the wait it is the game's: Select now is the game's too (its map), no menu.
    t += kFrame;
    out = c.filter(pad(kStart | kSelect), t);
    CHECK(down(out, kStart));
    t += MenuChord::kWindowNs;
    out = c.filter(pad(kStart | kSelect), t);
    CHECK(down(out, kStart) && down(out, kSelect));
    CHECK(!c.take_fired());
}

void test_tap() {
    MenuChord c;
    std::uint64_t t = 0;
    CHECK(!down(c.filter(pad(kSelect), t), kSelect));
    t += kFrame;
    // Let go before the wait ends: the game still gets the press, until it reads the pad.
    CHECK(down(c.filter(pad(0), t), kSelect));
    t += kFrame;
    CHECK(down(c.filter(pad(0), t), kSelect));
    c.read();
    t += kFrame;
    CHECK(!down(c.filter(pad(0), t), kSelect));
    CHECK(!c.take_fired());
    // A game that does not read the pad (a pause) does not get the tap later on.
    CHECK(!down(c.filter(pad(kStart), t), kStart));
    t += kFrame;
    c.filter(pad(0), t);
    for (int i = 0; i < MenuChord::kTapReads; ++i) {
        t += kFrame;
        c.filter(pad(0), t);
    }
    CHECK(!down(c.filter(pad(0), t), kStart));
}

}  // namespace

int main() {
    test_pair();
    test_lone_held();
    test_tap();
    return openrac::test::result();
}
