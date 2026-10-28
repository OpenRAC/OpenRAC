// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Diagnostic sphere compaction: immutable uploads and ordered device-written vertices/commands.

#include "enhanced/viewer/probe_sphere_culling.h"

#include <limits>

#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"

namespace openrac::lighting {
namespace {
using namespace openrac::gl;

// Dedicated slots follow the tiled selector's binding eight.
constexpr unsigned kSourceBinding = 9;
constexpr unsigned kOutputBinding = 10;
constexpr unsigned kCommandBinding = 11;
constexpr unsigned kWorkgroupSize = 128;
constexpr unsigned kCommandWords = 5;  // VkDrawIndexedIndirectCommand.
constexpr unsigned kRegionCapacity = 1024;
constexpr GLbitfield kDrawBarrier =
    GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT;
static_assert(sizeof(ProbeSphere) == 9 * sizeof(unsigned));
}  // namespace

ProbeSphereCulling::~ProbeSphereCulling() {
    release();
}

bool ProbeSphereCulling::prepare(std::span<const ProbeSphere> spheres, std::string& error) {
    GLint bindings = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);

    // The native indirect path and its three storage bindings are Vulkan-only.
    if (!enhanced_rendering_available() || bindings <= static_cast<GLint>(kCommandBinding)
        || spheres.empty()
        || spheres.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        error = "GPU sphere compaction is unavailable";
        return false;
    }

    if (!m_shader.build_compute("probe sphere compaction", shaders::probe_spheres_comp, error)) {
        return false;
    }

    glGenBuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
    const GLsizeiptr sizes[] =
        {static_cast<GLsizeiptr>(spheres.size_bytes()),
         static_cast<GLsizeiptr>(spheres.size_bytes()),
         static_cast<GLsizeiptr>((kCommandWords + kRegionCapacity) * sizeof(unsigned))};

    for (unsigned n = 0; n < m_buffers.size(); ++n) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[n]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, sizes[n], n == 0 ? spheres.data() : nullptr, GL_STATIC_DRAW
        );
    }

    m_count = static_cast<unsigned>(spheres.size());
    // A failed allocation must leave an actionable fallback message.
    if (glGetError() != GL_NO_ERROR) {
        error = "cannot allocate resident probe sphere buffers";
        return false;
    }

    return true;
}

void ProbeSphereCulling::dispatch(
    const renderer::Camera& camera,
    float range,
    const std::array<ProbeRegionSlot, kDenseProbeSlots>& slots
) {
    const unsigned bindings[] = {kSourceBinding, kOutputBinding, kCommandBinding};

    for (unsigned n = 0; n < m_buffers.size(); ++n) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[n], m_buffers[n]);
    }

    m_shader.use();
    const auto clip = renderer::multiply(camera.projection, camera.view);
    glUniformMatrix4fv(m_shader.uniform("world_to_clip"), 1, GL_FALSE, clip.data());
    glUniform3f(
        m_shader.uniform("camera_position"),
        camera.position[0],
        camera.position[1],
        camera.position[2]
    );
    glUniform1f(m_shader.uniform("draw_range"), range);
    glUniform1i(m_shader.uniform("source_count"), static_cast<int>(m_count));
    glUniform1i(m_shader.uniform("index_count"), static_cast<int>(kProbeSphereVertices));

    for (unsigned slot = 0; slot < slots.size(); ++slot) {
        const auto suffix = "[" + std::to_string(slot) + "]";
        glUniform1i(m_shader.uniform(("cpu_regions" + suffix).c_str()), slots[slot].region);
        glUniform1f(m_shader.uniform(("cpu_weights" + suffix).c_str()), slots[slot].weight);
    }

    // Previous vertex/argument reads finish before the next reset reuses the same resident buffers.
    glMemoryBarrier(kDrawBarrier);
    glUniform1i(m_shader.uniform("reset_command"), 1);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glUniform1i(m_shader.uniform("reset_command"), 0);
    glDispatchCompute((m_count + kWorkgroupSize - 1) / kWorkgroupSize, 1, 1);
    glMemoryBarrier(kDrawBarrier);
}

void ProbeSphereCulling::release() {
    m_shader.release();

    // A default or already released owner has no live GL names.
    if (m_buffers[0]) {
        glDeleteBuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
    }

    m_buffers = {};
    m_count = 0;
}

}  // namespace openrac::lighting
