// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame cap with VSync off, simulated: program::vsync's ticks and frontend::between's frames
// on a simulated clock, with frames that cost a varying time and a system sleep that wakes late,
// paced the old way (each frame held by its start, the system's sleep), with the precise wait
// only (as the low-latency frames still are), and the new way (held by its showing). It prints, and checks, how evenly the frames reach the screen
// (the spread of the times between two swaps) and how far the moment each picture shows is from
// the moment it is shown at (the motion's judder). The arithmetic is the frontend's own
// (frontend/pacing.h, frame_pacing.h, common/precise_wait.h); the loop is a model of
// interpolation.cpp's, which needs a window.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "common/precise_wait.h"
#include "game/common/frame_pacing.h"
#include "game/common/frontend/pacing.h"
#include "tests/check.h"

namespace {

using namespace openrac;
using frontend::CapEdge;
using std::chrono::duration;
using std::chrono::duration_cast;
using Clock = std::chrono::steady_clock;

Clock::duration span(double seconds) {
    return duration_cast<Clock::duration>(duration<double>(seconds));
}

double seconds(Clock::duration d) {
    return duration<double>(d).count();
}

// A system's sleep: how late it wakes, at most.
struct Timer {
    const char* name;
    double late_most;
};

constexpr Timer kLinux{"Linux/Android timer", 0.00008};
constexpr Timer kWindows{"Windows 1 ms timer", 0.001};

struct Result {
    double interval_spread_ms = 0.0;  // standard deviation of the time between two swaps
    double judder_ms = 0.0;           // standard deviation of (shown - drawn for)
    int frames = 0;
    double tick_late_most_ms = 0.0;  // the latest a tick ran after its time
};

double spread(const std::vector<double>& v) {
    double mean = 0.0;
    for (const double x : v) {
        mean += x / static_cast<double>(v.size());
    }
    double var = 0.0;
    for (const double x : v) {
        var += (x - mean) * (x - mean) / static_cast<double>(v.size());
    }
    return std::sqrt(var);
}

// One second of 60 ticks a second at `cap` frames a second, after a second to settle.
class Simulation {
public:
    Simulation(CapEdge edge, bool precise, Timer timer, double cap)
        : m_edge(edge),
          m_precise(precise),
          m_timer(timer),
          m_step(1.0 / cap) {}

    Result run() {
        const double period = 1.0 / 60.0;
        Clock::time_point next_tick = m_now;
        for (int tick = 0; tick < 240; ++tick) {
            m_measuring = tick >= 120;
            // The game's tick (program::vsync's wait, then its work), due at its time whenever
            // it runs (set_tick).
            wait(next_tick);
            if (m_measuring) {
                m_tick_late_most = std::max(m_tick_late_most, seconds(m_now - next_tick));
            }
            m_tick = next_tick;
            const double work = m_tick_work_dist(m_random);
            m_now += span(work);
            m_tick_work = std::max(work, 0.9 * m_tick_work);
            next_tick += span(period);
            // The tick's own frame (draw_tick_frame), when the cap lets it start.
            if (frontend::frame_may_start(1.0 / m_step, start_time(), m_now)) {
                draw(period);
            }
            // The frames between ticks (between).
            for (;;) {
                const Clock::time_point start = start_time();
                if (m_now < start) {
                    if (start >= next_tick) {
                        break;
                    }
                    wait(start);
                }
                if (shown_at() >= next_tick + slip(period)) {
                    break;
                }
                draw(period);
            }
        }
        Result r;
        r.interval_spread_ms = spread(m_intervals) * 1000.0;
        r.judder_ms = spread(m_judder) * 1000.0;
        r.frames = static_cast<int>(m_intervals.size());
        r.tick_late_most_ms = m_tick_late_most * 1000.0;
        return r;
    }

private:
    // The system's sleep (late by up to the timer's most), or the precise wait over it.
    void wait(Clock::time_point until) {
        const auto sleep = [this](Clock::time_point to) {
            std::uniform_real_distribution<double> late(0.0, m_timer.late_most);
            m_now = std::max(m_now, to + span(late(m_random)));
        };
        if (!m_precise) {
            if (m_now < until) {
                sleep(until);
            }
            return;
        }
        precise_wait::hybrid_wait(
            until,
            m_margin,
            [this] { return m_now; },
            sleep,
            [this] { m_now += std::chrono::microseconds(2); }
        );
    }

    Clock::duration slip(double period) const {
        return m_edge == CapEdge::Present
                   ? span(frontend::tick_slip(m_step, period, frontend::kLagTicks, m_tick_work))
                   : Clock::duration{};
    }

    Clock::time_point start_time() const {
        return frontend::cap_start(
            m_edge, m_due, frontend::present_lead(m_cost, m_cost_spread, m_step)
        );
    }

    // When a frame begun now is shown, by the frontend's estimate (next_display, VSync off).
    Clock::time_point shown_at() const {
        const auto drawn = frontend::shown_at(m_now, m_cost, 0.0, {});
        return m_edge == CapEdge::Present && m_due.time_since_epoch().count() != 0
                   ? std::max(drawn, m_due)
                   : drawn;
    }

    // A frame: drawn for the estimated moment it is shown, then swapped (interpolation.cpp's
    // draw_kept and swapped).
    void draw(double period) {
        const Clock::time_point started = m_now;
        const Clock::time_point drawn_for = shown_at();
        // Most frames cost about the same; one in fifty is a spike.
        double cost = m_cost_dist(m_random);
        if (m_spike(m_random) < 0.02) {
            cost += 0.003;
        }
        m_now += span(cost);
        m_cost_spread = 0.8 * m_cost_spread + 0.2 * std::abs(cost - m_cost);
        m_cost = 0.8 * m_cost + 0.2 * cost;
        if (m_edge == CapEdge::Present && m_due.time_since_epoch().count() != 0 && m_due > m_now) {
            wait(std::min(m_due, m_tick + span(period) + slip(period)));
        }
        const Clock::time_point shown = m_now;
        m_due = m_edge == CapEdge::Present
                    ? frontend::present_due(m_due, shown, span(m_step), duration<double>(period))
                    : frontend::frame_pacing::next_due(m_due, started, m_now, span(m_step));
        if (m_measuring && m_last_shown.time_since_epoch().count() != 0) {
            m_intervals.push_back(seconds(shown - m_last_shown));
            m_judder.push_back(seconds(shown - drawn_for));
        }
        m_last_shown = shown;
    }

    CapEdge m_edge;
    bool m_precise;
    Timer m_timer;
    double m_step;
    std::mt19937 m_random{20261030};
    std::uniform_real_distribution<double> m_tick_work_dist{0.001, 0.002};
    std::uniform_real_distribution<double> m_cost_dist{0.00175, 0.00325};
    std::uniform_real_distribution<double> m_spike{0.0, 1.0};
    precise_wait::Margin m_margin;
    Clock::time_point m_now = Clock::time_point{} + std::chrono::seconds(100);
    Clock::time_point m_tick{};
    Clock::time_point m_due{};
    Clock::time_point m_last_shown{};
    double m_cost = 0.004;
    double m_cost_spread = 0.0;
    double m_tick_work = 0.0;
    double m_tick_late_most = 0.0;
    bool m_measuring = false;
    std::vector<double> m_intervals;
    std::vector<double> m_judder;
};

}  // namespace

int main() {
    std::printf(
        "VSync off, 60 ticks a second of 1-2 ms, frames of 1.75-3.25 ms (2%% +3 ms spikes);\n"
        "the old pacing -> the precise wait only (low-latency frames) -> held by the showing:\n"
    );
    for (const Timer timer : {kLinux, kWindows}) {
        for (const double cap : {60.0, 120.0, 144.0, 240.0}) {
            const Result before = Simulation(CapEdge::Start, false, timer, cap).run();
            const Result precise = Simulation(CapEdge::Start, true, timer, cap).run();
            const Result after = Simulation(CapEdge::Present, true, timer, cap).run();
            std::printf(
                "%-19s cap %3.0f: frame time spread %.3f -> %.3f -> %.3f ms, judder %.3f -> "
                "%.3f -> %.3f ms, frames %d -> %d -> %d, tick late by %.2f -> %.2f -> %.2f ms\n",
                timer.name,
                cap,
                before.interval_spread_ms,
                precise.interval_spread_ms,
                after.interval_spread_ms,
                before.judder_ms,
                precise.judder_ms,
                after.judder_ms,
                before.frames,
                precise.frames,
                after.frames,
                before.tick_late_most_ms,
                precise.tick_late_most_ms,
                after.tick_late_most_ms
            );
            // Evener frames and motion, at the same frame rate, the tick never more than half a
            // tick after its time.
            CHECK(after.interval_spread_ms < 0.5 * before.interval_spread_ms);
            CHECK(after.judder_ms < 0.6 * before.judder_ms);
            CHECK(after.frames >= before.frames - 1);
            CHECK(after.tick_late_most_ms < 1000.0 / 60.0 / 2.0);
        }
    }
    return openrac::test::result();
}
