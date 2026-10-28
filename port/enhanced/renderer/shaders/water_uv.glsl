// SPDX-License-Identifier: GPL-3.0-or-later
// Original scrolling-layer phase convention, shared by coarse animation and fallback vertices.
vec2 water_authored_uv(vec3 p, vec2 uv, vec4 motion) {
    uint seed = ((floatBitsToUint(p.x) + floatBitsToUint(p.y)) >> 16u) & 255u;
    uint phase = (((seed << 6u) + uint(motion.w)) << 3u) & 4095u;
    float angle = float(phase) * (6.2831855 / 4096.0) - 3.1415927;
    return uv + motion.xy + vec2(sin(angle), cos(angle)) * motion.z;
}
