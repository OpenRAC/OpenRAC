// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded offline lighting sources inferred from labelled interior floor samples.

#pragma once

#include <span>

#include "common/result.h"
#include "enhanced/lighting/bake.h"

namespace openrac::lighting {

// Maximum ceiling source budget accepted for one connected interior region.
inline constexpr unsigned kMaximumInteriorLightsPerRegion = 128;

// Maximum ceiling source budget accepted across one level.
inline constexpr unsigned kMaximumInteriorLightsPerLevel = 1024;

// A supported floor position carrying an approximate linear lighting mood.
struct InteriorLightSeed {
    Float3 position{};    // Walkable floor position, with Z as height.
    Float3 colour{};      // Nonnegative linear brightness and colour reference.
    unsigned region = 0;  // Connected component, shared only by agreeing transition labels.
};

// Conservative artistic controls; limits apply per source rather than per colour channel.
struct InteriorLightSettings {
    float emitter_boost = 2;   // Maximum multiplier for existing emission, preserving hue.
    float emission_limit = 4;  // Maximum boosted surface radiance; brighter originals stay intact.
    float point_limit = 64;    // Maximum boosted point radiant intensity.
    float ceiling_limit = 12;  // Maximum generated linear radiance, preserving the target hue.
    float ceiling_strength = 1;  // Fraction of the approximate mood reference supplied by ceilings.
    float half_extent = 1.5f;    // Rectangle half-edge length, in world units.
    float separation = 6;        // Minimum source spacing within one floor-height band.
    float max_height = 32;       // Maximum floor-to-ceiling search distance.
    unsigned max_lights = kMaximumInteriorLightsPerLevel;  // Total generated source limit.
    unsigned max_per_region =
        kMaximumInteriorLightsPerRegion;  // Generated source limit for one connected region.
};

// Source diagnostics owned by the caller; no generated geometry is added to the scene.
struct InteriorLightReport {
    std::vector<BakeAreaEmitter> lights;  // Accepted sources, in deterministic input order.
    std::vector<Float3> receivers;        // Floor reference positions corresponding to each source.
    std::vector<Float3> targets;   // Approximate desired floor irradiance, before existing sources.
    std::vector<Float3> existing;  // Estimated direct irradiance after existing-emitter boosts.
    std::size_t boosted_surfaces = 0;  // Existing emissive triangles changed.
    std::size_t boosted_points = 0;    // Existing finite point sources changed.
    std::size_t capped = 0;            // Sources whose requested brightness exceeded a limit.
    std::size_t rejected = 0;          // Seeds lacking enclosure, ceiling or rectangle clearance.
    std::size_t budget_skipped = 0;    // Seeds outside source count budgets.
};

// Adds ceiling sources and bounded existing-emitter boosts, reporting invalid inputs as an error.
// Geometry and authored colours remain unchanged. The scene must have no active bake workers.
[[nodiscard]] Result<InteriorLightReport> prepare_interior_lighting(
    BakeScene& scene,
    std::span<const InteriorLightSeed> seeds,
    const InteriorLightSettings& settings = {}
);

}  // namespace openrac::lighting
