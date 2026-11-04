// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Embree acceleration shared by the offline level bake.
#pragma once
#include "enhanced/lighting/bake.h"

#include <embree4/rtcore.h>

namespace openrac::lighting::bake_detail {
// Build identical query intervals for closest-hit and occlusion traversal.
inline RTCRay make_ray(Float3 origin, Float3 direction, float near, float far) {
    RTCRay ray{};
    ray.org_x = origin[0];
    ray.org_y = origin[1];
    ray.org_z = origin[2];
    ray.dir_x = direction[0];
    ray.dir_y = direction[1];
    ray.dir_z = direction[2];
    ray.tnear = near;
    ray.tfar = far;
    ray.mask = 0xFFFFFFFFu;
    return ray;
}

// Immutable source geometry, traversal and emitter selection tables.
struct Scene {
    const BakeScene& input;      // borrowed immutable export
    RTCDevice device = nullptr;  // owned Embree error and allocation context
    RTCScene handle = nullptr;   // owned acceleration structure
    RTCScene rectangle_handle = nullptr;  // Owned transparent emitter acceleration.

    // Cumulative area/power selection entry for a surface or point emitter.
    struct Emitter {
        std::size_t triangle;  // Source triangle, or point index after triangle count.
        double cumulative;     // Inclusive area/power weight.
        float area;            // Zero for a point emitter.
        Float3 normal;         // Geometric surface orientation, ignored for point emitters.
    };

    std::vector<Emitter> emitters;  // Stable source order for deterministic sampling.
    double emission_weight = 0;     // Total weight of the emitter distribution.
    // Separate geometry keeps opaque hits out of the alpha callback.
    // Transport resolves local primitive IDs only when it needs material data.
    std::array<std::vector<std::uint32_t>, 2> triangle_ids;
    // Normalising the authored suns once preserves the per-bounce arithmetic result.
    std::vector<Float3> to_suns;

    // Cached rectangle coordinates and selection density, read concurrently by probe workers.
    struct Rectangle {
        Float3 position;     // Centre of the transparent emitting plane.
        Float3 normal;       // Emitting side, from the cross product of the axes.
        Float3 dual_u;       // World delta to first rectangle coordinate.
        Float3 dual_v;       // World delta to second rectangle coordinate.
        Float3 radiance;     // Linear emitted colour.
        Float3 half_extent;  // Conservative axis-aligned bounds around the centre.
        double area_pdf;     // Discrete selection probability divided by area.
    };

    std::vector<Rectangle> rectangles;  // Only positive-power synthetic emitters.

    // Precomputes rectangle coordinates after the complete emitter distribution is known.
    void prepare_rectangles();
    // Builds a separate emitter BVH without adding blockers to the geometry scene.
    void build_rectangles();
    // Supplies conservative bounds to Embree during the emitter BVH build.
    static void rectangle_bounds(const RTCBoundsFunctionArguments* args);
    // Accumulates a transparent emitter crossing without accepting a collision hit.
    static void intersect_rectangle(const RTCIntersectFunctionNArguments* args);

    // Build one acceleration structure; the source must outlive this scene. THREADS: Embree's own
    // build threads (0: one per CPU thread, Embree's default); several bakes at once each take
    // their share, not the whole CPU each.
    explicit Scene(const BakeScene& source, const std::optional<Float3>& sunlight_direction = {},
                   std::uint32_t threads = 0);
    // Release the Embree scene and device after all workers have joined.
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Report errors on the calling worker, as required by Embree's device API.
    void check_error() const;

    // Resolve a local Embree hit to its original material and geometry.
    const BakeTriangle& triangle(const RTCHit& hit) const {
        return input.triangles[triangle_ids[hit.geomID][hit.primID]];
    }

    // Return the nearest accepted two-sided surface hit.
    RTCRayHit trace(Float3 origin, Float3 direction, float near, float far) const {
        RTCRayHit hit{};
        hit.ray = make_ray(origin, direction, near, far);
        hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rtcIntersect1(handle, &hit);
        return hit;
    }

    // Test visibility without resolving the closest blocker or its material.
    bool occluded(Float3 origin, Float3 direction, float near, float far) const {
        auto ray = make_ray(origin, direction, near, far);
        rtcOccluded1(handle, &ray);
        return ray.tfar < 0;  // Embree marks an accepted occlusion with negative tfar.
    }

private:
    // Classify traversal geometry without changing emitter sampling order.
    void collect_triangles();
    // Upload one geometry group; empty groups have no Embree object.
    void build_geometry(unsigned group);
    // Reject cutout candidates for both closest-hit and occlusion traversal.
    static void filter_alpha(const RTCFilterFunctionNArguments* args);
};

// Reject invalid transport settings and source geometry before allocating Embree state.
void validate_bake_scene(const BakeScene& input, const BakeSettings& settings);
}  // namespace openrac::lighting::bake_detail
