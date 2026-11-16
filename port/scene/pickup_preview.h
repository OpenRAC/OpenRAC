#pragma once
#include <cstdlib>

#include "vendor_preview.h"

namespace openrac::scene::pickup_preview {
struct Adjustment {
    vendor_preview::Adjustment model;
    float z = 0;
};

// Approved pickup model settings; independent of the vendor displays.
inline Adjustment defaults(int id, bool infobot = false) {
    if (infobot) {
        switch (id) {
#include "infobot_pickup_defaults.inc"
            default:
                return {};
        }
    }
    switch (id) {
#include "pickup_defaults.inc"
        default:
            return {};
    }
}

inline bool equipment_source(int source) {
    switch (source) {
        case 1120:
        case 1005:
        case 1016:
        case 18:
        case 1354:
        case 1388:
        case 849:
        case 188:
        case 1251:
        case 1290:
        case 173:
        case 185:
        case 618:
            return true;
        default:
            return false;
    }
}

inline bool should_adjust(int source, int display, float height, bool own_camera) {
    return ((source == 0x46E && display != 0x46E) || equipment_source(source)
            || (source == 750 && display != 750))
           && height > 0 && !own_camera;
}

inline Adjustment get(int id, bool infobot = false) {
    return defaults(id, infobot);
}

inline renderer::Mat4 native_gold(
    const renderer::Vec3& home,
    float seconds,
    float scale,
    float hover,
    float amplitude,
    const std::array<float, 4>& degrees_per_second
) {
    vendor_preview::Adjustment a;
    a.pitch = std::fmod(seconds * degrees_per_second[0], 360.0f);
    a.yaw = std::fmod(seconds * degrees_per_second[1], 360.0f);
    a.roll = std::fmod(seconds * degrees_per_second[2], 360.0f);
    a.size = scale;
    auto matrix = vendor_preview::orient(renderer::identity(), a, {0, 0, 0}, true);
    matrix[12] = home[0];
    matrix[13] = home[1];
    matrix[14] =
        home[2] + hover + amplitude * std::sin(seconds * degrees_per_second[3] * .017453292f);
    return matrix;
}

inline renderer::Mat4 apply(
    renderer::Mat4 source, const renderer::Vec3& center, float angle, const Adjustment& entry
) {
    const auto& a = entry.model;
    source = vendor_preview::freeze_spin(source, angle, a.spin);
    // Rotate and size around the mesh center, then keep that point over the pickup.
    const renderer::Vec3 anchor{source[12], source[13], source[14]};
    source = vendor_preview::orient(source, a, center, true);
    const renderer::Vec3 pivot =
        a.pivot_enabled ? renderer::Vec3{a.pivot_x, a.pivot_y, a.pivot_z} : center;
    for (int axis = 0; axis < 3; ++axis) {
        source[12 + axis] = anchor[axis];
        for (int col = 0; col < 3; ++col) {
            source[12 + axis] -= source[col * 4 + axis] * pivot[col];
        }
    }
    source[12] += a.x;
    source[13] += a.y;
    source[14] += entry.z;
    return source;
}
}  // namespace openrac::scene::pickup_preview
