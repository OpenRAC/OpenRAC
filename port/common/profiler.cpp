// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Optional CPU zones and backend hooks; ordinary builds do not link Tracy.
#include "common/profiler.h"

#ifdef OPENRAC_TRACY
#include <atomic>
#include <cstring>
#include <thread>

namespace openrac::profiling {
namespace {
std::atomic<const GpuHooks*> gpu_hooks = nullptr;
}

Zone::Zone(std::string_view name, bool gpu, std::source_location location)
    : m_cpu(
          location.line(),
          location.file_name(),
          std::strlen(location.file_name()),
          location.function_name(),
          std::strlen(location.function_name()),
          name.data(),
          name.size(),
          0u
      ) {
    if (!connected()) {
        return;
    }
    const auto* hooks = gpu_hooks.load();
    if (gpu && hooks && hooks->begin && hooks->end) {
        m_gpu = hooks->begin(name, location);
        m_gpu_end = hooks->end;
    }
}

Zone::Zone(std::string_view name, ZoneColour colour, std::source_location location)
    : Zone(name, false, location) {
    m_cpu.Color(static_cast<std::uint32_t>(colour));
}

Zone::~Zone() {
    if (m_gpu) {
        m_gpu_end(m_gpu);
    }
}

bool connected() {
    return tracy::GetProfiler().IsConnected();
}

bool wait_for_connection(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!connected() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return connected();
}

void thread_name(const char* name) {
    tracy::SetThreadName(name);
}

void frame(const char* name) {
    tracy::Profiler::SendFrameMark(name);
}

void set_gpu_hooks(const GpuHooks* hooks) {
    gpu_hooks.store(hooks);
}
}  // namespace openrac::profiling
#endif
