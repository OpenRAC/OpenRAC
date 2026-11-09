// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// BakeThreadPool's queue and joining lifetime; batches drain even when progress callbacks fail.
#include "enhanced/lighting/bake_workers.h"

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <exception>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

#include "common/profiler.h"

namespace openrac::lighting {
namespace {
// Drains all submitted work and retains the original failure, even when later tasks also fail.
void drain(std::vector<std::future<void>>& futures, std::exception_ptr& failure) {
    const profiling::Zone zone("Bake pool drain", profiling::ZoneColour::Idle);

    for (auto& future : futures) {
        try {
            future.get();
        } catch (...) {
            // The first failure explains the batch; every other task still needs draining.
            if (!failure) {
                failure = std::current_exception();
            }
        }
    }
}
}  // namespace

BakeThreadPool::BakeThreadPool(unsigned threads)
    : m_count(threads ? threads : std::max(1u, std::thread::hardware_concurrency())) {
    // Serial work needs no background thread or queue synchronization.
    if (m_count == 1) {
        return;
    }

    try {
        m_workers.reserve(m_count);

        // Pool workers borrow this state until stop() joins them.
        for (unsigned index = 0; index < m_count; ++index) {
            m_workers.emplace_back([this] { worker(); });
        }
    } catch (...) {
        stop();
        throw;
    }
}

BakeThreadPool::~BakeThreadPool() {
    stop();
}

void BakeThreadPool::stop() {
    {
        const std::lock_guard lock(m_mutex);
        m_stopping = true;
    }
    m_ready.notify_all();
    m_workers.clear();
}

void BakeThreadPool::worker() {
    profiling::thread_name("Bake pool worker");

    // Shutdown exits only after outstanding queued work has been taken.
    for (;;) {
        std::packaged_task<void()> task;
        {
            std::unique_lock lock(m_mutex);
            const profiling::Zone zone("Bake pool idle", profiling::ZoneColour::Idle);
            m_ready.wait(lock, [this] { return m_stopping || !m_tasks.empty(); });

            // An empty shutdown queue has no borrowed task data left to access.
            if (m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop_front();
        }
        const profiling::Zone zone("Bake pool task");
        task();
    }
}

std::future<void> BakeThreadPool::enqueue(const std::function<void()>& task) {
    std::packaged_task<void()> queued(task);
    auto future = queued.get_future();
    {
        const std::lock_guard lock(m_mutex);
        m_tasks.push_back(std::move(queued));
    }
    m_ready.notify_one();
    return future;
}

Result<std::future<void>> BakeThreadPool::submit(const std::function<void()>& task) {
    try {
        // Parallel callers retain the same queue and fixed execution budget as synchronous batches.
        if (m_count > 1) {
            return enqueue(task);
        }

        std::packaged_task<void()> queued(task);
        auto future = queued.get_future();
        {
            const std::lock_guard serial(m_serial);
            queued();
        }
        return future;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"cannot submit bake worker task"});
    }
}

Result<void> BakeThreadPool::run(
    unsigned count, const std::function<void()>& task, const std::function<void()>& waiting
) {
    std::vector<std::future<void>> futures;
    std::exception_ptr failure;
    std::unique_lock serial(m_serial, std::defer_lock);

    try {
        // Single-thread pools run on their controllers, with one global execution permit.
        if (m_count == 1) {
            serial.lock();
        }

        futures.reserve(count);

        for (unsigned index = 0; index < count; ++index) {
            // Serial stages retain the caller's thread and arithmetic environment.
            if (m_count == 1) {
                task();
                continue;
            }

            futures.push_back(enqueue(task));
        }

        // Caller-owned reporting can cancel its own stage before unwinding.
        if (waiting) {
            waiting();
        }
    } catch (...) {
        failure = std::current_exception();
    }

    drain(futures, failure);

    try {
        // No callback can outlive run(), even on submission or reporting failure.
        if (failure) {
            std::rethrow_exception(failure);
        }

        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"unknown bake worker failure"});
    }
}

void parallel_bake_indices(
    std::size_t count, const BakeExecution& execution, const std::function<void(std::size_t)>& task
) {
    auto* pool = execution.pool;
    const auto budget = pool ? pool->size() : std::max(1u, std::thread::hardware_concurrency());
    const auto threads = static_cast<unsigned>(std::min<std::size_t>(
        count, execution.threads ? std::min(execution.threads, budget) : budget
    ));
    if (threads <= 1 && !pool) {
        for (std::size_t i = 0; i < count; ++i) {
            task(i);
        }
        return;
    }
    if (threads == 0) {
        return;
    }

    std::unique_ptr<BakeThreadPool> local_pool;
    if (!pool) {
        local_pool = std::make_unique<BakeThreadPool>(threads);
        pool = local_pool.get();
    }
    std::fenv_t environment;
    if (std::fegetenv(&environment) != 0) {
        throw std::runtime_error("could not read bake floating-point environment");
    }
    std::atomic<std::size_t> next{0};
    const auto result = pool->run(threads, [&] {
        try {
            if (std::fesetenv(&environment) != 0) {
                throw std::runtime_error("could not set bake floating-point environment");
            }
            for (auto i = next.fetch_add(1); i < count; i = next.fetch_add(1)) {
                task(i);
            }
        } catch (...) {
            next.store(count);
            throw;
        }
    });
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace openrac::lighting
