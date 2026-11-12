// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// MobyProc's distance test and fade (moby_proc.h), shared by Ratchet & Clank and Going Commando.
// Going Commando's MobyProc (USA v1.01 boot 0x2A4E68; each level's copy, level 1's at 0x30DB30)
// is the same code at 0x2A5228..0x2A53B0 as Ratchet & Clank's below: the sphere less the eye times
// 1024, the depth on the view matrix's third column, the near and far tests on (1, -1) times the
// radius, min(+0x32, cap) << 10 by pminw and dsll32, the culls on the lanes' signs and the fade by
// vftoi0, dsrl32 7 and pminw 0x80.

#include "game/common/moby_proc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "common/ps2_float.h"

namespace openrac::host {

namespace {

// How far Draw distance: Unlimited reaches, in units: farther than any level is wide.
constexpr float kUnlimitedReach = 4096.0f;

// MobyProc's distance tests and fade, instruction for instruction (Ratchet & Clank's NTSC-U boot
// 0x211B60..0x211D20; each level's copy, level 1's at 0x26AAF8, is the same code), on the VU's own
// arithmetic (common/ps2_float.h: every result cut toward zero). Its inputs, as it loads them: the camera's
// position (camera block +0x140) times 1024, the view matrix's rows (block +0x00..+0x20), of which
// the depth takes the third column, the near plane (view context +0xA0) and the global cap on
// the draw distance. Per moby, with the sphere (x y z r) at +0x00 and the draw distance at +0x32:
//
//   v      = sphere.xyz - eye * 1024                      vsub vf2, vf1, vf24
//   z      = (a.x * v.x + a.y * v.y) + a.z * v.z          vmula / vmadda / vmadd (a: the column)
//   near   = n - (r + z)                                  vf11 = (1, -1); vaddbc; vsub
//   far    = (itof(min(dd, cap) << 10) - r) - (-r + z)    pminw; dsll32; vitof0; vsubbc; vsub
//   culled when far's sign bit is set or near's is clear  qmfc2; bltz / bgez on the lanes
//   fade   = min(ftoi0(far) >> 7, 0x80)                   vftoi0; dsrl32 7; pminw 0x80
//
// So the fade runs over the last 16 units of the draw distance (0x80 << 7 = 16384 = 16 << 10).
// The moby's vertex alpha is fade * alpha (+0x23) >> 7, and below 0x80 it is drawn with the fading
// TEST_1 (slti 0x80; sll 3 into the mode bits).
std::optional<std::uint8_t> moby_proc_distance(
    const std::uint8_t* moby, const renderer::Vec3& eye, const GameState::MobyView& view
) {
    using namespace openrac::ps2;
    if (view.scale != 1.0f) {
        // Draw distance other than Original (settings.h): the same tests in the host's floats with
        // the reach scaled (none when Unlimited), and the fade over the scaled reach's last 16
        // units.
        float s[4];
        std::memcpy(s, moby, sizeof(s));
        std::int16_t dd;
        std::memcpy(&dd, moby + 0x32, sizeof(dd));
        const float z = view.depth_axis[0] * (s[0] - eye[0] * 1024.0f)
                        + view.depth_axis[1] * (s[1] - eye[1] * 1024.0f)
                        + view.depth_axis[2] * (s[2] - eye[2] * 1024.0f);
        if (view.near_plane - (s[3] + z) >= 0.0f
            || std::min<std::int32_t>(dd, view.draw_cap) <= 0) {
            return std::nullopt;
        }
        if (view.scale <= 0.0f) {
            // Unlimited still stops at kUnlimitedReach, past any level: Pokitaru's class 1161
            // mobys turned by -5 pi have spheres about 9000 units out, which the game's own draw
            // distance culls; drawn, they covered the sky in cyan.
            if (z - s[3] > kUnlimitedReach * 1024.0f) {
                return std::nullopt;
            }
            return std::uint8_t{0x80};
        }
        const float reach =
            static_cast<float>(std::min<std::int32_t>(dd, view.draw_cap)) * 1024.0f * view.scale;
        const float far = reach - z;
        if (far < 0.0f) {
            return std::nullopt;
        }
        return static_cast<std::uint8_t>(std::min(static_cast<long>(far) >> 7, 0x80L));
    }
    u32 sphere[4];
    std::memcpy(sphere, moby, sizeof(sphere));
    std::int16_t distance;
    std::memcpy(&distance, moby + 0x32, sizeof(distance));
    const u32 units = bits(1024.0f);
    u32 v[3];
    for (int i = 0; i < 3; ++i) {
        v[i] = sub(sphere[i], mul(bits(eye[i]), units));
    }
    const auto& a = view.depth_axis;
    const u32 z = add(add(mul(bits(a[0]), v[0]), mul(bits(a[1]), v[1])), mul(bits(a[2]), v[2]));
    const u32 r = sphere[3];
    const u32 near_side = add(mul(kOne, r), z);
    const u32 far_side = add(mul(kNegOne, r), z);
    const s32 capped = std::min<s32>(distance, view.draw_cap);
    const u32 reach = sub(itof0(static_cast<s32>(static_cast<u32>(capped) << 10)), r);
    const u32 near_test = sub(bits(view.near_plane), near_side);
    const u32 far_test = sub(reach, far_side);
    if ((far_test & kSign) != 0 || (near_test & kSign) == 0) {
        return std::nullopt;
    }
    const auto fade = static_cast<u32>(ftoi(far_test, 0)) >> 7;
    return static_cast<std::uint8_t>(std::min<u32>(fade, 0x80));
}

// Wholly outside the view's sides, by far: the game's sphere test culls it (the view block's
// clip constants), and the depth alone does not. Pokitaru's two class 1161 mobys turned by -5 pi
// have spheres thousands of units to the side; seen at a right angle to them (the camera at 507
// 411 232 looking -0.474 0.877, a player's report) their depth came within the draw distance and
// the window drew them: rows thousands long, a cyan cover over the sky and the ground. The bound
// is a tangent of 8, wider than any picture's field of view, so only such spheres are culled here
// and every moby the wider pictures show is still drawn.
bool beyond_the_sides(
    const float* sphere, const renderer::Vec3& eye, const renderer::Vec3& forward
) {
    const float to[3] =
        {sphere[0] / 1024.0f - eye[0], sphere[1] / 1024.0f - eye[1], sphere[2] / 1024.0f - eye[2]};
    const float depth = to[0] * forward[0] + to[1] * forward[1] + to[2] * forward[2];
    const float radius = std::fabs(sphere[3]) / 1024.0f;
    constexpr float kWidestTangent = 8.0f;
    const float side2 = to[0] * to[0] + to[1] * to[1] + to[2] * to[2] - depth * depth;
    const float reach = (depth + radius) * kWidestTangent + radius;
    return side2 > reach * reach;
}

}  // namespace

std::optional<std::uint8_t> moby_proc_distance_fade(
    const std::uint8_t* moby, const GameState& state
) {
    float sphere[4];
    std::memcpy(sphere, moby, sizeof(sphere));
    std::int16_t distance;
    std::memcpy(&distance, moby + 0x32, sizeof(distance));
    if (!std::isfinite(sphere[0]) || !std::isfinite(sphere[1]) || !std::isfinite(sphere[2])
        || !std::isfinite(sphere[3])) {
        return std::nullopt;
    }
    // A moby whose sphere was never built (all zero: the moby loop's matrix step never ran on it)
    // is tested as MobyProc tests it, as a point at the origin, which no camera in a level sees
    // within a moby's draw distance. Going Commando's Hrugis Cloud turrets each place two class
    // 0x134F mobys of scale 16 at their arms (func_L10_00417CE0); nothing updates them but their
    // copied rows and position, so the console never draws them. Drawn whole, they showed as
    // giant orange boxes turning with the turrets.
    const auto& eye = state.camera_position;
    if (state.moby_view) {
        const auto pass = moby_proc_distance(moby, eye, *state.moby_view);
        if (!pass) {
            return std::nullopt;
        }
        if (beyond_the_sides(sphere, eye, state.forward)) {
            return std::nullopt;
        }
        return pass;
    }
    // A program whose view block is not known: the same tests in the host's floats.
    const float to[3] =
        {sphere[0] / 1024.0f - eye[0], sphere[1] / 1024.0f - eye[1], sphere[2] / 1024.0f - eye[2]};
    const auto& forward = state.forward;
    const float depth = to[0] * forward[0] + to[1] * forward[1] + to[2] * forward[2];
    const float radius = std::fabs(sphere[3]) / 1024.0f;
    if (depth > static_cast<float>(distance) || depth + radius <= 32.0f / 1024.0f
        || beyond_the_sides(sphere, eye, forward)) {
        return std::nullopt;
    }
    return std::uint8_t{0x80};
}

}  // namespace openrac::host
