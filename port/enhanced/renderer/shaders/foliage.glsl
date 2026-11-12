// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Thin-sheet diffuse transmission and a rough dielectric surface, without screen-space diffusion.
// The diffuse energy split follows the public KHR_materials_diffuse_transmission model:
// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_diffuse_transmission
// Optical thickness controls Beer-Lambert absorption of the painted hue; it is not measured geometry.
#ifndef OPENRAC_FOLIAGE
#define OPENRAC_FOLIAGE 1
uniform int foliage_parameters0;
uniform int foliage_parameters1;
// openrac_mode: foliage_debug
uniform int foliage_debug;
uniform float foliage_reflectivity;

// Positive roughness marks reviewed leaves, including leaves with transmission set to zero.
vec4 foliage_material() {
    return vec4(unpackHalf2x16(uint(foliage_parameters0)), unpackHalf2x16(uint(foliage_parameters1)));
}

// UV-card curvature changes only material response; GI, GTAO, depth and shadow geometry stay smooth.
vec3 foliage_normal(vec3 normal, vec3 position, vec2 uv, float bend) {
    vec3 dx = dFdx(position), dy = dFdy(position);
    vec2 ux = dFdx(uv), uy = dFdy(uv);
    float determinant = ux.x * uy.y - ux.y * uy.x;
    if (bend <= 0. || abs(determinant) < 1e-10) return normal;
    vec3 tangent = (dx * uy.y - dy * ux.y) / determinant;
    vec3 bitangent = (dy * ux.x - dx * uy.x) / determinant;
    tangent -= normal * dot(normal, tangent);
    bitangent -= normal * dot(normal, bitangent);
    if (min(dot(tangent,tangent), dot(bitangent,bitangent)) < 1e-10) return normal;
    vec2 radial = fract(uv) * 2. - 1.;
    // Fade subpixel curvature rather than introducing distant highlight shimmer.
    float fade = 1. - smoothstep(.08, .3, max(length(ux), length(uy)));
    return probe_unit(normal + bend * fade * (radial.x * normalize(tangent) + radial.y * normalize(bitangent)));
}

// Dividing out brightness keeps dark paint from being applied twice to transmitted light.
vec3 foliage_transmittance(vec3 albedo, vec4 material) {
    float peak = max(albedo.r, max(albedo.g, albedo.b));
    if (peak <= 1e-6) return vec3(0.);
    vec3 hue = clamp(albedo / peak, vec3(.02), vec3(1.));
    // pow(hue, thickness) is Beer-Lambert attenuation with absorption coefficient -log(hue).
    return material.x * pow(hue, vec3(material.y));
}

// Partial wax coverage keeps grazing sheen subtle instead of whitening the painted leaves.
const float kFoliageWaxCoverage = .25;

// Per-fragment terms shared by every directional, point and spot light.
struct FoliageSurfaceData {
    vec3 normal;
    vec3 eye;
    vec3 albedo;
    vec3 transmitted;
    vec4 material;
    float diffuse_energy;
};

FoliageSurfaceData prepare_foliage(vec3 normal, vec3 eye, vec3 albedo, vec4 material) {
    float energy = 1. - kFoliageWaxCoverage * dielectric_fresnel(max(dot(normal, eye), 0.), foliage_reflectivity);
    return FoliageSurfaceData(normal, eye, albedo * material_diffuse_scale,
        foliage_transmittance(albedo, material) * material_diffuse_scale, material, energy);
}

// Keeps reflected diffuse, transmitted diffuse and sharp surface reflection on separate energy paths.
vec3 foliage_response(FoliageSurfaceData surface, vec3 light) {
    float cosine = dot(surface.normal, light);
    vec3 diffuse = surface.albedo * ((1. - surface.material.x) * max(cosine, 0.));
    vec3 transmitted = surface.transmitted * max(-cosine, 0.);
    if (foliage_debug == 2) return surface.diffuse_energy * transmitted;
    // Match the unit-albedo Lambert NoL convention of the existing direct-light shader.
    float specular = kFoliageWaxCoverage * 3.14159265 * ggx_specular_fp32(surface.normal, surface.eye, light, surface.material.w, foliage_reflectivity);
    return surface.diffuse_energy * (diffuse + transmitted) + vec3(specular);
}

// Backlit thin sheets need a light-facing shadow bias; external occluders still block their light.
float foliage_sun_visibility(vec3 position, vec3 normal, vec3 light, float front_visibility) {
#if defined(OPENRAC_EARLY_LIGHTING) || defined(OPENRAC_PROBE_FAST)
    // Early-lit and cheaper blended draws keep the front visibility for the back too (Enhanced,
    // lossy): a second cascade and hero shadow lookup here held registers for every world pixel.
    return front_visibility;
#else
    if (dot(normal, light) >= 0.) return front_visibility;
    vec3 facing = -normal;
    return min(sun_shadow_visibility(position, facing, facing), hero_shadow_visibility(position, facing));
#endif
}

// Uses the same directional selectors, sunlight override and Forward+ light list as ordinary materials.
vec3 foliage_direct(vec3 position, vec3 geometric, vec4 selector, float visibility, FoliageSurfaceData surface) {
    int a = (int(selector.x + .5) & 15) * 4;
    int b = (int(selector.y + .5) & 15) * 4;
    float blend = clamp(selector.z, 0., 1.);
    vec3 result = vec3(0.);
    for (int i = 0; i < 2; ++i) {
        int offset = i * 2;
        vec3 travel = mix(light_sets[a + offset + 1].xyz, light_sets[b + offset + 1].xyz, blend);
        if (i == 0 && probe_sunlight_direction.w != 0.) travel = probe_sunlight_direction.xyz;
        vec3 light = probe_unit(-travel);
        vec3 radiance = max(mix(light_sets[a + offset].rgb, light_sets[b + offset].rgb, blend), vec3(0.));
        if (i == 0) {
            radiance *= probe_sunlight_scale * foliage_sun_visibility(position, geometric, light, visibility);
        }
        result += radiance * foliage_response(surface, light);
    }
    LOCAL_LIGHT_LOOP(i) {
        vec4 point = LOCAL_POSITION(i);
        vec3 delta = point.xyz - position;
        float distance = length(delta);
        if (point.w <= 0. || distance >= point.w || distance <= 1e-6) continue;
        vec3 light = delta / distance;
        vec3 radiance = max(LOCAL_COLOUR(i).rgb, vec3(0.))
            * LOCAL_ATTENUATION(distance, point.w) * LOCAL_CONE(i, light);
        result += radiance * foliage_response(surface, light);
    }
    return result;
}

// Front GI retains its own AO. Direct backside transmission never receives front-side AO or emission.
vec3 foliage_surface(vec3 position, vec3 normal, vec3 eye, vec3 albedo, vec3 indirect,
                     vec4 selector, float visibility, vec4 material, vec2 uv) {
    vec3 shaped = foliage_normal(normal, position, uv, material.z);
    if (foliage_debug == 1) return vec3(.1, 1., .15);
    if (foliage_debug == 3) return shaped * .5 + .5;
    eye = probe_unit(eye);
    FoliageSurfaceData surface = prepare_foliage(shaped, eye, albedo, material);
    vec3 reflected = surface.albedo * ((1. - material.x) * surface.diffuse_energy) * indirect / 3.14159265;
    reflected += kFoliageWaxCoverage * gi_specular_fallback(indirect, max(dot(shaped, eye), 0.), foliage_reflectivity);
    vec3 direct = vec3(0.);
    if (probe_mode != 2) {
        direct = foliage_direct(position, normal, selector, visibility, surface);
    }
    if (foliage_debug == 2) return direct;
    return direct + (probe_mode == 3 ? vec3(0.) : reflected);
}
#endif
