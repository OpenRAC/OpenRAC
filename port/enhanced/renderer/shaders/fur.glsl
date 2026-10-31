// SPDX-License-Identifier: GPL-3.0-or-later
// Coverage and groom diagnostics share the same eligibility as the short-fur material.
uniform int hero_fur_debug;
uniform sampler2D hero_fur_map;
flat in int v_hero_fur;

vec3 fur_diagnostic(vec2 uv) {
    vec3 map = texture(hero_fur_map, uv).rgb;
    if (hero_fur_debug == 1) {
        return mix(vec3(.06), vec3(.15, 1., .3), map.r);
    }
    vec2 flow = map.gb * 2. - 1.;
    flow /= max(length(flow), .01);
    vec3 colour = vec3(.5 + .5 * flow, .2);
    // UV-space arrows make direction and mirrored islands readable on the animated surface.
    vec2 cell = mod(uv * vec2(textureSize(hero_fur_map, 0)), 12.) - 6.;
    float along = dot(cell, flow), across = abs(dot(cell, vec2(-flow.y, flow.x)));
    bool arrow = (along > -4. && along < 3. && across < .5)
        || (along > 0. && along < 3. && abs(across - (3. - along)) < .6);
    return mix(vec3(.06), arrow ? vec3(.02) : colour, map.r);
}
