// SPDX-License-Identifier: GPL-3.0-or-later
// Bounded lighting arithmetic uses the renderer's capability-gated FP16 aliases.
// Small medium coefficients and integrated radiance never pass through these helpers.
bool atmosphere_half_colour(vec3 colour) {
    // Stay within normal half range; retain very dim and unusually bright light values in FP32.
    return all(lessThanEqual(abs(colour),vec3(4096.)))
        && all(bvec3(colour.x==0. || abs(colour.x)>=.00006103515625,
                     colour.y==0. || abs(colour.y)>=.00006103515625,
                     colour.z==0. || abs(colour.z)>=.00006103515625));
}
float atmosphere_phase(float cosine,float anisotropy) {
    mp_float g=mp_float(anisotropy);
    mp_float c=mp_float(clamp(cosine,-1.,1.));
    mp_float denominator=max(mp_float(.01),mp_float(1)+g*g-mp_float(2)*g*c);
    return float(min(mp_float(4),(mp_float(1)-g*g)/pow(denominator,mp_float(1.5))));
}
vec3 atmosphere_illumination(vec3 sun,float phase,float visibility,float strength) {
#ifdef OPENRAC_USE_FP16
    if(!atmosphere_half_colour(sun)) {
        // Avoid mix lowering to 1+(lit-1)*strength, which cancels tiny lit values.
        precise vec3 full=vec3(1.-strength)+sun*(phase*visibility*strength);
        return full;
    }
#endif
    mp_vec3 lit=mp_vec3(sun)*mp_float(phase)*mp_float(visibility);
    mp_float weight=mp_float(strength);
    precise mp_vec3 result=mp_vec3(mp_float(1)-weight)+lit*weight;
    return vec3(result);
}
vec3 atmosphere_point_colour(vec3 colour,float attenuation) {
    colour=max(colour,vec3(0));
#ifdef OPENRAC_USE_FP16
    if(!atmosphere_half_colour(colour)) return colour*attenuation;
#endif
    return vec3(mp_vec3(colour)*mp_float(attenuation));
}
