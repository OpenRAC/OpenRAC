// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// ProbeLighting's diagnostic instance preparation: GPU compaction and CPU reference.

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include "common/log.h"
#include "enhanced/viewer/probe_lighting.h"
#include "renderer/gl/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::lighting {
using namespace openrac::gl;

namespace {
// Encodes a GPU byte offset in GL's pointer-shaped attribute argument, never a host address.
const void* attribute_offset(std::size_t offset) {
    // NOLINTNEXTLINE(performance-no-int-to-ptr): GL specifies buffer offsets through pointer parameters.
    return reinterpret_cast<const void*>(offset);
}
}  // namespace

bool ProbeLighting::prepare_gpu_spheres(const renderer::FrameInput& input, float range) {
    // The reference remains usable for comparisons and failed compute preparation.
    if (!gpu_spheres || all_volume_bounds) {
        return false;
    }

    if (!m_sphere_gpu_attempted) {
        m_sphere_gpu_attempted = true;
        std::string error;

        if (!m_sphere_culling.prepare(m_spheres, error)) {
            log::warn("probe spheres use CPU filtering: {}", error);
            m_sphere_culling.release();
        }
    }

    if (!m_sphere_culling.shader().id()) {
        return false;
    }

    m_sphere_culling.shader().use();
    m_selection.bind(m_sphere_culling.shader(), &input.camera);
    m_sphere_culling.dispatch(input.camera, range, m_selector.slots, m_selection.active());
    return true;
}

bool ProbeLighting::draw_gpu_spheres(renderer::RenderState& state) const {
#if defined(OPENRAC_VULKAN)
    namespace graph = renderer::graph;
    vkgl::GraphExecution execution;
    auto pass = execution.make_pass("probe sphere draw");
    const auto inputs = execution.capture_graphics_reads(pass);
    if (!inputs) throw std::runtime_error(inputs.error().message);
    const std::array buffers{
        execution.read_buffer("probe sphere vertices", m_sphere_culling.output(), graph::Stage::VertexInput),
        execution.read_buffer("probe sphere indices", m_sphere_indices, graph::Stage::VertexInput),
        execution.read_buffer("probe sphere arguments", m_sphere_culling.arguments(), graph::Stage::DrawIndirect),
    };
    for (const auto& buffer : buffers) {
        if (!buffer) throw std::runtime_error(buffer.error().message);
        pass.buffers.push_back(*buffer);
    }
    const auto colour = execution.import_attachment("probe sphere colour", state.target.framebuffer, graph::Aspect::Colour);
    const auto depth = execution.import_attachment("probe sphere depth", state.target.framebuffer, graph::Aspect::Depth);
    if (!colour || !depth) throw std::runtime_error("cannot import probe sphere draw attachments");
    pass.images.push_back({*colour, {graph::Aspect::Colour},
        {graph::Stage::ColourOutput, graph::Access::ReadWrite, graph::Layout::ColourAttachment}, graph::Load::Preserve});
    pass.images.push_back({*depth, {graph::Aspect::Depth},
        {graph::Stage::DepthTests, graph::Access::Read, graph::Layout::DepthAttachment}, graph::Load::Preserve});
    execution.add_pass(std::move(pass));
    bool drawn = false;
    const auto result = execution.run([&](unsigned) {
        drawn = vkgl::draw_indexed_indirect(m_sphere_culling.arguments(), kProbeSphereVertices);
    });
    if (!result) throw std::runtime_error(result.error().message);
    return drawn;
#else
    (void)state;
    return false;
#endif
}

void ProbeLighting::prepare_cpu_spheres(const renderer::FrameInput& input, float range) {
    m_visible_spheres.clear();

    for (auto sphere : m_spheres) {
        // CPU-selected dense regions retain their temporal fade.
        if (!all_volume_bounds && sphere.region >= 0 && !m_selection.active()) {
            const auto selected = std::find_if(
                m_selector.slots.begin(), m_selector.slots.end(), [&](const auto& slot) {
                    return slot.region == sphere.region && slot.weight > 0;
                }
            );

            if (selected == m_selector.slots.end()) {
                continue;
            }

            sphere.opacity = selected->weight;
        }

        if (all_volume_bounds) {
            sphere.opacity = 1;
        }

        float squared = 0;

        for (unsigned axis = 0; axis < 3; ++axis) {
            const float delta = sphere.position_radius[axis] - input.camera.position[axis];
            squared += delta * delta;
        }

        if (squared <= range * range) {
            m_visible_spheres.push_back(sphere);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_sphere_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_visible_spheres.size() * sizeof(Sphere)),
        m_visible_spheres.data(),
        GL_STREAM_DRAW
    );
}

void ProbeLighting::bind_sphere_attributes(unsigned buffer) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Sphere), nullptr);
    glVertexAttribDivisor(0, 1);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Sphere), attribute_offset(offsetof(Sphere, colour))
    );
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 1, GL_FLOAT, GL_FALSE, sizeof(Sphere), attribute_offset(offsetof(Sphere, opacity))
    );
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(
        3, 1, GL_INT, sizeof(Sphere), attribute_offset(offsetof(Sphere, region))
    );
    glVertexAttribDivisor(3, 1);
}

}  // namespace openrac::lighting
