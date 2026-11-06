// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe transport on a Vulkan ray-query device, with the CPU baker's paths and random sequences.
#pragma once
#include <memory>
#include <span>
#include <string>

#include "enhanced/lighting/bake_integration.h"
#include "enhanced/lighting/bake_scene.h"

namespace openrac::lighting::bake_detail {
// Uploads one prepared scene and traces probe rays; integration into texels stays on the CPU.
class GpuTransport {
public:
    // Null when no Vulkan device has ray queries; the reason goes to the log.
    static std::unique_ptr<GpuTransport> create(const Scene& scene, const BakeSettings& settings);
    ~GpuTransport();
    GpuTransport(const GpuTransport&) = delete;
    GpuTransport& operator=(const GpuTransport&) = delete;

    // Origin i is volume probe first + i (its seed). rays holds rays_per_probe entries per origin;
    // emission the same, or nothing when the scene has no emitters. Failures throw.
    void sample(
        std::span<const Float3> origins,
        std::uint32_t first,
        float max_distance,
        std::span<RaySample> rays,
        std::span<EmissionSample> emission
    );

    // The chosen adapter, for logs.
    const std::string& device_name() const;

private:
    struct Impl;
    explicit GpuTransport(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};
}  // namespace openrac::lighting::bake_detail
