// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The render handoff's ownership, single in-flight slot, shutdown and exception boundary.
// Expectations are caller-provided payloads, exact job counts and the C++ rounding constants.
#include <cfenv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <latch>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "common/breadcrumbs.h"
#include "game/common/frontend/render_worker.h"
#include "renderer/direct.h"
#include "tests/check.h"

namespace {
using openrac::frontend::RenderWorker;

// Producer mutations cannot change an in-flight snapshot or its thread-local draw diagnostics.
void test_owned_snapshot_and_completion() {
    RenderWorker worker;
    std::vector<int> source{7, 11};
    std::vector<int> observed;
    bool observed_dump = false;
    openrac::renderer::g_dump_draws = true;
    std::latch entered(1);
    std::latch release(1);
    std::string error;
    CHECK(worker.submit(
        [owned = source,
         dump = openrac::renderer::g_dump_draws,
         &observed,
         &observed_dump,
         &entered,
         &release] {
            openrac::renderer::g_dump_draws = dump;
            entered.count_down();
            release.wait();
            observed = owned;
            observed_dump = openrac::renderer::g_dump_draws;
        },
        error
    ));
    entered.wait();
    source.assign(2, 99);
    openrac::renderer::g_dump_draws = false;
    // The expired deadline is a nonblocking readiness check while the job is held by a latch.
    CHECK(!worker.ready_until(std::chrono::steady_clock::time_point::min()));
    release.count_down();
    CHECK(worker.finish(error));
    CHECK((observed == std::vector<int>{7, 11}));
    CHECK(observed_dump);
    CHECK(!openrac::renderer::g_dump_draws);
    CHECK(worker.ready_until(std::chrono::steady_clock::time_point::min()));
}

// Reusing the slot waits for each accepted job to complete.
void test_repeated_jobs_release_their_owners() {
    RenderWorker worker;
    std::string error;
    int completed = 0;

    // Several reuse cycles exercise the same slot rather than an accumulating command queue.
    for (int n = 0; n < 8; ++n) {
        CHECK(worker.submit([&completed] { ++completed; }, error));
    }

    CHECK(worker.finish(error));
    CHECK(completed == 8);
}

// Completion releases the job's final shared owner before the producer can reuse graphics storage.
void test_completion_releases_owned_storage() {
    RenderWorker worker;
    std::string error;
    auto owner = std::make_shared<int>(1);
    const std::weak_ptr<int> observer = owner;
    int observed = 0;
    CHECK(worker.submit([owned = std::move(owner), &observed] { observed = *owned; }, error));
    CHECK(worker.finish(error));
    CHECK(observer.expired());
    CHECK(observed == 1);
}

// A worker exception is reported on the producer and prevents further rendering.
void test_worker_failure_stops_submission() {
    RenderWorker worker;
    std::string error;
    CHECK(worker.submit([] { throw std::runtime_error("test render failure"); }, error));
    CHECK(!worker.finish(error));
    CHECK(error == "test render failure");
    CHECK(!worker.submit([] {}, error));
    CHECK(!worker.submit({}, error));
}

// Destruction joins the last accepted job before its referenced owners leave scope.
void test_shutdown_finishes_the_last_job() {
    int completed = 0;
    {
        RenderWorker worker;
        std::string error;
        CHECK(worker.submit([&completed] { completed = 1; }, error));
    }
    CHECK(completed == 1);
}

// Renderer arithmetic keeps the producer's rounding mode rather than the new thread's default.
void test_inherits_rounding_mode() {
    const int original = std::fegetround();
    CHECK(std::fesetround(FE_TOWARDZERO) == 0);
    int observed = FE_TONEAREST;
    {
        RenderWorker worker;
        std::string error;
        CHECK(worker.submit([&observed] { observed = std::fegetround(); }, error));
        CHECK(worker.finish(error));
    }
    CHECK(std::fesetround(original) == 0);
    CHECK(observed == FE_TOWARDZERO);
}

// Handoff markers keep the same submitted frame on separate main and render trails.
void test_breadcrumb_handoff() {
    using namespace openrac::crash;
    RenderWorker worker;
    std::string error;
    CHECK(worker.submit([] { breadcrumb("test worker pass"); }, error, 777));
    CHECK(worker.finish(error));
    Breadcrumbs::Snapshot entries{};
    const auto count = breadcrumbs().snapshot(entries);
    int queued = -1, started = -1, finished = -1;
    bool worker_pass = false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto& entry = entries[i];
        // Earlier tests leave their own history; only this submission uses frame 777.
        if (entry.value != 777) {
            continue;
        }
        const std::string_view message(entry.message);
        worker_pass |= message == "test worker pass" && entry.thread == BreadcrumbThread::Render;
        if (message == "render job queued") {
            queued = static_cast<int>(i);
            CHECK(entry.thread == BreadcrumbThread::Main);
        } else if (message == "render job started") {
            started = static_cast<int>(i);
            CHECK(entry.thread == BreadcrumbThread::Render);
        } else if (message == "render job finished") {
            finished = static_cast<int>(i);
            CHECK(entry.thread == BreadcrumbThread::Render);
        }
    }
    CHECK(queued >= 0 && started > queued && finished > started);
    CHECK(worker_pass);
    CHECK(worker.submit([] { throw std::runtime_error("breadcrumb failure"); }, error, 778));
    CHECK(!worker.finish(error));
    const auto failed_count = breadcrumbs().snapshot(entries);
    bool failed = false;
    for (std::size_t i = 0; i < failed_count; ++i) {
        failed |= entries[i].value == 778 && entries[i].thread == BreadcrumbThread::Render
                  && std::string_view(entries[i].message) == "render job failed";
    }
    CHECK(failed);
}

// Runs every worker contract check; unexpected startup exceptions fail the test program.
bool run_tests() {
    // Startup and allocation errors must produce a failing test result.
    try {
        test_owned_snapshot_and_completion();
        test_repeated_jobs_release_their_owners();
        test_completion_releases_owned_storage();
        test_worker_failure_stops_submission();
        test_shutdown_finishes_the_last_job();
        test_inherits_rounding_mode();
        test_breadcrumb_handoff();
        return true;
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "render_worker: %s\n", failure.what());
        return false;
    } catch (...) {
        std::fputs("render_worker: unknown exception\n", stderr);
        return false;
    }
}

}  // namespace

// Reports assertion failures or an unexpected exception to CTest.
int main() {
    return run_tests() ? openrac::test::result() : EXIT_FAILURE;
}
