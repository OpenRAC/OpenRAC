// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Which device the player is using now: a gamepad (or the touch screen) or the keyboard and mouse.
// The game starts on the gamepad. A key that plays (not F1 to F12, a volume or media key, or a
// phone's Back), a mouse button, the wheel or a real push of the mouse switches to the keyboard
// and mouse; a gamepad's button, a stick or trigger pushed well out, or a finger on the screen
// switches back. Both always press the game's buttons: this only says which one the cursor and
// the camera follow (frontend/input.cpp captures the cursor only for the keyboard and mouse).
//
// As other native ports do it (Dusklight's cursor, docs/port/KEYBOARD_MOUSE.md): a nudge of the
// desk does not count, so the mouse must move a few pixels within a quarter of a second, and the
// motion SDL reports while the cursor is captured or let go is not the player's.

#pragma once

#include <cstdint>

namespace openrac::platform {

enum class ActiveDevice {
    Gamepad,
    KeyboardMouse
};

// Whether a key (an SDL scancode) is one the player plays with, so that pressing it switches to
// the keyboard: letters, digits, Enter, Escape, Backspace, Tab, Space, punctuation, the arrows,
// the keypad and the modifiers but the system keys (Windows, Cmd).
bool plays_with_key(int scancode);

class DeviceSwitch {
public:
    // How far, in the window's pixels, the mouse must move within kMotionNs to count.
    static constexpr float kMotionPixels = 6.0f;
    static constexpr std::uint64_t kMotionNs = 250'000'000;

    ActiveDevice active() const { return m_active; }

    // Each takes the event's time (SDL's ns).
    void key(int scancode, std::uint64_t ns);
    void mouse_button(std::uint64_t ns);
    void wheel(std::uint64_t ns);
    void mouse_motion(float dx, float dy, std::uint64_t ns);
    void gamepad(std::uint64_t ns);
    void touch(std::uint64_t ns);

    // The cursor was captured or let go at `ns`: motion until `ns` + kSettleNs is SDL's, not the
    // player's (a warp to the window's middle, the cursor put back).
    static constexpr std::uint64_t kSettleNs = 100'000'000;

    void settle(std::uint64_t ns) { m_settle_until = ns + kSettleNs; }

    // When the mouse last moved (SDL's ns), for hiding a cursor left alone; 0 never.
    std::uint64_t last_motion() const { return m_last_motion; }

    // Whether the device changed since the last call.
    bool take_changed() {
        const bool changed = m_changed;
        m_changed = false;
        return changed;
    }

private:
    void use(ActiveDevice device);

    ActiveDevice m_active = ActiveDevice::Gamepad;
    bool m_changed = false;
    std::uint64_t m_settle_until = 0;
    std::uint64_t m_last_motion = 0;
    // The motion gathered toward kMotionPixels, since m_motion_from.
    float m_motion = 0.0f;
    std::uint64_t m_motion_from = 0;
};

}  // namespace openrac::platform
