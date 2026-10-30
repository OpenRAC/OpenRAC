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
// The session covers the threads that do a frame's work: the window thread, which runs the game
// and prepares and presents each frame, and the render thread, which records the world (Vulkan).
// A frame's work is the longer of the two: they overlap, the render thread drawing one frame while
// the window thread prepares the next, so the slower one sets the frame rate.

#pragma once

#include <string>

namespace openrac::platform {

// The time a frame may take, in seconds: the game's tick, or the display's refresh (or the
// player's frame cap) when frames are drawn between ticks. Starts the hint session the first time
// it is called, from the window thread, which it covers.
void set_frame_target(double seconds);

// The calling thread does frame work as well (the render thread): the session takes it in.
void add_this_thread();

// A wait has just ended (for the display, the frame cap or the next tick): the next frame's work
// counts from now.
void work_resumed();

// The window thread has waited this long since work_resumed() (for the render thread): not its
// work.
void work_waited(double seconds);

// The render thread's work on the frame it has just finished, in seconds.
void render_work_done(double seconds);

// A frame's work is done, its swap about to wait for the display: the window thread's time since
// work_resumed(), without its waits, or the render thread's last frame when that is longer, is
// reported as the frame's work.
void frame_work_done();

// A new level is drawn: the session's history of the last level's frames (and of the load's long
// ones) says nothing of the frames to come (Android 16, API 36; nothing before).
void workload_reset();

// For the diagnostics recording's header: whether a session runs, for how many threads and at
// what target, or why none does. Empty off Android.
std::string hint_status();

}  // namespace openrac::platform
