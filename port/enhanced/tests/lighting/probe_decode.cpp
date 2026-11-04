// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Pins the former two-pass CPU expansion against single-pass decoding with synthetic GPU planes.
#include <bit>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

#include "enhanced/lighting/probe_atlas.h"
#include "tests/check.h"

namespace {
using namespace openrac::lighting;

// Builds two atlas rows with varied quantised colour, moments, offsets and all four statuses.
ProbeVolume fixture() {
    ProbeGrid grid;
    grid.dimensions = {5, 5, 3};
    grid.map_resolution = 2;
    grid.frame.axis = {.6f, .8f};
    auto volume = ProbeVolume::allocate(grid);

    // Arithmetic test values cross channel exponents without approaching relocation limits.
    for (std::size_t index = 0; index < volume.probes.size(); ++index) {
        volume.probes[index] = {{.5f, -.25f, 1}, index % 2 == 0, index % 3 == 0};

        // Every padded texel varies, including gutters and the second atlas row.
        for (std::size_t texel = 0; texel < grid.texels_per_probe(); ++texel) {
            const auto at = index * grid.texels_per_probe() + texel;
            const float value = static_cast<float>(at % 127) / 16;
            volume.irradiance[at] = {value, value * value, value / 1024};
            volume.distance_moments[at] = {value, value * value + 1};
        }
    }

    return volume;
}

// Reproduces the previous public decoder's packed scan, expansion and final validation.
ProbeVolume former_decode(const std::vector<std::uint8_t>& bytes) {
    auto volume = decode_gpu_probes(bytes);
    read_probe_planes(volume, true);
    volume.gpu_bytes.clear();
    volume.gpu_offset = 0;
    volume.validate();
    return volume;
}

// Compares channel representations without accepting signed-zero or rounding differences.
bool same_bits(Float3 actual, Float3 expected) {
    // Each float channel has the same portable 32-bit representation used by the file format.
    for (unsigned channel = 0; channel < 3; ++channel) {
        // Any representation change would alter the former reference sampler.
        if (std::bit_cast<std::uint32_t>(actual[channel])
            != std::bit_cast<std::uint32_t>(expected[channel])) {
            return false;
        }
    }

    return true;
}

// Uses bit comparisons so signed zero and rounding differences cannot pass as equal values.
void compare_fields(const ProbeVolume& actual, const ProbeVolume& expected) {
    CHECK(actual.probes.size() == expected.probes.size());
    CHECK(actual.irradiance.size() == expected.irradiance.size());
    CHECK(actual.distance_moments == expected.distance_moments);

    // Metadata and map samples must retain their former rounding and indexing.
    for (std::size_t index = 0; index < actual.probes.size(); ++index) {
        CHECK(actual.probes[index].offset == expected.probes[index].offset);
        CHECK(actual.probes[index].valid == expected.probes[index].valid);
        CHECK(actual.probes[index].relocated == expected.probes[index].relocated);
    }

    // One comparison per texel retains every channel's exact decoded bits.
    for (std::size_t texel = 0; texel < actual.irradiance.size(); ++texel) {
        CHECK(same_bits(actual.irradiance[texel], expected.irradiance[texel]));
    }
}

// CPU expansion stays bit-identical in every supported runtime and offline rounding mode.
void test_expansion_matches_former_path() {
    const auto bytes = encode_probes(fixture());
    std::fenv_t previous{};
    CHECK(std::fegetenv(&previous) == 0);

    // Reused bake workers and runtime verification can select different arithmetic environments.
    for (int mode : {FE_TONEAREST, FE_TOWARDZERO, FE_DOWNWARD, FE_UPWARD}) {
        CHECK(std::fesetround(mode) == 0);
        const auto expected = former_decode(bytes);
        const auto actual = decode_probes(bytes);
        compare_fields(actual, expected);
        CHECK(actual.gpu_bytes.empty());
        CHECK(actual.gpu_bytes.capacity() == 0);
        CHECK(std::fegetround() == mode);
    }

    CHECK(std::fesetenv(&previous) == 0);
}

// Returns rejection text so both paths must reject the same malformed packed field.
std::string decode_error(const std::vector<std::uint8_t>& bytes, bool former) {
    // The two decoders must report the same malformed channel rather than returning a field.
    try {
        const auto volume = former ? former_decode(bytes) : decode_probes(bytes);
        return {};
    } catch (const std::runtime_error& error) {
        return error.what();
    }
}

// Corrupts one little-endian channel without relying on a platform struct layout.
void check_corruption(
    const std::vector<std::uint8_t>& original, std::size_t offset, std::uint32_t value
) {
    auto bytes = original;

    // Four bytes cover an irradiance word, float moment or two half metadata channels.
    for (unsigned byte = 0; byte < 4; ++byte) {
        bytes.at(offset + byte) = static_cast<std::uint8_t>(value >> (byte * 8));
    }

    const auto expected = decode_error(bytes, true);
    CHECK(!expected.empty());
    CHECK(decode_error(bytes, false) == expected);
}

// Single-pass expansion preserves packed finite, range, status and format rejection checks.
void test_invalid_packed_data_still_fails() {
    const auto bytes = encode_probes(fixture());
    const auto gpu = decode_gpu_probes(bytes);
    const auto irradiance = gpu.gpu_offset;
    const auto moments = irradiance + gpu.gpu_plane(0).size();
    const auto metadata = moments + gpu.gpu_plane(1).size();
    check_corruption(bytes, irradiance, 31u << 6);
    check_corruption(bytes, irradiance, 31u << 17);
    check_corruption(bytes, irradiance, 31u << 27);
    check_corruption(bytes, moments, 0x7FC00000u);
    check_corruption(bytes, moments, std::bit_cast<std::uint32_t>(-1.f));
    check_corruption(bytes, moments + 4, 0x7F800000u);
    check_corruption(
        bytes,
        moments + 4,
        std::bit_cast<std::uint32_t>(2 * gpu.grid.max_distance * gpu.grid.max_distance)
    );
    check_corruption(bytes, metadata, 0x7C00u);
    check_corruption(bytes, metadata + 4, 0x42000000u);
    check_corruption(bytes, metadata, 0x4C00u);
    check_corruption(bytes, 4, kProbeFormatVersion + 1);
    CHECK(!decode_error(std::vector<std::uint8_t>(bytes.begin(), bytes.end() - 1), false).empty());
}
}  // namespace

int main() {
    // Unexpected format or allocation errors fail this test without crossing the executable boundary.
    try {
        test_expansion_matches_former_path();
        test_invalid_packed_data_still_fails();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
