// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "common/slow_cpu.h"

#include <algorithm>
#include <atomic>
#include <chrono>

#if !defined(_WIN32)
#include <ctime>
#endif

namespace openrac::slow_cpu {
namespace {

std::atomic<int> g_factor{1};

// This thread's processor time in seconds, or a negative value when it cannot be read.
double thread_seconds() {
#if defined(_WIN32)
    return -1.0;
#else
    timespec now{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) {
        return -1.0;
    }
    return static_cast<double>(now.tv_sec) + static_cast<double>(now.tv_nsec) * 1e-9;
#endif
}

}  // namespace

bool available() {
    return thread_seconds() >= 0.0;
}

void set_factor(int factor) {
    g_factor = available() ? std::clamp(factor, 1, 8) : 1;
}

int factor() {
    return g_factor;
}

void pad() {
    // Each thread's processor time at the end of its last call: its work since then is padded.
    thread_local double last = -1.0;
    const int k = g_factor;
    const double now = thread_seconds();
    if (k > 1 && last >= 0.0 && now > last) {
        // Spun, not slept: a slower processor would be busy, and the scheduler sees it so.
        const auto until = std::chrono::steady_clock::now()
                           + std::chrono::duration<double>((now - last) * (k - 1));
        while (std::chrono::steady_clock::now() < until) {
        }
    }
    // The spin itself is not work to pad again.
    last = k > 1 ? thread_seconds() : -1.0;
}

}  // namespace openrac::slow_cpu
