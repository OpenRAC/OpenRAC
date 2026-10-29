// SPDX-License-Identifier: GPL-3.0-or-later
uniform int water_surface;
out vec3 v_water_rest;
vec3 effect_position(vec3 p) {
    v_water_rest=p;
    return water_surface==0?p:water_displaced_position(p,water_surface);
}

// Keep the shared effect interface separate from baked-mesh interpolation.
void effect_vertex(inout vec3 p, inout vec2 uv, inout vec4 rgba) {
    water_mesh_vertex(p, uv, rgba);
    p = effect_position(p);
}
