// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>

#include "enhanced/lighting/bake_material.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_distance_field.h"

namespace openrac::lighting::bake_detail {
namespace {
struct SectionPoint {
    Float3 p;
    Float2 uv;
};

struct Query {
    const Scene& scene;
    Float3 position;
    RoomWallDistance result;
};

// Intersect a triangle with the query's horizontal plane, retaining barycentric coordinates
// so alpha cutouts use the same material mask as ray traversal.
unsigned section(const BakeTriangle& triangle, float height, std::array<SectionPoint, 3>& points) {
    constexpr std::array<Float2, 3> uv{Float2{0, 0}, Float2{1, 0}, Float2{0, 1}};
    if (std::all_of(triangle.vertices.begin(), triangle.vertices.end(), [&](auto p) {
            return std::abs(p[2] - height) < 1e-5f;
        })) {
        return 0;
    }
    unsigned count = 0;
    auto append = [&](SectionPoint p) {
        for (unsigned i = 0; i < count; ++i) {
            const auto d = add(points[i].p, mul(p.p, -1));
            if (dot(d, d) < 1e-10f) {
                return;
            }
        }
        if (count < points.size()) {
            points[count++] = p;
        }
    };
    for (unsigned i = 0; i < 3; ++i) {
        const auto a = triangle.vertices[i], b = triangle.vertices[(i + 1) % 3];
        const float da = a[2] - height, db = b[2] - height;
        if (std::abs(da) < 1e-5f) {
            append({a, uv[i]});
        }
        if ((da < 0 && db > 0) || (da > 0 && db < 0)) {
            const float t = da / (da - db);
            append(
                {add(a, mul(add(b, mul(a, -1)), t)),
                 {uv[i][0] + t * (uv[(i + 1) % 3][0] - uv[i][0]),
                  uv[i][1] + t * (uv[(i + 1) % 3][1] - uv[i][1])}}
            );
        }
    }
    return count;
}

bool closest_section(RTCPointQueryFunctionArguments* args) {
    auto& query = *static_cast<Query*>(args->userPtr);
    const auto& scene = query.scene;
    const auto& triangle = scene.input.triangles[scene.triangle_ids[args->geomID][args->primID]];
    std::array<SectionPoint, 3> points;
    const unsigned count = section(triangle, query.position[2], points);
    if (!count) {
        return false;
    }
    bool changed = false;
    for (unsigned a = 0; a < count; ++a) {
        for (unsigned b = a; b < count; ++b) {
            const auto d = add(points[b].p, mul(points[a].p, -1));
            const float squared = dot(d, d);
            const float t =
                squared > 1e-12f
                    ? std::clamp(
                          dot(add(query.position, mul(points[a].p, -1)), d) / squared, 0.f, 1.f
                      )
                    : 0;
            const auto p = add(points[a].p, mul(d, t));
            const auto offset = add(query.position, mul(p, -1));
            const float distance = std::sqrt(dot(offset, offset));
            if (distance >= args->query->radius) {
                continue;
            }
            const auto u = points[a].uv[0] + t * (points[b].uv[0] - points[a].uv[0]);
            const auto v = points[a].uv[1] + t * (points[b].uv[1] - points[a].uv[1]);
            if (args->geomID == 1
                && material_opacity(scene.input, triangle, u, v) < triangle.alpha_cutoff) {
                continue;
            }
            query.result = {distance, p, true};
            args->query->radius = distance;
            changed = true;
        }
    }
    return changed;
}
}  // namespace

RoomWallDistance room_wall_distance(const Scene& scene, Float3 position) {
    RTCPointQuery point{position[0], position[1], position[2], 0, 32};
    RTCPointQueryContext context;
    rtcInitPointQueryContext(&context);
    Query query{scene, position, {}};
    rtcPointQuery(scene.handle, &point, &context, closest_section, &query);
    return query.result;
}
}  // namespace openrac::lighting::bake_detail
