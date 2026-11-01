// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded main/game and render activity, copied without allocation or waiting after a fault.
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace openrac::crash {

// Logical execution trails; synchronous rendering belongs to Main.
enum class BreadcrumbThread {
    Main,
    Render
};

// The optional numeric value is a captured frame or a level ID, never a formatted string.
enum class BreadcrumbValue {
    Frame,
    Level
};

// Two independent rings. Neither copied nor moved; slots use nonblocking claims so a snapshot
// can skip an interrupted writer. Messages are copied, never borrowed. Concurrent snapshots
// are best-effort, not an atomic view of all activity. No operation allocates or waits.
class Breadcrumbs {
public:
    // Retain a short history per thread and cap each message including its terminating NUL.
    static constexpr std::size_t kPerThread = 32;
    static constexpr std::size_t kMessageSize = 64;

    // One complete event, independent of the original message's lifetime.
    struct Entry {
        std::uint64_t tick = 0;      // Monotonic microseconds, not wall-clock time.
        std::uint64_t sequence = 0;  // Process-local tie-breaker for equal timestamps; 0 is empty.
        std::int64_t value = -1;     // Frame or level ID; -1 means unspecified.
        BreadcrumbThread thread{};   // Logical owner of the event.
        BreadcrumbValue kind{};      // Label of the numeric value.
        char message[kMessageSize]{};  // Truncated text, with controls replaced by spaces.
    };

    // Caller-owned snapshot storage; only snapshot()'s returned prefix is valid.
    using Snapshot = std::array<Entry, kPerThread * 2>;

    // Copies an event into `thread`'s ring. `tick` uses a shared monotonic microsecond clock.
    // Drops the event if its slot is busy; safe when interrupted by snapshot() on this thread.
    void record(
        BreadcrumbThread thread,
        std::string_view message,
        std::int64_t value,
        std::uint64_t tick,
        BreadcrumbValue kind = BreadcrumbValue::Frame
    ) noexcept;
    // Copies complete events to `out`, oldest first, returning their count. Skips busy slots.
    [[nodiscard]] std::size_t snapshot(Snapshot& out) noexcept;

private:
    // A short, nonblocking claim protects plain payload fields from concurrent copies/writes.
    struct Slot {
        std::atomic_flag busy = ATOMIC_FLAG_INIT;  // Never spin or wait on this flag.
        Entry entry{};                             // Read and written only while holding busy.
    };

    // Each producer advances only its own ring; a renderer cannot evict the main thread's events.
    std::array<std::array<Slot, kPerThread>, 2> m_slots{};
    std::array<std::atomic<std::uint64_t>, 2> m_next{};  // Next slot of each trail.
    std::atomic<std::uint64_t> m_sequence{0};            // Tie-breaker shared across trails.
};

// Returns process-lifetime, constant-initialised storage; safe before crash::configure().
Breadcrumbs& breadcrumbs() noexcept;
// Records a short event with the monotonic clock; no log output, heap use or blocking.
void breadcrumb(
    BreadcrumbThread thread, std::string_view message, std::int64_t frame = -1
) noexcept;

// Records level-loading activity on the main/game trail without formatting its numeric ID.
void level_breadcrumb(std::string_view message, int level) noexcept;
// Records a rendering pass only while a BreadcrumbScope is active on this thread.
void breadcrumb(std::string_view message) noexcept;

// Thread-local pass context, restored on exit (including exceptions). Never allocates.
// The explicit trail constructor is used at the render handoff; the frame-only constructor
// retains the current trail so synchronous rendering remains on Main. Neither copied nor moved.
class BreadcrumbScope {
public:
    // Selects a trail and captured frame for nested rendering calls.
    BreadcrumbScope(BreadcrumbThread thread, std::int64_t frame) noexcept;
    // Selects a captured frame without changing the current thread's trail.
    explicit BreadcrumbScope(std::int64_t frame) noexcept;
    // Restores the calling scope's context, or disables implicit pass recording.
    ~BreadcrumbScope();
    BreadcrumbScope(const BreadcrumbScope&) = delete;
    BreadcrumbScope& operator=(const BreadcrumbScope&) = delete;
    BreadcrumbScope(BreadcrumbScope&&) = delete;
    BreadcrumbScope& operator=(BreadcrumbScope&&) = delete;

private:
    BreadcrumbThread m_thread;  // Previous logical trail on this thread.
    std::int64_t m_frame;       // Previous captured frame.
    bool m_active;              // Whether the previous context recorded implicit passes.
};

}  // namespace openrac::crash
