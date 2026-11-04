// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded asynchronous output work with caller-owned, submission-ordered diagnostics.
#pragma once
#include <cfenv>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <ostream>
#include <string>

#include "common/joining_thread.h"
#include "common/result.h"

namespace openrac::lighting {
// One controller owns this queue. Output callbacks write only their own completion result.
// Every callback's borrowed input outlives the queue. Destruction drains without throwing.
class BakeOutputQueue {
public:
    // Borrows the controller's diagnostic stream until all output work has drained.
    explicit BakeOutputQueue(std::ostream& output) : m_output(output) {}

    // Joins outstanding work even when the controller or an earlier output job failed.
    ~BakeOutputQueue();
    BakeOutputQueue(const BakeOutputQueue&) = delete;
    BakeOutputQueue& operator=(const BakeOutputQueue&) = delete;
    BakeOutputQueue(BakeOutputQueue&&) = delete;
    BakeOutputQueue& operator=(BakeOutputQueue&&) = delete;

    // Waits for space before the caller allocates its next grid; publishes ready reports in order.
    [[nodiscard]] Result<void> make_room();
    // Submits an owned callback, restoring the caller's arithmetic environment on its worker.
    // Failures remain sticky; the destructor drains peers before borrowed inputs may be released.
    [[nodiscard]] Result<void> submit(std::function<std::string()> task);
    // Publishes every pending result in order; the first task or output-stream failure is returned.
    [[nodiscard]] Result<void> finish();

    // Returns queued jobs, including completed jobs awaiting ordered report publication.
    std::size_t size() const { return m_pending.size(); }

private:
    // Worker and controller share this result until its completion future is consumed.
    struct Completion {
        std::string text;  // Written by the worker, read by the controller after future.get().
        std::fenv_t
            environment{};  // Immutable caller arithmetic environment, captured before submission.
    };

    // One future owns the completion barrier for a copy-free callback payload.
    struct Pending {
        std::shared_ptr<Completion> result;  // Shared only by this queue and its worker callback.
        std::future<void> done;    // Publishes result text or propagates the task failure.
        JoiningThread controller;  // Joined before releasing callback inputs.
    };

    // Starts a bounded controller when compute callbacks need to submit work to the borrowed pool.
    std::future<void> schedule(const std::function<void()>& task, Pending& pending);
    // Removes the oldest job before propagating failure; all later jobs remain owned for draining.
    void retire();
    // Retains the first error so failed output can never become a publishable completed bake.
    Result<void> fail(Error error);
    std::ostream& m_output;  // Used only by the owning controller.
    std::optional<Error>
        m_failure;  // Controller-owned sticky failure, absent until the first error.
    std::deque<Pending> m_pending;  // Controller-owned queue, capped at two jobs.
};
}  // namespace openrac::lighting
