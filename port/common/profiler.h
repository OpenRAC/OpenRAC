// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Optional Tracy instrumentation, independent of the in-game statistics frame owner.
#pragma once

#include <chrono>
#include <source_location>
#include <string_view>

#ifdef OPENRAC_TRACY
#include <tracy/Tracy.hpp>
#endif

namespace openrac::profiling {

// Backend hooks are installed before rendering starts and removed after the worker stops.
struct GpuHooks {
    void* (*begin)(std::string_view, std::source_location) = nullptr;
    void (*end)(void*) = nullptr;
};

#ifdef OPENRAC_TRACY
// A thread-local lifetime: never move a zone across the scene-snapshot handoff.
class Zone {
public:
    explicit Zone(
        std::string_view name,
        bool gpu = false,
        std::source_location location = std::source_location::current()
    );
    ~Zone();
    Zone(const Zone&) = delete;
    Zone& operator=(const Zone&) = delete;

private:
    tracy::ScopedZone m_cpu;
    void* m_gpu = nullptr;
    void (*m_gpu_end)(void*) = nullptr;
};

// Whether the optional client has an attached capture.
bool connected();
// Waits at most the requested duration for a capture; offline tools may use it before loading.
bool wait_for_connection(std::chrono::milliseconds timeout);
// Names the calling thread; name must remain valid until this call returns.
void thread_name(const char* name);
// A displayed-frame marker, or a named simulation/bake stream (static names only).
void frame(const char* name = nullptr);
// Registers the graphics owner's timestamp implementation, or clears it with nullptr.
void set_gpu_hooks(const GpuHooks* hooks);
#else
class Zone {
public:
    explicit Zone(
        std::string_view, bool = false, std::source_location = std::source_location::current()
    ) {}

    Zone(const Zone&) = delete;
    Zone& operator=(const Zone&) = delete;
};

inline bool connected() {
    return false;
}

inline bool wait_for_connection(std::chrono::milliseconds) {
    return false;
}

inline void thread_name(const char*) {}

inline void frame(const char* = nullptr) {}

inline void set_gpu_hooks(const GpuHooks*) {}
#endif

}  // namespace openrac::profiling
