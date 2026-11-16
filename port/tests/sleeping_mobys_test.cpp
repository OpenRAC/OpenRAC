// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests frontend::SleepingMobys (game/common/frontend/sleeping_mobys.h): which mobys the game
// leaves asleep, which sequences loop, and the keys the port poses them with. It only reads the
// game's memory.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

#include "game/common/frontend/sleeping_mobys.h"
#include "tests/check.h"

namespace {

using openrac::frontend::SleepingMobys;

constexpr std::uint32_t kFields = 0x50;
constexpr std::uint32_t kClass = 0x1000;
constexpr std::uint32_t kSeq = 0x2000;
constexpr std::uint32_t kFrames[4] = {0x3000, 0x3100, 0x3200, 0x3300};
constexpr std::uint32_t kAsleep = 0x4000, kAwake = 0x4100, kGrouped = 0x4200, kGroupDrawn = 0x4300;
const std::array<float, 3> kEye{0.0f, 0.0f, 0.0f};

// The test's frames: each a rate (+0) and one pose, a turn (+4); the gap between two is the turn.
std::optional<openrac::scene::KeyGap> fake_gap(
    std::span<const std::uint8_t> ram, std::uint32_t, std::uint32_t a, std::uint32_t b
) {
    float ta = 0.0f, tb = 0.0f;
    std::memcpy(&ta, ram.data() + a + 4, 4);
    std::memcpy(&tb, ram.data() + b + 4, 4);
    return openrac::scene::KeyGap{std::fabs(ta - tb), 0.0f};
}

template <typename T>
void put(std::vector<std::uint8_t>& ram, std::uint32_t at, T value) {
    std::memcpy(ram.data() + at, &value, sizeof value);
}

template <typename T>
T get(const std::vector<std::uint8_t>& ram, std::uint32_t at) {
    T value{};
    std::memcpy(&value, ram.data() + at, sizeof value);
    return value;
}

// A moby of the class at `x` (game units), range 10, not drawn, in no group, on frame 0 of seq 0.
void place(std::vector<std::uint8_t>& ram, std::uint32_t m, float x) {
    put<float>(ram, m + 0x10, x);
    ram[m + 0x21] = 0xFF;
    put<std::uint32_t>(ram, m + 0x24, kClass);
    ram[m + 0x30] = 10;
    ram[m + 0x31] = 0;
    put<std::uint16_t>(ram, m + 0x34, 0);
    ram[m + kFields + 1] = 1;
    put<float>(ram, m + kFields + 8, 1.0f);    // speed
    put<float>(ram, m + kFields + 0xC, 0.5f);  // frame rate: two ticks a frame
    put<std::uint32_t>(ram, m + kFields + 0x18, kFrames[0]);
    put<std::uint32_t>(ram, m + kFields + 0x1C, kFrames[1]);
}

std::vector<std::uint8_t> level() {
    std::vector<std::uint8_t> ram(0x10000, 0);
    ram[kClass + 0x0C] = 1;
    put<std::uint32_t>(ram, kClass + 0x48, kSeq);
    ram[kSeq + 0x10] = 4;
    put<float>(ram, kSeq + 0x18, 0.5f);  // the sequence's rate
    // A loop: turns 0, 1, 2, 1, and back to 0.
    constexpr float kTurns[4] = {0.0f, 1.0f, 2.0f, 1.0f};
    for (std::uint32_t i = 0; i < 4; ++i) {
        put<std::uint32_t>(ram, kSeq + 0x1C + 4 * i, kFrames[i]);
        put<float>(ram, kFrames[i], 0.25f);
        put<float>(ram, kFrames[i] + 4, kTurns[i]);
    }
    place(ram, kAsleep, 100.0f);
    place(ram, kAwake, 0.0f);
    ram[kAwake + 0x31] = 1;
    place(ram, kGrouped, 200.0f);
    ram[kGrouped + 0x21] = 3;
    place(ram, kGroupDrawn, 300.0f);
    ram[kGroupDrawn + 0x21] = 3;
    ram[kGroupDrawn + 0x31] = 1;
    return ram;
}

// The awake moby's game advance: its blend moves, as the world runs.
void run_awake(std::vector<std::uint8_t>& ram) {
    put<float>(ram, kAwake + kFields + 4, get<float>(ram, kAwake + kFields + 4) + 0.01f);
}

}  // namespace

int main() {
    const std::vector<std::uint32_t> mobys{kAsleep, kAwake, kGrouped, kGroupDrawn};

    // The game's own test: drawn, range 255, or the eye within range.
    {
        auto ram = level();
        CHECK(!SleepingMobys::runs_itself(ram.data() + kAsleep, kEye));
        CHECK(SleepingMobys::runs_itself(ram.data() + kAwake, kEye));
        ram[kAsleep + 0x30] = 0xFF;
        CHECK(SleepingMobys::runs_itself(ram.data() + kAsleep, kEye));
        ram[kAsleep + 0x30] = 10;
        CHECK(SleepingMobys::runs_itself(ram.data() + kAsleep, {95.0f, 0.0f, 0.0f}));
    }

    // A sleeping moby loops its sequence at its own speed; the game's memory is never written.
    {
        auto ram = level();
        SleepingMobys sleeping(fake_gap);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(!sleeping.keys(ram, kAsleep));  // no last tick to compare with yet
        const auto before = ram;
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        auto k = sleeping.keys(ram, kAsleep);
        CHECK(k && k->a == kFrames[0] && k->b == kFrames[1] && k->t == 0.5f && k->plain);
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        k = sleeping.keys(ram, kAsleep);
        CHECK(k && k->a == kFrames[1] && k->b == kFrames[2] && k->t == 0.0f);
        // On to the last frame: its next key is the first again.
        for (int i = 0; i < 4; ++i) {
            run_awake(ram);
            sleeping.tick(ram, mobys, kEye, kFields, true);
        }
        k = sleeping.keys(ram, kAsleep);
        CHECK(k && k->a == kFrames[3] && k->b == kFrames[0] && !k->plain);
        CHECK(std::memcmp(ram.data() + kAsleep, before.data() + kAsleep, 0x100) == 0);

        // The awake moby is the game's own, and so is a sleeping one's group mate woken by another.
        CHECK(!sleeping.keys(ram, kAwake));
        CHECK(!sleeping.keys(ram, kGrouped));
        CHECK(!sleeping.keys(ram, kGroupDrawn));

        // A tick the world did not run, or where nothing awake animated: the clock holds.
        sleeping.tick(ram, mobys, kEye, kFields, false);
        CHECK(sleeping.keys(ram, kAsleep)->a == kFrames[3]);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(sleeping.keys(ram, kAsleep)->a == kFrames[3]);

        // The game ran it (its fields changed): its own pose again.
        run_awake(ram);
        put<float>(ram, kAsleep + kFields + 4, 0.25f);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(!sleeping.keys(ram, kAsleep));
    }

    // Mobys the moby loop never runs (mode 2) or never animates (0x40), a blend between two
    // sequences, a still sequence (rate 0) and one frame: their own pose.
    {
        auto ram = level();
        put<std::uint16_t>(ram, kAsleep + 0x34, 0x2);
        put<std::uint16_t>(ram, kGrouped + 0x34, 0x40);
        ram[kGroupDrawn + 0x31] = 0;
        ram[kGroupDrawn + kFields + 3] = 1;
        SleepingMobys sleeping(fake_gap);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(!sleeping.keys(ram, kAsleep));
        CHECK(!sleeping.keys(ram, kGrouped));
        CHECK(!sleeping.keys(ram, kGroupDrawn));

        auto still = level();
        put<float>(still, kAsleep + kFields + 0xC, 0.0f);
        SleepingMobys other(fake_gap);
        other.tick(still, mobys, kEye, kFields, true);
        run_awake(still);
        other.tick(still, mobys, kEye, kFields, true);
        CHECK(!other.keys(still, kAsleep));

        auto single = level();
        single[kSeq + 0x10] = 1;
        SleepingMobys one(fake_gap);
        one.tick(single, mobys, kEye, kFields, true);
        run_awake(single);
        one.tick(single, mobys, kEye, kFields, true);
        CHECK(!one.keys(single, kAsleep));
    }
    // A sequence played once (its last frame far from its first) and one of two frames: the
    // game's pose. Each is looked at once a level: a new level looks again.
    {
        auto ram = level();
        put<float>(ram, kFrames[3] + 4, 3.0f);
        SleepingMobys sleeping(fake_gap);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(!sleeping.keys(ram, kAsleep));
        put<float>(ram, kFrames[3] + 4, 1.5f);  // within one and a half steps of the first
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(!sleeping.keys(ram, kAsleep));
        sleeping.reset();
        sleeping.tick(ram, mobys, kEye, kFields, true);
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        CHECK(sleeping.keys(ram, kAsleep).has_value());

        auto two = level();
        two[kSeq + 0x10] = 2;
        SleepingMobys swing(fake_gap);
        swing.tick(two, mobys, kEye, kFields, true);
        run_awake(two);
        swing.tick(two, mobys, kEye, kFields, true);
        CHECK(!swing.keys(two, kAsleep));
    }

    // Each frame's own rate (the sequence's 0), as the game carries t into it; t near 1 is 1.
    {
        auto ram = level();
        put<float>(ram, kSeq + 0x18, 0.0f);
        put<float>(ram, kFrames[1], 0.125f);
        put<float>(ram, kAsleep + kFields + 4, 0.75f);
        put<float>(ram, kAsleep + kFields + 0xC, 0.3f);  // 0.75 + 0.3: a step 0.05 past frame 1
        SleepingMobys sleeping(fake_gap);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        run_awake(ram);
        sleeping.tick(ram, mobys, kEye, kFields, true);
        auto k = sleeping.keys(ram, kAsleep);
        CHECK(k && k->a == kFrames[1] && std::fabs(k->t - 0.05f / 0.3f * 0.125f) < 1e-6f);

        auto snap = level();
        put<float>(snap, kAsleep + kFields + 4, 0.498f);  // 0.498 + 0.5: within 1/256 of 1
        SleepingMobys near(fake_gap);
        near.tick(snap, mobys, kEye, kFields, true);
        run_awake(snap);
        near.tick(snap, mobys, kEye, kFields, true);
        k = near.keys(snap, kAsleep);
        CHECK(k && k->a == kFrames[1] && k->t == 0.0f);
    }
    return openrac::test::result();
}
