// SPDX-License-Identifier: GPL-3.0-or-later
// Soft anisotropic GGX sheen and a bounded forward/wrap scattering approximation for short fur.
// Microfacet equations: https://google.github.io/filament/Filament.html#materialsystem/anisotropicmodel
// Original implementation; the denominators, directions and multi-light sum remain FP32.
uniform int hero_fur_lighting_enabled;
uniform vec4 hero_fur_lighting_parameters; // Highlight strength, softness, directionality, scattering.
uniform float hero_fur_environment_strength;

float fur_highlight(FurSurfaceData surface, vec3 eye, vec3 light) {
    vec3 n = surface.normal, t = surface.tangent;
    float nv = max(dot(n, eye), 0.), nl = max(dot(n, light), 0.);
    vec3 sum = eye + light;
    if (nv <= 0. || nl <= 0. || dot(sum, sum) < 1e-8) return 0.;
    vec3 h = normalize(sum), b = cross(n, t);
    float roughness = hero_fur_lighting_parameters.y;
    float anisotropy = hero_fur_lighting_parameters.z;
    // Broad lobes cannot collapse into sharp, unstable highlights at glancing angles.
    vec2 axes = clamp(roughness * roughness * vec2(1. - anisotropy, 1. + anisotropy), vec2(.12), vec2(1.));
    vec2 ht = vec2(dot(h, t), dot(h, b)) / axes;
    float nh = max(dot(n, h), 0.);
    float denominator = dot(ht, ht) + nh * nh;
    float distribution = 1. / max(3.14159265 * axes.x * axes.y * denominator * denominator, 1e-6);
    vec2 vt = vec2(dot(eye, t), dot(eye, b)) * axes;
    vec2 lt = vec2(dot(light, t), dot(light, b)) * axes;
    float visibility = .5 / max(nl * sqrt(dot(vt, vt) + nv * nv) + nv * sqrt(dot(lt, lt) + nl * nl), 1e-6);
    mp_float f = mp_float(1.) - mp_float(clamp(dot(eye, h), 0., 1.));
    mp_float fresnel = mp_float(.04) + mp_float(.96) * f * f * f * f * f;
    return distribution * visibility * float(fresnel) * nl;
}

float fur_scattering(vec3 n, vec3 eye, vec3 light) {
    // Forward scattering is strongest where light passes toward the eye through the coat's edge.
    mp_float edge = mp_float(1.) - mp_float(clamp(dot(n, eye), 0., 1.));
    mp_float forward = mp_float(max(-dot(eye, light), 0.));
    mp_float wrap = mp_float(clamp((dot(n, light) + .35) / 1.8225, 0., 1.));
    return float(wrap * edge * edge * (mp_float(.25) + mp_float(.75) * forward * forward * forward * forward));
}

vec3 fur_light_response(FurSurfaceData surface, vec3 eye, vec3 light, vec3 albedo) {
    if (dot(light, light) < .5) return vec3(0.);
    float sheen = fur_highlight(surface, eye, light) * hero_fur_lighting_parameters.x;
    float scatter = fur_scattering(surface.normal, eye, light) * hero_fur_lighting_parameters.w;
    // Dielectric highlights stay mostly neutral; absorbed/scattered light keeps the fur's colour.
    mp_vec3 colour = mp_vec3(clamp(albedo, vec3(0.), vec3(1.)));
    mp_vec3 sheen_tint = mix(mp_vec3(1.), colour, mp_float(.15));
    mp_vec3 scatter_tint = colour * (mp_vec3(.5) + colour * mp_float(.5));
    return vec3(sheen_tint) * sheen + vec3(scatter_tint) * scatter;
}

vec3 fur_direct_lighting(FurSurfaceData surface, vec3 p, vec4 selector, float visibility, vec3 albedo) {
    // Eye direction is supplied separately from positions; point-light subtraction stays wide.
    vec3 eye = surface.eye;
    if (hero_fur_lighting_enabled == 0 || surface.coverage <= 0. || dot(eye, eye) < 1e-12) return vec3(0.);
    eye = normalize(eye);
    int a = (int(selector.x + .5) & 15) * 4, b = (int(selector.y + .5) & 15) * 4;
    float blend = clamp(selector.z, 0., 1.);
    vec3 result = vec3(0.);
    for (int light = 0; light < 2; ++light) {
        int offset = light * 2;
        vec3 colour = max(mix(light_sets[a + offset].rgb, light_sets[b + offset].rgb, blend), vec3(0.));
        vec3 direction = probe_unit(mix(light_sets[a + offset + 1].xyz, light_sets[b + offset + 1].xyz, blend));
        if (light == 0 && probe_sunlight_direction.w != 0.) direction = probe_sunlight_direction.xyz;
        vec3 correction = light == 0 ? probe_sunlight_scale : vec3(1.);
        // Added sheen/scattering has no legacy 10% floor: occluded directional lights cannot glow.
        result += colour * correction * clamp(visibility, 0., 1.) * fur_light_response(surface, eye, -direction, albedo);
    }
    LOCAL_LIGHT_LOOP(i) {
        vec3 delta = LOCAL_POSITION(i).xyz - p;
        float radius = LOCAL_POSITION(i).w, distance = length(delta);
        if (radius <= 0. || distance >= radius || distance <= 1e-6) continue;
        result += max(LOCAL_COLOUR(i).rgb, vec3(0.)) * LOCAL_ATTENUATION(distance,radius) * LOCAL_CONE(i,delta/distance)
            * fur_light_response(surface, eye, delta / distance, albedo);
    }
    return result * surface.coverage;
}

vec3 fur_indirect_reflection(FurSurfaceData surface, vec3 irradiance) {
    vec3 eye = surface.eye;
    if (dot(eye, eye) < 1e-12) return vec3(0.);
    // With no reflection capture, retain the dielectric response at every view angle, not just the rim.
    mp_float edge = mp_float(1.) - mp_float(clamp(dot(surface.normal, normalize(eye)), 0., 1.));
    mp_float fresnel = mp_float(.04) + mp_float(.96) * edge * edge * edge * edge * edge;
    return max(irradiance, vec3(0.)) * (float(fresnel) / 3.14159265);
}

vec3 fur_indirect_scattering(FurSurfaceData surface, vec3 irradiance, vec3 albedo) {
    if (dot(surface.eye, surface.eye) < 1e-12) return vec3(0.);
    mp_float edge = mp_float(1.) - mp_float(clamp(dot(surface.normal, normalize(surface.eye)), 0., 1.));
    mp_vec3 colour = mp_vec3(clamp(albedo, vec3(0.), vec3(1.)));
    mp_vec3 tint = colour * (mp_vec3(.5) + colour * mp_float(.5));
    // The surrounding hemisphere illuminates the whole coat; grazing views receive a stronger wrap.
    mp_float wrap = mp_float(.5) * (mp_float(.35) + mp_float(.65) * edge * edge);
    return max(irradiance, vec3(0.)) / 3.14159265 * vec3(tint * wrap * mp_float(hero_fur_lighting_parameters.w));
}

vec3 fur_indirect_lighting(FurSurfaceData surface, vec3 irradiance, vec3 albedo) {
    if (hero_fur_lighting_enabled == 0 || surface.coverage <= 0. || hero_fur_environment_strength <= 0.) return vec3(0.);
    vec3 reflection = fur_indirect_reflection(surface, irradiance) * hero_fur_lighting_parameters.x;
    vec3 scatter = fur_indirect_scattering(surface, irradiance, albedo);
    return (reflection + scatter) * (surface.coverage * hero_fur_environment_strength);
}
