// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OPENRAC_SHADING_BRDF
#define OPENRAC_SHADING_BRDF

float dielectric_fresnel(float cosine, float f0) {
    float f = 1. - clamp(cosine, 0., 1.);
    return f0 + (1. - f0) * f * f * f * f * f;
}

// GGX distribution and height-correlated Smith visibility, multiplied by NoL.
// Directions and the normal are unit vectors; roughness is perceptual roughness.
float ggx_specular(vec3 n, vec3 eye, vec3 light, float roughness, float f0) {
    float nv = max(dot(n, eye), 0.), nl = max(dot(n, light), 0.);
    vec3 sum = eye + light;
    if (nv <= 0. || nl <= 0. || dot(sum, sum) < 1e-8) {
        return 0.;
    }
    vec3 half_direction = normalize(sum);
    float nh = max(dot(n, half_direction), 0.);
    float a = roughness * roughness, a2 = a * a;
    float denominator = nh * nh * (a2 - 1.) + 1.;
    float distribution = a2 / (3.14159265 * denominator * denominator);
    // Includes 1/(4 NoV NoL).
    float visibility =
        .5 / max(nl * sqrt(nv * nv * (1. - a2) + a2) + nv * sqrt(nl * nl * (1. - a2) + a2), 1e-6);
    return distribution * visibility * dielectric_fresnel(dot(eye, half_direction), f0) * nl;
}
#endif
