// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
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
    std::vector<renderer::EffectVertex> output;
    CHECK(mesh.build(strip, source, output));
    CHECK(output.size() == 3);
    for (const auto& v : output) {
        CHECK(std::abs(v.z - 1.f) < 1e-6);
    }
    for (auto& v : source) {
        v.s += .25f;
        v.t -= .5f;
        v.rgba = 0x40404040;
    }
    CHECK(mesh.build(strip, source, output));
    for (unsigned i = 0; i < 3; ++i) {
        CHECK(output[i].rgba == source[i].rgba);
        CHECK(std::abs(output[i].s - source[i].s) < 1e-6);
        CHECK(std::abs(output[i].t - source[i].t) < 1e-6);
    }
    // Non-uniform UV and colour changes must rebuild rather than reuse a translation.
    source[1].s += .3f;
    source[1].rgba = 0x20406080;
    CHECK(mesh.build(strip, source, output));
    for (unsigned i = 0; i < 3; ++i) {
        CHECK(output[i].rgba == source[i].rgba);
        CHECK(output[i].s == source[i].s);
    }
    std::swap(source[1], source[2]);
    CHECK(mesh.build(strip, source, output));
    CHECK(output.size() == 3);
    for (const auto& v : output) {
        CHECK(v.z == 1.f);
    }
    CHECK(!mesh.build(strip, {}, output));
    strip.surface = 0;
    CHECK(!mesh.build(strip, source, output));
    strip.surface = 1;
    mesh.clear_cache();
    CHECK(mesh.build(strip, source, output));
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
