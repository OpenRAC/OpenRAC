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

// What the frame cap times: when a frame starts, or when it is shown.
enum class CapEdge {
    // The frame starts at its time and is shown as soon as it is drawn, so its frame time varies
    // with how long it takes to draw.
    Start,
    // The frame has a fixed time on the screen, one cap frame after the last (next_due from each
    // swap). It is drawn a little early, waits for that time before the swap, and is drawn for
    // that moment: even frame times and motion, for about 1-2 ms of latency. Only when there are
    // more frames than ticks: with one frame a tick the frame cannot start before the tick ends,
    // so a fixed time would be missed or drift later.
    Present,
};

// How long before its time a frame held by its showing starts: what a frame costs (`cost`), three
// times how much that moves from frame to frame (`spread`) and a margin, in seconds, and no more
// than the cap's frame (`step`).
double present_lead(double cost, double spread, double step);

// How long a tick may start late to let a Present frame out first, in seconds. This keeps the
// frames even however they fall between the ticks. At most one cap frame (`step`), the lag
// (`lag` ticks of `period`), and what still lets the tick's own work (`tick_work`) end within
// half a tick of its time; 0 if nothing is left. The tick's logic and alpha do not change.
double tick_slip(double step, double period, double lag, double tick_work);

// When a frame held to `due` (by `edge`) may start: `due` itself, or `lead` seconds before it when
// it is held by its showing. The clock's epoch (no frame held yet) stays the epoch.
std::chrono::steady_clock::time_point cap_start(
    CapEdge edge, std::chrono::steady_clock::time_point due, double lead
);

}  // namespace openrac::frontend
