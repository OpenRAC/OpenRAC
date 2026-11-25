// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GPU-driven indirect draw kernels shared by the Vulkan renderer and build recipes.

#pragma once

namespace openrac::shader_sources::renderer_vulkan_draw_gpu_draw {
inline constexpr const char* kGpuCullSource = R"GLSL(#version 450 core

layout(local_size_x = 64) in;

layout(std430, binding = 0) readonly buffer InputWords {

    uint src[];

};

layout(std430, binding = 1) writeonly buffer OutputWords {

    uint dst[];

};

layout(std430, binding = 2) buffer DrawWords {

    uint args[];

};

layout(std430, binding = 3) readonly buffer ResidentWords {

    uint res[];

};

// Word `at` of a candidate: from the resident words for a resident group, else the page's.

uint candidate(bool resident, uint at) {

    return resident ? res[at] : src[at];

}

shared uint prefix[64];

shared uint total;

#ifdef SUMMARIZE

shared uvec4 sums[64];

uvec4 addCounters(uvec4 a, uvec4 b) {

    uint low = a.z + b.z;

    return uvec4(a.xy + b.xy, low, a.w + b.w + uint(low < a.z));

}

#endif

void main() {

#ifdef SUMMARIZE

    uint lane = gl_LocalInvocationID.x;

    uvec4 sum = uvec4(0);

    for (uint group = lane; group < src[0]; group += 64) {

        sum.x += args[group];

    }

    for (uint drawID = lane; drawID < src[1]; drawID += 64) {

        uint at = 4096 + drawID * 5;

        uint count = args[at], instances = args[at + 1], high, low;

        umulExtended(count / 3, instances, high, low);

        sum = addCounters(sum, uvec4(0, uint(count != 0 && instances != 0), low, high));

    }

    sums[lane] = sum;

    barrier();

    for (uint step = 32; step > 0; step /= 2) {

        if (lane < step) {

            sums[lane] = addCounters(sums[lane], sums[lane + step]);

        }

        barrier();

    }

    if (lane == 0) {

        for (uint i = 0; i < 4; ++i) {

            args[SUMMARY_AT + i] = sums[0][i];

        }

    }

#elif defined(MAKE_ARGUMENTS)

    uint drawID = gl_GlobalInvocationID.x;

    if (drawID >= src[1]) {

        return;

    }

    uint record = src[2] + drawID * 4;

    uint group = src[record + 2];

    uint outBase = src[4 + group * 4 + 1];

    uint at = 4096 + drawID * 5;

    args[at] = src[record];

    args[at + 1] = args[group];

    args[at + 2] = src[record + 1];

    args[at + 3] = 0;

    args[at + 4] = outBase;

#else

    uint group = gl_WorkGroupID.x, lane = gl_LocalInvocationID.x;

    if (group >= src[0]) {

        return;

    }

    uint record = 4 + group * 4;

    uint begin = src[record], outputBase = src[record + 1], count = src[record + 2],

         view = src[record + 3];

    bool resident = (begin & 0x80000000u) != 0u;

    begin &= 0x7fffffffu;

    if (lane == 0) {

        total = 0;

    }

    barrier();

    for (uint chunk = 0; chunk < count; chunk += 64) {

        uint index = chunk + lane, base = begin + index * 38;

        bool visible = index < count;

        if (visible) {

            uint flags = candidate(resident, base + 7), word = candidate(resident, base + 6);

            if ((flags & 2) == 0 || (src[view + 57] & 2) == 0) {

                uint byte = word >> 8, bit = word & 255;

                if ((src[view + 57] & 1) != 0 && byte < 128 && bit != 0 && (bit & (bit - 1)) == 0) {

                    uint mask = (src[view + 24 + byte / 4] >> ((byte % 4) * 8)) & 255;

                    visible = (mask & bit) != 0;

                }

                if (visible && (flags & 1) != 0) {

                    vec3 low = vec3(

                        uintBitsToFloat(candidate(resident, base + 0)),

                        uintBitsToFloat(candidate(resident, base + 1)),

                        uintBitsToFloat(candidate(resident, base + 2))

                    );

                    vec3 high = vec3(

                        uintBitsToFloat(candidate(resident, base + 3)),

                        uintBitsToFloat(candidate(resident, base + 4)),

                        uintBitsToFloat(candidate(resident, base + 5))

                    );

                    for (uint plane = 0; plane < src[view + 56]; ++plane) {

                        vec4 p = uintBitsToFloat(uvec4(

                            src[view + plane * 4],

                            src[view + plane * 4 + 1],

                            src[view + plane * 4 + 2],

                            src[view + plane * 4 + 3]

                        ));

                        vec3 v = mix(low, high, greaterThanEqual(p.xyz, vec3(0)));

                        precise float distance = ((p.x * v.x + p.y * v.y) + p.z * v.z) + p.w;

                        if (distance < 0) {

                            visible = false;

                        }

                    }

                }

            }

        }

        prefix[lane] = visible ? 1 : 0;

        barrier();

        for (uint step = 1; step < 64; step *= 2) {

            uint add = lane >= step ? prefix[lane - step] : 0;

            barrier();

            prefix[lane] += add;

            barrier();

        }

        if (visible) {

            uint target = (outputBase + total + prefix[lane] - 1) * 30;

            for (uint w = 0; w < 30; ++w) {

                dst[target + w] = candidate(resident, base + 8 + w);

            }

        }

        barrier();

        if (lane == 0) {

            total += prefix[63];

        }

        barrier();

    }

    if (lane == 0) {

        args[group] = total;

    }

#endif

}

)GLSL";
}
