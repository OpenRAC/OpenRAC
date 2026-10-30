// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The low-latency frames' sleep (renderer/vulkan/latency_sleep.h): none until the times are
// known, none when the processor is the slower, and otherwise one that lets a frame be queued just
// as the graphics card is free, so that frames come as fast as two in flight would.
#include <algorithm>
#include <cmath>

#include "renderer/vulkan/latency_sleep.h"
#include "tests/check.h"

namespace {

using openrac::vkgl::LatencySleep;

// Frames as the Vulkan layer draws them with low-latency frames: each sleeps, works `cpu` seconds,
// waits for the frame before it to finish on the card and is queued, the card takes `gpu` for it,
// and `between` more passes on the processor before the next one sleeps.
struct Run {
    double frame = 0.0;    // the last frame's time, start to start
    double latency = 0.0;  // the last frame's beginning to its end on the card
    double wait = 0.0;     // the last frame's wait for the card
    double slept = 0.0;
};

Run run(LatencySleep& sleep, double cpu, double gpu, double between, int frames) {
    Run r;
    double now = 0.0, free = 0.0, last_start = 0.0;
    for (int i = 0; i < frames; ++i) {
        const double slept = sleep.delay();
        const double start = now + slept;
        const double queued = start + cpu;
        const double wait = std::max(0.0, free - queued);
        free = queued + wait + gpu;
        sleep.gpu_frame(gpu);
        sleep.cpu_frame(cpu, slept, wait);
        r = {start - last_start, free - start, wait, slept};
        last_start = start;
        now = queued + wait + between;
    }
    return r;
}

bool near(double a, double b, double within) {
    return std::abs(a - b) <= within;
}

}  // namespace

int main() {
    // Nothing is known before both times are measured: no sleep, the old wait.
    {
        LatencySleep sleep;
        CHECK(!sleep.known());
        CHECK(sleep.delay() == 0.0);
        sleep.gpu_frame(0.004);
        CHECK(!sleep.known());
        CHECK(sleep.delay() == 0.0);
        sleep.cpu_frame(0.002, 0.0, 0.002);
        CHECK(sleep.known());
        CHECK(near(sleep.delay(), 0.002 - LatencySleep::kMargin, 1e-9));
    }
    // A frame with no time is not a measure.
    {
        LatencySleep sleep;
        sleep.gpu_frame(0.0);
        sleep.cpu_frame(0.0, 0.0, 0.0);
        CHECK(!sleep.known());
    }
    // The card is the slower (2 ms of processor work, 4 ms on the card): frames come every 4 ms,
    // as with two in flight, and each is slept so that it waits only about the margin.
    {
        LatencySleep sleep;
        const Run r = run(sleep, 0.002, 0.004, 0.0003, 300);
        CHECK(near(r.frame, 0.004, 0.0001));
        CHECK(r.wait <= LatencySleep::kMargin + 0.0001);
        CHECK(r.slept > 0.001);
        // Begun 4 ms after the card was busy, without the sleep a frame would be shown 2 frames
        // of the card's (8 ms) after it began; slept, about one of each.
        CHECK(r.latency < 0.002 + 0.004 + LatencySleep::kMargin + 0.0002);
    }
    // The processor is the slower (the Odin 3's capture: 4.4 ms of work, 4.2 ms on the card):
    // nothing to sleep, frames come as fast as the processor makes them.
    {
        LatencySleep sleep;
        const Run r = run(sleep, 0.0044, 0.0042, 0.0001, 300);
        CHECK(sleep.delay() == 0.0);
        CHECK(near(r.frame, 0.0045, 0.0001));
    }
    // The card becomes faster: the sleep shrinks with it and frames keep up.
    {
        LatencySleep sleep;
        run(sleep, 0.002, 0.006, 0.0003, 200);
        const Run r = run(sleep, 0.002, 0.003, 0.0003, 300);
        CHECK(near(r.frame, 0.003, 0.0001));
    }
    // The card becomes slower: the sleep grows, and the wait comes down to the margin again.
    {
        LatencySleep sleep;
        run(sleep, 0.002, 0.003, 0.0003, 200);
        const Run r = run(sleep, 0.002, 0.006, 0.0003, 300);
        CHECK(near(r.frame, 0.006, 0.0001));
        CHECK(r.wait <= LatencySleep::kMargin + 0.0001);
    }
    // Never longer than kMaxDelay, however slow the card.
    {
        LatencySleep sleep;
        run(sleep, 0.001, 0.2, 0.0, 100);
        CHECK(sleep.delay() <= LatencySleep::kMaxDelay);
    }
    return openrac::test::result();
}
