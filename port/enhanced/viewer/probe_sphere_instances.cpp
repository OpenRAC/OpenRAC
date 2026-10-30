// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// ProbeLighting's diagnostic instance preparation: GPU compaction and CPU reference.

#include <algorithm>
#include <cstddef>

#include "common/log.h"
#include "enhanced/viewer/probe_lighting.h"
#include "renderer/gl.h"

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
    m_sphere_culling.dispatch(input.camera, range, m_selector.slots);
    return true;
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
