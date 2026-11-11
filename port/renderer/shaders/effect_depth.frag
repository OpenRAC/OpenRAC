#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

in vec3 v_world;
uniform mat4 view;
layout(location = 0) out vec4 surface_normal;

void main() {
    vec3 position = (view * vec4(v_world, 1.0)).xyz;
    vec3 normal = cross(dFdx(position), dFdy(position));
    float squared = dot(normal, normal);
    surface_normal = vec4(0.0);
    if (squared > 0.0 && !isinf(squared)) {
        normal *= inversesqrt(squared);
        if (dot(normal, position) > 0.0) {
            normal = -normal;
        }
        surface_normal = vec4(normal * 0.5 + 0.5, 1.0);
    }
}
