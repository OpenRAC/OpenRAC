// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private session records and per-level stream adapters for the bake coordinator.
#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

namespace openrac::lighting {
// One admitted controller's input mailbox; all mutable fields are guarded by mutex.
struct CoordinatorJob {
    unsigned id = 0;                     // Stable client index, assigned before admission.
    std::uint64_t weight = 0;            // Scheduling priority, normally exported asset bytes.
    std::uint64_t memory = 0;            // Conservative estimated resident scene/planner bytes.
    std::mutex mutex;                    // Guards command, plan and completion publication.
    std::condition_variable changed;     // Wakes command/plan readers or the completion waiter.
    std::vector<std::string> arguments;  // Owned command, empty until the client prepares sources.
    std::deque<std::string> inputs;      // Coverage request followed by the final region plan.
    bool closed = false;                 // Client published outputs, failed, or disconnected.
    bool configured = false;             // Prevents replacing an already submitted command.
    bool queried = false;  // At most one batched coverage request precedes the final plan.
    bool planned = false;  // Prevents delivering a second region plan.
};

// Serializes complete JSON events from independent controller threads to one output stream.
class CoordinatorEvents {
public:
    // Borrows the session output until every controller has joined.
    explicit CoordinatorEvents(std::ostream& output) : m_output(output) {}

    // Emits one newline-delimited event; text is escaped before writing.
    void emit(std::string_view event, unsigned id, std::string_view text = {}, int status = 0);

private:
    std::ostream& m_output;  // Borrowed session output.
    std::mutex m_mutex;      // Prevents interleaving event records.
};

// Turns the existing per-level diagnostic stream into tagged, complete line events.
class CoordinatorOutput : public std::streambuf {
public:
    // Borrows an event writer that outlives this controller's command.
    CoordinatorOutput(CoordinatorEvents& events, CoordinatorJob& job)
        : m_events(events),
          m_job(job),
          m_id(job.id) {}

    // Publishes a final diagnostic even if it lacks a trailing newline.
    void finish();

private:
    // Emits complete lines without forcing a JSON message for each formatted insertion.
    std::streamsize xsputn(const char* bytes, std::streamsize count) override;
    // Accepts the occasional single-character insertion from formatted output.
    int_type overflow(int_type value) override;
    CoordinatorEvents& m_events;  // Borrowed event writer.
    CoordinatorJob& m_job;        // Cancellation is guarded by the mailbox mutex.
    unsigned m_id;                // Stable job identity.
    std::string m_pending;        // Private partial diagnostic line.
};

// Blocks for the optional coverage request and the final region plan.
class CoordinatorInput : public std::streambuf {
public:
    // Borrows one controller mailbox until the command returns.
    explicit CoordinatorInput(CoordinatorJob& job) : m_job(job) {}

private:
    // Supplies queued input records, or EOF after cancellation/disconnection.
    int_type underflow() override;
    CoordinatorJob& m_job;  // Borrowed synchronized mailbox.
    std::string m_bytes;    // Owns the get area's bytes until command completion.
};
}  // namespace openrac::lighting
