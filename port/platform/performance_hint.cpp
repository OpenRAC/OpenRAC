// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// performance_hint.h. On Android the hint session's functions are looked up in libandroid.so at
// run time, since the port's Android build starts at API 28 and they came with API 33 (setting a
// session's threads with 34, a workload's reset with 36): a function this Android lacks is left
// out, and the session does without it.

#include "platform/performance_hint.h"

#if defined(__ANDROID__)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <dlfcn.h>
#include <format>
#include <mutex>
#include <unistd.h>
#include <vector>

#include "common/env.h"
#include "common/log.h"

namespace openrac::platform {
namespace {

struct Manager;
struct Session;

// The NDK's android/performance_hint.h, as its functions are declared there.
using GetManager = Manager* (*)();
using CreateSession = Session* (*)(Manager*, const std::int32_t*, std::size_t, std::int64_t);
using UpdateTarget = int (*)(Session*, std::int64_t);
using ReportActual = int (*)(Session*, std::int64_t);
using CloseSession = void (*)(Session*);
using SetThreads = int (*)(Session*, const pid_t*, std::size_t);
using WorkloadReset = int (*)(Session*, bool, bool, const char*);

struct Hints {
    bool tried = false;
    const char* why_none = "not started";  // the header's word when there is no session
    Manager* manager = nullptr;
    Session* session = nullptr;
    CreateSession create = nullptr;
    UpdateTarget update_target = nullptr;
    ReportActual report_actual = nullptr;
    CloseSession close = nullptr;
    SetThreads set_threads = nullptr;
    WorkloadReset workload_reset = nullptr;
    std::int64_t target_ns = 0;
    std::chrono::steady_clock::time_point resumed{};
    double waited = 0.0;  // seconds the window thread waited since it resumed
    // The threads in the session (the window thread first), and the count it was made or last set
    // with: a thread added since is taken in at the next frame, on the window thread.
    std::vector<std::int32_t> threads;
    std::size_t threads_in_session = 0;
};

Hints g_hints;
std::mutex g_threads_mutex;  // guards g_hints.threads: the render thread adds itself
std::atomic<std::int64_t> g_render_ns{0};

std::int64_t nanoseconds(double seconds) {
    return static_cast<std::int64_t>(seconds * 1e9);
}

template <typename F>
F look_up(void* library, const char* name) {
    return reinterpret_cast<F>(dlsym(library, name));
}

void start(std::int64_t target_ns) {
    g_hints.tried = true;
    if (env::flag(env::Var::AdpfOff)) {
        g_hints.why_none = "off (OPENRAC_ADPF_OFF)";
        log::info("performance hints: off (OPENRAC_ADPF_OFF)");
        return;
    }
    void* android = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (android == nullptr) {
        g_hints.why_none = "none (no libandroid.so)";
        return;
    }
    const auto get_manager = look_up<GetManager>(android, "APerformanceHint_getManager");
    g_hints.create = look_up<CreateSession>(android, "APerformanceHint_createSession");
    g_hints.update_target =
        look_up<UpdateTarget>(android, "APerformanceHint_updateTargetWorkDuration");
    g_hints.report_actual =
        look_up<ReportActual>(android, "APerformanceHint_reportActualWorkDuration");
    g_hints.close = look_up<CloseSession>(android, "APerformanceHint_closeSession");
    g_hints.set_threads = look_up<SetThreads>(android, "APerformanceHint_setThreads");
    g_hints.workload_reset =
        look_up<WorkloadReset>(android, "APerformanceHint_notifyWorkloadReset");
    if (get_manager == nullptr || g_hints.create == nullptr || g_hints.update_target == nullptr
        || g_hints.report_actual == nullptr || g_hints.close == nullptr) {
        g_hints.why_none = "none (Android before 13, API 33)";
        log::info("performance hints: not on this Android (API 33 and later)");
        return;
    }
    g_hints.manager = get_manager();
    {
        const std::lock_guard lock(g_threads_mutex);
        // The window thread first: it is the one that starts the session.
        const auto self = static_cast<std::int32_t>(gettid());
        std::erase(g_hints.threads, self);
        g_hints.threads.insert(g_hints.threads.begin(), self);
        g_hints.session =
            g_hints.manager != nullptr
                ? g_hints.create(
                      g_hints.manager, g_hints.threads.data(), g_hints.threads.size(), target_ns
                  )
                : nullptr;
        g_hints.threads_in_session = g_hints.threads.size();
    }
    if (g_hints.session == nullptr) {
        g_hints.why_none = "none (the system gave no session)";
        log::info("performance hints: the system gave no session");
        return;
    }
    g_hints.target_ns = target_ns;
    log::info(
        "performance hints: a session for {} thread(s), {:.2f} ms a frame",
        g_hints.threads_in_session,
        static_cast<double>(target_ns) / 1e6
    );
}

// A thread added since the session was made (the render thread starts with the first frame drawn
// on it): set the session's threads anew, or, before Android 14 (API 34), make it again with them.
void take_in_threads() {
    const std::lock_guard lock(g_threads_mutex);
    if (g_hints.session == nullptr || g_hints.threads.size() == g_hints.threads_in_session) {
        return;
    }
    if (g_hints.set_threads != nullptr
        && g_hints.set_threads(g_hints.session, g_hints.threads.data(), g_hints.threads.size())
               == 0) {
        g_hints.threads_in_session = g_hints.threads.size();
    } else {
        g_hints.close(g_hints.session);
        g_hints.session = g_hints.create(
            g_hints.manager, g_hints.threads.data(), g_hints.threads.size(), g_hints.target_ns
        );
        g_hints.threads_in_session = g_hints.threads.size();
        if (g_hints.session == nullptr) {
            g_hints.why_none = "none (the system gave no session for the render thread)";
            log::info("performance hints: no session for the render thread as well");
            return;
        }
    }
    log::info("performance hints: the session covers {} threads", g_hints.threads_in_session);
}

}  // namespace

void set_frame_target(double seconds) {
    const std::int64_t target = nanoseconds(seconds);
    if (target <= 0) {
        return;
    }
    if (!g_hints.tried) {
        start(target);
        return;
    }
    take_in_threads();
    if (g_hints.session != nullptr && target != g_hints.target_ns) {
        g_hints.update_target(g_hints.session, target);
        g_hints.target_ns = target;
    }
}

void add_this_thread() {
    const std::lock_guard lock(g_threads_mutex);
    const auto self = static_cast<std::int32_t>(gettid());
    if (std::find(g_hints.threads.begin(), g_hints.threads.end(), self) == g_hints.threads.end()) {
        g_hints.threads.push_back(self);
    }
}

void work_resumed() {
    if (g_hints.session != nullptr) {
        g_hints.resumed = std::chrono::steady_clock::now();
        g_hints.waited = 0.0;
    }
}

void work_waited(double seconds) {
    g_hints.waited += seconds;
}

void render_work_done(double seconds) {
    g_render_ns.store(nanoseconds(seconds), std::memory_order_relaxed);
}

void frame_work_done() {
    if (g_hints.session == nullptr || g_hints.resumed.time_since_epoch().count() == 0) {
        return;
    }
    const auto since = std::chrono::steady_clock::now() - g_hints.resumed;
    const std::int64_t window = std::chrono::duration_cast<std::chrono::nanoseconds>(since).count()
                                - nanoseconds(g_hints.waited);
    // The render thread's frame was handed over this frame (finish_render), before this.
    const std::int64_t work = std::max(window, g_render_ns.exchange(0, std::memory_order_relaxed));
    if (work > 0) {
        g_hints.report_actual(g_hints.session, work);
    }
}

void workload_reset() {
    if (g_hints.session != nullptr && g_hints.workload_reset != nullptr) {
        g_hints.workload_reset(g_hints.session, true, false, "level load");
    }
}

std::string hint_status() {
    if (g_hints.session == nullptr) {
        return g_hints.why_none;
    }
    return std::format(
        "active, {} thread(s), target {:.2f} ms",
        g_hints.threads_in_session,
        static_cast<double>(g_hints.target_ns) / 1e6
    );
}

}  // namespace openrac::platform

#else

namespace openrac::platform {

void set_frame_target(double) {}

void add_this_thread() {}

void work_resumed() {}

void work_waited(double) {}

void render_work_done(double) {}

void frame_work_done() {}

void workload_reset() {}

std::string hint_status() {
    return {};
}

}  // namespace openrac::platform

#endif
