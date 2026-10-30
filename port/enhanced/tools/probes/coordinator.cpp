// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Admits largest ready levels within an estimated memory budget and shares one tracing pool.
#include "enhanced/tools/probes/coordinator.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>

#include "common/joining_thread.h"
#include "common/profiler.h"
#include "enhanced/lighting/bake_workers.h"
#include "enhanced/tools/probes/batch_command.h"
#include "enhanced/tools/probes/coordinator_internal.h"
#include "enhanced/tools/probes/coordinator_protocol.h"

namespace openrac::lighting {
namespace {
// Bounded session controls; JSON numbers retain exact integer values below 2^53.
constexpr std::uint64_t kMaximumBytes = (std::uint64_t{1} << 53) - 1;
constexpr unsigned kMaximumJobs = 256;

// Validates command tokens before publishing a controller's immutable request.
std::vector<std::string> read_arguments(const json::Value& message) {
    if (!message["arguments"].is_array() || message["arguments"].items().empty()) {
        throw std::runtime_error("missing coordinated command arguments");
    }

    std::vector<std::string> arguments;

    for (const auto& value : message["arguments"].items()) {
        if (value.type() != json::Value::Type::String) {
            throw std::runtime_error("invalid coordinated command argument");
        }

        arguments.push_back(value.string());
    }

    return arguments;
}

// ---- Admission state ----

// Owns all controllers and their shared pool; controllers never execute inside that pool.
class Coordinator {
public:
    // Validates every reservation before any controller is admitted.
    Coordinator(
        const json::Value& schedule, std::ostream& output, const NativeLevelTask* task = nullptr
    )
        : m_task(task),
          m_events(output),
          m_pool(static_cast<unsigned>(coordinator_integer(schedule["threads"], kMaximumJobs))),
          m_limit(static_cast<unsigned>(coordinator_integer(schedule["jobs"], kMaximumJobs))),
          m_budget(coordinator_integer(schedule["memory"], kMaximumBytes)) {
        if (schedule["version"].number() != 1 || m_limit == 0 || m_budget == 0
            || !schedule["levels"].is_array() || schedule["levels"].items().empty()
            || schedule["levels"].items().size() > kMaximumJobs) {
            throw std::runtime_error("invalid coordinator schedule");
        }

        read_jobs(schedule["levels"]);
        m_controllers.reserve(m_jobs.size());
    }

    // Cancels blocked plan readers and joins controllers before destroying their shared pool.
    ~Coordinator() { stop(); }

    Coordinator(const Coordinator&) = delete;
    Coordinator& operator=(const Coordinator&) = delete;
    Coordinator(Coordinator&&) = delete;
    Coordinator& operator=(Coordinator&&) = delete;

    // Starts the first memory-admitted set after every record has been validated.
    void start() {
        const std::lock_guard lock(m_mutex);
        admit();
    }

    // Distinguishes a completed session from an interrupted planner connection.
    bool finished() {
        const std::lock_guard lock(m_mutex);
        return m_completed == m_jobs.size();
    }

    // Waits for all native controllers; the destructor then joins outside the admission lock.
    void wait() {
        std::unique_lock lock(m_mutex);
        m_changed.wait(lock, [this] { return m_completed == m_jobs.size() || m_failed; });

        // A failed admission must wake native callers instead of leaving queued levels unfinished.
        if (m_failed) {
            throw std::runtime_error("native level admission failed");
        }
    }

    // Delivers one command, plan or publication acknowledgement to its admitted controller.
    void message(const json::Value& message);

private:
    // Creates stable client IDs and a largest-first admission queue.
    void read_jobs(const json::Value& levels);
    // Starts jobs whose conservative reservations fit; called with m_mutex held.
    void admit();
    // Owns one prepared native pipeline until the Python client acknowledges publication.
    void controller(CoordinatorJob& job);
    // Runs one command after its Python preparation callback has supplied arguments.
    void execute_command(CoordinatorJob& job);
    // Releases a completed reservation and admits pending work while the session remains live.
    void release(CoordinatorJob& job);
    // Stops admissions and wakes every mailbox before joining outside the scheduler mutex.
    void stop();

    const NativeLevelTask* m_task =
        nullptr;                        // Borrowed callback, alive until all controllers join.
    std::condition_variable m_changed;  // Native completion wait; guarded by m_mutex.
    CoordinatorEvents m_events;         // Output writer outlives controllers.
    BakeThreadPool m_pool;              // Shared trace budget, destroyed after controllers join.
    unsigned m_limit;                   // Maximum simultaneously admitted levels.
    std::uint64_t m_budget;             // Estimated resident scene/planner byte limit.
    std::vector<std::unique_ptr<CoordinatorJob>>
        m_jobs;                       // Stable mailboxes, populated before start.
    std::vector<unsigned> m_pending;  // Largest-first IDs; guarded by m_mutex after start.
    std::mutex m_mutex;               // Guards admissions, counters and shutdown.
    unsigned m_active = 0;            // Guarded by m_mutex.
    unsigned m_completed = 0;         // Guarded by m_mutex.
    std::uint64_t m_reserved = 0;     // Guarded by m_mutex.
    bool m_failed = false;    // Admission failure wakes native completion; guarded by m_mutex.
    bool m_stopping = false;  // Guarded by m_mutex.
    std::vector<JoiningThread> m_controllers;  // Joined by stop(), before destroying mailboxes.
};

void Coordinator::read_jobs(const json::Value& levels) {
    for (const auto& record : levels.items()) {
        auto job = std::make_unique<CoordinatorJob>();
        job->id = static_cast<unsigned>(coordinator_integer(record["id"], kMaximumJobs));
        job->memory = coordinator_integer(record["memory"], kMaximumBytes);
        job->weight = coordinator_integer(record["weight"], kMaximumBytes);

        if (job->id != m_jobs.size() || job->memory == 0 || job->memory > m_budget) {
            throw std::runtime_error("invalid level ID or scene reservation exceeds memory budget");
        }

        m_pending.push_back(job->id);
        m_jobs.push_back(std::move(job));
    }

    // Stable ties retain numeric level order for reproducible small schedules.
    std::stable_sort(m_pending.begin(), m_pending.end(), [this](unsigned a, unsigned b) {
        return m_jobs[a]->weight > m_jobs[b]->weight;
    });
}

void Coordinator::admit() {
    for (auto next = m_pending.begin(); next != m_pending.end() && m_active < m_limit;) {
        auto& job = *m_jobs[*next];

        // A smaller pending scene may fit beside a large active one.
        if (job.memory > m_budget - m_reserved) {
            ++next;
            continue;
        }

        ++m_active;
        m_reserved += job.memory;
        next = m_pending.erase(next);
        m_events.emit("admitted", job.id, std::to_string(m_reserved));
        // Controllers borrow stable mailboxes and this scheduler until stop() joins them.
        m_controllers.emplace_back([this, &job] { controller(job); });
    }
}

// ---- Level execution ----

void Coordinator::execute_command(CoordinatorJob& job) {
    std::vector<std::string> arguments;
    {
        std::unique_lock lock(job.mutex);
        job.changed.wait(lock, [&job] { return job.configured || job.closed; });

        if (!job.closed) {
            arguments = job.arguments;
        }
    }

    if (!arguments.empty()) {
        CoordinatorOutput output_buffer(m_events, job);
        CoordinatorInput input_buffer(job);
        std::ostream output(&output_buffer);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        std::istream input(&input_buffer);
        const int status = run_batch_command(arguments, input, output, &m_pool);
        output_buffer.finish();
        m_events.emit("exit", job.id, {}, status);
    }
}

void Coordinator::controller(CoordinatorJob& job) {
    profiling::thread_name("Level controller");

    try {
        // Native flows own preparation and publication without a client mailbox.
        if (m_task) {
            (*m_task)(job.id, m_pool);
        } else {
            execute_command(job);
        }
    } catch (const std::exception& error) {
        m_events.emit("line", job.id, std::string(error.what()) + '\n');
        m_events.emit("exit", job.id, {}, 1);
    } catch (...) {
        m_events.emit("exit", job.id, "unknown level controller failure", 1);
    }

    // Protocol clients retain their reservation through publication; native callbacks already published.
    if (!m_task) {
        std::unique_lock lock(job.mutex);
        job.changed.wait(lock, [&job] { return job.closed; });
    }

    try {
        release(job);
    } catch (const std::exception& error) {
        {
            const std::lock_guard lock(m_mutex);
            m_failed = true;
        }
        m_changed.notify_all();
        m_events.emit("fatal", job.id, error.what(), 1);
    } catch (...) {
        {
            const std::lock_guard lock(m_mutex);
            m_failed = true;
        }
        m_changed.notify_all();
        m_events.emit("fatal", job.id, "cannot release level reservation", 1);
    }
}

void Coordinator::release(CoordinatorJob& job) {
    const std::lock_guard lock(m_mutex);
    m_reserved -= job.memory;
    --m_active;
    ++m_completed;
    m_events.emit("released", job.id, std::to_string(m_reserved));

    if (m_stopping) {
        return;
    }

    admit();

    if (m_completed == m_jobs.size()) {
        m_events.emit("done", 0);
        m_changed.notify_all();
    }
}

void Coordinator::message(const json::Value& message) {
    const auto id = coordinator_integer(message["id"], kMaximumJobs);

    if (id >= m_jobs.size()) {
        throw std::runtime_error("coordinator message has unknown level ID");
    }

    auto& job = *m_jobs[static_cast<std::size_t>(id)];
    const auto& type = message["type"].string();
    {
        const std::lock_guard lock(job.mutex);

        if (type == "command" && !job.configured && !job.closed) {
            job.arguments = read_arguments(message);
            job.configured = true;
        } else if (type == "coverage" && job.configured && !job.queried && !job.planned
                   && !job.closed) {
            job.inputs.push_back(message["text"].string());
            job.queried = true;
        } else if (type == "plan" && job.configured && !job.planned && !job.closed) {
            job.inputs.push_back(message["text"].string());
            job.planned = true;
        } else if (type == "complete" && !job.closed) {
            job.closed = true;
        } else {
            throw std::runtime_error("unexpected or repeated coordinator message");
        }
    }
    job.changed.notify_all();
}

void Coordinator::stop() {
    {
        const std::lock_guard lock(m_mutex);
        m_stopping = true;
    }

    for (auto& job : m_jobs) {
        {
            const std::lock_guard lock(job->mutex);
            job->closed = true;
        }
        job->changed.notify_all();
    }

    m_controllers.clear();
}
}  // namespace

// ---- Session entry points ----

Result<void> schedule_native_levels(const json::Value& schedule, const NativeLevelTask& task) {
    // Scheduler validation and allocation errors cannot escape the native pipeline boundary.
    try {
        std::ostringstream events;
        Coordinator coordinator(schedule, events, &task);
        coordinator.start();
        coordinator.wait();
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

int coordinate_levels(std::istream& input, std::ostream& output) {
    try {
        json::Value schedule;

        if (!read_coordinator_message(input, schedule)) {
            throw std::runtime_error("missing coordinator schedule");
        }

        Coordinator coordinator(schedule, output);
        coordinator.start();
        json::Value message;

        while (read_coordinator_message(input, message)) {
            coordinator.message(message);
        }

        if (!coordinator.finished()) {
            throw std::runtime_error("coordinator input closed before every level completed");
        }

        return 0;
    } catch (const std::exception& error) {
        output << "{\"event\":\"fatal\",\"text\":" << coordinator_string(error.what()) << "}\n"
               << std::flush;
        return 1;
    } catch (...) {
        output << "{\"event\":\"fatal\",\"text\":\"unknown coordinator failure\"}\n" << std::flush;
        return 1;
    }
}
}  // namespace openrac::lighting
