// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A wait that ends at its deadline, not a millisecond or more after it: the frame limiter's and
// the game's tick's waits. The system's sleep wakes late by an amount of its own (tens of
// microseconds on Linux and Android, a millisecond or a whole 15.6 ms timer tick on Windows without
// a high-resolution timer), and every late wake is a frame shown late, an uneven frame time. So
// the system is asked to wake a little early, by a margin learned from how late it woke before,
// and the rest is spun (with the processor's pause hint) to the deadline, as RTSS's limiter does. The
// sleep is to an absolute time, so no wait adds to the next one's error.
//
// The hybrid wait is a template on its clock, sleep and spin step, so that
// tests/precise_wait_test.cpp can run it on a simulated system clock; until() is the real one.

#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>

namespace openrac::precise_wait {

using Clock = std::chrono::steady_clock;

// How much earlier than the deadline the system's sleep is asked to end: the late wakes it saw,
// smoothed, plus some of their spread, so that few wakes come after the deadline.
class Margin {
public:
    // Before a wake is seen: a millisecond, which covers a system with a 1 ms timer.
    static constexpr double kStart = 0.001;
    // The spin is never longer than this (a slower timer is late whatever is spun): it costs a
    // processor that much each wait.
    static constexpr double kMax = 0.002;
    // Even a timer that has always been on time is asked for this much early.
    static constexpr double kMin = 0.00005;
    // The weight of a new wake in the smoothed ones.
    static constexpr double kWeight = 0.1;
    // How many times the spread is added to the mean lateness.
    static constexpr double kSpread = 3.0;

    // A sleep asked to end at a time ended `late` seconds after it (an early end counts as 0).
    void woke(double late) {
        late = std::max(late, 0.0);
        if (m_mean < 0.0) {
            m_mean = late;
            m_spread = late * 0.5;
            return;
        }
        m_mean += kWeight * (late - m_mean);
        m_spread += kWeight * (std::abs(late - m_mean) - m_spread);
    }

    // How early to ask the system to wake, in seconds.
    double seconds() const {
        if (m_mean < 0.0) {
            return kStart;
        }
        return std::clamp(m_mean + kSpread * m_spread, kMin, kMax);
    }

private:
    double m_mean = -1.0;
    double m_spread = 0.0;
};

// Waits until `deadline` on `now()`'s clock: `sleep(time)` to `margin` before it, then `relax()`
// until it comes. Returns at once when the deadline is past, and never before it.
template <class Now, class Sleep, class Relax>
void hybrid_wait(Clock::time_point deadline, Margin& margin, Now now, Sleep sleep, Relax relax) {
    Clock::time_point t = now();
    if (t >= deadline) {
        return;
    }
    const auto wake = deadline
                      - std::chrono::duration_cast<Clock::duration>(
                          std::chrono::duration<double>(margin.seconds())
                      );
    if (wake > t) {
        sleep(wake);
        t = now();
        margin.woke(std::chrono::duration<double>(t - wake).count());
    }
    while (t < deadline) {
        relax();
        t = now();
    }
}

// Waits on the steady clock until `deadline`, with the system's most precise sleep (a
// high-resolution waitable timer on Windows, clock_nanosleep to an absolute time on Linux and
// Android, the standard library's sleep elsewhere). Each thread learns its own margin.
void until(Clock::time_point deadline);

}  // namespace openrac::precise_wait
