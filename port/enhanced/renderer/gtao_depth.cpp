// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GTAO depth hierarchy allocation and dispatch; synchronisation belongs to the pass graph or reference path.
#include "enhanced/renderer/gtao_depth.h"

#include <utility>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl/gl.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"

namespace openrac::renderer {
namespace {
// A 96-pixel maximum horizon and distance/8 mip selection address at most four half-size levels.
constexpr unsigned kMaxLevels = 4;
// Two FP32 values per cell, matching std430 vec2 and RG32F texel-buffer storage.
constexpr unsigned kCellBytes = 8;
// The depth reducer and sampler use separate bindings from GTAO's horizon output at binding 11.
constexpr unsigned kDepthBinding = 12;
constexpr unsigned kDepthUnit = 3;
// Reduction kernels use eight-by-eight workgroups, matching gtao_depth.comp.
constexpr int kWorkgroupSize = 8;

// Plans only the depth levels addressable by GTAO's maximum horizon footprint.
std::vector<GtaoDepthLevel> depth_levels(int width, int height) {
    std::vector<GtaoDepthLevel> levels;
    std::uint64_t bytes = 0;

    // Ceil division keeps odd viewport edges present at every level.
    for (unsigned level = 0; level < kMaxLevels; ++level) {
        width = width / 2 + width % 2;
        height = height / 2 + height % 2;
        const std::uint64_t size =
            static_cast<std::uint64_t>(width) * static_cast<unsigned>(height) * kCellBytes;
        levels.push_back({width, height, bytes, size});
        bytes += size;

        // A one-cell level already bounds the complete input; further reductions would duplicate it.
        if (width == 1 && height == 1) {
            break;
        }
    }

    return levels;
}
}  // namespace

GtaoDepth::~GtaoDepth() {
    release();
}

bool GtaoDepth::prepare(int width, int height) {
    using namespace gl;

    // The hierarchy is optional and never changes the reference path's allocations.
    if (m_failed || width < 1 || height < 1) {
        return false;
    }

    std::string error;

    // Compile once; changing viewport size only replaces storage.
    if (!m_program.id()
        && !m_program.build_compute("GTAO depth bounds", shaders::gtao_depth_comp, error)) {
        log::error("GTAO depth: {}", error);
        m_failed = true;
        return false;
    }

    // Reusing the allocation avoids per-frame device memory churn.
    if (m_width == width && m_height == height) {
        return true;
    }

    auto levels = depth_levels(width, height);
    const std::uint64_t bytes = levels.back().offset + levels.back().bytes;
    GLint limit = 0;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);

    // Texel indices must fit both the device view limit and the shader's signed offsets.
    if (limit <= 0 || bytes / kCellBytes > static_cast<std::uint64_t>(limit)) {
        return false;
    }

    // Names remain stable when a viewport resize replaces their backing allocation.
    if (!m_buffer) {
        glGenBuffers(1, &m_buffer);
        glGenTextures(1, &m_texture);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(bytes), nullptr, GL_DYNAMIC_DRAW
    );
    glActiveTexture(GL_TEXTURE0 + kDepthUnit);
    glBindTexture(world::gl::GL_TEXTURE_BUFFER, m_texture);
    glTexBuffer(world::gl::GL_TEXTURE_BUFFER, GL_RG32F, m_buffer);
    m_width = width;
    m_height = height;
    m_levels = std::move(levels);
    return true;
}

void GtaoDepth::build_level(unsigned depth, unsigned level) {
    using namespace gl;
    const stats::Scope timing("GTAO depth mip", stats::kGpu);
    const auto& destination = m_levels.at(level);
    const int width = level ? m_levels[level - 1].width : m_width;
    const int height = level ? m_levels[level - 1].height : m_height;
    m_program.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kDepthBinding, m_buffer);
    glUniform1i(m_program.uniform("first_level"), level == 0);
    glUniform1i(m_program.uniform("source_width"), width);
    glUniform1i(m_program.uniform("source_height"), height);
    glUniform1i(
        m_program.uniform("source_offset"),
        level ? static_cast<int>(m_levels[level - 1].offset / kCellBytes) : 0
    );
    glUniform1i(
        m_program.uniform("destination_offset"), static_cast<int>(destination.offset / kCellBytes)
    );
    glDispatchCompute(
        static_cast<unsigned>((destination.width + kWorkgroupSize - 1) / kWorkgroupSize),
        static_cast<unsigned>((destination.height + kWorkgroupSize - 1) / kWorkgroupSize),
        1
    );
}

void GtaoDepth::bind(const Shader& evaluate, bool enabled) const {
    using namespace gl;
    glUniform1i(evaluate.uniform("hi_z_levels"), enabled ? static_cast<int>(sampled_levels().size()) : 0);
    glUniform1i(evaluate.uniform("hi_z_depth"), kDepthUnit);
    glActiveTexture(GL_TEXTURE0 + kDepthUnit);
    glBindTexture(world::gl::GL_TEXTURE_BUFFER, enabled ? m_texture : 0);
    glActiveTexture(GL_TEXTURE0);
}

std::span<const GtaoDepthLevel> GtaoDepth::sampled_levels() const {
    // An in-bounds tap cannot be farther than the viewport diagonal. Pad each axis for rounding
    // the tap to a pixel. The horizon shader first reaches level n at a distance of 8 * 2^n.
    const auto x = static_cast<std::uint64_t>(m_width) + 1;
    const auto y = static_cast<std::uint64_t>(m_height) + 1;
    const auto diagonal_squared = x * x + y * y;
    std::size_t count = m_levels.size();
    while (count > 1) {
        const auto distance = std::uint64_t{8} << (count - 1);
        if (distance * distance <= diagonal_squared) {
            break;
        }
        --count;
    }
    return levels().first(count);
}

void GtaoDepth::release() {
    using namespace gl;

    // The view is created together with its storage owner.
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        glDeleteBuffers(1, &m_buffer);
    }

    m_texture = 0;
    m_buffer = 0;
    m_width = 0;
    m_height = 0;
    m_levels.clear();
    m_failed = false;
    m_program.release();
}
}  // namespace openrac::renderer
