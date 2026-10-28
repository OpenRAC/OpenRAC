// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Interior source preparation: two-sided ceiling/clearance queries and hue-preserving bounds.

#include "enhanced/lighting/interior_lights.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <optional>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/bake_transport.h"

namespace openrac::lighting {
namespace {
using bake_detail::add;
using bake_detail::cross;
using bake_detail::dot;
using bake_detail::finite;
using bake_detail::mul;
using bake_detail::unit;

// Query margins and enclosure thresholds are artistic placement proxies, not a navigation model.
constexpr float kBias = .01f;
constexpr float kFloorOffset = 1;
constexpr float kMinimumHeight = 3;
constexpr float kCeilingMargin = .75f;
constexpr float kHorizontalNormal = .65f;
constexpr float kWallDistance = 24;
constexpr unsigned kWallDirections = 8;
constexpr unsigned kMinimumWalls = 5;
// Two elevation bands reject low fences, while upper-hemisphere rays detect open courtyards.
constexpr float kUpperWallFraction = .5f;
constexpr unsigned kSkyRings = 4;
constexpr unsigned kMaximumSkyEscapes = 2;
constexpr float kSkyDistance = 128;
constexpr unsigned kCeilingSamplesPerAxis = 3;  // Centre and neighbouring ceiling samples.
constexpr float kFloorBand = 4;
constexpr float kEmitterReach = 8;
// A fixed estimate avoids fitting the new sources to a camera or changing the transport sequence.
constexpr unsigned kReferenceSamples = 128;

// Rejects nonfinite settings and bounds excessive source counts before constructing acceleration.
void validate_inputs(
    std::span<const InteriorLightSeed> seeds, const InteriorLightSettings& settings
) {
    const float positive[] =
        {settings.emission_limit,
         settings.point_limit,
         settings.ceiling_limit,
         settings.half_extent,
         settings.separation,
         settings.max_height};

    // Positive lengths and energy limits keep placement and falloff defined.
    for (const float value : positive) {
        if (!std::isfinite(value) || value <= 0) {
            throw std::runtime_error("invalid interior light limit");
        }
    }

    // Boosts never dim existing sources, and the experiment remains bounded in size.
    if (!std::isfinite(settings.emitter_boost) || settings.emitter_boost < 1
        || settings.emitter_boost > 4 || !std::isfinite(settings.ceiling_strength)
        || settings.ceiling_strength < 0 || settings.ceiling_strength > 1
        || settings.max_height <= kMinimumHeight
        || settings.max_lights > kMaximumInteriorLightsPerLevel
        || settings.max_per_region > kMaximumInteriorLightsPerRegion || seeds.size() > 100'000) {
        throw std::runtime_error("invalid interior light settings");
    }

    // Source positions and colour references come from untrusted sidecar data.
    for (const auto& seed : seeds) {
        if (!finite(seed.position) || !finite(seed.colour)
            || *std::min_element(seed.colour.begin(), seed.colour.end()) < 0) {
            throw std::runtime_error("invalid interior light seed");
        }
    }
}

// Requires surrounding geometry at a given height, independent of authored normals or winding.
bool surrounded(const bake_detail::Scene& acceleration, Float3 receiver) {
    unsigned walls = 0;

    // Horizontal tests depend only on geometry, never authored normals or winding.
    for (unsigned n = 0; n < kWallDirections; ++n) {
        const float angle = 2 * bake_detail::kPi * static_cast<float>(n) / kWallDirections;
        walls += acceleration.occluded(
            receiver, {std::cos(angle), std::sin(angle), 0}, kBias, kWallDistance
        );
    }

    return walls >= kMinimumWalls;
}

// A few blocked overhead directions alone cannot turn an outdoor overhang into a room.
bool enclosed(const bake_detail::Scene& acceleration, Float3 receiver, float ceiling_height) {
    // Low walls must not count as room enclosure above the player's head.
    if (!surrounded(acceleration, receiver)
        || !surrounded(acceleration, add(receiver, {0, 0, ceiling_height * kUpperWallFraction}))) {
        return false;
    }

    unsigned escapes = 0;

    // Uniform solid-angle elevation bands include low-angle openings beneath broad roofs.
    for (unsigned ring = 0; ring < kSkyRings; ++ring) {
        const float z = (static_cast<float>(ring) + .5f) / kSkyRings;
        const float radius = std::sqrt(1 - z * z);

        for (unsigned n = 0; n < kWallDirections; ++n) {
            const float angle = 2 * bake_detail::kPi
                                * (static_cast<float>(n) + .5f * static_cast<float>(ring))
                                / kWallDirections;
            escapes += !acceleration.occluded(
                receiver,
                {radius * std::cos(angle), radius * std::sin(angle), z},
                kBias,
                kSkyDistance
            );

            // More than two open directions out of 32 is insufficient evidence of an enclosed room.
            if (escapes > kMaximumSkyEscapes) {
                return false;
            }
        }
    }

    return true;
}

// Returns a downward geometric normal for a ceiling hit, independent of mesh winding.
Float3 ceiling_normal(const RTCRayHit& hit) {
    const auto normal = unit({hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z});
    // Ceiling sources emit into the room below, even when triangle winding points upward.
    return mul(normal, normal[2] > 0 ? -1.f : 1.f);
}

// Averages a local ceiling footprint, excluding steep faces and hits on unrelated storeys.
Float3 average_ceiling_normal(
    const bake_detail::Scene& acceleration,
    Float3 receiver,
    const RTCRayHit& centre,
    const InteriorLightSettings& settings
) {
    Float3 sum = ceiling_normal(centre);

    // A regular footprint approximates the ceiling surface covered by the source rectangle.
    for (unsigned y = 0; y < kCeilingSamplesPerAxis; ++y) {
        for (unsigned x = 0; x < kCeilingSamplesPerAxis; ++x) {
            // The central hit already contributes once.
            if (x == 1 && y == 1) {
                continue;
            }

            const Float3 offset{
                (static_cast<float>(x) - 1) * settings.half_extent,
                (static_cast<float>(y) - 1) * settings.half_extent,
                0
            };
            const auto hit =
                acceleration.trace(add(receiver, offset), {0, 0, 1}, kBias, settings.max_height);

            // Nearby samples must describe the same approximate roof height as the central hit.
            if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID
                || std::abs(hit.ray.tfar - centre.ray.tfar) > 2 * settings.half_extent) {
                continue;
            }

            const auto normal = ceiling_normal(hit);

            // Walls or sharply inclined details must not pull the source sideways.
            if (-normal[2] >= kHorizontalNormal) {
                sum = add(sum, normal);
            }
        }
    }

    return unit(sum);
}

// Returns a rectangle aligned beneath the local ceiling, or no source when clearance is uncertain.
std::optional<BakeAreaEmitter> ceiling_light(
    const bake_detail::Scene& acceleration,
    const InteriorLightSeed& seed,
    const InteriorLightSettings& settings
) {
    const auto receiver = add(seed.position, {0, 0, kFloorOffset});
    const auto hit = acceleration.trace(receiver, {0, 0, 1}, kBias, settings.max_height);

    // Low obstacles and steep surfaces are not plausible ceiling patches.
    if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID || hit.ray.tfar < kMinimumHeight
        || std::abs(unit({hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z})[2]) < kHorizontalNormal
        || !enclosed(acceleration, receiver, hit.ray.tfar)) {
        return std::nullopt;
    }

    const auto normal = average_ceiling_normal(acceleration, receiver, hit, settings);
    const auto tangent = unit(cross(normal, {0, 1, 0}));
    BakeAreaEmitter light;
    light.position = add(add(receiver, {0, 0, hit.ray.tfar}), mul(normal, kCeilingMargin));
    light.axis_u = mul(tangent, settings.half_extent);
    light.axis_v = mul(cross(normal, tangent), settings.half_extent);

    // All rectangle corners must stay in the same visible free space as the supported receiver.
    for (const float u : {-1.f, 1.f}) {
        for (const float v : {-1.f, 1.f}) {
            const auto corner =
                add(light.position, add(mul(light.axis_u, u), mul(light.axis_v, v)));
            const auto delta = add(corner, mul(receiver, -1));
            const float distance = std::sqrt(dot(delta, delta));

            if (acceleration.occluded(receiver, unit(delta), kBias, distance + kBias)) {
                return std::nullopt;
            }
        }
    }

    const float height = light.position[2] - receiver[2];
    const float area = 4 * settings.half_extent * settings.half_extent;
    // A white diffuse receiver reflects E/pi; convert the authored radiance target to irradiance.
    light.radiance =
        mul(seed.colour,
            std::numbers::pi_v<float> * settings.ceiling_strength * height * height / area);
    return light;
}

// Tests spacing without merging lights across unrelated storeys.
bool separated(Float3 position, const InteriorLightReport& report, float distance) {
    return std::none_of(
        report.lights.begin(), report.lights.end(), [&](const BakeAreaEmitter& light) {
            const float x = position[0] - light.position[0];
            const float y = position[1] - light.position[1];
            return std::abs(position[2] - light.position[2]) < kFloorBand
                   && x * x + y * y < distance * distance;
        }
    );
}

// Applies one scalar bound, preserving RGB ratios even when one channel reaches the limit.
bool boost_colour(Float3& colour, float requested, float limit, InteriorLightReport& report) {
    const float peak = *std::max_element(colour.begin(), colour.end());

    // Zero emission is not evidence of an authored source.
    if (peak <= 0) {
        return false;
    }

    const float gain = std::max(1.f, std::min(requested, limit / peak));
    report.capped += gain < requested;
    colour = mul(colour, gain);
    return gain > 1;
}

// Requires a nearby labelled receiver with an unobstructed path to the existing source.
bool near_interior(
    const bake_detail::Scene& acceleration,
    Float3 position,
    std::span<const InteriorLightSeed> seeds
) {
    return std::any_of(seeds.begin(), seeds.end(), [&](const InteriorLightSeed& seed) {
        const auto receiver = add(seed.position, {0, 0, kFloorOffset});
        const auto delta = add(position, mul(receiver, -1));
        const float distance = std::sqrt(dot(delta, delta));
        // A finite segment avoids boosting emitters behind walls or on another storey.
        return distance > kBias && distance < kEmitterReach
               && !acceleration.occluded(receiver, unit(delta), kBias, distance - kBias);
    });
}

// Boosts only existing sources visible from accepted interior samples; source colours retain their hue.
void boost_existing(
    BakeScene& scene,
    const bake_detail::Scene& acceleration,
    std::span<const InteriorLightSeed> seeds,
    const InteriorLightSettings& settings,
    InteriorLightReport& report
) {
    // Surface emission retains its authored texture and hue.
    for (auto& triangle : scene.triangles) {
        const auto centre =
            mul(add(add(triangle.vertices[0], triangle.vertices[1]), triangle.vertices[2]),
                1.f / 3);

        // Explicit bake settings must retain their configured radiance.
        if (!triangle.fixed_emission
            && *std::max_element(triangle.emission.begin(), triangle.emission.end()) > 0
            && near_interior(acceleration, centre, seeds)) {
            report.boosted_surfaces += boost_colour(
                triangle.emission, settings.emitter_boost, settings.emission_limit, report
            );
        }
    }

    // Finite authored lights use a separate intensity limit because their units differ.
    for (auto& light : scene.point_emitters) {
        if (near_interior(acceleration, light.position, seeds)) {
            report.boosted_points +=
                boost_colour(light.intensity, settings.emitter_boost, settings.point_limit, report);
        }
    }
}

// Generates a bounded source list while acceleration still refers to the unchanged geometry.
InteriorLightReport generate_lights(
    BakeScene& scene,
    std::span<const InteriorLightSeed> seeds,
    const InteriorLightSettings& settings
) {
    bake_detail::Scene acceleration(scene);
    InteriorLightReport report;
    std::map<unsigned, unsigned> counts;
    std::vector<InteriorLightSeed> accepted;

    // Stable input order makes source selection reproducible across runs and worker counts.
    for (const auto& seed : seeds) {
        if (report.lights.size() >= settings.max_lights
            || counts[seed.region] >= settings.max_per_region) {
            ++report.budget_skipped;
            continue;
        }

        auto light = ceiling_light(acceleration, seed, settings);

        if (!light) {
            ++report.rejected;
            continue;
        }

        // Nearby samples support one broad light instead of accumulating overlapping fill.
        if (!separated(light->position, report, settings.separation)) {
            continue;
        }

        accepted.push_back(seed);
        report.lights.push_back(*light);
        report.receivers.push_back(add(seed.position, {0, 0, kFloorOffset}));
        // Existing emitter estimates and generated targets must both use irradiance units.
        report.targets.push_back(
            mul(seed.colour, std::numbers::pi_v<float> * settings.ceiling_strength)
        );
        ++counts[seed.region];
    }

    boost_existing(scene, acceleration, accepted, settings, report);
    return report;
}

// Estimates visible existing sources after their boosts, then adds only the remaining approximate fill.
void reduce_existing_contribution(
    const BakeScene& scene, const InteriorLightSettings& settings, InteriorLightReport& report
) {
    const bake_detail::Scene acceleration(scene);

    // A scalar residual retains the authored reference hue rather than forcing an RGB match.
    for (std::size_t n = 0; n < report.lights.size(); ++n) {
        Float3 existing{};
        bake_detail::Random random{1};

        // These samples are independent of the probe transport random streams.
        for (unsigned sample = 0; sample < kReferenceSamples; ++sample) {
            if (acceleration.emitters.empty()) {
                break;
            }

            const auto light =
                bake_detail::sample_emitter(acceleration, report.receivers[n], kBias, random);
            existing =
                add(existing,
                    mul(light.radiance_weight,
                        std::max(0.f, light.direction[2]) / kReferenceSamples));
        }

        report.existing.push_back(existing);
        const auto& target = report.targets[n];
        const float target_sum = target[0] + target[1] + target[2];
        const float existing_sum = existing[0] + existing[1] + existing[2];
        // A dark reference needs no generated fill; excess existing light is never subtracted.
        const float residual =
            target_sum > 0 ? std::clamp(1 - existing_sum / target_sum, 0.f, 1.f) : 0;
        auto& light = report.lights[n];
        light.radiance = mul(light.radiance, residual);
        const float peak = *std::max_element(light.radiance.begin(), light.radiance.end());

        // One scalar cap preserves chromaticity when the requested source is too bright.
        if (peak > settings.ceiling_limit) {
            light.radiance = mul(light.radiance, settings.ceiling_limit / peak);
            ++report.capped;
        }
    }
}
}  // namespace

Result<InteriorLightReport> prepare_interior_lighting(
    BakeScene& scene,
    std::span<const InteriorLightSeed> seeds,
    const InteriorLightSettings& settings
) {
    // Validation errors cross the module boundary as a result, rather than an exception.
    try {
        validate_inputs(seeds, settings);
        bake_detail::validate_bake_scene(scene, BakeSettings{});
        auto report = generate_lights(scene, seeds, settings);
        reduce_existing_contribution(scene, settings, report);
        scene.area_emitters
            .insert(scene.area_emitters.end(), report.lights.begin(), report.lights.end());
        return report;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

}  // namespace openrac::lighting
