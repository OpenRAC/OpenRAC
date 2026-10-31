// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Finite emitter sampling and complementary emission hits for probe transport.
#include <stdexcept>

#include "enhanced/lighting/bake_material.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"

namespace openrac::lighting::bake_detail {
namespace {
// One transparent emitter crossing, accumulated in source order after traversal.
struct RectangleCrossing {
    unsigned source;  // Stable rectangle index in the immutable scene.
    Float3 radiance;  // MIS-weighted contribution along the segment.
};

// Per-ray query state stays on its worker; callbacks allocate nothing and never throw.
struct RectangleQuery : RTCRayQueryContext {
    RTCRay ray;            // Borrowed geometry visibility interval, copied for traversal.
    double direction_pdf;  // Competing primary or BSDF direction density.
    std::span<RectangleCrossing> crossings;  // Borrowed stack scratch for visible candidates.
    std::size_t count = 0;                   // Occupied entries in the scratch span.
    bool overflow = false;                   // Large crossing sets use an ordered fallback.
    std::span<std::uint64_t> seen;  // Worker-owned bits prevent repeated primitive callbacks.
};

// A power-weighted emitter choice with its discrete selection probability.
struct SelectedEmitter {
    const Scene::Emitter& emitter;  // Borrowed from the immutable acceleration scene.
    double probability;             // Probability before point/area conversion.
};

// Samples a finite one-sided rectangle with an area PDF and ordinary shadow visibility.
EmissionSample sample_rectangle(
    const Scene& scene, const SelectedEmitter& selected, Float3 origin, float bias, Random& random
) {
    const auto index = selected.emitter.triangle - scene.input.triangles.size()
                       - scene.input.point_emitters.size();
    const auto& light = scene.input.area_emitters.at(index);
    const float u = 2 * random.next() - 1;
    const float v = 2 * random.next() - 1;
    const auto position = add(light.position, add(mul(light.axis_u, u), mul(light.axis_v, v)));
    const auto delta = add(position, mul(origin, -1));
    const float squared = dot(delta, delta);
    const float distance = std::sqrt(squared);

    // Coincident endpoints cannot form a visibility interval.
    if (distance <= 4 * bias) {
        return {};
    }

    const auto direction = mul(delta, 1 / distance);
    const float cosine = std::max(0.0f, -dot(selected.emitter.normal, direction));

    // Only the emitting side contributes, and real geometry still blocks it.
    if (cosine <= 0 || scene.occluded(origin, direction, bias, distance - 2 * bias)) {
        return {};
    }

    const double weight = cosine * selected.emitter.area / (squared * selected.probability);
    return {direction, mul(light.radiance, static_cast<float>(weight)), 1 / weight};
}

// Samples inverse-square point lighting over a finite visibility segment.
EmissionSample sample_point_emitter(
    const Scene& scene, const SelectedEmitter& selected, Float3 origin, float bias
) {
    const auto& light =
        scene.input.point_emitters.at(selected.emitter.triangle - scene.input.triangles.size());
    const auto delta = add(light.position, mul(origin, -1));
    const float squared = dot(delta, delta);
    const float distance = std::sqrt(squared);

    // Near-coincident endpoints cannot form a stable visibility interval.
    if (distance <= 4 * bias) {
        return {};
    }

    const auto direction = mul(delta, 1 / distance);

    // Stop at any accepted blocker before the sampled light.
    if (scene.occluded(origin, direction, bias, distance - 2 * bias)) {
        return {};
    }

    return {
        direction,
        mul(light.intensity, static_cast<float>(1 / (squared * selected.probability))),
        0,
        true
    };
}

// Samples a two-sided triangle with uniform area barycentrics.
EmissionSample sample_area_emitter(
    const Scene& scene, const SelectedEmitter& selected, Float3 origin, float bias, Random& random
) {
    const double pdf_area = selected.probability / selected.emitter.area;
    const auto& triangle = scene.input.triangles[selected.emitter.triangle];
    const float root = std::sqrt(random.next());
    const float u = root * (1 - random.next());
    const float v = root - u;
    Float3 position{};

    // Barycentric sampling is uniform over the selected triangle area.
    for (unsigned c = 0; c < 3; ++c) {
        position[c] = triangle.vertices[0][c] * (1 - u - v) + triangle.vertices[1][c] * u
                      + triangle.vertices[2][c] * v;
    }

    const auto delta = add(position, mul(origin, -1));
    const float squared = dot(delta, delta);
    const float distance = std::sqrt(squared);

    // Reject near-coincident or transparent emitter samples.
    if (distance <= 4 * bias
        || material_opacity(scene.input, triangle, u, v) < triangle.alpha_cutoff) {
        return {};
    }

    const auto direction = mul(delta, 1 / distance);
    const float cosine = std::abs(dot(selected.emitter.normal, direction));

    // Two-sided emitters use the absolute geometric receiving angle.
    if (cosine <= 0 || scene.occluded(origin, direction, bias, distance - 2 * bias)) {
        return {};
    }

    return {
        direction,
        mul(bake_emission(scene.input, triangle, u, v),
            static_cast<float>(
                static_cast<double>(cosine) / (static_cast<double>(squared) * pdf_area)
            )),
        static_cast<double>(squared) * pdf_area / cosine
    };
}

// ---- Hit emission ----

// Requires complete source containment so reused materials outside a room retain their original gain.
bool contains_source(const SurfaceEmissionRegion& region, const BakeTriangle& triangle) {
    // A source gain applies only when every vertex lies inside the configured room.
    for (const auto& position : triangle.vertices) {
        // Bounds are axis-aligned in the emission profile.
        for (unsigned axis = 0; axis < 3; ++axis) {
            // A triangle crossing a room boundary retains the global gain.
            if (position[axis] < region.minimum[axis] || position[axis] > region.maximum[axis]) {
                return false;
            }
        }
    }

    return true;
}

// Resolves absolute surface gain without modifying authored material brightness.
float surface_emission_scale(const BakeTriangle& triangle, const BakeSettings& settings) {
    float scale = settings.surface_emission_scale;

    // Ordered overrides preserve authored room-specific brightness.
    for (const auto& region : settings.surface_emission_regions) {
        // Later entries explicitly override earlier rooms.
        if (contains_source(region, triangle)) {
            scale = region.scale;
        }
    }

    return scale;
}

// Resolves the same discrete probability used by explicit light selection.
double area_pdf(const Scene& scene, const Scene::Emitter& emitter) {
    const auto index = static_cast<std::size_t>(&emitter - scene.emitters.data());
    // The first entry has no preceding cumulative weight.
    const double previous = index == 0 ? 0 : scene.emitters[index - 1].cumulative;
    return (emitter.cumulative - previous) / (scene.emission_weight * emitter.area);
}

// Evaluates a visible mesh hit with the complementary directional sampling weight.
Float3 mesh_emission(
    const Scene& scene, const BakeSettings& settings, const RTCRayHit& hit, double direction_pdf
) {
    // Escaping rays have no material emission.
    if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return {};
    }

    const auto source = scene.triangle_ids[hit.hit.geomID][hit.hit.primID];
    const auto& triangle = scene.input.triangles[source];

    // Most geometry is non-emitting and needs no distribution lookup.
    if (triangle.emission == Float3{}) {
        return {};
    }

    const auto emitter = std::lower_bound(
        scene.emitters.begin(),
        scene.emitters.end(),
        source,
        [](const Scene::Emitter& candidate, std::size_t id) { return candidate.triangle < id; }
    );

    // Degenerate or zero-power geometry cannot be sampled as an emitter.
    if (emitter == scene.emitters.end() || emitter->triangle != source) {
        return {};
    }

    const Float3 direction{hit.ray.dir_x, hit.ray.dir_y, hit.ray.dir_z};
    const double cosine = std::abs(dot(emitter->normal, direction));

    // A tangent ray has zero emitting solid angle.
    if (cosine == 0) {
        return {};
    }

    const double pdf =
        area_pdf(scene, *emitter) * static_cast<double>(hit.ray.tfar) * hit.ray.tfar / cosine;
    const float weight =
        emission_mis_weight(direction_pdf, pdf) * surface_emission_scale(triangle, settings);
    return mul(bake_emission(scene.input, triangle, hit.hit.u, hit.hit.v), weight);
}

// Intersects a transparent one-sided rectangle within the already traced visibility segment.
Float3 rectangle_emission(const Scene::Rectangle& light, const RTCRay& ray, double direction_pdf) {
    const Float3 origin{ray.org_x, ray.org_y, ray.org_z};
    const Float3 direction{ray.dir_x, ray.dir_y, ray.dir_z};
    const float cosine = -dot(light.normal, direction);

    // Back-facing and tangent rays cannot receive one-sided rectangle emission.
    if (cosine <= 0) {
        return {};
    }

    const auto delta = add(light.position, mul(origin, -1));
    const float distance = -dot(light.normal, delta) / cosine;

    // The real scene hit bounds visibility without making rectangles collision geometry.
    if (distance < ray.tnear || distance > ray.tfar) {
        return {};
    }

    const auto local = add(mul(direction, distance), mul(delta, -1));

    // Dual coordinates also handle nonorthogonal rectangle axes.
    if (std::abs(dot(local, light.dual_u)) > 1 || std::abs(dot(local, light.dual_v)) > 1) {
        return {};
    }

    const double pdf = light.area_pdf * static_cast<double>(distance) * distance / cosine;
    return mul(light.radiance, emission_mis_weight(direction_pdf, pdf));
}

// Handles oversized candidate sets without dropping light or allocating inside Embree callbacks.
Float3 rectangle_emission_sum(const Scene& scene, const RTCRay& ray, double direction_pdf) {
    Float3 result{};

    // Source order matches the bounded query's deterministic reduction.
    for (const auto& light : scene.rectangles) {
        result = add(result, rectangle_emission(light, ray, direction_pdf));
    }

    return result;
}

// Finds transparent crossings using bounded stack scratch and a source-ordered reduction.
Float3 rectangle_segment_emission(const Scene& scene, const RTCRay& ray, double direction_pdf) {
    Float3 result{};

    // An empty light scene needs no secondary traversal or worker scratch storage.
    if (!scene.rectangle_handle) {
        return result;
    }

    // Bounded stack scratch avoids allocation in each path segment.
    constexpr std::size_t kMaximumTrackedRectangles = 4096;
    constexpr std::size_t kMaximumCrossings = 64;
    constexpr std::size_t kWordBits = 64;

    // Oversized emitter scenes retain exact results without imposing a source-count limit.
    if (scene.rectangles.size() > kMaximumTrackedRectangles) {
        return add(result, rectangle_emission_sum(scene, ray, direction_pdf));
    }

    std::array<std::uint64_t, kMaximumTrackedRectangles / kWordBits> seen;
    const auto words = (scene.rectangles.size() + kWordBits - 1) / kWordBits;
    std::fill_n(seen.begin(), words, 0);
    std::array<RectangleCrossing, kMaximumCrossings> crossings;
    RectangleQuery query{};
    rtcInitRayQueryContext(&query);
    query.ray = ray;
    query.direction_pdf = direction_pdf;
    query.seen = std::span(seen).first(words);
    query.crossings = crossings;
    RTCIntersectArguments arguments;
    rtcInitIntersectArguments(&arguments);
    arguments.context = &query;
    RTCRayHit candidate{};
    candidate.ray = ray;
    candidate.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcIntersect1(scene.rectangle_handle, &candidate, &arguments);

    // Rare oversized crossing sets use the same source-order sum without truncating illumination.
    if (query.overflow) {
        return add(result, rectangle_emission_sum(scene, ray, direction_pdf));
    }

    auto accepted = std::span(crossings).first(query.count);
    // BVH build order must not change floating-point accumulation between worker budgets.
    std::sort(accepted.begin(), accepted.end(), [](const auto& a, const auto& b) {
        return a.source < b.source;
    });

    // Source order preserves deterministic results across BVH builds.
    for (const auto& crossing : accepted) {
        result = add(result, crossing.radiance);
    }

    return result;
}

}  // namespace

// Area/power selection and uniform triangle barycentrics. The returned value
// is Le / pdf(solid angle); multiply by the receiving cosine and average.
// Two-sided emission deliberately does not depend on authored normals/winding.
EmissionSample sample_emitter(
    const Scene& scene, Float3 origin, float bias, Random& random, const BakeSettings& settings
) {
    // A scene without emitters consumes no emitter random samples.
    if (scene.emitters.empty()) {
        return {};
    }

    const double choice = random.next() * scene.emission_weight;
    auto it = std::upper_bound(
        scene.emitters.begin(),
        scene.emitters.end(),
        choice,
        [](double value, const Scene::Emitter& emitter) { return value < emitter.cumulative; }
    );

    // Rounding at the cumulative upper bound selects the final emitter.
    if (it == scene.emitters.end()) {
        it = scene.emitters.end() - 1;
    }

    const double previous = it == scene.emitters.begin() ? 0 : (it - 1)->cumulative;
    const SelectedEmitter selected{*it, (it->cumulative - previous) / scene.emission_weight};

    // Non-geometric rectangles occupy indices after the original surface and point sources.
    if (it->triangle >= scene.input.triangles.size() + scene.input.point_emitters.size()) {
        return sample_rectangle(scene, selected, origin, bias, random);
    }

    // Point lights use inverse-square falloff instead of an area PDF.
    if (it->area == 0) {
        return sample_point_emitter(scene, selected, origin, bias);
    }

    auto sample = sample_area_emitter(scene, selected, origin, bias, random);
    const float scale =
        surface_emission_scale(scene.input.triangles[selected.emitter.triangle], settings);
    sample.radiance_weight = mul(sample.radiance_weight, scale);
    return sample;
}

// ---- Transparent acceleration ----

void Scene::prepare_rectangles() {
    // Source IDs keep synthetic rectangles after all mesh and point emitters.
    for (const auto& emitter : emitters) {
        const auto first = input.triangles.size() + input.point_emitters.size();

        // Mesh and point sources do not need analytic rectangle coordinates.
        if (emitter.triangle < first) {
            continue;
        }

        const auto& light = input.area_emitters[emitter.triangle - first];
        const auto normal = cross(light.axis_u, light.axis_v);
        const float squared = dot(normal, normal);
        rectangles.push_back(
            {light.position,
             emitter.normal,
             mul(cross(light.axis_v, normal), 1 / squared),
             mul(cross(normal, light.axis_u), 1 / squared),
             light.radiance,
             {std::abs(light.axis_u[0]) + std::abs(light.axis_v[0]),
              std::abs(light.axis_u[1]) + std::abs(light.axis_v[1]),
              std::abs(light.axis_u[2]) + std::abs(light.axis_v[2])},
             area_pdf(*this, emitter)}
        );
    }
}

void Scene::rectangle_bounds(const RTCBoundsFunctionArguments* args) {
    const auto& scene = *static_cast<const Scene*>(args->geometryUserPtr);
    const auto& light = scene.rectangles[args->primID];
    // Expand planar bounds so floating-point rounding cannot reject coplanar endpoints.
    const auto lower = add(light.position, mul(light.half_extent, -1));
    const auto upper = add(light.position, light.half_extent);
    const float infinity = std::numeric_limits<float>::infinity();
    auto& bounds = *args->bounds_o;
    bounds.lower_x = std::nextafter(lower[0], -infinity);
    bounds.lower_y = std::nextafter(lower[1], -infinity);
    bounds.lower_z = std::nextafter(lower[2], -infinity);
    bounds.upper_x = std::nextafter(upper[0], infinity);
    bounds.upper_y = std::nextafter(upper[1], infinity);
    bounds.upper_z = std::nextafter(upper[2], infinity);
}

void Scene::intersect_rectangle(const RTCIntersectFunctionNArguments* args) {
    auto& query = *static_cast<RectangleQuery*>(args->context);
    const auto& scene = *static_cast<const Scene*>(args->geometryUserPtr);
    constexpr unsigned kWordBits = 64;
    auto& seen = query.seen[args->primID / kWordBits];
    const std::uint64_t bit = std::uint64_t{1} << (args->primID % kWordBits);

    // Duplicate BVH candidates must not count the same emitting plane twice.
    if ((seen & bit) != 0) {
        return;
    }

    seen |= bit;
    const auto value =
        rectangle_emission(scene.rectangles[args->primID], query.ray, query.direction_pdf);

    // Zero contributions need no ordered reduction entry.
    if (value != Float3{}) {
        // An unusually long ray can cross more emitters than the stack buffer holds.
        if (query.count == query.crossings.size()) {
            query.overflow = true;
            return;
        }

        query.crossings[query.count++] = {args->primID, value};
    }

    // Never write a hit: transparent lights must leave the complete visibility interval open.
}

void Scene::build_rectangles() {
    // Mesh-only scenes do not pay for a separate acceleration structure.
    if (rectangles.empty()) {
        return;
    }

    // An Embree built without user geometry cannot trace them (tools/build_port.py).
    if (rtcGetDeviceProperty(device, RTC_DEVICE_PROPERTY_USER_GEOMETRY_SUPPORTED) == 0) {
        throw std::runtime_error("this Embree has no user geometry for the area lights");
    }

    rectangle_handle = rtcNewScene(device);
    check_error();
    RTCGeometry geometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);

    // Failed builds release the geometry locally; Scene owns the scene handle.
    try {
        check_error();
        rtcSetSceneFlags(rectangle_handle, RTC_SCENE_FLAG_ROBUST);
        rtcSetGeometryUserPrimitiveCount(geometry, static_cast<unsigned>(rectangles.size()));
        rtcSetGeometryUserData(geometry, this);
        rtcSetGeometryBoundsFunction(geometry, rectangle_bounds, nullptr);
        rtcSetGeometryIntersectFunction(geometry, intersect_rectangle);
        rtcCommitGeometry(geometry);
        rtcAttachGeometry(rectangle_handle, geometry);
        rtcCommitScene(rectangle_handle);
        check_error();
    } catch (...) {
        // Embree can fail before returning an allocated geometry handle.
        if (geometry) {
            rtcReleaseGeometry(geometry);
        }

        throw;
    }

    rtcReleaseGeometry(geometry);
}

// ---- Estimator weights ----

float emission_mis_weight(double sampled_pdf, double competing_pdf) {
    // Empty competing distributions leave the selected estimator unchanged.
    if (competing_pdf <= 0) {
        return 1;
    }

    // Rejected or tangent samples have no contribution.
    if (sampled_pdf <= 0) {
        return 0;
    }

    const double scale = std::max(sampled_pdf, competing_pdf);
    const double sampled = sampled_pdf / scale;
    const double competing = competing_pdf / scale;
    return static_cast<float>(sampled * sampled / (sampled * sampled + competing * competing));
}

void weight_emitter(EmissionSample& sample, double direction_pdf) {
    // Delta sources cannot be reached by a continuous direction sampler.
    if (!sample.delta) {
        sample.radiance_weight =
            mul(sample.radiance_weight, emission_mis_weight(sample.pdf, direction_pdf));
    }
}

Float3 segment_emission(
    const Scene& scene, const BakeSettings& settings, const RTCRayHit& hit, double direction_pdf
) {
    const auto mesh = mesh_emission(scene, settings, hit, direction_pdf);
    return add(mesh, rectangle_segment_emission(scene, hit.ray, direction_pdf));
}

}  // namespace openrac::lighting::bake_detail
