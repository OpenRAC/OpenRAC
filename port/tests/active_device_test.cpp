// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which device the player uses now (platform/active_device.h): the game starts on the gamepad; a
// key that plays, a click, the wheel or a real push of the mouse switches to the keyboard and
// mouse; a gamepad or a finger switches back; system keys, a nudge and SDL's own motion do not.
#include <cstdint>

#include "platform/active_device.h"
#include "tests/check.h"

namespace {

using openrac::platform::ActiveDevice;
using openrac::platform::DeviceSwitch;
using openrac::platform::plays_with_key;

constexpr std::uint64_t kMs = 1'000'000;

void test_keys() {
    CHECK(plays_with_key(4));     // A
    CHECK(plays_with_key(26));    // W
    CHECK(plays_with_key(44));    // Space
    CHECK(plays_with_key(41));    // Escape
    CHECK(plays_with_key(82));    // Up
    CHECK(plays_with_key(225));   // Left Shift
    CHECK(!plays_with_key(58));   // F1
    CHECK(!plays_with_key(69));   // F12
    CHECK(!plays_with_key(227));  // Left Windows / Cmd
    CHECK(!plays_with_key(226));  // Left Alt (Alt+Tab)
    CHECK(!plays_with_key(230));  // Right Alt
    CHECK(!plays_with_key(128));  // Volume Up
    CHECK(!plays_with_key(270));  // AC Back (a phone's Back)
}

void test_starts_on_gamepad() {
    DeviceSwitch d;
    CHECK(d.active() == ActiveDevice::Gamepad);
    CHECK(!d.take_changed());
}

void test_keyboard_and_back() {
    DeviceSwitch d;
    d.key(58, 10 * kMs);  // F1: the port's key
    CHECK(d.active() == ActiveDevice::Gamepad);
    d.key(26, 20 * kMs);  // W
    CHECK(d.active() == ActiveDevice::KeyboardMouse);
    CHECK(d.take_changed());
    CHECK(!d.take_changed());
    d.gamepad(30 * kMs);
    CHECK(d.active() == ActiveDevice::Gamepad);
    CHECK(d.take_changed());
    d.mouse_button(40 * kMs);
    CHECK(d.active() == ActiveDevice::KeyboardMouse);
    d.touch(50 * kMs);
    CHECK(d.active() == ActiveDevice::Gamepad);
    d.wheel(60 * kMs);
    CHECK(d.active() == ActiveDevice::KeyboardMouse);
}

void test_motion() {
    DeviceSwitch d;
    // A nudge: three pixels, then three more long after.
    d.mouse_motion(3.0f, 0.0f, 100 * kMs);
    d.mouse_motion(0.0f, 3.0f, 600 * kMs);
    CHECK(d.active() == ActiveDevice::Gamepad);
    CHECK(d.last_motion() == 600 * kMs);
    // A push: six pixels within a quarter of a second.
    d.mouse_motion(2.0f, 2.0f, 700 * kMs);
    d.mouse_motion(3.0f, 0.0f, 750 * kMs);
    CHECK(d.active() == ActiveDevice::KeyboardMouse);
    // SDL's motion as the cursor is let go does not count, nor move the last motion.
    d.gamepad(800 * kMs);
    d.settle(800 * kMs);
    d.mouse_motion(400.0f, 300.0f, 850 * kMs);
    CHECK(d.active() == ActiveDevice::Gamepad);
    CHECK(d.last_motion() == 750 * kMs);
    d.mouse_motion(10.0f, 0.0f, 950 * kMs);
    CHECK(d.active() == ActiveDevice::KeyboardMouse);
}

}  // namespace

int main() {
    test_keys();
    test_starts_on_gamepad();
    test_keyboard_and_back();
    test_motion();
    return openrac::test::result();
}
