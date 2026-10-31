// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Ordered completion and exception-safe draining for bounded bake output work.
#include "enhanced/lighting/bake_output_queue.h"

#include <chrono>
#include <stdexcept>
#include <utility>

#include "common/profiler.h"

namespace openrac::lighting {
namespace {
// Two pending grids bound raw maps and encoded payloads while later tracing continues.
constexpr std::size_t kMaximumPending = 2;
}  // namespace

BakeOutputQueue::~BakeOutputQueue() {
    // No borrowed task data may escape when a controller or earlier output has failed.
    for (auto& pending : m_pending) {
        // Destruction preserves the controller failure instead of propagating a worker exception.
        try {
            pending.done.get();
        } catch (...) {
            // A failed task has completed; later tasks still borrow controller inputs.
            continue;
        }
    }
}

void BakeOutputQueue::retire() {
    const profiling::Zone timing("Wait for bake output", profiling::ZoneColour::Idle);
    auto pending = std::move(m_pending.front());
    m_pending.pop_front();
    pending.done.get();
    m_output << pending.result->text << std::flush;

    // Default streams set failure flags instead of throwing when a diagnostic cannot be written.
    if (!m_output) {
        throw std::runtime_error("cannot publish bake output diagnostics");
    }
}

Result<void> BakeOutputQueue::fail(Error error) {
    // Later failures cannot replace the first reason this batch became unpublishable.
    if (!m_failure) {
        m_failure = std::move(error);
    }

    return unexpected(*m_failure);
}

Result<void> BakeOutputQueue::make_room() {
    // No later output may be submitted or published after a verification failure.
    if (m_failure) {
        return unexpected(*m_failure);
    }

    // Scheduling and task errors remain inside the queue interface.
    try {
        // Completed diagnostics are emitted in submission order before starting another grid.
        while (!m_pending.empty()
               && m_pending.front().done.wait_for(std::chrono::seconds(0))
                      == std::future_status::ready) {
            retire();
        }

        // Capacity is enforced before callers allocate the next encoded payload.
        if (m_pending.size() >= kMaximumPending) {
            retire();
        }

        return {};
    } catch (const std::exception& error) {
        return fail(Error{error.what()});
    } catch (...) {
        return fail(Error{"unknown bake output failure"});
    }
}

std::future<void> BakeOutputQueue::schedule(const std::function<void()>& task, Pending& pending) {
    std::packaged_task<void()> operation(task);
    auto done = operation.get_future();
    pending.controller = JoiningThread(std::move(operation));
    return done;
}

Result<void> BakeOutputQueue::submit(std::function<std::string()> task) {
    const auto room = make_room();

    // A failed earlier output makes this batch unpublishable.
    if (!room) {
        return unexpected(room.error());
    }

    // Scheduling and task errors remain inside the queue interface.
    try {
        auto result = std::make_shared<Completion>();

        // Output metadata must retain the controller arithmetic mode.
        if (std::fegetenv(&result->environment) != 0) {
            return fail(Error{"cannot read output floating-point environment"});
        }

        // Build callback storage before scheduling so allocation failure cannot orphan borrowed input work.
        const std::function<void()> callback = [result, task = std::move(task)] {
            // Dedicated controllers submit compute jobs without occupying a shared worker themselves.
            profiling::thread_name("Bake output controller");
            const profiling::Zone timing("Bake output task");

            // Reused workers may retain arithmetic state from a preceding stage.
            if (std::fesetenv(&result->environment) != 0) {
                throw std::runtime_error("cannot restore output floating-point environment");
            }

            result->text = task();
        };
        m_pending.push_back({result, {}, {}});
        m_pending.back().done = schedule(callback, m_pending.back());
        return {};
    } catch (const std::exception& error) {
        // A failed submission has no running task; preallocated bookkeeping must not survive as a bad future.
        if (!m_pending.empty() && !m_pending.back().done.valid()) {
            m_pending.pop_back();
        }

        return fail(Error{error.what()});
    } catch (...) {
        // Remove bookkeeping only when no worker was accepted.
        if (!m_pending.empty() && !m_pending.back().done.valid()) {
            m_pending.pop_back();
        }

        return fail(Error{"cannot submit bake output"});
    }
}

Result<void> BakeOutputQueue::finish() {
    // A failed verification cannot be converted into a successful completion.
    if (m_failure) {
        return unexpected(*m_failure);
    }

    // Scheduling and task errors remain inside the queue interface.
    try {
        // Every output must verify before density checks, reflections or publication can begin.
        while (!m_pending.empty()) {
            retire();
        }

        return {};
    } catch (const std::exception& error) {
        return fail(Error{error.what()});
    } catch (...) {
        return fail(Error{"unknown bake output failure"});
    }
}
}  // namespace openrac::lighting
