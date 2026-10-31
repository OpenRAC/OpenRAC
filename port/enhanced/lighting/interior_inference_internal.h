// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Private collision queries shared by floor sampling and enclosed-air propagation.
#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>

#include "enhanced/lighting/interiors.h"

#include <embree4/rtcore.h>

namespace openrac::lighting::interiors::detail {
// ---- Collision queries ----

inline Point sub(Point a, Point b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline Point add(Point a, Point b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline Point mul(Point a, float b) {
    return {a[0] * b, a[1] * b, a[2] * b};
}

inline float dot(Point a, Point b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline Point cross(Point a, Point b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

inline float length(Point a) {
    return std::sqrt(dot(a, a));
}

// Owns collision acceleration; partially constructed scenes release every successful Embree allocation.
struct Scene {
    std::unique_ptr<std::remove_pointer_t<RTCDevice>, decltype(&rtcReleaseDevice)> device{
        rtcNewDevice(nullptr), rtcReleaseDevice
    };
    std::unique_ptr<std::remove_pointer_t<RTCScene>, decltype(&rtcReleaseScene)> scene{
        nullptr, rtcReleaseScene
    };

    // Builds one immutable collision scene while local geometry remains independently owned.
    explicit Scene(const std::vector<Triangle>& triangles) {
        // Failed allocation leaves the remaining owners empty.
        if (!device) {
            throw std::runtime_error("Embree device unavailable");
        }
        scene.reset(rtcNewScene(device.get()));
        std::unique_ptr<std::remove_pointer_t<RTCGeometry>, decltype(&rtcReleaseGeometry)> geometry{
            rtcNewGeometry(device.get(), RTC_GEOMETRY_TYPE_TRIANGLE), rtcReleaseGeometry
        };
        // Buffer setup requires both valid acceleration objects.
        if (!scene || !geometry) {
            throw std::runtime_error("Embree collision allocation failed");
        }
        auto* vertices = static_cast<Point*>(rtcSetNewGeometryBuffer(
            geometry.get(),
            RTC_BUFFER_TYPE_VERTEX,
            0,
            RTC_FORMAT_FLOAT3,
            sizeof(Point),
            triangles.size() * 3
        ));
        auto* indices = static_cast<std::array<unsigned, 3>*>(rtcSetNewGeometryBuffer(
            geometry.get(),
            RTC_BUFFER_TYPE_INDEX,
            0,
            RTC_FORMAT_UINT3,
            sizeof(std::array<unsigned, 3>),
            triangles.size()
        ));
        // Out-of-memory failures must not become writes through null buffer pointers.
        if (!vertices || !indices) {
            throw std::runtime_error("Embree collision buffer allocation failed");
        }
        for (unsigned i = 0; i < triangles.size(); ++i) {
            for (unsigned k = 0; k < 3; ++k) {
                vertices[i * 3 + k] = triangles[i].vertices[k];
            }
            indices[i] = {i * 3, i * 3 + 1, i * 3 + 2};
        }
        rtcCommitGeometry(geometry.get());
        rtcAttachGeometry(scene.get(), geometry.get());
        rtcCommitScene(scene.get());
        // The owners also release resources when Embree rejects the build.
        if (rtcGetDeviceError(device.get()) != RTC_ERROR_NONE) {
            throw std::runtime_error("Embree collision build failed");
        }
    }

    // Finds the nearest two-sided collision hit along a finite or unbounded ray.
    RTCRayHit hit(Point p, Point d, float far, float near = 0.015f) const {
        RTCRayHit h{};
        h.ray.org_x = p[0];
        h.ray.org_y = p[1];
        h.ray.org_z = p[2];
        h.ray.dir_x = d[0];
        h.ray.dir_y = d[1];
        h.ray.dir_z = d[2];
        h.ray.tnear = near;
        h.ray.tfar = far;
        h.ray.mask = ~0u;
        h.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rtcIntersect1(scene.get(), &h);
        return h;
    }

    bool clear(Point a, Point b) const {
        auto d = sub(b, a);
        const float distance = length(d);
        return distance < 0.02f
               || hit(a, mul(d, 1 / distance), distance).hit.geomID == RTC_INVALID_GEOMETRY_ID;
    }
};

// Attach authored endpoint evidence, then propagate through enclosed air.
void assign_transition_labels(
    const Scene& scene,
    std::span<const Transition> transitions,
    const Settings& settings,
    Result& result
);
}  // namespace openrac::lighting::interiors::detail
