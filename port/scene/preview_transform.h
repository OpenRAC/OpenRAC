#pragma once
#include <algorithm>

#include "renderer/math.h"

namespace openrac::scene {
// model_class == -1 means the moby's own class, not an unknown replacement.
// Keep the original pose for an inset requesting its existing mesh. The centre
// hologram retains its established rendering until separately tested.
inline bool preview_replaces_class(int requested, int model_class, int own_class, bool inset) {
    return requested != (inset && model_class < 0 ? own_class : model_class);
}

// RAC1 models look along local +X. Keep them upright while facing the display's own eye.
inline renderer::Mat4 facing_preview(
    const renderer::Mat4& source,
    const renderer::Vec3& eye,
    const renderer::Vec3& camera_forward,
    renderer::Vec3 up
) {
    up = renderer::normalize(up);
    renderer::Vec3 front{eye[0] - source[12], eye[1] - source[13], eye[2] - source[14]};
    const auto flatten = [&](renderer::Vec3 direction) {
        const float height = renderer::dot(direction, up);
        for (int axis = 0; axis < 3; ++axis) {
            direction[axis] -= up[axis] * height;
        }
        return direction;
    };
    front = flatten(front);
    if (renderer::dot(front, front) < 0.000001f) {
        front = flatten({-camera_forward[0], -camera_forward[1], -camera_forward[2]});
    }
    front = renderer::normalize(front);
    const auto left = renderer::normalize(renderer::cross(up, front));
    const float scale =
        std::sqrt(source[8] * source[8] + source[9] * source[9] + source[10] * source[10]);
    auto result = renderer::identity();
    for (int axis = 0; axis < 3; ++axis) {
        result[axis] = front[axis] * scale;
        result[4 + axis] = left[axis] * scale;
        result[8 + axis] = up[axis] * scale;
        result[12 + axis] = source[12 + axis];
    }
    return result;
}

}  // namespace openrac::scene
