// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Alpha-only material queries shared by traversal and emitter sampling.
#pragma once
#include <span>

#include "enhanced/lighting/bake.h"

namespace openrac::lighting::bake_detail {
// Shared by sequential stages of one bake. The borrowed scene's textures stay immutable.
class EmissionAverageCache {
public:
    explicit EmissionAverageCache(const BakeScene& scene);
    ~EmissionAverageCache();
    EmissionAverageCache(const EmissionAverageCache&) = delete;
    EmissionAverageCache& operator=(const EmissionAverageCache&) = delete;

private:
    friend struct Scene;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Match diffuse bilinear coverage without evaluating colour or emission.
float material_opacity(const BakeScene& scene, const BakeTriangle& triangle, float u, float v);

struct EmissionIntegral {
    Float3 average{};
    double coverage = 0;
};

// Integrates linear radiance and the fraction of the triangle admitted by its emission mask.
EmissionIntegral integrate_bake_emission(
    const BakeScene& scene, const BakeTriangle& triangle, bool known_opaque = false
);

struct EmissionIntegralRequest {
    std::size_t triangle;
    bool known_opaque;
};

std::vector<EmissionIntegral> integrate_mesh_emission(
    const BakeScene& scene, std::span<const EmissionIntegralRequest> requests
);
// Area average of emitted linear RGB over the accepted emitting surface.
std::optional<Float3> average_bake_emission(
    const BakeScene& scene, const BakeTriangle& triangle, bool known_opaque = false
);
}  // namespace openrac::lighting::bake_detail
