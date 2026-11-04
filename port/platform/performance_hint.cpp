// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// performance_hint.h. On Android the hint session's functions are looked up in libandroid.so at
// run time, since the port's Android build starts at API 28 and they came with API 33.

#include "platform/performance_hint.h"

#if defined(__ANDROID__)

#include <chrono>
#include <cstdint>
#include <dlfcn.h>
#include <format>
#include <unistd.h>

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

struct Hints {
    bool tried = false;
    const char* why_none = "not started";  // the header's word when there is no session
    Manager* manager = nullptr;
    Session* session = nullptr;
    CreateSession create = nullptr;
    UpdateTarget update_target = nullptr;
    ReportActual report_actual = nullptr;
    CloseSession close = nullptr;
    std::int64_t target_ns = 0;
    std::chrono::steady_clock::time_point resumed{};
};

Hints g_hints;

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
    if (get_manager == nullptr || g_hints.create == nullptr || g_hints.update_target == nullptr
        || g_hints.report_actual == nullptr || g_hints.close == nullptr) {
        g_hints.why_none = "none (Android before 13, API 33)";
        log::info("performance hints: not on this Android (API 33 and later)");
        return;
    }
    g_hints.manager = get_manager();
    // The window thread only: it runs the game and prepares and presents each frame.
    const auto self = static_cast<std::int32_t>(gettid());
    g_hints.session =
        g_hints.manager != nullptr ? g_hints.create(g_hints.manager, &self, 1, target_ns) : nullptr;
    if (g_hints.session == nullptr) {
        g_hints.why_none = "none (the system gave no session)";
        log::info("performance hints: the system gave no session");
        return;
    }
    g_hints.target_ns = target_ns;
    log::info(
        "performance hints: a session for the window thread, {:.2f} ms a frame",
        static_cast<double>(target_ns) / 1e6
    );
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
    if (g_hints.session != nullptr && target != g_hints.target_ns) {
        g_hints.update_target(g_hints.session, target);
        g_hints.target_ns = target;
    }
}

void work_resumed() {
    if (g_hints.session != nullptr) {
        g_hints.resumed = std::chrono::steady_clock::now();
    }
}

void frame_work_done() {
    if (g_hints.session == nullptr || g_hints.resumed.time_since_epoch().count() == 0) {
        return;
    }
    const auto work = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - g_hints.resumed
    );
    if (work.count() > 0) {
        g_hints.report_actual(g_hints.session, work.count());
    }
}

std::string hint_status() {
    if (g_hints.session == nullptr) {
        return g_hints.why_none;
    }
    return std::format(
        "active, window thread, target {:.2f} ms", static_cast<double>(g_hints.target_ns) / 1e6
    );
}

}  // namespace openrac::platform

#else

namespace openrac::platform {

void set_frame_target(double) {}

void work_resumed() {}

void frame_work_done() {}

std::string hint_status() {
    return {};
}

}  // namespace openrac::platform

#endif
