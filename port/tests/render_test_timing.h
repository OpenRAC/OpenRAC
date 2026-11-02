// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Host elapsed time and Vulkan compilation deltas for rendering test phases.
// Wall time includes driver pipeline creation and synchronous GPU readbacks;
// glslang time is measured separately by the backend, not inferred from wall time.
// "pipelines" is the drawing-thread counter; "driver_calls" also includes prewarm jobs.

#pragma once

#include <chrono>
#include <string_view>

#include "common/log.h"
#include "renderer/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::test {

// Prints successive phase deltas without finishing GPU work or changing render state.
class RenderTestTiming {
public:
    // Starts in the current graphics context; name must outlive this timer.
    explicit RenderTestTiming(std::string_view name) : m_name(name) {}

    // Ends this host phase; pipeline counts include lazy compiles during drawing.
    void lap(std::string_view phase) {
        const auto now = std::chrono::steady_clock::now();
        const double wall = std::chrono::duration<double>(now - m_start).count();
#if defined(OPENRAC_VULKAN)
        // OpenGL builds and unavailable Vulkan contexts have no Vulkan counters.
        if (gl::is_vulkan()) {
            const auto counters = vkgl::submission_counters();
            log::info(
                "test timing {} {}: wall={:.6f}s glslang={:.6f}s driver={:.6f}s "
                "compiles={} driver_calls={} pipelines={} program_reuses={}",
                m_name,
                phase,
                wall,
                counters.shader_compile_s - m_counters.shader_compile_s,
                counters.pipeline_compile_s - m_counters.pipeline_compile_s,
                counters.shader_compiles - m_counters.shader_compiles,
                counters.pipeline_creation_calls - m_counters.pipeline_creation_calls,
                counters.pipelines_created - m_counters.pipelines_created,
                counters.test_program_reuses - m_counters.test_program_reuses
            );
            m_counters = counters;
        } else
#endif
        {
            log::info("test timing {} {}: wall={:.6f}s", m_name, phase, wall);
        }
        m_start = std::chrono::steady_clock::now();
    }

private:
    std::string_view m_name;  // Borrowed for the timing object's lifetime.
    std::chrono::steady_clock::time_point m_start =
        std::chrono::steady_clock::now();  // Start of the current host phase.
#if defined(OPENRAC_VULKAN)
    vkgl::SubmissionCounters m_counters = vkgl::submission_counters();  // Previous phase totals.
#endif
};

}  // namespace openrac::test
