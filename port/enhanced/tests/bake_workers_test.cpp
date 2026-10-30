// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks persistent worker identity, serial dispatch and draining from arithmetic-only tasks.
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

#include "common/joining_thread.h"
#include "enhanced/lighting/bake_workers.h"
#include "tests/check.h"

namespace {
using openrac::lighting::BakeThreadPool;

// Runs two simultaneous tasks so both persistent workers must participate.
std::set<std::thread::id> worker_ids(BakeThreadPool& pool) {
    std::set<std::thread::id> ids;
    std::mutex mutex;
    std::condition_variable changed;
    bool released = false;
    // Both callbacks borrow local synchronization until the synchronous batch drains.
    const auto result = pool.run(
        2,
        [&] {
            std::unique_lock lock(mutex);
            ids.insert(std::this_thread::get_id());
            changed.notify_all();
            changed.wait(lock, [&] { return released; });
        },
        [&] {
            std::unique_lock lock(mutex);
            changed.wait(lock, [&] { return ids.size() == 2; });
            released = true;
            lock.unlock();
            changed.notify_all();
        }
    );
    CHECK(result.has_value());
    return ids;
}

// Reuses the same two operating-system threads across separate batches.
void test_workers_survive_between_batches() {
    BakeThreadPool pool(2);
    const auto first = worker_ids(pool);
    CHECK(first.size() == 2);
    CHECK(worker_ids(pool) == first);
}

// Drains failed tasks and remains usable after a task or caller callback throws.
void test_failures_drain_and_allow_reuse() {
    BakeThreadPool pool(2);
    std::atomic<unsigned> completed{0};
    const auto failed = pool.run(2, [&] {
        const unsigned index = completed.fetch_add(1);

        // The first task fails while its peer must still be drained.
        if (index == 0) {
            throw std::runtime_error("task failure");
        }
    });
    CHECK(!failed);
    CHECK(completed == 2);
    const auto reporting =
        pool.run(2, [&] { ++completed; }, [] { throw std::runtime_error("progress failure"); });
    CHECK(!reporting);
    CHECK(completed == 4);
    CHECK(pool.run(2, [&] { ++completed; }).has_value());
    CHECK(completed == 6);
}

// Keeps the one-thread execution path on its caller and permits empty batches.
void test_serial_and_empty_batches() {
    BakeThreadPool pool(1);
    const auto caller = std::this_thread::get_id();
    std::thread::id executed;
    CHECK(pool.run(1, [&] { executed = std::this_thread::get_id(); }).has_value());
    CHECK(executed == caller);
    CHECK(pool.run(0, [] { throw std::runtime_error("unexpected empty task"); }).has_value());
}

// Counts one level task before reporting the deliberately failing level's exception.
void count_level_task(std::atomic<unsigned>& completed, unsigned index) {
    ++completed;

    // Only one controller fails; the shared pool must preserve its peer's work.
    if (index == 0) {
        throw std::runtime_error("one level failed");
    }
}

// A failing level's batch cannot cancel or poison another controller's outstanding work.
void test_concurrent_batches_keep_failures_local() {
    BakeThreadPool pool(2);
    std::atomic<unsigned> completed{0};
    std::array<bool, 2> succeeded{};
    std::vector<openrac::JoiningThread> controllers;
    controllers.reserve(succeeded.size());

    for (unsigned index = 0; index < succeeded.size(); ++index) {
        // Each controller borrows its result slot until the joining owners leave scope.
        controllers.emplace_back([&pool, &completed, &succeeded, index] {
            const auto result =
                pool.run(2, [&completed, index] { count_level_task(completed, index); });
            succeeded[index] = result.has_value();
        });
    }

    controllers.clear();
    CHECK(!succeeded[0]);
    CHECK(succeeded[1]);
    CHECK(completed == 4);
}
}  // namespace

// Runs pool lifetime, draining and serial-dispatch checks without game data.
int main() {
    try {
        test_workers_survive_between_batches();
        test_failures_drain_and_allow_reuse();
        test_serial_and_empty_batches();
        test_concurrent_batches_keep_failures_local();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        std::fputs("unknown bake pool test failure\n", stderr);
        return 1;
    }
}
