// SPDX-License-Identifier: GPL-3.0-or-later
// Fine UV-anchored strand slopes; derivative frames and normal reconstruction remain FP32.
uniform int hero_fur_detail_enabled;
uniform sampler2D hero_fur_detail;
uniform vec4 hero_fur_detail_parameters; // Normal strength, colour contrast, fade start/end.

struct FurSurfaceData {
    vec3 normal;
    vec3 tangent;
    vec3 eye;
    float coverage;
    float grain;
};

// A UV growth vector is a physical tangent, unlike the covectors used for height-map slopes.
vec3 fur_world_groom(vec3 n, vec3 px, vec3 py, vec4 uv_derivatives, vec2 flow) {
    float determinant = uv_derivatives.x * uv_derivatives.w - uv_derivatives.y * uv_derivatives.z;
    vec3 tangent = ((px * uv_derivatives.w - py * uv_derivatives.y) * flow.x
        + (py * uv_derivatives.x - px * uv_derivatives.z) * flow.y) * sign(determinant);
    tangent -= n * dot(n, tangent);
    float squared = dot(tangent, tangent);
    return squared > 1e-20 ? tangent * inversesqrt(squared) : vec3(0.);
}

FurSurfaceData fur_surface_data(vec3 n, vec3 p, vec2 uv, vec3 toward_camera, float wetness) {
    // Compute derivatives before any per-pixel coverage/fade decisions; explicit gradients select mips.
    vec2 dx = dFdx(uv), dy = dFdy(uv);
    vec3 px = dFdx(p), py = dFdy(p);
    vec3 map = textureGrad(hero_fur_map, uv, dx, dy).rgb;
    FurSurfaceData surface = FurSurfaceData(n, vec3(0.), toward_camera, map.r * mix(1., .25, clamp(wetness, 0., 1.)), 1.);
    vec2 flow = map.gb * 2. - 1.;
    surface.tangent = fur_world_groom(n, px, py, vec4(dx, dy), flow);
    if (dot(surface.tangent, surface.tangent) < .5) {
        surface.coverage = 0.;
    }
    if (hero_fur_detail_enabled == 0) {
        return surface;
    }
    vec3 detail = (textureGrad(hero_fur_detail, uv, dx, dy).rgb * 255. - 128.) / 127.;
    vec2 extent = vec2(textureSize(hero_fur_detail, 0));
    float footprint = max(dot(dx * extent, dx * extent), dot(dy * extent, dy * extent));
    float distance_to_eye = length(toward_camera);
    float facing = abs(dot(n, toward_camera / max(distance_to_eye, 1e-6)));
    // Fade unresolved strands and grazing silhouettes before they can sparkle during movement.
    float fade = (1. - smoothstep(4., 64., footprint)) * smoothstep(.05, .25, facing)
        * (1. - smoothstep(hero_fur_detail_parameters.z, hero_fur_detail_parameters.w, distance_to_eye));
    float amount = map.r * fade * mix(1., .25, clamp(wetness, 0., 1.));
    // The cotangent frame follows skinning, UV mirroring and nonuniform scale without authored tangents.
    vec3 perpendicular_y = cross(py, n), perpendicular_x = cross(n, px);
    vec3 t = perpendicular_y * dx.x + perpendicular_x * dy.x;
    vec3 b = perpendicular_y * dx.y + perpendicular_x * dy.y;
    float frame_squared = max(dot(t, t), dot(b, b));
    if (frame_squared < 1e-20 || amount <= 0.) {
        return surface;
    }
    // Only the bounded strand amplitudes and colour variation benefit from narrow arithmetic.
    mp_vec2 slope = mp_vec2(detail.rg) * mp_float(hero_fur_detail_parameters.x) * mp_float(amount);
    vec3 perturbed = normalize(n - (t * float(slope.x) + b * float(slope.y)) * inversesqrt(frame_squared));
    mp_float grain = mp_float(1.) + mp_float(detail.b) * mp_float(hero_fur_detail_parameters.y) * mp_float(amount);
    surface.normal = perturbed;
    surface.grain = float(grain);
    surface.tangent = fur_world_groom(perturbed, px, py, vec4(dx, dy), flow);
    return surface;
}
