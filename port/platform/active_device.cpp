// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/active_device.h"

#include <cmath>

namespace openrac::platform {

bool plays_with_key(int scancode) {
    // SDL scancodes are USB HID usages: 4 (A) to 56 (/) are the letters, digits, Enter, Escape,
    // Backspace, Tab, Space and punctuation; 57 is Caps Lock; 58 to 69 F1 to F12.
    if (scancode >= 4 && scancode <= 56) {
        return true;
    }
    // Insert, Home, Page Up, Delete, End, Page Down, then the arrows (73 to 82), and the keypad
    // (83 Num Lock is not; 84 to 99 its keys).
    if ((scancode >= 73 && scancode <= 82) || (scancode >= 84 && scancode <= 99)) {
        return true;
    }
    // The modifiers: Ctrl, Shift, Alt on both sides (224 to 226, 228 to 230); not the system key
    // (227, 231).
    return (scancode >= 224 && scancode <= 226) || (scancode >= 228 && scancode <= 230);
}

void DeviceSwitch::use(ActiveDevice device) {
    if (device != m_active) {
        m_active = device;
        m_changed = true;
    }
    m_motion = 0.0f;
}

void DeviceSwitch::key(int scancode, std::uint64_t) {
    if (plays_with_key(scancode)) {
        use(ActiveDevice::KeyboardMouse);
    }
}

void DeviceSwitch::mouse_button(std::uint64_t) {
    use(ActiveDevice::KeyboardMouse);
}

void DeviceSwitch::wheel(std::uint64_t) {
    use(ActiveDevice::KeyboardMouse);
}

void DeviceSwitch::mouse_motion(float dx, float dy, std::uint64_t ns) {
    if (ns < m_settle_until) {
        return;
    }
    m_last_motion = ns;
    if (m_active == ActiveDevice::KeyboardMouse) {
        return;
    }
    if (ns - m_motion_from > kMotionNs) {
        m_motion = 0.0f;
        m_motion_from = ns;
    }
    m_motion += std::sqrt(dx * dx + dy * dy);
    if (m_motion >= kMotionPixels) {
        use(ActiveDevice::KeyboardMouse);
    }
}

void DeviceSwitch::gamepad(std::uint64_t) {
    use(ActiveDevice::Gamepad);
}

void DeviceSwitch::touch(std::uint64_t) {
    use(ActiveDevice::Gamepad);
}

}  // namespace openrac::platform
