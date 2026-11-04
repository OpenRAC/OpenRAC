// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The system's half of the precise wait (precise_wait.h): its most precise sleep to a time.

#include "common/precise_wait.h"

#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(__linux__)
#include <cerrno>
#include <ctime>
#endif
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(_M_ARM64)
#include <intrin.h>
#endif

#include "common/sampler.h"
#include "common/slow_cpu.h"

namespace openrac::precise_wait {
namespace {

#if defined(_WIN32)
// Windows 10 1803 and later; older systems leave the timer out and sleep the standard way.
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

// This thread's high-resolution timer, made at its first wait; null where the system has none.
struct Timer {
    HANDLE handle = CreateWaitableTimerExW(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS
    );

    Timer() = default;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    ~Timer() {
        if (handle) {
            CloseHandle(handle);
        }
    }
};
#endif

void sleep_until(Clock::time_point wake) {
#if defined(_WIN32)
    thread_local Timer timer;
    const auto left = wake - Clock::now();
    if (timer.handle && left > Clock::duration::zero()) {
        // A negative due time is relative, in 100 ns units: the timer counts from now.
        LARGE_INTEGER due;
        due.QuadPart = -std::max<LONGLONG>(
            1, std::chrono::duration_cast<std::chrono::nanoseconds>(left).count() / 100
        );
        if (SetWaitableTimer(timer.handle, &due, 0, nullptr, nullptr, FALSE)) {
            WaitForSingleObject(timer.handle, INFINITE);
            return;
        }
    }
    std::this_thread::sleep_until(wake);
#elif defined(__linux__)
    // The steady clock's time as CLOCK_MONOTONIC's, by what is left of the wait (the two count
    // alike; their epochs need not be the same in every standard library).
    timespec at{};
    clock_gettime(CLOCK_MONOTONIC, &at);
    const auto left = std::chrono::duration_cast<std::chrono::nanoseconds>(wake - Clock::now());
    if (left <= std::chrono::nanoseconds::zero()) {
        return;
    }
    const long long ns = static_cast<long long>(at.tv_nsec) + left.count();
    at.tv_sec += static_cast<time_t>(ns / 1000000000);
    at.tv_nsec = static_cast<long>(ns % 1000000000);
    // An absolute time: a signal that wakes the sleep early sleeps again to the same time.
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &at, nullptr) == EINTR) {
    }
#else
    std::this_thread::sleep_until(wake);
#endif
}

// One step of the spin: the processor's hint that it waits (it saves power and lends the core to
// its other hardware thread), not a system call. A yield could hand the core to another thread
// for its whole time slice, a millisecond or more past the deadline.
void relax() {
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#elif defined(_M_ARM64)
    __yield();
#endif
}

}  // namespace

void until(Clock::time_point deadline) {
    thread_local Margin margin;
    // The spin at the end is a wait: no work for the simulated slow processor to pad, and not the
    // game's hot functions in a capture.
    const slow_cpu::Unpadded unpadded;
    sampler::leave_out(true);
    hybrid_wait(deadline, margin, [] { return Clock::now(); }, sleep_until, relax);
    sampler::leave_out(false);
}

}  // namespace openrac::precise_wait
