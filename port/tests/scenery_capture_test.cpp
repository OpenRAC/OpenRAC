// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic scenery records prove camera-independent caster residency, persistent hiding,
// distance limits and owned matrix capture. No retail data or renderer is needed.

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "game/common/frontend/scenery_capture.h"
#include "openrac/guest.h"
#include "tests/check.h"

// Synthetic pointer globals use identity relocation and never access a guest runtime.
extern "C" gaddr openrac_relocate_data(gaddr address) {
    return address;
}

namespace {

// Synthetic globals and aligned tables, deliberately unrelated to any game's addresses.
constexpr std::array<std::uint32_t, 4> kWords{0x40, 0x44, 0x100, 0x200};
constexpr std::array<std::size_t, 2> kCounts{3, 3};
constexpr std::uint32_t kTieRecords = 0x800;
constexpr std::uint32_t kShrubRecords = 0x900;

// One owned memory image with visible, explicitly hidden and distant instances in both layers.
struct Fixture {
    std::vector<std::uint8_t> ram = std::vector<std::uint8_t>(4096);  // Synthetic guest memory.
    openrac::host::GameState state;  // Camera used by the far cull.

    // Writes a synthetic field without aliasing the byte buffer.
    template <typename T>
    void put(std::size_t offset, const T& value) {
        std::memcpy(ram.data() + offset, &value, sizeof(value));
    }

    // Populates the existing capture layout with arithmetic values and no game bytes.
    Fixture() {
        put(kWords[0], std::uint32_t{0x400});
        put(kWords[1], std::uint32_t{0x500});
        put(kWords[2], kTieRecords);
        put(kWords[3], kShrubRecords);
        put(kWords[2] + 0x48, std::uint32_t{96});  // Six 16-unit distance steps.
        put(kWords[3] + 0x10, 96.0f);
        state.moby_view.emplace();
        state.moby_view->depth_axis = {1, 0, 0};
        state.moby_view->near_plane = 1024;  // One world unit in the game's view units.
        constexpr std::array<float, 3> kDepths{-10, 20, 200};

        // A shared synthetic layout exercises both ties and shrubs with the same camera.
        for (std::size_t i = 0; i < kDepths.size(); ++i) {
            const std::array<float, 4> sphere{kDepths[i], 0, 0, 1};
            const std::size_t tie = kTieRecords + i * 0x20;
            const std::size_t shrub = kShrubRecords + i * 0x20;
            put(tie, sphere);
            put(shrub, sphere);
            ram[tie + 0x1C] = 6;  // Six 16-unit distance steps.
            put(shrub + 0x10, 96.0f);
        }

        ram[kTieRecords + 0x20 + 0x1E] = 4;    // Persistent tie hiding bit.
        ram[kShrubRecords + 0x20 + 0x16] = 1;  // Persistent shrub hiding bit.
    }

    // Captures owned records using a selectable draw-distance multiplier.
    openrac::frontend::CapturedScenery capture(float scale = 1.0f) const {
        return openrac::frontend::capture_scenery(ram, kWords, state, scale, kCounts);
    }
};

// Turning or translating the camera never removes nearby offscreen shadow casters.
void test_camera_independent_residency() {
    Fixture fixture;
    const auto forward = fixture.capture();
    CHECK(forward.scenery.tie_hidden == std::vector<std::uint8_t>({0, 1, 1}));
    CHECK(forward.scenery.shrub_hidden == std::vector<std::uint8_t>({0, 1, 1}));
    auto& view = fixture.state.moby_view.emplace();
    view.depth_axis = {-1, 0, 0};
    view.near_plane = 100 * 1024;  // Must not affect caster residency.
    fixture.state.camera_position = {1, 0, 0};
    const auto backward = fixture.capture();
    CHECK(backward.scenery.tie_hidden == std::vector<std::uint8_t>({0, 1, 0}));
    CHECK(backward.scenery.shrub_hidden == std::vector<std::uint8_t>({0, 1, 0}));
}

// Scaled and unlimited distance keep explicit game hiding while changing only the far limit.
void test_draw_distance_and_persistent_hiding() {
    Fixture fixture;
    const auto scaled = fixture.capture(4.0f);
    const auto unlimited = fixture.capture(0.0f);
    CHECK(scaled.tie_cap == 96);
    CHECK(scaled.shrub_cap == 96.0f);
    CHECK(scaled.scenery.tie_hidden == std::vector<std::uint8_t>({0, 1, 0}));
    CHECK(scaled.scenery.shrub_hidden == std::vector<std::uint8_t>({0, 1, 0}));
    CHECK(unlimited.scenery.tie_hidden == scaled.scenery.tie_hidden);
    CHECK(unlimited.scenery.shrub_hidden == scaled.scenery.shrub_hidden);
}

// Affine rows are copied out of guest records and survive later guest-memory changes.
void test_matrix_ownership_and_invalid_tables() {
    Fixture fixture;
    constexpr std::array<float, 4> kRow{2, 3, 4, 99};  // Fourth lane is not part of the affine row.
    fixture.put(0x400, kRow);
    const auto captured = fixture.capture();
    fixture.put(0x400, std::array<float, 4>{});
    CHECK(captured.scenery.matrices[0][0][0] == 2);
    CHECK(captured.scenery.matrices[0][0][1] == 3);
    CHECK(captured.scenery.matrices[0][0][2] == 4);
    fixture.put(kWords[0], std::uint32_t{4090});  // Unaligned and outside the available extent.
    CHECK(fixture.capture().scenery.matrices[0].empty());
}

// Terrain keeps its occlusion words while far rejection converts the stored 1/1024-unit spheres.
void test_terrain_capture_and_distance_units() {
    Fixture fixture;
    constexpr std::uint32_t kTerrain = 0xB00;  // Synthetic aligned fragment table.
    fixture.put(kWords[2] - 0x44, kTerrain);
    fixture.put(kWords[2] - 0x40, std::uint32_t{2});
    fixture.put(kWords[2] - 0x14, 96.0f * 1024.0f);  // Same world-space cap as the other layers.
    fixture.put(kTerrain, std::array<float, 4>{-10 * 1024, 0, 0, 1024});
    fixture.put(kTerrain + 0x40, std::array<float, 4>{200 * 1024, 0, 0, 1024});
    fixture.ram[kTerrain + 0x3A] = 0x12;  // Occlusion bit byte.
    fixture.ram[kTerrain + 0x3B] = 0x34;  // Occlusion mask-byte index.
    fixture.ram[kTerrain + 0x40 + 0x3A] = 0x56;
    fixture.ram[kTerrain + 0x40 + 0x3B] = 0x78;
    const auto captured = fixture.capture();
    CHECK(captured.scenery.terrain_occlusion == std::vector<std::uint16_t>({0x3412, 0x7856}));
    CHECK(captured.scenery.terrain_far == std::vector<std::uint8_t>({0, 1}));
    CHECK(fixture.capture(0.0f).scenery.terrain_far == std::vector<std::uint8_t>({0, 0}));
}

}  // namespace

// Runs CPU-only capture regressions independently of any region, backend or optional lighting.
int main() {
    // Allocation or capture exceptions fail the test instead of escaping the program entry point.
    try {
        test_camera_independent_residency();
        test_draw_distance_and_persistent_hiding();
        test_matrix_ownership_and_invalid_tables();
        test_terrain_capture_and_distance_units();
    } catch (...) {
        CHECK(false);
    }
    return openrac::test::result();
}
