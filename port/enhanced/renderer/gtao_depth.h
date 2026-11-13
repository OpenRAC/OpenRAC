// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Optional min/max depth hierarchy for GTAO; each level occupies a disjoint storage-buffer range.
#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "renderer/gl/shader.h"

namespace openrac::renderer {
// One ceil-divided hierarchy level; min/max FP32 depths retain empty pixels and discontinuities.
struct GtaoDepthLevel {
    int width = 0;             // Cells across this level.
    int height = 0;            // Cells down this level.
    std::uint64_t offset = 0;  // First byte in the shared allocation.
    std::uint64_t bytes = 0;   // Byte extent, including both depth bounds per cell.
};

// Owns the optional GPU hierarchy; used and released on the context's owning thread.
class GtaoDepth {
public:
    GtaoDepth() = default;
    ~GtaoDepth();
    GtaoDepth(const GtaoDepth&) = delete;
    GtaoDepth& operator=(const GtaoDepth&) = delete;
    GtaoDepth(GtaoDepth&&) = delete;
    GtaoDepth& operator=(GtaoDepth&&) = delete;
    // Allocates only when size changes; reports unsupported sizes or shader errors without enabling Hi-Z.
    [[nodiscard]] bool prepare(int width, int height);
    // Dispatches one declared level; the caller supplies ordering between dependent levels.
    void build_level(unsigned depth, unsigned level);
    // Binds the completed hierarchy and its level count to a GTAO evaluator, or disables sampling.
    void bind(const Shader& evaluate, bool enabled) const;
    // Releases all owned GL resources while the context exists.
    void release();

    // Returns borrowed storage used to declare exact graph dependencies.
    unsigned buffer() const { return m_buffer; }

    std::span<const GtaoDepthLevel> levels() const { return m_levels; }

    // Prefix the horizon shader can reach at this viewport size.
    std::span<const GtaoDepthLevel> sampled_levels() const;

private:
    bool m_failed = false;   // Suppresses retries after an unsupported kernel fails to compile.
    Shader m_program;        // Min/max reduction kernel.
    unsigned m_buffer = 0;   // Stable GPU storage for all levels.
    unsigned m_texture = 0;  // RG32F texture-buffer view of that storage.
    int m_width = 0;         // Original depth width.
    int m_height = 0;        // Original depth height.
    std::vector<GtaoDepthLevel> m_levels;  // Levels from half resolution through the bounded tail.
};
}  // namespace openrac::renderer
