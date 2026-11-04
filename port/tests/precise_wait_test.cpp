// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The precise wait (common/precise_wait.h): the margin it learns from the system's late wakes, the
// hybrid wait on a simulated clock (never early, late by no more than a spin step whatever the
// system's sleep does), and the real wait on this machine against the standard library's sleep,
// whose numbers it prints (a loaded machine moves them, so only "never early" is checked there).

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

#include "common/precise_wait.h"
#include "tests/check.h"

namespace {

using openrac::precise_wait::Clock;
using openrac::precise_wait::Margin;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::microseconds;

Clock::duration span(double seconds) {
    return duration_cast<Clock::duration>(duration<double>(seconds));
}

double seconds(Clock::duration d) {
    return duration<double>(d).count();
}

void test_margin() {
    Margin m;
    CHECK(m.seconds() == Margin::kStart);
    // A timer that is always on time: the smallest margin.
    for (int i = 0; i < 100; ++i) {
        m.woke(0.0);
    }
    CHECK(m.seconds() == Margin::kMin);
    // A steady 300 us late: the margin settles on it (its spread dies away).
    for (int i = 0; i < 200; ++i) {
        m.woke(0.0003);
    }
    CHECK(m.seconds() > 0.00029 && m.seconds() < 0.00035);
    // A timer with a whole Windows tick of lateness: the spin is held to its most.
    for (int i = 0; i < 100; ++i) {
        m.woke(0.0156);
    }
    CHECK(m.seconds() == Margin::kMax);
    // An early wake counts as on time.
    Margin early;
    early.woke(-0.001);
    CHECK(early.seconds() == Margin::kMin);
}

// The hybrid wait on a simulated clock whose sleep wakes late by up to `late_most` seconds.
struct Simulated {
    Clock::time_point t = Clock::time_point{} + std::chrono::seconds(10);
    std::mt19937 random{1234};
    double late_most;
    Clock::duration step = microseconds(2);  // what one spin step takes
    int relaxes = 0;

    explicit Simulated(double late) : late_most(late) {}

    double wait(Clock::time_point deadline, Margin& margin) {
        openrac::precise_wait::hybrid_wait(
            deadline,
            margin,
            [this] { return t; },
            [this](Clock::time_point wake) {
                std::uniform_real_distribution<double> late(0.0, late_most);
                t = std::max(t, wake + span(late(random)));
            },
            [this] {
                t += step;
                ++relaxes;
            }
        );
        return seconds(t - deadline);
    }
};

void test_hybrid_wait() {
    // A deadline already past returns at once, without sleeping or spinning.
    {
        Simulated sim(0.001);
        Margin m;
        CHECK(sim.wait(sim.t - microseconds(5), m) == seconds(microseconds(5)));
        CHECK(sim.relaxes == 0);
    }
    // Linux-like (up to 100 us late) and Windows-like timers (up to 1 ms late): once the margin is
    // learned, nearly every wait ends within one spin step of its deadline, the rest a fraction of
    // the timer's lateness after it, and none before it.
    for (const double late_most : {0.0001, 0.001}) {
        Simulated sim(late_most);
        Margin m;
        std::vector<double> lates;
        bool early = false;
        for (int i = 0; i < 1000; ++i) {
            const double late = sim.wait(sim.t + span(0.004), m);
            early = early || late < 0.0;
            if (i >= 50) {
                lates.push_back(late);
            }
        }
        std::sort(lates.begin(), lates.end());
        CHECK(!early);
        CHECK(lates[lates.size() * 99 / 100] <= seconds(sim.step));
        CHECK(lates.back() <= 0.3 * late_most);
        // The spin stays short: the margin covers the timer's lateness, not much more.
        CHECK(m.seconds() <= late_most * 2.0 + Margin::kMin);
    }
    // A timer later than the most the margin spins: the wait is late, but by no more than the
    // timer's lateness beyond that.
    {
        Simulated sim(0.010);
        Margin m;
        for (int i = 0; i < 200; ++i) {
            const double late = sim.wait(sim.t + span(0.020), m);
            CHECK(late >= 0.0 && late <= 0.010 - Margin::kMax + seconds(sim.step) + 1e-9);
        }
    }
}

// Lateness of `count` real waits of 2 ms: mean, 99th percentile and most, in microseconds.
template <class Wait>
void measure(const char* name, Wait wait, int count, bool& early) {
    std::vector<double> lates;
    for (int i = 0; i < count; ++i) {
        const auto deadline = Clock::now() + microseconds(2000);
        wait(deadline);
        const double late = seconds(Clock::now() - deadline) * 1e6;
        early = early || late < 0.0;
        lates.push_back(late);
    }
    std::sort(lates.begin(), lates.end());
    double mean = 0.0;
    for (const double l : lates) {
        mean += l / static_cast<double>(lates.size());
    }
    std::printf(
        "%-28s late by: mean %7.1f us, p99 %7.1f us, most %7.1f us\n",
        name,
        mean,
        lates[lates.size() * 99 / 100],
        lates.back()
    );
}

void test_real_wait() {
    bool early = false;
    bool std_early = false;
    measure(
        "std::this_thread::sleep_until",
        [](Clock::time_point d) { std::this_thread::sleep_until(d); },
        200,
        std_early
    );
    measure(
        "precise_wait::until",
        [](Clock::time_point d) { openrac::precise_wait::until(d); },
        200,
        early
    );
    CHECK(!early);
}

}  // namespace

int main() {
    test_margin();
    test_hybrid_wait();
    test_real_wait();
    return openrac::test::result();
}
