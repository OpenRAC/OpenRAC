// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fixed storage and nonblocking slot claims, including when a fault interrupts a writer.
#include "common/breadcrumbs.h"

#include <algorithm>
#include <chrono>

namespace openrac::crash {

namespace {
// Constant-initialised thread-local state; render passes never consult mutable game-thread data.
struct Context {
    BreadcrumbThread thread = BreadcrumbThread::Main;  // Main is also the synchronous-render path.
    std::int64_t frame = -1;                           // Captured game frame, never a live global.
    bool active = false;                               // Explicit scope enables pass recording.
};

// Returns this thread's context without dynamic initialisation or registration.
Context& context() noexcept {
    static thread_local constinit Context tls_context;
    return tls_context;
}

// Returns a monotonic timestamp for normal recording; the crash snapshot never reads a clock.
std::uint64_t tick() noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now).count()
    );
}
}  // namespace

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

void Breadcrumbs::record(
    BreadcrumbThread thread,
    std::string_view message,
    std::int64_t value,
    std::uint64_t tick,
    BreadcrumbValue kind
) noexcept {
    const std::size_t owner = thread == BreadcrumbThread::Render ? 1 : 0;
    const auto index = m_next[owner].fetch_add(1, std::memory_order_relaxed) % kPerThread;
    auto& slot = m_slots[owner][index];
    // An interrupted writer or simultaneous snapshot must never make recording wait.
    if (slot.busy.test_and_set(std::memory_order_acquire)) {
        return;
    }
    auto& entry = slot.entry;
    entry.tick = tick;
    entry.sequence = m_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    entry.value = value;
    entry.thread = thread;
    entry.kind = kind;
    const auto length = std::min(message.size(), kMessageSize - 1);
    // Copy a bounded view rather than retaining pointers into temporary or damaged storage.
    for (std::size_t i = 0; i < length; ++i) {
        const auto c = static_cast<unsigned char>(message[i]);
        entry.message[i] = c < 0x20 || c == 0x7f ? ' ' : static_cast<char>(c);
    }
    entry.message[length] = '\0';
    slot.busy.clear(std::memory_order_release);
}

std::size_t Breadcrumbs::snapshot(Snapshot& out) noexcept {
    std::size_t count = 0;
    // Try each slot once: a writer stopped by the fault cannot block its own report.
    for (auto& trail : m_slots) {
        for (auto& slot : trail) {
            if (slot.busy.test_and_set(std::memory_order_acquire)) {
                continue;
            }
            // Empty slots contain no event; copying while claimed cannot observe a torn message.
            if (slot.entry.sequence != 0) {
                out[count++] = slot.entry;
            }
            slot.busy.clear(std::memory_order_release);
        }
    }
    // Bounded insertion sort uses no library scratch allocation. Sequence resolves clock ties.
    for (std::size_t i = 1; i < count; ++i) {
        const Entry entry = out[i];
        std::size_t at = i;
        while (at > 0
               && (out[at - 1].tick > entry.tick
                   || (out[at - 1].tick == entry.tick && out[at - 1].sequence > entry.sequence))) {
            out[at] = out[at - 1];
            --at;
        }
        out[at] = entry;
    }
    return count;
}

Breadcrumbs& breadcrumbs() noexcept {
    // Crash callbacks have no user pointer; retain this constant-initialised owner through shutdown.
    static constinit Breadcrumbs history;
    return history;
}

void breadcrumb(BreadcrumbThread thread, std::string_view message, std::int64_t frame) noexcept {
    breadcrumbs().record(thread, message, frame, tick());
}

void level_breadcrumb(std::string_view message, int level) noexcept {
    breadcrumbs().record(BreadcrumbThread::Main, message, level, tick(), BreadcrumbValue::Level);
}

void breadcrumb(std::string_view message) noexcept {
    const auto& current = context();
    // Standalone renderers outside a captured game/view frame need no implicit trail traffic.
    if (current.active) {
        breadcrumb(current.thread, message, current.frame);
    }
}

BreadcrumbScope::BreadcrumbScope(BreadcrumbThread thread, std::int64_t frame) noexcept
    : m_thread(context().thread),
      m_frame(context().frame),
      m_active(context().active) {
    context() = {thread, frame, true};
}

BreadcrumbScope::BreadcrumbScope(std::int64_t frame) noexcept
    : BreadcrumbScope(context().thread, frame) {}

BreadcrumbScope::~BreadcrumbScope() {
    context() = {m_thread, m_frame, m_active};
}

}  // namespace openrac::crash
