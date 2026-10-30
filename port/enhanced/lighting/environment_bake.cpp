// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Embree captures shade visible surfaces from the completed runtime-reference GI field.
#include "enhanced/lighting/environment_bake.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cfenv>
#include <memory>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"
#include "enhanced/lighting/environment_filter.h"
#include "enhanced/lighting/environment_gi.h"
#include "enhanced/lighting/environment_placement.h"
#include "enhanced/lighting/probe_regions.h"

namespace openrac::lighting {
namespace {
using namespace bake_detail;

// Caps an explicit stage limit by the borrowed pool's budget.
unsigned worker_budget(const BakeExecution& execution) {
    // Without a shared pool, zero retains hardware concurrency selection.
    if (!execution.pool) {
        return execution.threads;
    }

    // Zero requests the full existing pool rather than another hardware-sized set.
    if (execution.threads == 0) {
        return execution.pool->size();
    }

    return std::min(execution.threads, execution.pool->size());
}

// Immutable inputs borrowed for the lifetime of the capture workers.
struct Capture {
    const Scene& scene;
    const BakeSettings& settings;
    const CaptureGi& gi;
    const EnvironmentBakeSettings& options;
};

// Visible geometry receives runtime GI and shadowed sun; emission retains its visible intensity.
Float3 capture(const Capture& context, Float3 origin, Float3 direction) {
    const auto& [scene, settings, gi, options] = context;
    const auto hit =
        scene.trace(origin, direction, settings.ray_bias, std::numeric_limits<float>::infinity());

    if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        auto sky = settings;
        sky.sky_intensity = 1;
        sky.sky_weight = 1;
        return sample_bake_environment(scene.input, direction, sky);
    }

    auto n = unit({hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z});

    if (dot(n, direction) > 0) {
        n = mul(n, -1);
    }

    const auto p = add(origin, mul(direction, hit.ray.tfar));
    const auto& triangle = scene.triangle(hit.hit);
    const auto albedo = bake_material(
        scene.input, triangle, hit.hit.u, hit.hit.v, {{1, 1, 1}, settings.terrain_vertex_albedo}
    );
    auto irradiance = gi.sample(p, n, mul(direction, -1), options);
    const auto receiver = add(p, mul(n, settings.ray_bias));

    for (std::size_t i = 0; i < scene.input.suns.size(); ++i) {
        const auto& sun = scene.input.suns[i];
        const float cosine = std::max(0.f, dot(n, scene.to_suns[i]));

        if (cosine <= 0) {
            continue;
        }

        const float visibility = scene.occluded(
                                     receiver,
                                     scene.to_suns[i],
                                     settings.ray_bias,
                                     std::numeric_limits<float>::infinity()
                                 )
                                     ? .1f
                                     : 1.f;

        for (unsigned c = 0; c < 3; ++c) {
            irradiance[c] += sun.irradiance[c] * (sun.primary ? settings.sunlight_scale[c] : 1)
                             * cosine * visibility;
        }
    }

    // Reuse emission texture/vertex modulation with the visible, rather than bake-boosted, colour.
    auto visible = triangle;
    visible.emission = triangle.visible_emission;
    auto result = bake_emission(scene.input, visible, hit.hit.u, hit.hit.v);

    for (unsigned c = 0; c < 3; ++c) {
        result[c] += albedo[c] * irradiance[c] / kPi;
    }

    return result;
}

// Capture six source faces before integrating any roughness levels.
std::vector<Float3> capture_faces(const Capture& context, Float3 origin, unsigned size) {
    const profiling::Zone zone("Capture environment faces");
    std::vector<Float3> faces(std::size_t(6) * size * size);

    for (unsigned face = 0; face < 6; ++face) {
        for (unsigned y = 0; y < size; ++y) {
            for (unsigned x = 0; x < size; ++x) {
                const auto direction = environment_direction(
                    face, {(float(x) + .5f) / float(size), (float(y) + .5f) / float(size)}
                );
                faces[(std::size_t(face) * size + y) * size + x] =
                    capture(context, origin, direction);
            }
        }
    }

    return faces;
}

// Workers write disjoint probe bands and inherit the caller's arithmetic environment on each batch.
void capture_parallel(
    EnvironmentAsset& asset, const Capture& context, const BakeExecution& execution
) {
    std::atomic<unsigned> next{0};
    std::unique_ptr<BakeThreadPool> local;

    // Standalone captures can run without the batch coordinator's persistent pool.
    if (!execution.pool) {
        local = std::make_unique<BakeThreadPool>(execution.threads);
    }

    auto& pool = execution.pool ? *execution.pool : *local;
    const unsigned count = std::min(
        static_cast<unsigned>(asset.probes.size()),
        execution.pool ? worker_budget(execution) : pool.size()
    );
    std::fenv_t environment;

    // A previous GI batch may have used directed rounding on these same workers.
    if (std::fegetenv(&environment) != 0) {
        throw std::runtime_error("cannot read reflection floating-point environment");
    }

    // Tasks borrow capture state until run() drains them, including failed captures.
    const auto result = pool.run(count, [&] {
        try {
            // Reused workers must not retain a preceding stage's arithmetic mode.
            if (std::fesetenv(&environment) != 0) {
                throw std::runtime_error("cannot set reflection floating-point environment");
            }

            for (unsigned id = next.fetch_add(1); id < asset.probes.size();
                 id = next.fetch_add(1)) {
                const auto faces =
                    capture_faces(context, asset.probes[id].position, asset.resolution);
                filter_environment(asset, id, faces, context.options.filter_samples);
            }
        } catch (...) {
            next.store(static_cast<unsigned>(asset.probes.size()));
            throw;
        }
    });

    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

EnvironmentAsset bake_environment(
    const BakeScene& source,
    const BakeSettings& settings,
    std::span<const ProbeVolume> gi,
    std::span<const EnvironmentFloorSample> floors,
    const EnvironmentBakeSettings& options,
    const BakeExecution& execution,
    EnvironmentPlacementReport* placement
) {
    const profiling::Zone trace("Bake environment probes");

    if (gi.empty() || options.maximum_probes == 0 || options.maximum_probes > kEnvironmentProbeLimit
        || options.resolution < 4 || options.resolution > 256
        || !std::has_single_bit(options.resolution) || options.filter_samples == 0
        || options.filter_samples > 4096 || !std::isfinite(options.spacing) || options.spacing <= 0
        || !std::isfinite(options.box_distance) || options.box_distance <= 1
        || !std::isfinite(options.minimum_capture_spacing) || options.minimum_capture_spacing < 0
        || options.minimum_capture_spacing > 64 || !std::isfinite(options.minimum_space_area)
        || options.minimum_space_area < 0 || options.minimum_space_area > 256
        || !std::isfinite(options.merge_overlap) || options.merge_overlap < .8f
        || options.merge_overlap > 1 || !std::isfinite(options.merge_loss_fraction)
        || options.merge_loss_fraction < 0 || options.merge_loss_fraction > .01f
        || options.merge_max_misses > 8 || !std::isfinite(options.merge_brightness_ratio)
        || options.merge_brightness_ratio < 1 || options.merge_brightness_ratio > 8
        || !std::isfinite(options.merge_colour_distance) || options.merge_colour_distance < 0
        || options.merge_colour_distance > 1) {
        throw std::runtime_error("invalid environment bake settings");
    }

    const unsigned threads = worker_budget(execution);
    Scene scene(source, settings.sunlight_direction, threads);
    const CaptureGi field(gi);
    EnvironmentAsset asset;
    asset.resolution = options.resolution;
    asset.level = gi[0].grid.level;
    asset.scene_hash = gi[0].grid.scene_hash;
    asset.settings_hash = gi[0].grid.settings_hash;
    asset.probes = place_environment_probes(scene, gi, floors, settings, options, placement);

    if (asset.height() > 16384 || asset.width() > 16384
        || std::uint64_t(asset.width()) * asset.height() * 4 > 512ull * 1024 * 1024) {
        throw std::runtime_error(
            "environment atlas exceeds texture/storage safety limits; reduce resolution"
        );
    }

    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    capture_parallel(asset, {scene, settings, field, options}, execution);
    scene.check_error();
    integrate_environment_brdf(asset, options.filter_samples * 2);
    asset.validate();
    return asset;
}
}  // namespace openrac::lighting
