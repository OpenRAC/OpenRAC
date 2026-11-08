// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <random>

#include "game/common/profile.h"
#include "game/rac1/game_state.h"
#include "openrac/ps2float.h"
#include "tests/check.h"

namespace {
using namespace openrac;

std::uint32_t bits_of(float f) {
    std::uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

float float_of(std::uint32_t u) {
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

// MobyProc's per-moby distance steps as hostgen's asmc translates them (NTSC-U 0x211B60 on, the
// asm shadow's statements kept in order, lanes and 64-bit sign tests included).
std::optional<std::uint8_t> asmc_moby_proc(
    const std::uint8_t* moby, const renderer::Vec3& eye, const host::GameState::MobyView& view
) {
    float vf1[4], vf2[3], vf3[2], vf24[3], vf19[2];
    std::memcpy(vf1, moby, sizeof(vf1));
    for (int i = 0; i < 3; ++i) {
        vf24[i] = openrac_ps2_mul(eye[i], 1024.0f);
    }
    std::int16_t lh;
    std::memcpy(&lh, moby + 0x32, sizeof(lh));
    // pminw with the cap, dsll32 by 10 (r2 << 42), qmtc2: the y lane is the low word of r2 << 10.
    const std::int32_t lane = std::min<std::int32_t>(lh, view.draw_cap);
    const auto r2 = static_cast<std::uint64_t>(static_cast<std::int64_t>(lane)) << 42;
    const auto vf4y = static_cast<std::uint32_t>(r2 >> 32);
    for (int i = 0; i < 3; ++i) {
        vf2[i] = openrac_ps2_sub(vf1[i], vf24[i]);
    }
    vf3[0] = openrac_ps2_mul(1.0f, vf1[3]);
    vf3[1] = openrac_ps2_mul(-1.0f, vf1[3]);
    vf19[1] = openrac_ps2_itof(static_cast<std::int32_t>(vf4y));
    const auto& a = view.depth_axis;
    float acc = openrac_ps2_mul(a[0], vf2[0]);
    acc = openrac_ps2_add(acc, openrac_ps2_mul(a[1], vf2[1]));
    const float z = openrac_ps2_add(acc, openrac_ps2_mul(a[2], vf2[2]));
    vf19[1] = openrac_ps2_sub(vf19[1], vf1[3]);
    vf19[0] = view.near_plane;
    vf1[0] = openrac_ps2_add(vf3[0], z);
    vf1[1] = openrac_ps2_add(vf3[1], z);
    vf3[0] = openrac_ps2_sub(vf19[0], vf1[0]);
    vf3[1] = openrac_ps2_sub(vf19[1], vf1[1]);
    const std::uint64_t r1 = bits_of(vf3[0]) | (std::uint64_t{bits_of(vf3[1])} << 32);
    if (static_cast<std::int64_t>(r1) < 0) {
        return std::nullopt;
    }
    if (static_cast<std::int64_t>(r1 << 32) >= 0) {
        return std::nullopt;
    }
    vf3[1] = float_of(static_cast<std::uint32_t>(openrac_ps2_ftoi(vf3[1])));
    const std::uint64_t r = (bits_of(vf3[0]) | (std::uint64_t{bits_of(vf3[1])} << 32)) >> 39;
    const auto fade = std::min<std::int32_t>(static_cast<std::int32_t>(r), 0x80);
    return static_cast<std::uint8_t>(fade);
}

void place(std::uint8_t* moby, const float sphere[4], std::int16_t distance) {
    std::memcpy(moby, sphere, 16);
    std::memcpy(moby + 0x32, &distance, sizeof(distance));
}

// The port's replay agrees with the translation on random spheres, cameras and views, the fade's
// last 16 units included.
void matches_the_asm() {
    std::mt19937 rng(1161);
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> spot(-600.0f, 600.0f);
    std::uniform_real_distribution<float> deep(-40.0f, 300.0f);
    std::uniform_int_distribution<int> distance(-8, 400);
    int culled = 0, fading = 0, whole = 0;
    for (int n = 0; n < 200000; ++n) {
        host::GameState state;
        renderer::Vec3 axis{unit(rng), unit(rng), unit(rng)};
        const float length = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
        if (length < 0.1f) {
            continue;
        }
        for (auto& c : axis) {
            c /= length;
        }
        state.camera_position = {spot(rng), spot(rng), spot(rng) * 0.2f};
        state.forward = axis;
        host::GameState::MobyView view;
        view.depth_axis = {axis[0], axis[1], axis[2]};
        view.draw_cap = n % 7 == 0 ? std::uniform_int_distribution<int>(0, 200)(rng) : 0x7FFF;
        view.near_plane = n % 3 == 0 ? 0.0f : 0.1f * 1024.0f;
        state.moby_view = view;
        const auto dd = static_cast<std::int16_t>(distance(rng));
        // Near the fade's edge often: the sphere's far side within the last 16 units.
        const float depth =
            n % 2 == 0 ? deep(rng)
                       : static_cast<float>(dd) - 18.0f + 20.0f * (unit(rng) + 1.0f) * 0.5f;
        const float side = unit(rng) * depth * 0.5f;
        const float radius = std::fabs(unit(rng)) * 6.0f;
        float sphere[4];
        for (int i = 0; i < 3; ++i) {
            const float across = i == 2 ? axis[0] : (i == 0 ? -axis[1] : axis[2]);
            sphere[i] = (state.camera_position[i] + axis[i] * depth + across * side) * 1024.0f;
        }
        sphere[3] = radius * 1024.0f;
        std::uint8_t moby[0x100] = {};
        place(moby, sphere, dd);
        const auto expect = asmc_moby_proc(moby, state.camera_position, view);
        const auto got = game::rac1::moby_distance_fade(moby, state);
        CHECK(expect == got);
        if (!expect) {
            ++culled;
        } else if (*expect < 0x80) {
            ++fading;
        } else {
            ++whole;
        }
    }
    // Every branch was taken many times.
    CHECK(culled > 1000 && fading > 1000 && whole > 1000);
}

// Hand-picked cases: the fade over the last 16 units, the cap, the eye inside the sphere.
void the_last_16_units() {
    host::GameState state;
    state.camera_position = {100.0f, 0.0f, 0.0f};
    state.forward = {1.0f, 0.0f, 0.0f};
    state.moby_view = host::GameState::MobyView{{1.0f, 0.0f, 0.0f}, 0x7FFF, 0.1f * 1024.0f};
    std::uint8_t moby[0x100] = {};
    const auto at = [&](float depth, float r, std::int16_t dd) {
        const float sphere[4] = {(100.0f + depth) * 1024.0f, 0.0f, 0.0f, r * 1024.0f};
        place(moby, sphere, dd);
        return game::rac1::moby_distance_fade(moby, state);
    };
    CHECK(at(40.0f, 1.0f, 64) == std::uint8_t{0x80});  // 22 units short of the edge
    CHECK(at(56.0f, 0.0f, 64) == std::uint8_t{0x40});  // 8 of the last 16 units left
    CHECK(at(63.0f, 0.0f, 64) == std::uint8_t{0x08});
    CHECK(!at(65.0f, 0.0f, 64));                       // past it
    CHECK(at(-0.5f, 1.0f, 64) == std::uint8_t{0x80});  // the eye inside the sphere
    CHECK(!at(-5.0f, 1.0f, 64));                       // behind the eye
    state.moby_view->draw_cap = 48;                    // the cap wins over +0x32
    CHECK(at(40.0f, 0.0f, 64) == std::uint8_t{0x40});
    CHECK(!at(50.0f, 0.0f, 64));
    // Pokitaru's class 1161 turned by -5 pi: thousands of units to the side, culled by the sides.
    // Seen at a right angle (a player's camera): its depth is within the draw distance.
    state.camera_position = {507.0f, 411.0f, 232.0f};
    state.forward = {-0.474f, 0.877f, 0.0f};
    state.moby_view = host::GameState::MobyView{{-0.474f, 0.877f, 0.0f}, 0x7FFF, 0.1f * 1024.0f};
    const float far_side[4] =
        {9286.5f * 1024.0f, 5217.6f * 1024.0f, 235.1f * 1024.0f, 1.08f * 1024.0f};
    place(moby, far_side, 64);
    CHECK(!game::rac1::moby_distance_fade(moby, state));
    // A sphere never built (all zero) is a point at the origin, as MobyProc reads it: Hrugis
    // Cloud's turret arms (class 0x134F, draw distance 0x1FF), seen from the start of the level.
    state.camera_position = {425.0f, 470.0f, 503.0f};
    state.forward = {1.0f, 0.0f, 0.0f};
    state.moby_view = host::GameState::MobyView{{1.0f, 0.0f, 0.0f}, 960, 0.1f * 1024.0f};
    const float none[4] = {};
    place(moby, none, 0x1FF);
    CHECK(!asmc_moby_proc(moby, state.camera_position, *state.moby_view));
    CHECK(!game::rac1::moby_distance_fade(moby, state));
    // Looking back at the origin, it is past the draw distance.
    state.forward = {-0.53f, -0.58f, -0.62f};
    state.moby_view->depth_axis = state.forward;
    CHECK(!asmc_moby_proc(moby, state.camera_position, *state.moby_view));
    CHECK(!game::rac1::moby_distance_fade(moby, state));
}
}  // namespace

int main() {
    matches_the_asm();
    the_last_16_units();
    return openrac::test::result();
}
