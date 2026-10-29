// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OPENRAC_SHADING_BRDF
#define OPENRAC_SHADING_BRDF

float dielectric_fresnel(float cosine, float f0) {
    // Bounded Schlick term only; GGX denominators and grazing-angle visibility stay FP32.
    mp_float f = mp_float(1) - clamp(mp_float(cosine), mp_float(0), mp_float(1));
    mp_float base = mp_float(f0);
    return float(base + (mp_float(1) - base) * f * f * f * f * f);
}

// Broad indirect reflection when no local radiance capture covers the receiver.
// Irradiance is already cosine-filtered: E/pi supplies local colour, not sharp reflected detail.
vec3 gi_specular_fallback(vec3 irradiance,float cosine,float f0) {
    return max(irradiance,vec3(0))/3.14159265359
        *dielectric_fresnel(cosine,clamp(f0,0.,1.));
}

// GGX distribution and height-correlated Smith visibility, multiplied by NoL.
// Directions and the normal are unit vectors; roughness is perceptual roughness.
float ggx_specular_fp32(vec3 n, vec3 eye, vec3 light, float roughness, float f0) {
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
    float f = 1. - clamp(dot(eye, half_direction), 0., 1.);
    float fresnel = f0 + (1. - f0) * f * f * f * f * f;
    return distribution * visibility * fresnel * nl;
}

// GGX distribution and height-correlated Smith visibility, multiplied by NoL.
// Directions and the normal are unit vectors; roughness is perceptual roughness.
float ggx_specular(vec3 n, vec3 eye, vec3 light, float roughness, float f0) {
#ifdef OPENRAC_USE_FP16
    mp_vec3 hn=mp_vec3(n), hv=mp_vec3(eye), hl=mp_vec3(light);
    mp_float nv=clamp(dot(hn,hv),mp_float(0),mp_float(1));
    mp_float nl=clamp(dot(hn,hl),mp_float(0),mp_float(1));
    vec3 sum=eye+light;
    if(nv<=mp_float(0)||nl<=mp_float(0)||dot(sum,sum)<1e-8) return 0.;
    vec3 half_direction=normalize(sum);
    mp_float nh=clamp(dot(hn,mp_vec3(half_direction)),mp_float(0),mp_float(1));
    // Keep roughness^4 and the sharp distribution denominator wide to avoid zero/overflow.
    float a=roughness*roughness,a2=a*a;
    float denominator=float(nh)*float(nh)*(a2-1.)+1.;
    float distribution=a2/(3.14159265*denominator*denominator);
    mp_vec2 cosines=mp_vec2(nv,nl);
    mp_vec2 smith=sqrt(max(cosines*cosines*(mp_float(1)-mp_float(a2))+mp_vec2(a2),mp_vec2(0)));
    float visibility=.5/max(float(nl*smith.x+nv*smith.y),1e-6);
    return distribution*visibility*dielectric_fresnel(float(dot(hv,mp_vec3(half_direction))),f0)*float(nl);
#else
    return ggx_specular_fp32(n, eye, light, roughness, f0);
#endif
}
#endif
