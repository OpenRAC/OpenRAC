// SPDX-License-Identifier: GPL-3.0-or-later
// Small layer parameters travel with the captured frame; geometry templates remain resident.
uniform usamplerBuffer water_coarse;
uniform int water_mesh;
uniform int water_first_triangle;
uniform int water_uv_animate;
uniform vec4 water_uv_motion; // scroll U/V, signed distortion amplitude, original animation phase.

void water_mesh_vertex(inout vec3 p, inout vec2 uv, inout vec4 rgba) {
    if (water_mesh != 0) {
        int at = (water_first_triangle + gl_InstanceID) * 7;
        vec3 weights = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
        p = vec3(0);
        uv = vec2(0);
        rgba = vec4(0);
        // Coarse UV motion was computed once per original vertex; interpolate its original shape.
        for (int i = 0; i < 3; ++i) {
            uvec4 source = texelFetch(water_coarse, at + i * 2);
            vec3 point = uintBitsToFloat(source.xyz);
            vec2 base_uv = uintBitsToFloat(texelFetch(water_coarse, at + i * 2 + 1).zw);
            uint colour = source.w;
            vec4 c = vec4(colour & 255u, (colour >> 8u) & 255u,
                          (colour >> 16u) & 255u, colour >> 24u);
            p += point * weights[i];
            uv += base_uv * weights[i];
            rgba += c * weights[i];
        }
        rgba = floor(rgba + 0.5);
    } else if (water_uv_animate != 0) {
        uv = water_authored_uv(p, uv, water_uv_motion);
    }
}
