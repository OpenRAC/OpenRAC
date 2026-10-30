// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <filesystem>

#include "common/result.h"
#include "enhanced/viewer/debug_lines.h"

namespace openrac::lighting {
// Optional baker diagnostics. Missing reports leave ordinary rendering unaffected.
Result<std::vector<DebugLineVertex>> medial_axis_debug_vertices(std::string_view report);

class MedialAxisOverlay {
public:
    Result<std::size_t> load(const std::filesystem::path& path);
    bool draw(const renderer::FrameInput& input, renderer::RenderState& state, std::string& error);
    void release();

private:
    DebugLines m_lines;
};
}  // namespace openrac::lighting
