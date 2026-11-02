// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU codec regression tests: HDR energy, paired gutters and untouched visibility/BRDF planes.
#include <algorithm>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string_view>

#include "enhanced/lighting/environment_filter.h"
#include "enhanced/lighting/probe_atlas.h"
#include "enhanced/lighting/probe_depth.h"
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
    CHECK(gpu.depth_format == ProbeDepthFormat::Rg16);
    CHECK(gpu.gpu_plane(1).size() == gpu.atlas_capacity() * 256);
    CHECK(gpu.gpu_plane(3).empty());
    CHECK(gpu.gpu_plane(4).size() == gpu.atlas_capacity() * 16);
    CHECK(std::ranges::equal(gpu.gpu_plane(2), raw_gpu.gpu_plane(2)));
    const auto decoded = decode_probes(encoded);
    CHECK(decoded.distance_moments == source.distance_moments);
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

// Packed inputs must be rejected at the public boundary before assuming a decoded raw plane layout.
void test_rejects_packed_probe_inputs() {
    for (auto depth : {ProbeDepthFormat::Rg8, ProbeDepthFormat::Rg16}) {
        const auto bytes =
            encode_baked_probes(fixture(), ProbeTextureFormat::R11G11B10, {1}, depth);
        const auto packed = decode_gpu_probes(bytes);

        for (auto colour : {ProbeTextureFormat::R11G11B10, ProbeTextureFormat::Bc6h}) {
            bool rejected = false;

            try {
                const auto result = encode_baked_probes(packed, colour);
            } catch (const std::runtime_error& error) {
                rejected = std::string_view(error.what())
                           == "probe compression requires decoded probe maps";
            }

            CHECK(rejected);
        }
    }
}

// Omitting raw depth must preserve the raw encoder's colour, padding and relocation bytes.
void test_colour_metadata_packing() {
    const auto source = fixture();
    const auto raw = pack_probe_planes(source);
    const auto compact = pack_probe_colour_metadata(source);
    // Each eight-by-eight probe has four-byte colour texels plus one eight-byte metadata record.
    const std::size_t colour_bytes = source.atlas_capacity() * source.grid.texels_per_probe() * 4;
    const std::size_t metadata_bytes = source.atlas_capacity() * 8;
    CHECK(compact.size() == colour_bytes + metadata_bytes);
    CHECK(std::equal(compact.begin(), compact.begin() + colour_bytes, raw.begin()));
    CHECK(std::equal(compact.end() - metadata_bytes, compact.end(), raw.end() - metadata_bytes));
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

// Separate FP32 range operations must not reconstruct an endpoint above the original ray cap.
void test_depth_range_round_trip() {
    constexpr float kCap = 511.831634521484375f;
    constexpr float kLow = 187.6856536865234375f;
    const float uncorrected_range = kCap - kLow;
    CHECK(kLow + uncorrected_range > kCap);
    auto source = fixture();
    source.grid.max_distance = kCap;
    for (std::size_t p = 0; p < source.probes.size(); ++p) {
        for (unsigned y = 1; y <= 6; ++y) {
            for (unsigned x = 1; x <= 6; ++x) {
                // Both endpoints appear in each independently fitted probe map.
                const float mean = x <= 3 ? kLow : kCap;
                source.distance_moments[p * 64 + std::size_t{y} * 8 + x] = {mean, mean * mean};
            }
        }
    }
    update_probe_gutters(source);
    const auto decoded = decode_probes(encode_baked_probes(source, ProbeTextureFormat::R11G11B10));
    for (const auto& value : decoded.distance_moments) {
        CHECK(value[0] <= kCap);
    }
}

// RG8 accepts every byte value, but malformed ranges and truncated payloads must still fail.
void test_invalid_depth() {
    const auto original = encode_baked_probes(fixture(), ProbeTextureFormat::Bc6h);
    const auto gpu = decode_gpu_probes(original);
    const auto offset = static_cast<std::size_t>(gpu.gpu_plane(4).data() - gpu.gpu_bytes.data());
    auto bytes = original;
    bytes[offset + 2] = 0xc0;
    bytes[offset + 3] = 0x7f;
    for (bool expand : {false, true}) {
        bool rejected = false;
        try {
            const auto decoded = expand ? decode_probes(bytes) : decode_gpu_probes(bytes);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        CHECK(rejected);
    }
}

// Corrupted byte values are finite but cannot be allowed to open a seam in hardware gathers.
void test_invalid_unorm_gutter(ProbeDepthFormat format) {
    auto bytes = encode_baked_probes(fixture(), ProbeTextureFormat::R11G11B10, {1}, format);
    // Legacy RG8 keeps its version-seven compatibility check.
    if (format == ProbeDepthFormat::Rg8) {
        bytes[4] = 7;
    }
    const auto gpu = decode_gpu_probes(bytes);
    const auto offset = static_cast<std::size_t>(gpu.gpu_plane(1).data() - gpu.gpu_bytes.data());
    // Corrupt the high byte too, so RG16 validation cannot accidentally inspect only eight bits.
    const unsigned byte = format == ProbeDepthFormat::Rg16 ? 1 : 0;
    bytes[offset + byte] ^= 1;
    bool rejected = false;
    try {
        const auto decoded = decode_gpu_probes(bytes);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CHECK(rejected);
}

// A shallow blocker must retain tiny within-texel spread despite a much wider local depth range.
void test_rg16_small_spread_visibility() {
    auto source = fixture();
    source.grid.max_distance = 32.f;
    constexpr float kMean = .033f, kSpread = .008f, kReceiverOffset = .001f;

    // Other directions force wide mean and spread ranges in the same probe tile.
    for (auto& moment : source.distance_moments) {
        moment = {16.f, 16.f * 16.f + 15.f * 15.f};
    }

    source.distance_moments[9] = {kMean, kMean * kMean + kSpread * kSpread};
    source.distance_moments[10] = {.027f, .027f * .027f};
    source.distance_moments[11] = {32.f, 32.f * 32.f};
    update_probe_gutters(source);
    const auto decoded = decode_probes(encode_baked_probes(source, ProbeTextureFormat::R11G11B10));
    const auto value = decoded.distance_moments[9];
    const float variance = std::max(0.f, value[1] - value[0] * value[0]);
    const float gap = kMean + kReceiverOffset - value[0];
    const float visibility = std::pow(variance / (variance + gap * gap), 3.f);
    const float expected =
        std::pow(kSpread * kSpread / (kSpread * kSpread + kReceiverOffset * kReceiverOffset), 3.f);
    CHECK(std::abs(value[0] - kMean) < .00001f);
    CHECK(std::abs(visibility - expected) < .01f);
}

// Legacy ASTC depth assets remain readable alongside the RG16 bake default.
void test_legacy_astc_depth() {
    const auto source = fixture();
    auto bytes =
        encode_baked_probes(source, ProbeTextureFormat::R11G11B10, {1}, ProbeDepthFormat::Astc4x4);
    bytes[4] = 7;  // Previously published ASTC assets used the version-seven header.
    const auto gpu = decode_gpu_probes(bytes);
    CHECK(gpu.depth_format == ProbeDepthFormat::Astc4x4);
    CHECK(gpu.gpu_plane(3).size() == gpu.atlas_capacity() * 64);
    CHECK(decode_probes(bytes).distance_moments == source.distance_moments);
}

// New readers accept the exact version-six header without treating its colour tag as a depth tag.
void test_legacy_depth() {
    const auto source = fixture();
    auto bytes = encode_probes(source);
    const auto gpu = decode_gpu_probes(bytes);
    bytes.erase(bytes.begin() + gpu.gpu_offset - 8, bytes.begin() + gpu.gpu_offset - 4);
    bytes[4] = 6;  // Version six has no depth-format word.
    const auto legacy = decode_gpu_probes(bytes);
    CHECK(legacy.depth_format == ProbeDepthFormat::Moments);
    CHECK(decode_probes(bytes).distance_moments == source.distance_moments);
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

        test_probe_planes(ProbeTextureFormat::R11G11B10);
        for (auto format : {ProbeTextureFormat::Bc6h, ProbeTextureFormat::Astc8x8}) {
            test_probe_planes(format);
            test_reflection_planes(format);
        }

        test_rejects_packed_probe_inputs();
        test_colour_metadata_packing();
        test_astc_verification_rounding();
        test_parallel_edges();
        test_malformed_blocks();
        test_invalid_depth();
        test_invalid_unorm_gutter(ProbeDepthFormat::Rg8);
        test_invalid_unorm_gutter(ProbeDepthFormat::Rg16);
        test_legacy_depth();
        test_legacy_astc_depth();
        test_depth_range_round_trip();
        test_rg16_small_spread_visibility();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
