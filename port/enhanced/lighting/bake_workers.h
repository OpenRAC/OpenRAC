// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Persistent offline workers shared by GI grids and environment capture stages.
#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <vector>

#include "common/joining_thread.h"
#include "common/result.h"

namespace openrac::lighting {
// Owns a fixed worker budget; controller threads submit concurrent batches, never from a worker.
// Tasks may run concurrently and must establish their own floating-point environment.
// Destruction wakes and joins every worker. One-thread pools execute on the caller.
class BakeThreadPool {
public:
    // Starts the requested budget; zero selects hardware concurrency. Creation errors throw.
    explicit BakeThreadPool(unsigned threads = 0);
    ~BakeThreadPool();
    BakeThreadPool(const BakeThreadPool&) = delete;
    BakeThreadPool& operator=(const BakeThreadPool&) = delete;
    BakeThreadPool(BakeThreadPool&&) = delete;
    BakeThreadPool& operator=(BakeThreadPool&&) = delete;

    // Returns the fixed maximum number of concurrent tasks, including serial execution.
    unsigned size() const { return m_count; }

    // Submits one independently owned task; its future propagates execution errors.
    // One-thread pools execute inline under the serial permit. Borrowed inputs outlive the future.
    [[nodiscard]] Result<std::future<void>> submit(const std::function<void()>& task);

    // Runs `count` copies of `task`, then drains them before returning a failure or success.
    // `waiting` runs on the caller after submission, for caller-owned progress/cancellation.
    // Both callbacks may borrow data until this call returns; nested run calls are unsupported.
    [[nodiscard]] Result<void> run(
        unsigned count, const std::function<void()>& task, const std::function<void()>& waiting = {}
    );

private:
    // Runs queued tasks until shutdown has drained the queue.
    void worker();
    // Publishes one owned task and returns its completion handle; submission failures throw.
    std::future<void> enqueue(const std::function<void()>& task);
    // Wakes idle workers and joins them, including partially constructed pools.
    void stop();

    unsigned m_count;                 // Immutable worker budget.
    std::mutex m_serial;              // Serializes one-thread batches across callers.
    std::mutex m_mutex;               // Guards the queue and shutdown flag.
    std::condition_variable m_ready;  // Wakes workers for work or shutdown.
    std::deque<std::packaged_task<void()>> m_tasks;  // Guarded by m_mutex.
    bool m_stopping = false;                         // Guarded by m_mutex.
    std::vector<JoiningThread> m_workers;            // Joined before queue state is destroyed.
};

// Execution controls excluded from the bake identity; a borrowed pool outlives the stage.
struct BakeExecution {
    unsigned threads = 0;            // Zero selects hardware concurrency without a shared pool.
    BakeThreadPool* pool = nullptr;  // Optional shared workers, used only by the calling stage.
};
}  // namespace openrac::lighting
