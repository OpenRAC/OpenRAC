// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU codec regression tests: HDR energy, paired gutters and untouched visibility/BRDF planes.
#include <algorithm>
#include <cfenv>
#include <cmath>
#include <cstdio>

#include "enhanced/lighting/environment_filter.h"
#include "enhanced/lighting/probe_texture_encode.h"
#include "tests/check.h"

using namespace openrac::lighting;

namespace {
// Builds varied directional colour while keeping the source visibility and relocation nontrivial.
ProbeVolume fixture() {
    ProbeGrid grid;
    grid.map_resolution = 6;
    grid.dimensions = {2, 2, 2};
    auto volume = ProbeVolume::allocate(grid);

    for (std::size_t probe = 0; probe < volume.probes.size(); ++probe) {
        volume.probes[probe] = {{.125f, -.25f, .375f}, true, true};

        for (unsigned y = 1; y <= 6; ++y) {
            for (unsigned x = 1; x <= 6; ++x) {
                const auto at = probe * 64 + std::size_t(y) * 8 + x;
                const float value = .5f + .05f * static_cast<float>(x + y);
                volume.irradiance[at] = {value, value * .5f, value * .25f};
                volume.distance_moments[at] = {2, 5};
            }
        }
    }

    update_probe_gutters(volume);
    return volume;
}

// Measure decoded linear-light gutter differences against source luminance, including the black floor.
double paired_error(const ProbeVolume& source, const ProbeVolume& decoded) {
    auto regenerated = decoded;
    update_probe_gutters(regenerated);

    double maximum_pair_error = 0;

    for (std::size_t i = 0; i < decoded.irradiance.size(); ++i) {
        const auto& value = source.irradiance[i];
        const double scale =
            std::max(.0001, .2126 * value[0] + .7152 * value[1] + .0722 * value[2]);

        for (unsigned c = 0; c < 3; ++c) {
            maximum_pair_error = std::max(
                maximum_pair_error,
                std::abs(decoded.irradiance[i][c] - regenerated.irradiance[i][c]) / scale
            );
        }
    }

    return maximum_pair_error;
}

// Runtime verification uses truncation, while Arm's decoder requires round-to-nearest internally.
void test_astc_verification_rounding() {
    const auto encoded = encode_baked_probes(fixture(), ProbeTextureFormat::Astc8x8);
    const auto reference = decode_probes(encoded);
    const int previous = std::fegetround();

    for (const int mode : {FE_TOWARDZERO, FE_DOWNWARD, FE_UPWARD}) {
        CHECK(std::fesetround(mode) == 0);
        const auto gpu = decode_gpu_probes(encoded);
        CHECK(gpu.colour_format == ProbeTextureFormat::Astc8x8);
        CHECK(std::fegetround() == mode);
        const auto decoded = decode_probes(encoded);
        CHECK(decoded.probes.size() == reference.probes.size());
        CHECK(std::fegetround() == mode);
    }

    CHECK(std::fesetround(FE_TOWARDZERO) == 0);
    bool rejected = false;

    try {
        // All-zero ASTC is invalid, so this exercises restoration during decoder failure.
        validate_probe_texture(ProbeTextureFormat::Astc8x8, 8, 8, std::vector<std::uint8_t>(16));
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    CHECK(rejected);
    CHECK(std::fegetround() == FE_TOWARDZERO);
    CHECK(std::fesetround(previous) == 0);
}

// The two visibility channels and metadata must remain bit-identical under colour compression.
void test_probe_planes(ProbeTextureFormat format) {
    const auto source = fixture();
    const auto raw = encode_probes(source);
    const auto raw_gpu = decode_gpu_probes(raw);
    const auto encoded = encode_baked_probes(source, format);
    BakeThreadPool workers(3);  // Exercise independent tile batches on a borrowed worker budget.
    CHECK(encode_baked_probes(source, format, {0, &workers}) == encoded);
    const auto gpu = decode_gpu_probes(encoded);
    CHECK(gpu.colour_format == format);
    CHECK(encoded.size() < raw.size());
    CHECK(std::ranges::equal(gpu.gpu_plane(1), raw_gpu.gpu_plane(1)));
    CHECK(std::ranges::equal(gpu.gpu_plane(2), raw_gpu.gpu_plane(2)));
    const auto decoded = decode_probes(encoded);
    double squared = 0, reference = 0;

    for (std::size_t i = 0; i < source.irradiance.size(); ++i) {
        for (unsigned c = 0; c < 3; ++c) {
            const double delta = decoded.irradiance[i][c] - source.irradiance[i][c];
            squared += delta * delta;
            reference += double(source.irradiance[i][c]) * source.irradiance[i][c];
        }
    }

    CHECK(std::sqrt(squared / reference) < .25);
    CHECK(paired_error(source, decoded) <= .005);
    bool rejected = false;

    try {
        decode_gpu_probes(std::vector<std::uint8_t>(encoded.begin(), encoded.end() - 1));
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    CHECK(rejected);
}

// GGX filtering retains HDR energy and the material-response lookup is never block-compressed.
void test_reflection_planes(ProbeTextureFormat format) {
    EnvironmentAsset asset;
    asset.colour_format = format;
    asset.resolution = 8;
    asset.probes.push_back({{0, 0, 0}, {-2, -2, -2}, {2, 2, 2}});
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());
    const Float3 colour{8, 2, .5f};
    filter_environment(asset, 0, std::vector<Float3>(std::size_t{6} * 8 * 8, colour), 8);
    integrate_environment_brdf(asset, 16);
    std::vector<std::uint32_t> brdf;

    for (unsigned y = 0; y < kEnvironmentBrdfSize; ++y) {
        for (unsigned x = 0; x < kEnvironmentBrdfSize; ++x) {
            brdf.push_back(asset.pixels[std::size_t(asset.brdf_row() + y) * asset.width() + x]);
        }
    }

    BakeThreadPool workers(3);  // Reflection blocks use the same pool as paired GI tiles.
    auto parallel_asset = asset;
    const auto encoded = encode_baked_environment(asset, format);
    CHECK(encode_baked_environment(parallel_asset, format, {0, &workers}) == encoded);
    const auto gpu = decode_gpu_environment(encoded);
    CHECK(gpu.pixels.empty());
    CHECK(gpu.colour_format == format);
    CHECK(gpu.brdf == brdf);
    CHECK(encode_environment(gpu) == encoded);
    const auto decoded = decode_environment(encoded);

    float maximum_error = 0;

    for (unsigned mip = 0; mip < asset.levels(); ++mip) {
        const auto at = std::size_t(asset.level_row(mip) + 1) * asset.width() + 1;
        const auto sample = unpack_environment_colour(decoded.pixels[at]);

        for (unsigned c = 0; c < 3; ++c) {
            maximum_error = std::max(maximum_error, std::abs(sample[c] / colour[c] - 1));
        }
    }

    CHECK(maximum_error < .05f);
}

// The fixture crosses tile, block and worker-batch boundaries.
constexpr unsigned kEdgeWidth = 37, kEdgeHeight = 65;

// Checks each codec under supported arithmetic modes on workers reused across prior encodes.
void compare_parallel_edges(std::span<const Float3> pixels, BakeThreadPool& workers, int rounding) {
    CHECK(std::fesetround(rounding) == 0);

    for (auto format :
         {ProbeTextureFormat::R11G11B10, ProbeTextureFormat::Bc6h, ProbeTextureFormat::Astc8x8}) {
        // Arm's codec checks for round-to-nearest during context creation.
        if (format == ProbeTextureFormat::Astc8x8 && rounding != FE_TONEAREST) {
            continue;
        }

        const auto serial = encode_probe_texture(format, kEdgeWidth, kEdgeHeight, pixels);
        const auto parallel =
            encode_probe_texture(format, kEdgeWidth, kEdgeHeight, pixels, {false, {0, &workers}});
        CHECK(serial == parallel);
    }
}

// Partial image edges and altered controller rounding must produce identical bytes on reused workers.
void test_parallel_edges() {
    std::vector<Float3> pixels(std::size_t{kEdgeWidth} * kEdgeHeight);

    for (std::size_t i = 0; i < pixels.size(); ++i) {
        const float value = static_cast<float>(i % 47) / 16;  // Varied HDR texels and black pixels.
        pixels[i] = {value, value * .3f, value * 2};
    }

    BakeThreadPool workers(3);
    const int original_rounding = std::fegetround();
    compare_parallel_edges(pixels, workers, FE_DOWNWARD);
    compare_parallel_edges(pixels, workers, FE_TONEAREST);
    CHECK(std::fesetround(original_rounding) == 0);
}

// Reserved BC6H and malformed ASTC blocks must fail streamed validation before GPU upload.
void test_malformed_blocks() {
    for (auto format : {ProbeTextureFormat::Bc6h, ProbeTextureFormat::Astc8x8}) {
        std::vector<std::uint8_t> bytes(probe_texture_bytes(format, 8, 8));

        // BC6H mode 31 is reserved; an all-zero ASTC block is invalid.
        if (format == ProbeTextureFormat::Bc6h) {
            bytes[0] = 31;
        }

        bool rejected = false;

        try {
            validate_probe_texture(format, 8, 8, bytes);
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        CHECK(rejected);
    }
}
}  // namespace

// Codec tests need no Vulkan device, level assets or game process.
int main() {
    try {
        CHECK(probe_texture_bytes(ProbeTextureFormat::Bc6h, 8, 8) == 64);
        CHECK(probe_texture_bytes(ProbeTextureFormat::Astc8x8, 8, 8) == 16);

        for (auto format : {ProbeTextureFormat::Bc6h, ProbeTextureFormat::Astc8x8}) {
            test_probe_planes(format);
            test_reflection_planes(format);
        }

        test_astc_verification_rounding();
        test_parallel_edges();
        test_malformed_blocks();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
