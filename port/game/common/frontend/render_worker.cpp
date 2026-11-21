// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// RenderWorker's bounded handoff and exception boundary.
#include "game/common/frontend/render_worker.h"

#include <exception>
#include <format>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
#include <utility>

#include "common/breadcrumbs.h"
#include "common/crash.h"
#include "common/profiler.h"

namespace openrac::frontend {

namespace {

// Keep a diagnostic available if exception formatting runs out of memory.
const char* failure_message(const std::string& message) {
    return message.empty() ? "render job failed while formatting its exception" : message.c_str();
}

// Reports a job failure without letting diagnostic formatting escape the worker.
std::string report_failure(
    std::int64_t frame, std::source_location submitted_at, const char* type, const char* message
) {
    crash::breadcrumb(crash::BreadcrumbThread::Render, "render job failed", frame);
    std::string diagnostic;
    try {
        std::string_view source = submitted_at.file_name();
        if (const auto separator = source.find_last_of("/\\");
            separator != std::string_view::npos) {
            source.remove_prefix(separator + 1);
        }
        diagnostic = std::format(
            "render job for frame {} submitted from {} ({}:{}) threw {}: {}",
            frame < 0 ? "unspecified" : std::to_string(frame),
            submitted_at.function_name(),
            source,
            submitted_at.line(),
            type,
            *message == '\0' ? "exception provided no message" : message
        );
    } catch (...) {
        // The crash reporter and caller can both use the fallback message.
    }
    crash::report(failure_message(diagnostic), crash::Kind::Exception);
    return diagnostic;
}

// Captures the producer's arithmetic mode before the worker can run any renderer calculations.
std::fenv_t capture_environment() {
    std::fenv_t environment{};

    // A default worker mode can change culling, interpolation and uniform values on Windows.
    if (std::fegetenv(&environment) != 0) {
        throw std::runtime_error("cannot capture render floating-point environment");
    }

    return environment;
}

}  // namespace

RenderWorker::RenderWorker() : m_environment(capture_environment()), m_thread([this] { run(); }) {}

RenderWorker::~RenderWorker() {
    {
        std::unique_lock lock(m_mutex);
        m_changed.wait(lock, [this] { return !m_busy; });
        m_stopping = true;
    }
    m_changed.notify_one();
    m_thread.join();
}

bool RenderWorker::submit(
    std::function<void()> job,
    std::string& error,
    std::int64_t frame,
    std::source_location submitted_at
) {
    const profiling::Zone trace("Render handoff");
    // An empty function cannot wake a consumer waiting for an owned job.
    if (!job) {
        error = "empty render job";
        return false;
    }

    std::unique_lock lock(m_mutex);
    m_changed.wait(lock, [this] { return !m_busy; });

    // A failed renderer cannot accept another snapshot.
    if (m_failure) {
        error = failure_message(*m_failure);
        return false;
    }

    m_job = std::move(job);
    m_frame = frame;
    m_submitted_at = submitted_at;
    m_busy = true;
    crash::breadcrumb(crash::BreadcrumbThread::Main, "render job queued", frame);
    lock.unlock();
    m_changed.notify_all();
    return true;
}

bool RenderWorker::finish(std::string& error) {
    const profiling::Zone trace("Render completion wait", profiling::ZoneColour::Idle);
    std::unique_lock lock(m_mutex);
    m_changed.wait(lock, [this] { return !m_busy; });
    error = m_failure ? failure_message(*m_failure) : "";
    return !m_failure;
}

bool RenderWorker::ready_until(std::chrono::steady_clock::time_point deadline) {
    const profiling::Zone trace("Render readiness wait", profiling::ZoneColour::Idle);
    std::unique_lock lock(m_mutex);
    return m_changed.wait_until(lock, deadline, [this] { return !m_busy; });
}

void RenderWorker::run() {
    profiling::thread_name("Render");
    // A render fault needs its own alternate signal stack on POSIX hosts.
    crash::prepare_thread();

    // Destruction ends the loop after the final accepted snapshot has completed.
    for (;;) {
        std::unique_lock lock(m_mutex);
        {
            const profiling::Zone trace("Render worker idle", profiling::ZoneColour::Idle);
            m_changed.wait(lock, [this] { return m_stopping || static_cast<bool>(m_job); });
        }

        // Shutdown follows completion, so no snapshot is discarded.
        if (m_stopping) {
            return;
        }

        auto job = std::exchange(m_job, {});
        const auto frame = m_frame;
        const auto submitted_at = m_submitted_at;
        lock.unlock();
        std::optional<std::string> failure;
        const crash::BreadcrumbScope breadcrumbs(crash::BreadcrumbThread::Render, frame);
        crash::breadcrumb(crash::BreadcrumbThread::Render, "render job started", frame);

        // Render failures return to the producer instead of escaping the thread entry point.
        try {
            // Every snapshot uses the same rounding and denormal handling as synchronous recording.
            if (std::fesetenv(&m_environment) != 0) {
                throw std::runtime_error("cannot restore render floating-point environment");
            }

            const profiling::Zone trace("Render snapshot");
            job();
        } catch (const std::exception& exception) {
            failure =
                report_failure(frame, submitted_at, typeid(exception).name(), exception.what());
        } catch (...) {
            failure = report_failure(
                frame, submitted_at, "a non-standard exception", "no diagnostic message available"
            );
        }

        // Release captured owners before publishing completion.
        job = {};
        crash::breadcrumb(crash::BreadcrumbThread::Render, "render job finished", frame);
        lock.lock();
        m_failure = std::move(failure);
        m_busy = false;
        lock.unlock();
        m_changed.notify_all();
    }
}

}  // namespace openrac::frontend
