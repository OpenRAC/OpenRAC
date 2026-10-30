// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Persistent GPU rigid water attachments, with valid fallback descriptors on dry levels.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace openrac::scene {
class LevelScene;
}

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
    void prepare(const renderer::FrameInput& input, const scene::LevelScene* scene = nullptr);
    // Binds shared water textures and selects attachment behaviour for one group.
    void bind(const renderer::Shader& shader, bool enabled) const;
    // Releases owned poses and fallback descriptors, never the water simulation.
    void release();

private:
    void bind_maps(const renderer::Shader& shader) const;
    renderer::Shader m_filter;
    unsigned m_anchor_buffer = 0;
    unsigned m_pose_buffers[2]{}, m_pose_textures[2]{};
    unsigned m_current = 0;
    int m_pose_count = 0;
    std::vector<std::pair<renderer::Vec3, std::uint64_t>> m_anchors, m_previous;
    std::unordered_map<std::uint64_t, std::size_t> m_previous_ids;
    std::vector<std::array<float, 4>> m_records;
    unsigned m_buffer = 0, m_bodies = 0, m_map = 0;
    std::uint64_t m_frame = ~std::uint64_t{0};
    double m_seconds = -1;
    bool m_requested = false;
    float m_momentum = -1;
    mutable std::unordered_set<unsigned> m_bound_shaders;
};
}  // namespace openrac::enhanced
