// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Resident diagnostic spheres and GPU-generated visible instances and draw arguments.

#pragma once

#include <span>

#include "enhanced/lighting/probe_regions.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"

namespace openrac::lighting {

// Matches the existing sphere tessellation, twelve longitude by eight latitude cells.
inline constexpr unsigned kProbeSphereVertices = 12 * 8 * 6;

// Packed nine-word record shared by vertex attributes and compute storage.
struct ProbeSphere {
    std::array<float, 4> position_radius;  // World-space centre and radius.
    std::array<float, 3> colour;           // Placement status RGB.
    int region = -1;                       // Dense-region index, or -1 for the sparse grid.
    float opacity = 1;                     // CPU selector fade; one with GPU region selection.
};

// Owns resident buffers on the render thread; context must outlive the owner.
class ProbeSphereCulling {
public:
    // Releases buffers and shader while the owning context is still current.
    ~ProbeSphereCulling();
    ProbeSphereCulling() = default;
    ProbeSphereCulling(const ProbeSphereCulling&) = delete;
    ProbeSphereCulling& operator=(const ProbeSphereCulling&) = delete;
    // Uploads source records once; false with an error when compute is unavailable or allocation fails.
    [[nodiscard]] bool prepare(std::span<const ProbeSphere> spheres, std::string& error);
    // Compacts by range, frustum and selected region without CPU readback; selector uniforms are bound first.
    void dispatch(
        const renderer::Camera& camera,
        float range,
        const std::array<ProbeRegionSlot, kDenseProbeSlots>& slots
    );

    // Returns the program whose selection uniforms must be bound before dispatch.
    renderer::Shader& shader() { return m_shader; }

    // Returns the compacted instance buffer, valid until release.
    unsigned output() const { return m_buffers[1]; }

    // Returns the storage buffer beginning with the generated indexed draw command.
    unsigned arguments() const { return m_buffers[2]; }

    // Releases resources; may be called repeatedly before the context closes.
    void release();

private:
    renderer::Shader m_shader;  // Compute reset and compaction program.
    std::array<unsigned, 3>
        m_buffers{};       // Source, compacted vertices, indirect arguments/region lookup.
    unsigned m_count = 0;  // Resident source count, unchanged between frames.
};

}  // namespace openrac::lighting
