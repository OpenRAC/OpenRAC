// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Short shell slices share deterministic strand coverage between colour and motion passes.
// openrac_mode: hero_fur_shell_enabled
uniform int hero_fur_shell_enabled; // Zero compiles the coat out of every draw.
uniform float hero_fur_shell;
uniform float hero_fur_shell_distance;
uniform vec4 hero_fur_shell_parameters; // Length, UV density, fade start/end in scene units.

float fur_shell_extent(float wetness) {
    float fade = 1. - smoothstep(hero_fur_shell_parameters.z, hero_fur_shell_parameters.w, hero_fur_shell_distance);
    return hero_fur_shell * hero_fur_shell_parameters.x * fade
        * mix(1., .25, clamp(wetness, 0., 1.));
}

#ifdef OPENRAC_FUR_FRAGMENT
bool fur_shell_reject(sampler2D groom, vec2 uv, int eligible) {
    // Derivatives precede coverage decisions, including the verified atlas boundary.
    vec2 dx = dFdx(uv), dy = dFdy(uv);
    if (hero_fur_shell_enabled == 0 || hero_fur_shell <= 0.) return false;
    vec3 map = textureGrad(groom, uv, dx, dy).rgb;
    if (eligible == 0 || map.r < .98) return true;
    vec2 flow = map.gb * 2. - 1.;
    vec2 at = uv * hero_fur_shell_parameters.y - flow * hero_fur_shell * .65;
    vec2 cell = floor(at);
    // Fixed UV roots stay attached to the skinned mesh, with different hair lengths per root.
    float seed = fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
    float fade = 1. - smoothstep(hero_fur_shell_parameters.z, hero_fur_shell_parameters.w, hero_fur_shell_distance);
    if (seed >= fade) return true;
    float height = .35 + .65 * seed;
    vec2 centre = vec2(.35 + .3 * seed, .35 + .3 * fract(seed * 17.));
    vec2 delta = fract(at) - centre;
    float radius = .38 * sqrt(max(0., 1. - hero_fur_shell / height));
    return hero_fur_shell >= height || dot(delta, delta) > radius * radius;
}
#endif
