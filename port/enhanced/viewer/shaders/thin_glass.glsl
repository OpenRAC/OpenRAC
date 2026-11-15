// SPDX-License-Identifier: GPL-3.0-or-later
uniform vec4 object_glass_transmission; // Linear tint and clear transmission fraction.
// Glass changes the optical response, while the original cutout mask still defines coverage.
float thin_glass_coverage() {
    float coverage = v_colour.a * material_colour.a;
    if (cutout != 0) {
        coverage *= sample_upscaled_texture_colours(v_uv).a;
        if (coverage < .02) discard;
    }
    return clamp(coverage, 0., 1.);
}
vec4 thin_glass_surface() {
    vec3 eye_view = -(view * vec4(v_world_position, 1.)).xyz;
    vec3 eye = probe_unit(transpose(mat3(view)) * eye_view);
    vec3 n = probe_unit(v_normal);
    if (dot(n, eye) < 0.) n = -n;
    float f0 = object_reflection_material.y;
    float fresnel = dielectric_fresnel(dot(n, eye), f0);
    // The framebuffer supplies the clear transmitted scene via the destination blend factor.
    float reflected_weight = fresnel*object_reflection_tint.a;
    float tint_weight = (1.-object_glass_transmission.a)*(1.-reflected_weight);
    float opacity = reflected_weight+tint_weight;
    vec3 reflection = environment_specular(v_world_position,n,eye,object_reflection_material.x,f0,
                                            vec3(.11,.19,.25)*fresnel);
    reflection *= object_reflection_material.z*object_reflection_tint.rgb*object_reflection_tint.a;
    // Keep bright reflections through fog and premultiplication, before framebuffer clamping.
    return vec4(probe_display_unclamped(
        (reflection+object_glass_transmission.rgb*tint_weight)/max(opacity,1e-5)),opacity);
}
