// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Optional reflection atlas owned by the same scene as the validated GI bake.
#pragma once
#include "enhanced/lighting/environment.h"
#include "enhanced/viewer/debug_lines.h"
#include "enhanced/viewer/environment_selection.h"
#include "renderer/shader.h"

namespace openrac::lighting {
class EnvironmentLighting {
public:
    EnvironmentLighting() = default;
    // Texture ownership cannot be duplicated; the scene releases it while its context is alive.
    EnvironmentLighting(const EnvironmentLighting&) = delete;
    EnvironmentLighting& operator=(const EnvironmentLighting&) = delete;
    // Missing captures are optional; stale/corrupt captures report their rejection.
    void load(const std::filesystem::path& directory, const ProbeGrid& gi);
    void bind(
        const renderer::Shader& shader, bool enabled, const renderer::Camera* camera = nullptr
    ) const;
    // Draws every loaded capture, including boxes not selected for the current water fragment.
    void draw_debug(
        const renderer::FrameInput& input, renderer::RenderState& state, bool shown = true
    );
    void release();

private:
    // One procedural sphere draw samples each capture directly, without spatial blending.
    bool draw_previews(
        const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
    );
    EnvironmentSelection m_selection;
    DebugLines m_debug;
    renderer::Shader m_preview_shader;
    unsigned m_preview_vao = 0;
    bool m_debug_failed =
        false;  // A failed diagnostic does not disable reflections or retry every frame.
    unsigned m_texture = 0;
    mutable unsigned m_fallback = 0;  // Vulkan requires a valid descriptor even with zero probes.
    unsigned m_resolution = 128, m_columns = 1;
    std::vector<std::array<float, 4>> m_metadata;
};
}  // namespace openrac::lighting
