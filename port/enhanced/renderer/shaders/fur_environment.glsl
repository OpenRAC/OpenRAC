// SPDX-License-Identifier: GPL-3.0-or-later
// Reuses the renderer's local prefiltered reflection captures for shaded and interior fur.
vec3 fur_environment_normal(FurSurfaceData surface) {
    vec3 eye = normalize(surface.eye);
    vec3 projected = eye - surface.tangent * dot(eye, surface.tangent);
    float squared = dot(projected, projected);
    if (squared < 1e-8) return surface.normal;
    projected *= inversesqrt(squared);
    float bend = hero_fur_lighting_parameters.z * (1. - .5 * hero_fur_lighting_parameters.y);
    return normalize(mix(surface.normal, projected, bend));
}

vec3 fur_environment_lighting(FurSurfaceData surface, vec3 p, vec3 irradiance, vec3 albedo) {
    if (hero_fur_lighting_enabled == 0 || surface.coverage <= 0. || hero_fur_environment_strength <= 0.
        || dot(surface.eye, surface.eye) < 1e-12) return vec3(0.);
    if (environment_count == 0) return fur_indirect_lighting(surface, irradiance, albedo);
    vec3 fallback = fur_indirect_reflection(surface, irradiance);
    vec3 n = fur_environment_normal(surface);
    // A broader prefilter and a groom-dependent bent normal approximate anisotropic environment reflection.
    float roughness = clamp(hero_fur_lighting_parameters.y * sqrt(1. + .5 * hero_fur_lighting_parameters.z), .35, 1.);
    vec3 eye = normalize(surface.eye);
    vec3 reflection = environment_specular_frame(p, n, eye, dot(surface.normal, eye), roughness, .04, fallback);
    vec3 scatter = fur_indirect_scattering(surface, irradiance, albedo);
    return (reflection * hero_fur_lighting_parameters.x + scatter) * (surface.coverage * hero_fur_environment_strength);
}
