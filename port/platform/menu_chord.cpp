// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/menu_chord.h"

#include "platform/pad.h"

namespace openrac::platform {

std::uint16_t MenuChord::filter(std::uint16_t buttons, std::uint64_t now_ns) {
    constexpr auto kStart = static_cast<std::uint16_t>(Button::Start);
    constexpr auto kSelect = static_cast<std::uint16_t>(Button::Select);
    constexpr std::uint16_t kPair = kStart | kSelect;
    const auto held = static_cast<std::uint16_t>(~buttons & kPair);

    if (m_chord) {
        m_chord = held != 0;
        m_waiting = m_given = 0;
    } else if (held == kPair && m_given == 0) {
        // Both down and neither the game's yet: the menu's.
        m_chord = true;
        m_fired = true;
        m_waiting = m_given = 0;
    } else {
        const auto down = static_cast<std::uint16_t>(held & ~(m_waiting | m_given));
        if (down != 0 && m_waiting == 0) {
            m_since = now_ns;
        }
        m_waiting |= down;
        // Let go while it waited: a tap, which the game still gets.
        if (const auto tapped = static_cast<std::uint16_t>(m_waiting & ~held); tapped != 0) {
            m_taps |= tapped;
            m_tap_reads = kTapReads;
        }
        m_waiting &= held;
        m_given &= held;
        if (m_waiting != 0 && now_ns - m_since >= kWindowNs) {
            m_given |= m_waiting;
            m_waiting = 0;
        }
    }

    std::uint16_t out = buttons;
    if (m_chord) {
        out |= kPair;
    } else {
        out |= m_waiting;
    }
    if (m_taps != 0) {
        out = static_cast<std::uint16_t>(out & ~m_taps);
        if (m_tap_reads == 0 || --m_tap_reads == 0) {
            m_taps = 0;
        }
    }
    return out;
}

}  // namespace openrac::platform
