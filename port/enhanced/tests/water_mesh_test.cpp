// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Baked shape reuse is independent of changing UVs and colours, with ordered coarse instances.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>

#include "enhanced/renderer/water_mesh.h"
#include "enhanced/tests/water_test_bake.h"
#include "tests/check.h"
using namespace openrac;

int main() {
    test::WaterTestBake data;
    enhanced::WaterMesh mesh(data.folder / "water.bin", data.fingerprint);
    renderer::EffectStrip strip;
    strip.surface = 1;
    strip.count = 3;
    std::array<renderer::EffectVertex, 3> source{
        {{0, 0, 1.1f, 0, 0, 0x80808080},
         {1, 0, 1.1f, 1, 0, 0x80808080},
         {0, 1, 1.1f, 0, 1, 0x80808080}}
    };
    const auto* baked = mesh.weights().data();
    for (unsigned frame = 0; frame < 3; ++frame) {
        mesh.begin_pass();
        source[1].s = 1.f + float(frame) * .3f;
        source[1].rgba = 0x20406080;
        strip.uv_motion = {{.2f, -.3f}, .05f, frame * 77, true};
        const auto output = mesh.prepare(strip, source);
        CHECK(output.vertices == 6 && output.draws == 1);
        CHECK(mesh.sources().size() == 1);
        CHECK(mesh.weights().data() == baked);
        const auto& coarse = mesh.sources()[0];
        for (unsigned i = 0; i < 3; ++i) {
            CHECK(std::bit_cast<float>(coarse[i * 2][2]) == 1.f);
            CHECK(coarse[i * 2][3] == source[i].rgba);
            CHECK(std::bit_cast<float>(coarse[i * 2 + 1][0]) == source[i].s);
        }
        CHECK(coarse[6][3] == (0x80000000u | frame * 77));
        CHECK(mesh.plan(output.handle).batches[0].count == 1);
    }
    // Permutations still resolve the authored triangle, without changing the baked stream.
    std::swap(source[1], source[2]);
    mesh.begin_pass();
    CHECK(mesh.prepare(strip, source).vertices == 6);
    CHECK(mesh.weights().data() == baked);
    // Several strips keep distinct instance ranges until the pass is drawn.
    const auto second = mesh.prepare(strip, source);
    CHECK(second.handle == 1);
    CHECK(mesh.plan(second.handle).batches[0].first == 1);
    auto wrong = data.fingerprint;
    wrong[0] = 'y';
    bool rejected = false;
    try {
        enhanced::WaterMesh invalid(data.folder / "water.bin", wrong);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);
    std::filesystem::resize_file(data.folder / "water.mesh", 49);
    rejected = false;
    try {
        enhanced::WaterMesh invalid(data.folder / "water.bin", data.fingerprint);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);
    return test::result();
}
