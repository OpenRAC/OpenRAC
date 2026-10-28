// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exact baked ceiling rectangles and their emitting directions, drawn as diagnostic wireframes.

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "common/result.h"
#include "enhanced/viewer/debug_lines.h"

namespace openrac::renderer {
struct FrameInput;
struct RenderState;
}  // namespace openrac::renderer

namespace openrac::lighting {

// Source marker vertices share the lighting diagnostics line layout.
using AreaLightVertex = DebugLineVertex;

// Validates a source report and builds rectangle edges plus one-sided emitting-direction arrows.
// Invalid or excessive source arrays return an error without exposing partial geometry.
[[nodiscard]] Result<std::vector<AreaLightVertex>> area_light_debug_vertices(
    std::string_view report
);

// Render-thread-owned debug geometry; the graphics context outlives this object or its release().
class AreaLightOverlay {
public:
    AreaLightOverlay() = default;
    // Releases diagnostic GPU resources while the owning scene's graphics context is current.
    ~AreaLightOverlay();
    AreaLightOverlay(const AreaLightOverlay&) = delete;
    AreaLightOverlay& operator=(const AreaLightOverlay&) = delete;
    AreaLightOverlay(AreaLightOverlay&&) = delete;
    AreaLightOverlay& operator=(AreaLightOverlay&&) = delete;

    // Loads the companion bake report; a missing report means no markers, not an invalid bake.
    [[nodiscard]] Result<std::size_t> load(const std::filesystem::path& path);
    // Draws opaque visible edges and faint hidden edges without writing scene depth.
    [[nodiscard]] bool draw(
        const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
    );
    // Clears CPU markers and releases GPU objects, with the graphics context current.
    void release();

private:
    DebugLines m_lines;  // Owned source markers and their shared wireframe renderer.
};

}  // namespace openrac::lighting
