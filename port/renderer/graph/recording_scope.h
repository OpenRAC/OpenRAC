// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Scopes native work to a compiled graph pass on the recording thread.
#pragma once

#include <stdexcept>

#include "renderer/graph/frame_graph.h"

namespace openrac::renderer::graph {
namespace detail {
inline thread_local unsigned recording_depth = 0;
}

// Executors enter this only while recording a compiled work pass. Native GPU
// work requires this boundary; an export cannot authorize work of its own.
class RecordingScope {
public:
    explicit RecordingScope(const PlannedPass& pass) {
        if (pass.kind != PassKind::Work || pass.name.empty()) {
            throw std::logic_error("native GPU work requires a named graph work pass");
        }
        ++detail::recording_depth;
    }

    ~RecordingScope() { --detail::recording_depth; }

    RecordingScope(const RecordingScope&) = delete;
    RecordingScope& operator=(const RecordingScope&) = delete;
};

[[nodiscard]] inline bool recording_active() {
    return detail::recording_depth != 0;
}

inline void require_recording() {
    if (!recording_active()) {
        throw std::logic_error("native GPU work bypassed the render graph");
    }
}
}  // namespace openrac::renderer::graph
