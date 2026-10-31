// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synchronised output tasks verify bounded ownership, ordered reports and failure draining without disk I/O.
#include <atomic>
#include <cfenv>
#include <condition_variable>
#include <cstdio>
#include <exception>
#include <mutex>
#include <sstream>
#include <stdexcept>

#include "enhanced/lighting/bake_output_queue.h"
#include "enhanced/lighting/bake_workers.h"
#include "tests/check.h"

namespace {
using openrac::lighting::BakeOutputQueue;
using openrac::lighting::BakeThreadPool;

// Report order follows submission even when the later task completes first.
void test_ordered_reports() {
    std::ostringstream report;
    std::mutex mutex;
    std::condition_variable changed;
    bool release_first = false;
    bool second_done = false;
    BakeOutputQueue outputs(report);
    // The first callback remains blocked until the controller observes its peer's completion.
    CHECK(outputs.submit([&mutex, &changed, &release_first] {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return release_first; });
        return std::string("first\n");
    }));
    // This peer needs a different pool worker while the first callback waits.
    CHECK(outputs.submit([&mutex, &changed, &second_done] {
        {
            const std::lock_guard lock(mutex);
            second_done = true;
        }
        changed.notify_all();
        return std::string("second\n");
    }));
    CHECK(outputs.size() == 2);
    {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return second_done; });
        release_first = true;
    }
    changed.notify_all();
    CHECK(outputs.finish());
    CHECK(report.str() == "first\nsecond\n");
}

// Short tasks preserve report order and cannot accumulate more than the documented two jobs.
void check_budget() {
    std::ostringstream report;
    BakeOutputQueue outputs(report);
    std::string expected;
    // Enough submissions exercise repeated retirement and reuse of the same fixed workers.
    constexpr unsigned kSubmissions = 32;

    // Repeated submissions exercise the bound independently of worker completion order.
    for (unsigned index = 0; index < kSubmissions; ++index) {
        const std::string line = std::to_string(index) + "\n";
        expected += line;
        // The worker constructs its own text without copying a retained payload.
        CHECK(outputs.submit([index] { return std::to_string(index) + "\n"; }));
        CHECK(outputs.size() <= 2);
    }

    CHECK(outputs.finish());
    CHECK(outputs.size() == 0);
    CHECK(report.str() == expected);
}

// Controllers may submit compute jobs even with a one-worker budget, without nested-pool deadlock.
void test_compute_controllers(unsigned threads) {
    BakeThreadPool pool(threads);
    std::ostringstream report;
    std::atomic<unsigned> calls{0};
    BakeOutputQueue outputs(report);

    for (unsigned index = 0; index < 4; ++index) {
        // Joined controllers borrow the compute pool until the output queue drains.
        CHECK(outputs.submit([&pool, &calls, index] {
            const auto done = pool.run(pool.size(), [&calls] { ++calls; });

            if (!done) {
                throw std::runtime_error(done.error().message);
            }

            return std::to_string(index);
        }));
        CHECK(outputs.size() <= 2);
    }

    CHECK(outputs.finish());
    CHECK(calls == 4 * pool.size());
    CHECK(report.str() == "0123");
}

// Failure is sticky, prevents later submission and leaves every accepted peer owned until draining.
void test_failure_drains_peers() {
    std::ostringstream report;
    std::mutex mutex;
    std::condition_variable changed;
    bool second_started = false;
    bool first_failed = false;
    std::atomic<bool> second_done{false};
    std::atomic<bool> third_ran{false};
    // Pool callbacks borrow synchronisation state until the inner queue has drained.
    const auto first = [&mutex, &changed, &second_started, &first_failed]() -> std::string {
        {
            std::unique_lock lock(mutex);
            changed.wait(lock, [&second_started] { return second_started; });
            first_failed = true;
        }
        changed.notify_all();
        throw std::runtime_error("output failure");
    };
    const auto second = [&mutex, &changed, &second_started, &first_failed, &second_done] {
        std::unique_lock lock(mutex);
        second_started = true;
        // The first task must observe acceptance before this peer waits for its failure.
        changed.notify_all();
        changed.wait(lock, [&first_failed] { return first_failed; });
        second_done = true;
        return std::string("unpublished\n");
    };
    {
        BakeOutputQueue outputs(report);
        CHECK(outputs.submit(first));
        CHECK(outputs.submit(second));
        const auto failed = outputs.finish();
        CHECK(!failed);
        CHECK(failed.error().message == "output failure");
        CHECK(!outputs.submit([&third_ran] {
            third_ran = true;
            return std::string{};
        }));
        CHECK(!outputs.finish());
    }
    CHECK(second_done);
    CHECK(!third_ran);
    CHECK(report.str().empty());
}

// Failed diagnostic writes prevent successful completion even when streams do not throw.
void test_report_failure_is_sticky() {
    std::ostringstream report;
    report.setstate(std::ios::badbit);
    BakeOutputQueue outputs(report);
    CHECK(outputs.submit([] { return std::string("verified\n"); }));
    const auto failed = outputs.finish();
    CHECK(!failed);
    CHECK(failed.error().message == "cannot publish bake output diagnostics");
    CHECK(!outputs.finish());
    CHECK(!outputs.submit([] { return std::string{}; }));
}

// Destruction waits for a failing task without letting its exception escape an unwinding controller.
void test_destructor_drains_without_throwing() {
    std::ostringstream report;
    std::atomic<unsigned> completed{0};
    {
        BakeOutputQueue outputs(report);
        CHECK(outputs.submit([&completed]() -> std::string {
            ++completed;
            throw std::runtime_error("discarded output failure");
        }));
    }
    CHECK(completed == 1);
}

// Every output controller inherits the submitting thread's arithmetic environment.
void test_output_rounding() {
    const int original = std::fegetround();
    CHECK(std::fesetround(FE_TOWARDZERO) == 0);
    std::ostringstream report;
    std::atomic<int> observed{0};
    BakeOutputQueue outputs(report);
    CHECK(outputs.submit([&observed] {
        observed = std::fegetround();
        return std::string{};
    }));
    CHECK(outputs.finish());
    CHECK(observed == FE_TOWARDZERO);
    CHECK(std::fegetround() == FE_TOWARDZERO);
    CHECK(std::fesetround(original) == 0);
}
}  // namespace

int main() {
    // Unexpected allocation and scheduling errors fail the test after queue owners drain their tasks.
    try {
        test_ordered_reports();
        check_budget();
        test_compute_controllers(1);
        test_compute_controllers(4);
        test_failure_drains_peers();
        test_report_failure_is_sticky();
        test_destructor_drains_without_throwing();
        test_output_rounding();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
