// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mouse's rate (platform/mouse_rate.h): a steady motion at any report rate, a mouse that
// stopped, and motion stamped after the time asked for.

#include <cmath>

#include "check.h"
#include "platform/mouse_rate.h"

namespace {

using openrac::platform::MouseRate;

constexpr std::uint64_t kMs = 1'000'000;

// 1000 units a second, reported at `hz`, read at `now`.
float steady(int hz, std::uint64_t now) {
    MouseRate rate;
    const std::uint64_t step = 1'000'000'000ull / static_cast<std::uint64_t>(hz);
    for (std::uint64_t t = step; t <= now; t += step) {
        rate.add(t, 1000.0f / static_cast<float>(hz), 0.0f);
    }
    float vx = 0.0f;
    float vy = 0.0f;
    rate.rate(now, vx, vy);
    CHECK(vy == 0.0f);
    return vx;
}

void test_a_steady_motion() {
    for (const int hz : {125, 500, 1000, 8000}) {
        CHECK(std::abs(steady(hz, 200 * kMs) - 1000.0f) < 1000.0f * 0.35f);
    }
    CHECK(std::abs(steady(1000, 200 * kMs) - 1000.0f) < 50.0f);
}

void test_a_stopped_mouse() {
    MouseRate rate;
    for (std::uint64_t t = 1; t <= 100; ++t) {
        rate.add(t * kMs, 1.0f, -1.0f);
    }
    float vx = 1.0f;
    float vy = 1.0f;
    rate.rate(100 * kMs + MouseRate::kWindowNs, vx, vy);
    CHECK(vx == 0.0f);
    CHECK(vy == 0.0f);
}

void test_later_motion_is_not_counted() {
    MouseRate rate;
    rate.add(50 * kMs, 5.0f, 0.0f);
    float vx = 1.0f;
    float vy = 1.0f;
    rate.rate(40 * kMs, vx, vy);
    CHECK(vx == 0.0f);
}

}  // namespace

int main() {
    test_a_steady_motion();
    test_a_stopped_mouse();
    test_later_motion_is_not_counted();
    return openrac::test::result();
}
