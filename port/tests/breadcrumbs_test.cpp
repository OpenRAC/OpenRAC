// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Independent retention, timestamp merging, owned bounded text and snapshots during writes.
#include <atomic>
#include <cstring>
#include <string_view>

#include "common/breadcrumbs.h"
#include "common/joining_thread.h"
#include "tests/check.h"

namespace {
using openrac::crash::Breadcrumbs;
using openrac::crash::BreadcrumbThread;

// Merges timestamps rather than ring traversal order, with sequence order for equal times.
void test_order_and_owned_text() {
    Breadcrumbs history;
    Breadcrumbs::Snapshot entries{};
    CHECK(history.snapshot(entries) == 0);
    char message[] = "queued";
    history.record(BreadcrumbThread::Main, message, 7, 20);
    message[0] = 'X';
    history.record(BreadcrumbThread::Render, "earlier", 6, 10);
    history.record(BreadcrumbThread::Render, "started", 7, 20);
    CHECK(history.snapshot(entries) == 3);
    CHECK(std::string_view(entries[0].message) == "earlier");
    CHECK(std::string_view(entries[1].message) == "queued");
    CHECK(std::string_view(entries[2].message) == "started");
    CHECK(entries[1].value == 7);
    CHECK(entries[1].thread == BreadcrumbThread::Main);
    CHECK(entries[2].thread == BreadcrumbThread::Render);
    CHECK(entries[1].sequence < entries[2].sequence);
    char long_message[Breadcrumbs::kMessageSize * 2];
    std::memset(long_message, 'x', sizeof(long_message));
    history.record(BreadcrumbThread::Main, {long_message, sizeof(long_message)}, -1, 30);
    history.record(BreadcrumbThread::Render, "a\nb\tc\rd\x7f", -1, 40);
    CHECK(history.snapshot(entries) == 5);
    CHECK(std::strlen(entries[3].message) == Breadcrumbs::kMessageSize - 1);
    CHECK(std::string_view(entries[4].message) == "a b c d ");
    history.record(BreadcrumbThread::Main, {}, -1, 50);
    CHECK(history.snapshot(entries) == 6);
    CHECK(entries[5].message[0] == '\0');
}

// Several render-ring wraps cannot evict a main-thread event; both rings keep their newest slots.
void test_independent_wraparound() {
    Breadcrumbs history;
    Breadcrumbs::Snapshot entries{};
    history.record(BreadcrumbThread::Main, "main retained", 0, 0);
    constexpr std::size_t kWrites = Breadcrumbs::kPerThread * 4;
    for (std::size_t i = 1; i <= kWrites; ++i) {
        history.record(BreadcrumbThread::Render, "render", static_cast<std::int64_t>(i), i);
    }
    CHECK(history.snapshot(entries) == Breadcrumbs::kPerThread + 1);
    CHECK(std::string_view(entries[0].message) == "main retained");
    CHECK(entries[1].value == static_cast<std::int64_t>(kWrites - Breadcrumbs::kPerThread + 1));
    CHECK(entries[Breadcrumbs::kPerThread].value == static_cast<std::int64_t>(kWrites));
    for (std::size_t i = 1; i <= kWrites; ++i) {
        history.record(BreadcrumbThread::Main, "main", static_cast<std::int64_t>(i), kWrites + i);
    }
    CHECK(history.snapshot(entries) == entries.size());
    CHECK(entries.back().value == static_cast<std::int64_t>(kWrites));
    CHECK(entries.back().thread == BreadcrumbThread::Main);
}

// Checks that concurrent snapshots contain intact payloads and are chronologically sorted.
void check_concurrent_snapshot(const Breadcrumbs::Snapshot& entries, std::size_t count) {
    CHECK(count <= entries.size());
    for (std::size_t i = 0; i < count; ++i) {
        CHECK(entries[i].tick == static_cast<std::uint64_t>(entries[i].value));
        CHECK(
            std::string_view(entries[i].message)
            == (entries[i].thread == BreadcrumbThread::Main ? "main complete event"
                                                            : "render complete event")
        );
        // Equal timestamps use the sequence tie-breaker; the first event has no predecessor.
        const bool ordered = i == 0 || entries[i - 1].tick < entries[i].tick
                             || (entries[i - 1].tick == entries[i].tick
                                 && entries[i - 1].sequence < entries[i].sequence);
        CHECK(ordered);
    }
}

// Context nesting restores the caller, and a new thread must not inherit the main scope.
void test_pass_context() {
    using namespace openrac::crash;
    Breadcrumbs::Snapshot entries{};
    const auto before = breadcrumbs().snapshot(entries);
    breadcrumb("inactive pass");
    CHECK(breadcrumbs().snapshot(entries) == before);
    {
        const BreadcrumbScope main(BreadcrumbThread::Main, 11);
        breadcrumb("outer pass");
        {
            const BreadcrumbScope nested(12);
            breadcrumb("nested pass");
        }
        openrac::JoiningThread worker([] {
            breadcrumb("inactive worker pass");
            const BreadcrumbScope render(BreadcrumbThread::Render, 21);
            breadcrumb("worker pass");
        });
        worker.join();
        breadcrumb("restored main pass");
    }
    breadcrumb("inactive after scope");
    CHECK(breadcrumbs().snapshot(entries) == before + 4);
    CHECK(entries[before].thread == BreadcrumbThread::Main && entries[before].value == 11);
    CHECK(entries[before + 1].thread == BreadcrumbThread::Main && entries[before + 1].value == 12);
    CHECK(
        entries[before + 2].thread == BreadcrumbThread::Render && entries[before + 2].value == 21
    );
    CHECK(entries[before + 3].thread == BreadcrumbThread::Main && entries[before + 3].value == 11);
    level_breadcrumb("level program load", 9);
    CHECK(breadcrumbs().snapshot(entries) == before + 5);
    CHECK(entries[before + 4].kind == BreadcrumbValue::Level);
    CHECK(entries[before + 4].value == 9);
    breadcrumb(BreadcrumbThread::Main, "game tick resumed", 42, BreadcrumbValue::Tick);
    CHECK(breadcrumbs().snapshot(entries) == before + 6);
    CHECK(entries[before + 5].kind == BreadcrumbValue::Tick);
    CHECK(entries[before + 5].value == 42);
}

// Payload text, frame and clock must remain one complete event while writers wrap both rings.
void test_concurrent_snapshots() {
    Breadcrumbs history;
    std::atomic<unsigned> done{0};
    const auto writer = [&](BreadcrumbThread thread, std::string_view message) {
        // Repeated wraps make readers contend with writes to every slot.
        for (std::uint64_t i = 1; i <= 20000; ++i) {
            history.record(thread, message, static_cast<std::int64_t>(i), i);
        }
        done.fetch_add(1, std::memory_order_release);
    };
    openrac::JoiningThread main(writer, BreadcrumbThread::Main, "main complete event");
    openrac::JoiningThread render(writer, BreadcrumbThread::Render, "render complete event");
    do {
        Breadcrumbs::Snapshot entries{};
        const auto count = history.snapshot(entries);
        check_concurrent_snapshot(entries, count);
    } while (done.load(std::memory_order_acquire) != 2);
    main.join();
    render.join();
}
}  // namespace

// Runs the deterministic retention and concurrent publication contracts.
int main() {
    test_order_and_owned_text();
    test_independent_wraparound();
    test_pass_context();
    test_concurrent_snapshots();
    return openrac::test::result();
}
