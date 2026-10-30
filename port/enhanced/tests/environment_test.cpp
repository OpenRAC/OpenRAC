// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reflection face orientation, filtering energy, portable files and geometric parallax.
#include <cmath>
#include <stdexcept>

#include "enhanced/lighting/environment_filter.h"
#include "tests/check.h"

using namespace openrac::lighting;

namespace {
// Constant radiance must survive every GGX level and cube seam without changing its energy.
void constant_filter() {
    EnvironmentAsset asset;
    asset.resolution = 8;
    asset.probes.push_back({{0, 0, 0}, {-2, -2, -2}, {2, 2, 2}});
    asset.scene_hash = 123;
    asset.settings_hash = 456;
    asset.gi_hash = 789;
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    const Float3 colour{2, .5f, .125f};
    std::vector<Float3> faces(6 * 8 * 8, colour);
    filter_environment(asset, 0, faces, 64);
    integrate_environment_brdf(asset, 128);

    for (unsigned mip = 0; mip < asset.levels(); ++mip) {
        const unsigned side = asset.face_resolution(mip) + 2;

        for (unsigned face = 0; face < 6; ++face) {
            for (unsigned y = 0; y < side; ++y) {
                for (unsigned x = 0; x < side; ++x) {
                    const auto value = unpack_environment_colour(
                        asset.pixels
                            [std::size_t(asset.level_row(mip) + y) * asset.width() + face * side
                             + x]
                    );

                    for (unsigned c = 0; c < 3; ++c) {
                        CHECK(std::abs(value[c] - colour[c]) < .001f);
                    }
                }
            }
        }
    }

    const auto bytes = encode_environment(asset);
    CHECK(encode_environment(decode_environment(bytes)) == bytes);
    bool rejected = false;

    try {
        decode_environment(std::span(bytes).first(bytes.size() - 1));
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    CHECK(rejected);
    asset.pixels[0] = 0x7c0u;
    rejected = false;

    try {
        asset.validate();
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    CHECK(rejected);
}

// A sharply directional source becomes broader at high roughness, while a black source stays black.
void directional_filter() {
    EnvironmentAsset asset;
    asset.resolution = 16;
    asset.probes.push_back({{0, 0, 0}, {-2, -2, -2}, {2, 2, 2}});
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    std::vector<Float3> faces(6 * 16 * 16);

    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 16; ++x) {
            faces[(std::size_t(4) * 16 + y) * 16 + x] = {4, 2, 1};
        }
    }

    filter_environment(asset, 0, faces, 128);
    const unsigned centre = asset.face_resolution() / 2 + 1;
    const auto sharp = unpack_environment_colour(
        asset.pixels
            [std::size_t(centre) * asset.width() + 4 * (asset.face_resolution() + 2) + centre]
    );
    const auto rough = unpack_environment_colour(
        asset.pixels
            [std::size_t(asset.level_row(asset.levels() - 1) + 1) * asset.width() + 4 * 3 + 1]
    );
    CHECK(sharp[0] > 3.9f);
    CHECK(rough[0] > 0 && rough[0] < sharp[0]);
}

// Wider packing supports full-level budgets without exceeding either GPU texture dimension.
void wide_atlas() {
    EnvironmentAsset asset;
    asset.resolution = 4;
    asset.probes.resize(64, {{0, 0, 0}, {-2, -2, -2}, {2, 2, 2}});
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    CHECK(asset.columns() > 1);
    for (unsigned i = 0; i < asset.probes.size(); ++i) {
        const Float3 colour{float(i % 8) / 8, .25f, .5f};
        filter_environment(asset, i, std::vector<Float3>(6 * 4 * 4, colour), 1);
        const auto p = std::size_t(asset.probe_y(i) + 1) * asset.width() + asset.probe_x(i) + 1;
        CHECK(unpack_environment_colour(asset.pixels[p]) == colour);
    }
    const auto bytes = encode_environment(asset);
    CHECK(encode_environment(decode_environment(bytes)) == bytes);
    auto old = bytes;
    old[8] = 1;
    bool rejected = false;
    try {
        decode_environment(old);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);
    asset.probes.resize(kEnvironmentProbeLimit);
    for (unsigned resolution : {4u, 8u, 64u, 128u, 256u}) {
        asset.resolution = resolution;
        CHECK(asset.width() <= 16384 && asset.height() <= 16384);
        CHECK(
            asset.probe_x(kEnvironmentProbeLimit - 1) + 6 * (asset.face_resolution() + 2)
            <= asset.width()
        );
        CHECK(asset.probe_y(kEnvironmentProbeLimit - 1) + asset.stack_height() <= asset.brdf_row());
    }
}

// A 128-square capture retains the original roughness schedule, starting at 64-square faces.
void test_omitted_sharpest_mip() {
    EnvironmentAsset asset;
    asset.resolution = 128;
    asset.probes.resize(110, {{0, 0, 0}, {-2, -2, -2}, {2, 2, 2}});
    CHECK(asset.face_resolution() == 64);
    CHECK(asset.levels() == 7);
    CHECK(asset.face_resolution(asset.levels() - 1) == 1);
    // The preceding full atlas used 96,096,000 pixel bytes for this capture count.
    CHECK(std::size_t(asset.width()) * asset.height() * 4 < 96'096'000 / 3);
}

// Cube face inversion and the shifted lookup direction agree with a ray-box intersection.
void projection() {
    for (unsigned face = 0; face < 6; ++face) {
        const auto [index, uv] = environment_face(environment_direction(face, {.2f, .7f}));
        CHECK(index == face);
        CHECK(std::abs(uv[0] - .2f) < 1e-5f);
        CHECK(std::abs(uv[1] - .7f) < 1e-5f);
    }

    EnvironmentProbe probe{{0, 0, 0}, {-5, -5, -5}, {5, 5, 5}};
    const auto direction = environment_box_direction(probe, {2, 1, 0}, {1, 0, 0});
    CHECK(direction == Float3({5, 1, 0}));
    CHECK(environment_box_direction(probe, {6, 1, 0}, {1, 0, 0}) == Float3({1, 0, 0}));
    probe.minimum = {-10, -10, -10};
    probe.maximum = {10, 10, 10};
    CHECK(environment_box_direction(probe, {0, 1, 0}, {1, 0, 0}) == direction);
    CHECK(environment_probe_coverage(probe, {0, 0, -9.25f}) == 1);
    CHECK(environment_probe_coverage(probe, {0, 0, -9.625f}) == .5f);
    CHECK(environment_probe_coverage(probe, {0, 0, -10}) == 0);
    probe.frame.axis = {.6f, .8f};
    const auto world = probe.frame.to_world({2, 1, 0});
    const auto ray = probe.frame.to_world({1, 0, 0});
    const auto projected = environment_box_direction(probe, world, ray);
    const auto expected = probe.frame.to_world({5, 1, 0});
    for (unsigned a = 0; a < 3; ++a) {
        CHECK(std::abs(projected[a] - expected[a]) < .0001f);
    }
    CHECK(environment_probe_coverage(probe, probe.frame.to_world({0, 0, -9.25f})) == 1);
    EnvironmentAsset rotated;
    rotated.resolution = 4;
    rotated.probes = {probe};
    rotated.pixels.resize(std::size_t(rotated.width()) * rotated.height());
    const auto bytes = encode_environment(rotated);
    const auto decoded = decode_environment(bytes);
    CHECK(decoded.probes[0].frame.axis == probe.frame.axis);
    CHECK(encode_environment(decoded) == bytes);
    auto unrotated = rotated.probes;
    unrotated[0].frame = {};
    CHECK(environment_capture_hash(rotated.probes) != environment_capture_hash(unrotated));
}
}  // namespace

int main() {
    test_omitted_sharpest_mip();
    constant_filter();
    directional_filter();
    projection();
    wide_atlas();
    return openrac::test::result();
}
