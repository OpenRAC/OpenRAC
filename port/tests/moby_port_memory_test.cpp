// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests that scene::moby_palette (scene/moby_pose.h) reads a moby's joint modifiers where the game
// keeps them when that is the port's own memory (scene::set_port_memory): a mod's copy of an enemy
// keeps pvars larger than a pool slot's there (port/game/rac2/host/enemy_copies.c), and with them
// the joint modifier a Megacorp Chickenbot turns its head by. Read as RAM, the node was the
// executable's code and the copy's joints went to infinity: triangles across the screen.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "scene/moby_pose.h"
#include "tests/check.h"

namespace {

constexpr std::uint32_t kMoby = 0x100;
constexpr std::uint32_t kClass = 0x1000;
constexpr std::uint32_t kSkeleton = 0x1100;
constexpr std::uint32_t kRest = 0x1200;
constexpr std::uint32_t kKey = 0x2000;
constexpr std::uint32_t kRamNode = 0x3000;
constexpr std::uint32_t kPortBase = 0xE0000000u;
constexpr std::uint32_t kPortNode = 0x40;
constexpr std::uint32_t kFields = 0x40;  // Going Commando's animation fields
constexpr std::int16_t kOne = 0x7FFF;

template <typename T>
void put(std::vector<std::uint8_t>& memory, std::uint32_t at, T value) {
    std::memcpy(memory.data() + at, &value, sizeof value);
}

// A joint modifier node at `at` of `memory`: joint 1, a half turn about z, full weight.
void modifier(std::vector<std::uint8_t>& memory, std::uint32_t at, float w) {
    put<std::uint32_t>(memory, at + 4, 0x70000040u);  // joint 1's scratchpad record
    put<std::uint32_t>(memory, at + 8, 0);            // no next node
    put<float>(memory, at + 0xC, 1.0f);
    put<float>(memory, at + 0x18, 1.0f);  // the quaternion: z
    put<float>(memory, at + 0x1C, w);
    for (std::uint32_t k = 0; k < 3; ++k) {
        put<float>(memory, at + 0x20 + 4 * k, 1.0f);
    }
}

}  // namespace

int main() {
    namespace scene = openrac::scene;
    scene::set_moby_anim_fields(kFields);

    std::vector<std::uint8_t> ram(0x4000, 0);
    std::vector<std::uint8_t> port(0x1000, 0);
    // A class of two joints at rest, identity bind matrices.
    put<std::uint32_t>(ram, kMoby + 0x24, kClass);
    ram[kClass + 0x08] = 2;
    put<std::uint32_t>(ram, kClass + 0x14, kSkeleton);
    put<std::uint32_t>(ram, kClass + 0x18, kRest);
    for (std::uint32_t j = 0; j < 2; ++j) {
        for (std::uint32_t i = 0; i < 4; ++i) {
            put<float>(ram, kSkeleton + 0x40 * j + 0x14 * i, 1.0f);
        }
    }
    put<float>(ram, kRest + 0x10, 64.0f);  // joint 1 stands 64 units along x
    // One key, both joints unturned.
    put<float>(ram, kKey, 0.25f);
    put<std::uint16_t>(ram, kKey + 6, 1);
    put<std::uint16_t>(ram, kKey + 8, 16);
    put<std::uint16_t>(ram, kKey + 0xC, 16);
    put<std::int16_t>(ram, kKey + 0x10 + 6, kOne);
    put<std::int16_t>(ram, kKey + 0x18 + 6, kOne);
    put<std::uint32_t>(ram, kMoby + kFields + 0x18, kKey);

    const auto plain = scene::moby_palette(ram, kMoby);
    CHECK(plain.size() == 2);

    // The modifier in RAM: the reference pose.
    modifier(ram, kRamNode, 0.0f);
    put<std::uint32_t>(ram, kMoby + kFields + 0x14, kRamNode);
    const auto in_ram = scene::moby_palette(ram, kMoby);
    CHECK(in_ram.size() == 2 && in_ram != plain);

    // The same node in the port's memory: the same pose once the port's memory is known.
    modifier(port, kPortNode, 0.0f);
    put<std::uint32_t>(ram, kMoby + kFields + 0x14, kPortBase + kPortNode);
    scene::set_port_memory(kPortBase, port);
    CHECK(scene::moby_palette(ram, kMoby) == in_ram);
    CHECK(scene::moby_pose_matrices(ram, kMoby).size() == 2);

    // A pose that comes out NaN is not drawn posed (the moby is drawn as stored).
    put<float>(port, kPortNode + 0x10, std::numeric_limits<float>::quiet_NaN());
    CHECK(scene::moby_palette(ram, kMoby).empty());
    return openrac::test::result();
}
