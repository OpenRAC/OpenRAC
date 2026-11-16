// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests scene::key_gap (scene/moby_pose.h): how far apart two frames of a class pose its joints,
// read from the game's memory as the frames are laid out (a 0x10-byte header, then 8 bytes of
// quaternion a joint and the translation records).

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "scene/moby_pose.h"
#include "tests/check.h"

namespace {

constexpr std::uint32_t kClass = 0x1000;
constexpr std::uint32_t kRest = 0x3800;
constexpr std::uint32_t kStill = 0x2000, kTurned = 0x2100, kBare = 0x2200, kFlipped = 0x2300;
constexpr std::int16_t kOne = 0x7FFF;

template <typename T>
void put(std::vector<std::uint8_t>& ram, std::uint32_t at, T value) {
    std::memcpy(ram.data() + at, &value, sizeof value);
}

// A frame of two joints, their quaternions `q`, and a translation of joint 0 by `x` when `moved`.
void frame(
    std::vector<std::uint8_t>& ram,
    std::uint32_t at,
    const std::array<std::array<std::int16_t, 4>, 2>& q,
    bool moved,
    std::int16_t x
) {
    const std::uint16_t quat_bytes = 16, trans_count = moved ? 1 : 0;
    put<float>(ram, at, 0.25f);
    put<std::uint16_t>(
        ram, at + 6, static_cast<std::uint16_t>((quat_bytes + 8 * trans_count + 15) >> 4)
    );
    put<std::uint16_t>(ram, at + 8, quat_bytes);
    put<std::uint16_t>(ram, at + 0xC, quat_bytes);
    put<std::uint16_t>(ram, at + 0xE, trans_count);
    for (std::uint32_t j = 0; j < 2; ++j) {
        for (std::uint32_t k = 0; k < 4; ++k) {
            put<std::int16_t>(ram, at + 0x10 + 8 * j + 2 * k, q[j][k]);
        }
    }
    if (moved) {
        put<std::int16_t>(ram, at + 0x10 + quat_bytes, x);
    }
}

}  // namespace

int main() {
    std::vector<std::uint8_t> ram(0x4000, 0);
    ram[kClass + 0x08] = 2;
    put<std::uint32_t>(ram, kClass + 0x18, kRest);  // rest translations: joint 0 at x 4
    put<float>(ram, kRest, 4.0f);
    const std::array<std::int16_t, 4> rest{0, 0, 0, kOne};
    const std::array<std::int16_t, 4> quarter{0, 0, 23170, 23170};  // 90 degrees about z
    const std::array<std::int16_t, 4> back{0, 0, 0, -kOne};         // rest, negated
    frame(ram, kStill, {rest, rest}, true, 10);
    frame(ram, kTurned, {rest, quarter}, true, 40);
    frame(ram, kBare, {rest, rest}, false, 0);
    frame(ram, kFlipped, {back, rest}, true, 10);

    // One joint turned a quarter and joint 0 moved 30 units.
    auto gap = openrac::scene::key_gap(ram, kClass, kStill, kTurned);
    CHECK(gap && std::fabs(gap->turn - 1.5707964f) < 1e-3f && gap->move == 30.0f);
    // Either order alike; a frame against itself is no gap.
    gap = openrac::scene::key_gap(ram, kClass, kTurned, kStill);
    CHECK(gap && std::fabs(gap->turn - 1.5707964f) < 1e-3f && gap->move == 30.0f);
    gap = openrac::scene::key_gap(ram, kClass, kStill, kStill);
    CHECK(gap && gap->turn == 0.0f && gap->move == 0.0f);
    // q and -q are one turn.
    gap = openrac::scene::key_gap(ram, kClass, kStill, kFlipped);
    CHECK(gap && gap->turn < 1e-3f && gap->move == 0.0f);
    // A frame that does not translate joint 0 leaves it at rest (x 4).
    gap = openrac::scene::key_gap(ram, kClass, kStill, kBare);
    CHECK(gap && gap->turn == 0.0f && gap->move == 6.0f);
    // No frame there (no quaternions), or no class: none.
    CHECK(!openrac::scene::key_gap(ram, kClass, kStill, 0x3000));
    CHECK(!openrac::scene::key_gap(ram, 0, kStill, kTurned));
    return openrac::test::result();
}
