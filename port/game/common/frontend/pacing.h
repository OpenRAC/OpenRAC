// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frontend's pacing arithmetic for the frames drawn between ticks: the refresh a frame reaches,
// how far from one tick to the next the scene drawn for that refresh is, and whether the frame cap
// lets a frame start. Arithmetic on what it is given only: frontend.cpp passes in its pacing state
// and the cap it works out from the settings (frame_cap), so that tests/frontend_pacing_test.cpp
// can pin each result. The cap's own arithmetic and its schedule are frame_pacing.h's.

#pragma once

#include <chrono>

namespace openrac::frontend {

// The frames between ticks show the game half a tick late, so that the next tick is ready before
// the refresh that needs it.
constexpr double kLagTicks = 0.5;

// The refresh a frame started at `now` is shown at: the first after the frame is drawn. `cost`:
// what drawing a frame takes, in seconds. `interval`: the display's refresh interval in seconds, 0
// when the display does not say. `last_swap`: the last swap that waited for a refresh, the clock's
// epoch when none did. Without an interval or such a swap, the time the frame is drawn by.
std::chrono::steady_clock::time_point shown_at(
    std::chrono::steady_clock::time_point now,
    double cost,
    double interval,
    std::chrono::steady_clock::time_point last_swap
);

// The player's low-latency frames do not hold the scene back: between() then stops at the next
// tick, which starts on time, and the frames drawn before it reach the kept tick. Without a swap
// that waits, the lag would only let them reach half way before the next tick came (the scene
// stood still for half a tick and then jumped).
constexpr double kLowLatencyLagTicks = 0.0;

// The time from which between() draws no more frames for the kept tick. `next_tick`: when the next
// tick is due; `lag`: how many ticks of `period` the scene runs behind; `cost`, `interval` and
// `last_swap` as for shown_at. With a lag: the next tick plus the lag. Without one (low-latency
// frames): the refresh the next tick's own frame is shown at, so the refresh before it still gets
// a frame (else a 120 Hz display showed one frame a tick); the next tick itself when the refreshes
// are not known.
std::chrono::steady_clock::time_point between_limit(
    std::chrono::steady_clock::time_point next_tick,
    std::chrono::duration<double> period,
    double lag,
    double cost,
    double interval,
    std::chrono::steady_clock::time_point last_swap
);

// How far the scene is from the last tick's to the kept one's at the refresh `display`, from 0 to
// 1: `tick` is when the kept tick came, `period` the game's tick length, and the scene runs `lag`
// ticks behind.
float tick_alpha(
    std::chrono::steady_clock::time_point display,
    std::chrono::steady_clock::time_point tick,
    std::chrono::duration<double> period,
    double lag = kLagTicks
);

// Whether the frame cap lets a frame start at `now` (within a millisecond: a sleep wakes that
// coarsely). `cap`: the frames a second it allows, 0 or less for none. `due`: when the cap lets the
// next frame start (frame_pacing::next_due), the clock's epoch before the first capped frame.
bool frame_may_start(
    double cap, std::chrono::steady_clock::time_point due, std::chrono::steady_clock::time_point now
);

// Which edge of a frame the frame cap holds to its time (the names are RTSS's limiter modes).
enum class CapEdge {
    // Its start (RTSS's back edge sync): the frame reads its time and input as late as the cap
    // lets it, and is shown when it is drawn, so its frame time moves with what it costs.
    Start,
    // Its showing (RTSS's front edge sync): the frame starts early enough to be drawn by its time
    // and waits for that time before the swap, so each picture reaches the screen a cap's frame
    // after the last and shows the moment it is shown at. The evenest frame times, for a little
    // latency: the frame's time and input are read its cost (and a margin) before it is shown.
    Present,
};

// How long before its time a frame held by its showing starts: what a frame costs (`cost`), three
// times how much that moves from frame to frame (`spread`) and a margin, in seconds, and no more
// than the cap's frame (`step`).
double present_lead(double cost, double spread, double step);

// When the frame after one shown at `shown` is to be shown, held by its showing (`previous`: when
// that one was to be, the clock's epoch for none). A cap's frame (`step`) after the last one's
// time, so that a frame shown late is made up for by the next, as frame_pacing::next_due counts.
// With no more than a frame a tick (`step` no shorter than the tick's `period`), each frame is a
// tick's own, drawn after the tick's work: one shown late moves the times to it, so that they
// settle where that frame is drawn by its time (else every one of them was late, and uneven).
std::chrono::steady_clock::time_point present_due(
    std::chrono::steady_clock::time_point previous,
    std::chrono::steady_clock::time_point shown,
    std::chrono::steady_clock::duration step,
    std::chrono::duration<double> period
);

// How long after the next tick is due a frame held by its showing may still be shown before it,
// in seconds: the tick then runs as soon as that frame is shown, so that the frames reach the
// screen evenly however the cap's frames fall against the ticks (RTSS's front edge sync paces the
// game's whole frame, its work too). No more than a cap's frame (`step`), than the scene runs
// behind (`lag` ticks of `period`, which it still shows), nor than leaves the tick's own work
// (`tick_work`) done by half a tick after its time; 0 where that leaves nothing. The game's tick
// is not changed by it, only when it runs: its time and alpha stay the ones it was due at.
double tick_slip(double step, double period, double lag, double tick_work);

// When a frame held to `due` (by `edge`) may start: `due` itself, or `lead` seconds before it when
// it is held by its showing. The clock's epoch (no frame held yet) stays the epoch.
std::chrono::steady_clock::time_point cap_start(
    CapEdge edge, std::chrono::steady_clock::time_point due, double lead
);

}  // namespace openrac::frontend
