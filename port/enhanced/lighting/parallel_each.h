// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic parallel loops for planning stages that write one slot per index.
#pragma once
#include <algorithm>
#include <atomic>
#include <cfenv>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

#include "common/joining_thread.h"

namespace openrac::lighting::bake_detail {
// Runs task(i) for every i on all CPU threads; each task writes only its own slot, so the result
// does not depend on scheduling. The caller's rounding mode carries over; the first error is rethrown.
template <class Task>
void parallel_each(std::size_t count, const Task& task) {
    std::fenv_t environment;
    std::fegetenv(&environment);
    std::atomic<std::size_t> next{0};
    std::exception_ptr failure;
    std::mutex mutex;
    auto worker = [&] {
        std::fesetenv(&environment);

        try {
            for (auto i = next.fetch_add(1); i < count; i = next.fetch_add(1)) {
                task(i);
            }
        } catch (...) {
            const std::lock_guard lock(mutex);
            failure = failure ? failure : std::current_exception();
            next = count;
        }
    };
    const auto threads =
        std::min<std::size_t>(std::max(1u, std::thread::hardware_concurrency()), count);
    {
        std::vector<JoiningThread> pool;

        for (std::size_t t = 1; t < threads; ++t) {
            pool.emplace_back(worker);
        }

        worker();
    }

    if (failure) {
        std::rethrow_exception(failure);
    }
}
}  // namespace openrac::lighting::bake_detail
