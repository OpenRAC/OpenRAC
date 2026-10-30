// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The late camera latch's fade (frontend/camera_latch.cpp): where the latch's heading takes over
// from another (a new tick, the latch going on or off), the picture goes on turning as the heading
// did and the step to the new heading is carried, fading within a few frames, so that the picture
// never jumps. Arithmetic on what it is given only, so that tests/frontend_latch_fade_test.cpp can
// pin it with frame rates at, under and over the game's tick rate.

#pragma once

namespace openrac::frontend {

// a - b in radians, the short way round.
float angle_between(float a, float b);

class LatchFade {
public:
    // How fast a carried step fades: down to a third after this long.
    static constexpr double kFadeSeconds = 0.05;
    // A step bigger than this is a cut (another camera, a teleport): shown at once, not carried.
    static constexpr float kCutRadians = 0.6f;

    // The heading a frame shows, `since` seconds after the last frame (ignored for the first).
    // `target`: the latch's heading for the frame (the interpolated camera's, turned by the latch
    // when `on`). `new_tick`: the frame's kept tick is not the last frame's. `tick_rate`: what the
    // game's camera turned in the kept tick, radians a second, the rate the heading goes on at
    // across a new tick. `enabled` false (the setting off, no frames between ticks): nothing is
    // carried, the target is shown.
    //
    // The rate the heading goes on at is the heading's own, never the picture's: with one frame a
    // tick (a frame cap at the game's tick rate) every frame is a new tick, and a rate read from
    // a picture that the carry held still was none, so that the picture stood still until the
    // step grew to a cut and then jumped.
    float frame(float target, bool on, bool new_tick, float tick_rate, double since, bool enabled);

    float carry() const { return m_carry; }

private:
    float m_heading = 0.0f;  // the last frame's target
    float m_carry = 0.0f;    // what the picture is off the target by, fading
    float m_rate = 0.0f;     // how fast the target turned, radians a second
    bool m_was_on = false;   // the latch turned the last frame
    bool m_shown = false;    // a frame was shown
};

}  // namespace openrac::frontend
