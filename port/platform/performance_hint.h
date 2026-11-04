// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Telling the system how long each frame's work took and how long it may take, so that it keeps
// the CPU fast enough for the game. A phone's CPU governor sees a game thread that works a few
// milliseconds and then waits on the display, and lowers its clocks until frames are late; Android
// 12 and later (API 31; here through the NDK's APerformanceHint, API 33) take a hint session
// instead (Android's "Adaptive Performance Framework", ADPF). On a desktop, and where Android has
// no hint sessions, these do nothing. OPENRAC_ADPF_OFF leaves the session out, to compare a
// recording with and without it.
//
// The session covers the window thread, which runs the game and prepares and presents each frame,
// and is asked for the whole frame. A frame's work counts from the last swap (or frame cap wait),
// the wait for the next tick included, as in 0.10.0. That release held 120 fps on a Galaxy S26
// Ultra; a session asked for 85% of the frame, with the render thread and without the waits, came
// with drops to 40-90 fps the longer the game ran.

#pragma once

#include <string>

namespace openrac::platform {

// The time a frame may take, in seconds: the game's tick, or the display's refresh (or the
// player's frame cap) when frames are drawn between ticks. Starts the hint session the first time
// it is called, from the window thread, which it covers.
void set_frame_target(double seconds);

// A wait has just ended (for the display or the frame cap): the next frame's work counts from now.
void work_resumed();

// A frame's work is done, its swap about to wait for the display: the window thread's time since
// work_resumed() is reported as the frame's work.
void frame_work_done();

// For the diagnostics recording's header: whether a session runs and at what target, or why none
// does. Empty off Android.
std::string hint_status();

}  // namespace openrac::platform
