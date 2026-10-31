// SPDX-License-Identifier: GPL-3.0-or-later
// Common finite-segment integration. Extinction, coordinates and accumulation remain FP32.
vec4 atmosphere_step(vec4 accumulated, vec3 source, float extinction, float distance) {
    float optical_depth=max(0.,extinction*distance);
    float transmission=exp(-optical_depth);
    // Series near zero avoids cancellation in 1-exp(-x).
    float integral=distance*(optical_depth<.001
        ? 1.-optical_depth*.5+optical_depth*optical_depth/6.
        : (1.-transmission)/optical_depth);
    return vec4(accumulated.rgb+accumulated.a*source*integral,
                accumulated.a*transmission);
}
