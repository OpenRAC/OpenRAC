// SPDX-License-Identifier: GPL-3.0-or-later
// The strip mesh carries barycentric weights and a local coarse triangle index.
uniform usamplerBuffer water_coarse;
uniform int water_mesh;
layout(location = 3) in uvec3 water_vertex;
// Vulkan may carry this draw integer separately; OpenGL keeps the ordinary uniform.
#define OPENRAC_DRAW_INT water_first_triangle
uniform int water_first_triangle;
uniform int water_uv_animate;
uniform vec4 water_uv_motion; // scroll U/V, signed distortion amplitude, original animation phase.
flat out vec2 v_water_art_axis;

void water_mesh_vertex(inout vec3 p, inout vec2 uv, inout vec4 rgba) {
    v_water_art_axis=vec2(0);
    if (water_mesh != 0) {
        int at = (water_first_triangle + int(water_vertex.z)) * 7;
        vec2 barycentric = uintBitsToFloat(water_vertex.xy);
        vec3 weights = vec3(1.0 - barycentric.x - barycentric.y, barycentric);
        p = vec3(0);
        uv = vec2(0);
        rgba = vec4(0);
        vec2 points[3],texcoords[3];
        // Coarse UV motion was computed once per original vertex; interpolate its original shape.
        for (int i = 0; i < 3; ++i) {
            uvec4 source = texelFetch(water_coarse, at + i * 2);
            vec3 point = uintBitsToFloat(source.xyz);
            vec2 base_uv = uintBitsToFloat(texelFetch(water_coarse, at + i * 2 + 1).zw);
            points[i]=point.xy;
            texcoords[i]=base_uv;
            uint colour = source.w;
            vec4 c = vec4(colour & 255u, (colour >> 8u) & 255u,
                          (colour >> 16u) & 255u, colour >> 24u);
            p += point * weights[i];
            uv += base_uv * weights[i];
            rgba += c * weights[i];
        }
        // Derive the art tangent once per source triangle. Screen derivatives lose
        // precision at large world coordinates and turn ripple phase into pixel noise.
        vec2 edge1=points[1]-points[0],edge2=points[2]-points[0];
        vec2 uv1=texcoords[1]-texcoords[0],uv2=texcoords[2]-texcoords[0];
        vec2 axis=edge1*uv2.y-edge2*uv1.y;
        if(dot(axis,axis)>1e-14) {
            axis=normalize(axis);
            if(axis.x<0. || (abs(axis.x)<1e-5 && axis.y<0.)) axis=-axis;
            v_water_art_axis=axis;
        }
        rgba = floor(rgba + 0.5);
    } else if (water_uv_animate != 0) {
        uv = water_authored_uv(p, uv, water_uv_motion);
    }
}
