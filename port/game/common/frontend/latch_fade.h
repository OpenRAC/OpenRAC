// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The late camera latch's fade (frontend/camera_latch.cpp). When the latch's heading changes
// suddenly (a new tick, the latch turning on or off), the picture keeps turning at the same rate
// and the step is carried as an offset that fades within a few frames, so the picture never jumps.
// Pure arithmetic, so that tests/frontend_latch_fade_test.cpp can check it at any frame rate.

#pragma once

namespace openrac::frontend {

// a - b in radians, the short way round.
float angle_between(float a, float b);

class LatchFade {
public:
    // How fast a carried offset fades: to about a third after this long.
    static constexpr double kFadeSeconds = 0.05;
    // A bigger step is a cut (another camera, a teleport): shown at once, not carried.
    static constexpr float kCutRadians = 0.6f;

    // The heading to show for a frame drawn `since` seconds after the last one.
    // `target`: the latch's heading for this frame. `on`: the latch turned it.
    // `new_tick`: this frame shows a different tick than the last frame.
    // `tick_rate`: how fast the game's camera turned in that tick, in radians a second.
    // `enabled`: false shows `target` as it is, with nothing carried.
    //
    // Across a new tick the expected heading moves on at `tick_rate`, never at the rate the
    // picture itself turned. With one frame a tick (a frame cap at the tick rate) every frame is a
    // new tick: a rate read from the picture was zero once the carry held it still, so the picture
    // froze until the step grew into a cut and jumped.
    float frame(float target, bool on, bool new_tick, float tick_rate, double since, bool enabled);

    float carry() const { return m_carry; }

    bool was_on() const { return m_was_on; }  // `on` of the last frame

private:
    float m_heading = 0.0f;  // the last frame's target
    float m_carry = 0.0f;    // the picture's offset from the target, fading
    float m_rate = 0.0f;     // how fast the target turns, radians a second
    bool m_was_on = false;
    bool m_shown = false;  // a frame was shown
};

}  // namespace openrac::frontend
