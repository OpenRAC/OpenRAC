// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Shader-only rigid water attachments, with valid fallback descriptors on dry levels.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>

#include "renderer/renderer.h"
#include "renderer/shader.h"

namespace openrac::enhanced {
class WaterFloat {
public:
    WaterFloat() = default;
    ~WaterFloat();
    WaterFloat(const WaterFloat&) = delete;
    WaterFloat& operator=(const WaterFloat&) = delete;
    // Adds the shared water evaluator only on enhanced Vulkan builds.
    static std::string vertex_source(std::string source);
    // Allocates fallback descriptors on the render thread.
    void load();
    // Advances water once before depth, shadow and colour consumers.
    void prepare(const renderer::FrameInput& input);
    // Binds shared water textures and selects attachment behaviour for one group.
    void bind(const renderer::Shader& shader, bool enabled) const;
    // Releases only the owned fallback descriptors, never the water simulation.
    void release();

private:
    unsigned m_buffer = 0, m_bodies = 0, m_map = 0;
    std::uint64_t m_frame = ~std::uint64_t{0};
    double m_seconds = -1;
    bool m_requested = false;
    mutable std::unordered_set<unsigned> m_bound_shaders;
};
}  // namespace openrac::enhanced
