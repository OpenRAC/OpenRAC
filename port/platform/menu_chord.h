// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Start and Select pressed together open the port's menu, and the game never sees the pair: no
// pause, no map. A lone Start or Select waits up to kWindowNs for the other before it reaches the
// game; let go sooner, it still reaches the game as a tap. Held past the wait, it is the game's,
// and a later press of the other is the game's too (no menu then).

#pragma once

#include <cstdint>
#include <utility>

namespace openrac::platform {

class MenuChord {
public:
    // How long a lone Start or Select waits for the other: about 6 frames at 60.
    static constexpr std::uint64_t kWindowNs = 100'000'000;
    // How many reads of the pad a tap stays held for at most, if the game does not read it.
    static constexpr std::uint8_t kTapReads = 6;

    // The pad's buttons (active low, pad.h) as the game should see them at `now_ns`.
    std::uint16_t filter(std::uint16_t buttons, std::uint64_t now_ns);

    // Says whether the pair was pressed since the last call.
    bool take_fired() { return std::exchange(m_fired, false); }

    // The game has read the pad: a tap given to it counts no more.
    void read() { m_taps = 0; }

    // Forgets what waits (Input holds what is held back until let go).
    void reset() { *this = MenuChord{}; }

private:
    std::uint16_t m_waiting = 0;  // held, not given to the game yet
    std::uint16_t m_given = 0;    // held and the game's
    std::uint16_t m_taps = 0;     // let go while waiting: given until the game reads the pad
    std::uint8_t m_tap_reads = 0;
    std::uint64_t m_since = 0;  // when the first of m_waiting went down
    bool m_chord = false;       // the pair is down: neither is the game's until both are let go
    bool m_fired = false;
};

}  // namespace openrac::platform
