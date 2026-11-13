// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Baked shape reuse is independent of changing UVs and colours, with ordered coarse instances.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "enhanced/renderer/water_mesh.h"
#include "enhanced/tests/water_test_bake.h"
#include "renderer/settings/render_opt.h"
#include "tests/check.h"
using namespace openrac;

void test_cache_equivalence() {
    test::WaterTestBake data;
    // Add an overlapping authored layer: changing legacy Z must still select the nearest one.
    const auto path = data.folder / "water.mesh";
    std::ifstream input(path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)), {});
    input.close();
    std::array<char, 40> layer{};
    std::copy_n(bytes.begin() + 48, 40, layer.begin());
    for (unsigned vertex = 0; vertex < 3; ++vertex) {
        const float z = 3;
        std::memcpy(layer.data() + 4 + (vertex * 3 + 2) * 4, &z, 4);
    }
    const unsigned triangles = 2;
    std::memcpy(bytes.data() + 8, &triangles, 4);
    bytes.insert(bytes.begin() + 88, layer.begin(), layer.end());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();
    enhanced::WaterMesh reference(path.parent_path() / "water.bin", data.fingerprint);
    enhanced::WaterMesh cached(path.parent_path() / "water.bin", data.fingerprint);
    renderer::EffectStrip strip;
    strip.surface = 1;
    std::vector<renderer::EffectVertex> source{
        {0, 0, 1.1f, 0, 0, 0x80808080},
        {1, 0, 1.1f, 1, 0, 0x80808080},
        {0, 1, 1.1f, 0, 1, 0x80808080}
    };
    unsigned height_hits = 0, topology_hits = 0;
    for (unsigned frame = 0; frame < 10; ++frame) {
        for (auto& vertex : source) {
            vertex.z = frame == 1 ? 2.9f : frame == 2 ? 2.f : frame == 9 ? 2.5f : 1.1f;
            vertex.s += .13f;
            vertex.rgba ^= 0x10101010u;
        }
        if (frame == 4) {
            source[1].x += .125f;
        }
        if (frame == 5) {
            strip.surface = 2;
        }
        if (frame == 6) {
            strip.surface = 1;
            source.insert(source.end(), {source[2], source[1], source[0]});
        }
        if (frame == 7) {
            source[0].x = -0.f;
        }
        if (frame == 8) {
            for (auto& vertex : source) {
                vertex.x += 100;
            }
        }
        strip.uv_motion = {{.1f, -.2f}, frame % 2 ? -.03f : .03f, frame * 51u, true};
        renderer::render_opt::set(renderer::render_opt::Opt::WaterMeshReuse, false);
        reference.begin_pass();
        const auto expected = reference.prepare(strip, source);
        renderer::render_opt::set(renderer::render_opt::Opt::WaterMeshReuse, true);
        cached.begin_pass();
        const auto actual = cached.prepare(strip, source);
        CHECK(actual.vertices == expected.vertices && actual.draws == expected.draws);
        CHECK(
            std::equal(
                reference.sources().begin(),
                reference.sources().end(),
                cached.sources().begin(),
                cached.sources().end()
            )
        );
        const auto& a = reference.plan(expected.handle);
        const auto& b = cached.plan(actual.handle);
        CHECK(a.vertices == b.vertices && a.first_triangle == b.first_triangle);
        height_hits += cached.cache_counts()[1];
        topology_hits += cached.cache_counts()[3];
        if (frame == 1 || frame == 2) {
            CHECK(std::bit_cast<float>(cached.sources()[0][0][2]) == (frame == 1 ? 3.f : 1.f));
        }
    }
    CHECK(height_hits > 0 && topology_hits > 0);
    renderer::render_opt::reset();
}

int main() {
    test_cache_equivalence();
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
        CHECK(mesh.plan(output.handle).vertices[0].triangle == 0);
    }
    // Permutations still resolve the authored triangle, without changing the baked stream.
    std::swap(source[1], source[2]);
    mesh.begin_pass();
    CHECK(mesh.prepare(strip, source).vertices == 6);
    CHECK(mesh.weights().data() == baked);
    // Several strips keep distinct instance ranges until the pass is drawn.
    const auto second = mesh.prepare(strip, source);
    CHECK(second.handle == 1);
    CHECK(mesh.plan(second.handle).first_triangle == 1);
    // A strip can disappear and return at a different draw-list slot without rebuilding.
    auto shifted = source;
    for (auto& vertex : shifted) {
        vertex.x += 10.f;
    }
    mesh.begin_pass();
    mesh.prepare(strip, source);
    mesh.prepare(strip, shifted);
    mesh.begin_pass();
    const auto visible = mesh.prepare(strip, shifted);
    CHECK(mesh.cache_counts()[2] == 0 && mesh.cache_counts()[3] == 1);
    CHECK(mesh.plan(visible.handle).first_triangle == 0);
    mesh.begin_pass();
    const auto first_returned = mesh.prepare(strip, shifted);
    const auto second_returned = mesh.prepare(strip, source);
    CHECK(mesh.cache_counts()[2] == 0 && mesh.cache_counts()[3] == 2);
    CHECK(mesh.plan(first_returned.handle).first_triangle == 0);
    CHECK(mesh.plan(second_returned.handle).first_triangle == 1);
    // A baked template followed by the coarse fallback still draws as one strip.
    std::array mixed{source[0], source[1], source[2], renderer::EffectVertex{2, 2, 1, 0, 0, 0}};
    mesh.begin_pass();
    strip.count = static_cast<unsigned>(mixed.size());
    const auto combined = mesh.prepare(strip, mixed);
    CHECK(combined.vertices == 9 && combined.draws == 1);
    const auto& vertices = mesh.plan(combined.handle).vertices;
    if (vertices.size() == 9) {
        CHECK(vertices[5].triangle == 0);
        CHECK(vertices[6].triangle == 1 && vertices[8].triangle == 1);
    }
    // At one unit the right triangle splits once, on its hypotenuse, from the shape alone.
    enhanced::WaterMesh coarse(data.folder / "water.bin", data.fingerprint);
    renderer::render_opt::set(renderer::render_opt::Opt::WaterMeshCoarse, true);
    strip.count = 3;
    coarse.begin_pass();
    const auto coarse_output = coarse.prepare(strip, source);
    renderer::render_opt::reset();
    CHECK(coarse_output.vertices == 6);
    const auto& coarse_vertices = coarse.plan(coarse_output.handle).vertices;
    if (coarse_vertices.size() == 6) {
        CHECK(coarse_vertices[1].weights == (std::array{.5f, .5f}));
    }
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
