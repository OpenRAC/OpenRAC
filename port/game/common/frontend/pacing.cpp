// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frontend's pacing arithmetic (pacing.h).

#include "game/common/frontend/pacing.h"

#include <algorithm>
#include <cmath>

#include "game/common/frame_pacing.h"

namespace openrac::frontend {

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::steady_clock;

steady_clock::time_point shown_at(
    steady_clock::time_point now, double cost, double interval, steady_clock::time_point last_swap
) {
    const auto ready = now + duration_cast<steady_clock::duration>(duration<double>(cost));

    // Without a refresh to count from, the frame is shown when it is drawn.
    if (interval <= 0.0 || last_swap.time_since_epoch().count() == 0) {
        return ready;
    }

    const double since = duration<double>(ready - last_swap).count();
    const double n = std::max(1.0, std::ceil(since / interval));
    return last_swap + duration_cast<steady_clock::duration>(duration<double>(n * interval));
}

steady_clock::time_point between_limit(
    steady_clock::time_point next_tick,
    duration<double> period,
    double lag,
    double cost,
    double interval,
    steady_clock::time_point last_swap
) {
    if (lag > 0.0) {
        return next_tick + duration_cast<steady_clock::duration>(period * lag);
    }
    if (interval <= 0.0 || last_swap.time_since_epoch().count() == 0) {
        return next_tick;
    }
    return std::max(next_tick, shown_at(next_tick, cost, interval, last_swap));
}

float tick_alpha(
    steady_clock::time_point display,
    steady_clock::time_point tick,
    duration<double> period,
    double lag
) {
    const double t = duration<double>(display - tick).count() / period.count() - lag;
    return static_cast<float>(std::clamp(t, 0.0, 1.0));
}

bool frame_may_start(double cap, steady_clock::time_point due, steady_clock::time_point now) {
    return cap <= 0.0 || due.time_since_epoch().count() == 0
           || now + std::chrono::milliseconds(1) >= due;
}

double present_lead(double cost, double spread, double step) {
    // What a frame begun this much early still covers: the wake of a wait and the swap's own call.
    constexpr double kMargin = 0.0005;
    return std::clamp(cost + 3.0 * spread + kMargin, 0.0, std::max(step, 0.0));
}

steady_clock::time_point present_due(
    steady_clock::time_point previous,
    steady_clock::time_point shown,
    steady_clock::duration step,
    duration<double> period
) {
    // A frame a tick or fewer (within a thousandth, for the clock's rounding of either).
    if (previous.time_since_epoch().count() != 0 && shown > previous
        && duration<double>(step).count() >= 0.999 * period.count()) {
        previous = shown;
    }
    return frame_pacing::next_due(previous, shown, shown, step);
}

double tick_slip(double step, double period, double lag, double tick_work) {
    return std::max(0.0, std::min({step, lag * period, 0.5 * period - tick_work}));
}

steady_clock::time_point cap_start(CapEdge edge, steady_clock::time_point due, double lead) {
    if (edge == CapEdge::Start || due.time_since_epoch().count() == 0) {
        return due;
    }
    return due - duration_cast<steady_clock::duration>(duration<double>(lead));
}

}  // namespace openrac::frontend
