// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's moby shadow state (scene::moby_shadow_state_on_axis) against the values
// retail's own shadow list held (func_002DFD88's header at level 3, field 2849): Ratchet's slab
// 128.0 +- 0.12, cast along direction slot 1.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "scene/moby_shadow.h"
#include "tests/check.h"

using namespace openrac;

namespace {

constexpr std::uint32_t kMoby = 0x1000;
constexpr std::uint32_t kDirections = 0x2000;

void put(std::vector<std::uint8_t>& ram, std::uint32_t at, float value) {
    std::memcpy(ram.data() + at, &value, 4);
}

void put_bits(std::vector<std::uint8_t>& ram, std::uint32_t at, std::uint32_t bits) {
    std::memcpy(ram.data() + at, &bits, 4);
}

// Ratchet standing at z 127.88 with his slab 0.12 above his position and 0.12 either side.
std::vector<std::uint8_t> level3_hero() {
    std::vector<std::uint8_t> ram(0x3000);
    put(ram, kMoby + 0x00, 240.0f * 1024);  // bounding sphere, x1024
    put(ram, kMoby + 0x04, 190.0f * 1024);
    put(ram, kMoby + 0x08, 128.5f * 1024);
    put(ram, kMoby + 0x0c, 1.0f * 1024);
    put(ram, kMoby + 0x10, 240.0f);  // position
    put(ram, kMoby + 0x14, 190.0f);
    put(ram, kMoby + 0x18, 127.88f);
    const std::uint16_t mode = 0x4416;
    std::memcpy(ram.data() + kMoby + 0x34, &mode, 2);
    ram[kMoby + 0x6f] = 64;  // fade distance, units
    put(ram, kMoby + 0x70, 0.12f);
    put_bits(ram, kMoby + 0x74, 0x3df5c28fu);  // 0.12, as retail's header
    ram[kMoby + 0xbd] = 1;
    put(ram, kMoby + 0xe8, 1.0f);  // up axis (0, 0, 1)
    // Slot 1 as retail's table held it in level 3.
    put_bits(ram, kDirections + 0x10, 0xbecca905u);
    put_bits(ram, kDirections + 0x14, 0x3ecca901u);
    put_bits(ram, kDirections + 0x18, 0xbf532bb6u);
    return ram;
}

// A camera at the origin looking down -z: identity.
renderer::Mat4 view_at(float depth) {
    renderer::Mat4 view{};
    view[0] = view[5] = view[10] = view[15] = 1;
    view[14] = -depth;
    return view;
}

void test_retail_slab() {
    auto ram = level3_hero();
    const auto view = view_at(0);
    const auto s = scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view);
    CHECK(s.enabled);
    CHECK(s.direction_index == 1);
    CHECK(std::fabs(s.heights[0] - 127.88f) < 1e-3f);
    CHECK(std::fabs(s.heights[1] - 128.12f) < 1e-3f);
    CHECK(std::fabs(s.extrusion[0] + 399.73f) < 0.05f);
    CHECK(std::fabs(s.extrusion[1] - 399.73f) < 0.05f);
    CHECK(std::fabs(s.extrusion[2] + 824.89f) < 0.05f);
}

// The fade: (byte +0x6F * 1024 - view depth * 1024) / 2, at most 0x1000.
void test_fade() {
    auto ram = level3_hero();
    // The cast centre lies on z 128.0; a camera on +z looking down sees it 128 - depth away.
    CHECK(
        scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view_at(128.0f + 10)).scale == 1
    );
    const auto near_edge =
        scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view_at(128.0f + 60));
    CHECK(std::fabs(near_edge.scale - 0.5f) < 1e-3f);  // (64 - 60) * 1024 / 2 = 2048
    CHECK(!scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view_at(128.0f + 70)).enabled);
}

// A negative slot casts along the up axis reversed; no slab or no 0x400 casts nothing.
void test_gates() {
    auto ram = level3_hero();
    ram[kMoby + 0xbd] = 0xff;
    const auto down = scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view_at(0));
    CHECK(down.enabled);
    CHECK(std::fabs(down.extrusion[2] + 1000.0f) < 1e-3f);
    ram = level3_hero();
    put_bits(ram, kMoby + 0x74, 0);
    CHECK(!scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view_at(0)).enabled);
    ram = level3_hero();
    ram[kMoby + 0x35] &= ~0x04;
    CHECK(!scene::moby_shadow_state_on_axis(ram, kMoby, kDirections, view_at(0)).enabled);
}

}  // namespace

int main() {
    test_retail_slab();
    test_fade();
    test_gates();
    return openrac::test::result();
}
