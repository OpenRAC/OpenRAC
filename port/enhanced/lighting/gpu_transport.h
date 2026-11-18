// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe transport on a Vulkan ray-query device, with the CPU baker's paths and random sequences.
#pragma once
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "enhanced/lighting/bake_integration.h"
#include "enhanced/lighting/bake_scene.h"

namespace openrac::lighting::bake_detail {
// Paired crossing parity (classify) and short clearance rays for one position.
struct GpuPlacement {
    float nearest;               // nearest accepted crossing, infinity when none
    Float3 exit_direction;       // ray toward that crossing
    std::uint32_t inside_pairs;  // pairs whose forward and reverse rays both saw odd crossings
    float clearance;             // capped surface clearance, the cap when not asked for
};

// Uploads one prepared scene and traces probe rays; integration into texels stays on the CPU.
class GpuTransport {
    struct Impl;

    // Only create() can make one; the key keeps the constructor usable by std::make_unique.
    struct Key {
        explicit Key() = default;
    };

public:
    // Null when no Vulkan device has ray queries; the reason goes to the log.
    static std::unique_ptr<GpuTransport> create(const Scene& scene, const BakeSettings& settings);
    GpuTransport(Key, std::unique_ptr<Impl> impl);
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

    // classify() and, when cap > 0, the alignment's clearance for each position. At most 64 pairs.
    std::vector<GpuPlacement> place(
        std::span<const Float3> positions, std::span<const std::array<Float3, 2>> pairs, float cap
    );

    // The chosen adapter, for logs.
    const std::string& device_name() const;

private:
    std::unique_ptr<Impl> m_impl;
};
}  // namespace openrac::lighting::bake_detail
