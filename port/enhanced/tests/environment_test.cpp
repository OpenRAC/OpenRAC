// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reflection face orientation, filtering energy, portable files and geometric parallax.
#include <chrono>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include "enhanced/lighting/bake_lz4.h"
#include "enhanced/lighting/environment_filter.h"
#include "tests/check.h"

using namespace openrac::lighting;

namespace {
void gi_hash_reuses_loaded_payload() {
    namespace fs = std::filesystem;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = fs::temp_directory_path() / ("openrac-gi-hash-" + std::to_string(stamp));
    CHECK(fs::create_directory(directory));
    const auto sparse = directory / "sparse.orpg";
    const auto region = directory / "region-000.orpg";
    const std::vector<std::uint8_t> raw(4096, 'x');
    const auto write = [&](const auto& bytes) {
        std::ofstream(sparse, std::ios::binary)
            .write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    };
    write(raw);
    std::ofstream(region, std::ios::binary) << "region";
    const auto expected = environment_gi_hash(directory);
    if (bake_lz4_available()) {
        const auto frame = compress_bake_asset(raw);
        CHECK(is_lz4_bake_asset(frame));
        write(frame);
        CHECK(environment_gi_hash(directory) == expected);
    }
    std::ofstream(sparse, std::ios::binary) << "replacement";
    CHECK(environment_gi_hash(directory) != expected);
    CHECK(
        environment_gi_hash(directory, {{fs::absolute(sparse).lexically_normal(), raw}}) == expected
    );
    CHECK(fs::remove(region));
    CHECK(fs::remove(sparse));
    CHECK(fs::remove(directory));
}

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
    if (bake_lz4_available()) {
        auto frame = compress_bake_asset(bytes);
        CHECK(is_lz4_bake_asset(frame));
        CHECK(encode_environment(decode_environment(frame)) == bytes);
        CHECK(encode_environment(decode_gpu_environment(frame)) == bytes);
        frame.at(frame.size() - 1) ^= 1;  // Corrupt the content checksum.
        bool rejected_frame = false;
        try {
            decode_gpu_environment(frame);
        } catch (const std::runtime_error&) {
            rejected_frame = true;
        }
        CHECK(rejected_frame);
    }
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
    probe.frame.pitch = {.8f, .6f};
    probe.frame.roll = {.6f, .8f};
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
    CHECK(decoded.probes[0].frame.pitch == probe.frame.pitch);
    CHECK(decoded.probes[0].frame.roll == probe.frame.roll);
    CHECK(encode_environment(decoded) == bytes);
    auto upright = rotated;
    upright.probes[0].frame.pitch = {1, 0};
    upright.probes[0].frame.roll = {1, 0};
    auto legacy = encode_environment(upright);
    legacy.erase(legacy.begin() + 72, legacy.begin() + 80);
    legacy[6] = '0';
    legacy.erase(legacy.begin() + 52, legacy.begin() + 56);  // Version 8 region count.
    legacy[7] = '7';
    legacy[8] = 7;
    CHECK(decode_environment(legacy).probes[0].frame.axis == probe.frame.axis);
    legacy.erase(legacy.begin() + 60, legacy.begin() + 68);
    legacy[7] = '6';
    legacy[8] = 6;
    CHECK(decode_environment(legacy).probes[0].frame.pitch == Float2({1, 0}));
    auto unrotated = rotated.probes;
    unrotated[0].frame = {};
    CHECK(environment_capture_hash(rotated.probes) != environment_capture_hash(unrotated));
}

void independent_influences() {
    EnvironmentAsset asset;
    asset.resolution = 4;
    asset.probes = {{{0, 0, 0}, {-2, -2, -2}, {2, 2, 2}}};
    asset.influences =
        {{0, {}, {-2, -2, -2}, {2, 2, 2}},
         {0, ProbeFrame{{.6f, .8f}, {.8f, .6f}}, {8, 2, 1}, {12, 4, 3}}};
    asset.influences.back().projection =
        EnvironmentProjection{ProbeFrame{{0, 1}, {1, 0}}, {7, 1, 0}, {13, 5, 4}};
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    asset.validate();  // The second region intentionally does not contain the camera.
    const auto bytes = encode_environment(asset);
    const auto decoded = decode_environment(bytes);
    CHECK(decoded.probes.size() == 1 && decoded.influences.size() == 2);
    CHECK(encode_environment(decoded) == bytes);
    CHECK(encode_environment(decode_gpu_environment(bytes)) == bytes);
    const auto metadata = environment_metadata(decoded.probes, decoded.influences);
    CHECK(metadata.size() == 2 * kEnvironmentMetadataStride);
    CHECK(metadata[kEnvironmentMetadataStride + 4][3] == 0);
    CHECK(
        (metadata[kEnvironmentMetadataStride + 5]
         == asset.influences.back().projection->frame.quaternion())
    );
    CHECK(metadata[kEnvironmentMetadataStride + 3][0] == 7);
    CHECK(
        environment_capture_hash(asset.probes, 0, asset.influences)
        == environment_capture_hash(decoded.probes, 0, decoded.influences)
    );
    auto legacy = bytes;
    // Convert the rolled layout to version nine before removing its per-region proxies.
    for (std::size_t i = 2; i-- > 0;) {
        const auto base = 56 + 84 + i * 100;
        legacy.erase(legacy.begin() + base + 68, legacy.begin() + base + 76);
        legacy.erase(legacy.begin() + base + 20, legacy.begin() + base + 28);
    }
    legacy.erase(legacy.begin() + 72, legacy.begin() + 80);
    legacy[6] = '0';
    legacy[7] = '9';
    legacy[8] = 9;
    CHECK(decode_environment(legacy).influences.back().projection.has_value());
    for (std::size_t i = 2; i-- > 0;) {
        const auto at = 56 + 76 + i * 84 + 44;
        legacy.erase(legacy.begin() + at, legacy.begin() + at + 40);
    }
    legacy[7] = '8';
    legacy[8] = 8;
    const auto old = decode_environment(legacy);
    CHECK(old.influences.size() == 2 && !old.influences.back().projection);
    CHECK(
        (environment_metadata(old.probes, old.influences)[kEnvironmentMetadataStride + 5]
         == ProbeFrame{}.quaternion())
    );
    auto changed = asset.influences;
    changed.back().minimum[0] += .1f;
    CHECK(
        environment_capture_hash(asset.probes, 0, asset.influences)
        != environment_capture_hash(asset.probes, 0, changed)
    );
    asset.influences.back().capture = 1;
    bool rejected = false;
    try {
        encode_environment(asset);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);
}
}  // namespace

int main() {
    try {
        gi_hash_reuses_loaded_payload();
        independent_influences();
        test_omitted_sharpest_mip();
        constant_filter();
        directional_filter();
        projection();
        wide_atlas();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "environment test exception: %s\n", e.what());
        return 1;
    }
    return openrac::test::result();
}
