// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>

#include "enhanced/lighting/environment_gi.h"
#include "enhanced/lighting/probe_density.h"
#include "enhanced/lighting/probe_texture_encode.h"
#include "tests/check.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
ProbeVolume field(bool coarse = false) {
    ProbeGrid grid;
    grid.origin = coarse ? Float3{-16, -16, -16} : Float3{};
    grid.spacing = coarse ? Float3{32, 32, 32} : Float3{1, 1, 1};
    grid.dimensions = coarse ? std::array<unsigned, 3>{2, 2, 2} : std::array<unsigned, 3>{9, 9, 9};
    auto volume = ProbeVolume::allocate(grid);

    for (auto& probe : volume.probes) {
        probe.valid = true;
    }

    for (auto& texel : volume.irradiance) {
        texel = {.25f, .5f, 1};
    }

    return volume;
}

std::vector<ProbeDensityReceiver> receivers() {
    std::vector<ProbeDensityReceiver> result;

    for (float x : {.1f, 1.f, 3.2f, 4.f, 7.f, 7.9f}) {
        for (auto normal : {Float3{1, 0, 0}, Float3{0, 1, 0}, Float3{0, 0, 1}}) {
            result.push_back({{x, 4.2f, 4.2f}, normal});
        }
    }

    return result;
}

// Keeps geometric assertions focused on the chosen raw grid while also checking retained payloads.
ProbeVolume reduce_field(
    ProbeVolume fine,
    const ProbeDensityInput& input,
    const ProbeDensitySettings& settings,
    ProbeDensityReport& report
) {
    auto result = simplify_probe_density(std::move(fine), input, settings);
    report = result.report;
    CHECK(result.bytes == encode_baked_probes(result.volume, settings.texture_format));
    return std::move(result.volume);
}

// Preserves uniform lighting, endpoints and per-probe metadata under reduction.
void uniform_field() {
    auto fine = field();
    const auto original = fine.grid;
    const auto coarse = field(true);
    ProbeDensityReport report;
    const auto reduced = reduce_field(fine, {coarse, receivers()}, {true}, report);
    CHECK((reduced.grid.dimensions == std::array<unsigned, 3>({5, 5, 5})));
    CHECK(reduced.grid.spacing == Float3({2, 2, 2}));
    CHECK(report.original_probes == 729 && report.final_probes == 125);
    CHECK(report.accepted_axes == 3 && report.receivers == receivers().size());

    for (unsigned a = 0; a < 3; ++a) {
        CHECK(reduced.grid.origin[a] == original.origin[a]);
        CHECK(reduced.grid.spacing[a] * float(reduced.grid.dimensions[a] - 1) == 8);
    }

    // Kept probes retain their own offsets and unmodified directional maps/moments.
    fine.probes[2].offset = {.1f, 0, 0};
    fine.probes[2].relocated = true;
    const auto copy = reduce_field(fine, {coarse, receivers()}, {true}, report);
    CHECK(copy.probes[1].offset == fine.probes[2].offset);
    CHECK(copy.probes[1].relocated);
    CHECK(copy.distance_moments[0] == fine.distance_moments[0]);
    CHECK(copy.irradiance[0] == fine.irradiance[0]);
}

// Rejects density changes that lose directional light or visibility boundaries.
void retain_lighting_and_visibility_changes() {
    for (bool visibility : {false, true}) {
        auto fine = field();

        for (unsigned z = 0; z < 9; ++z) {
            for (unsigned y = 0; y < 9; ++y) {
                const auto probe = 3 + 9 * (y + 9 * z);

                for (std::size_t texel = 0; texel < fine.grid.texels_per_probe(); ++texel) {
                    const auto index = probe * fine.grid.texels_per_probe() + texel;

                    if (visibility) {
                        fine.distance_moments[index] = {.1f, .01f};
                    } else {
                        fine.irradiance[index] = {2, .01f, .01f};
                    }
                }
            }
        }

        ProbeDensityReport report;
        const auto reduced = reduce_field(fine, {field(true), receivers()}, {true}, report);
        CHECK(reduced.grid.spacing[0] == 1);
        CHECK(report.rejected_axes >= 1);
    }
}

// Keeps unsupported or incompatible lattices at their original density.
void retain_missing_evidence_and_odd_endpoints() {
    auto fine = field();
    ProbeDensityReport report;
    CHECK(reduce_field(fine, {field(true), {}}, {true}, report).probes.size() == 729);
    CHECK(reduce_field(fine, {field(true), receivers()}, {}, report).probes.size() == 729);
    fine.grid.dimensions = {4, 4, 4};
    fine = ProbeVolume::allocate(fine.grid);
    const std::array<ProbeDensityReceiver, 1> points{{{{1, 1, 1}, {0, 0, 1}}}};
    CHECK(reduce_field(fine, {field(true), points}, {true}, report).probes.size() == 64);
}

// Density checks must use the shared compression workers without changing any reduction decisions.
void compressed_density_workers() {
    auto fine = field();
    const auto coarse = field(true);
    const auto points = receivers();
    BakeThreadPool workers(3);  // Exercise a borrowed pool across multiple encoded candidates.
    // A nonzero shape-dependent hash catches stale accepted-candidate headers without production helpers.
    const auto identity = [](const ProbeGrid& grid) {
        return std::uint64_t{1000} + grid.probe_count();
    };
    fine.grid.settings_hash = identity(fine.grid);

    for (auto format : {ProbeTextureFormat::Bc6h, ProbeTextureFormat::Astc8x8}) {
        ProbeDensitySettings settings{true};
        settings.texture_format = format;
        const auto serial = simplify_probe_density(fine, {coarse, points, {1}, identity}, settings);
        const auto parallel =
            simplify_probe_density(fine, {coarse, points, {0, &workers}, identity}, settings);
        CHECK(serial.bytes == parallel.bytes);
        CHECK(serial.bytes == encode_baked_probes(serial.volume, format));
        CHECK(serial.reference_bytes == encode_baked_probes(fine, format));
        CHECK(serial.volume.grid.settings_hash == identity(serial.volume.grid));
        CHECK(serial.report.accepted_axes == parallel.report.accepted_axes);
        CHECK(serial.report.rejected_axes == parallel.report.rejected_axes);
        CHECK(serial.report.maximum_error == parallel.report.maximum_error);
    }
}

void oriented_field_bins() {
    auto local = field();
    local.grid.origin = {-40, 0, 0};
    local.grid.frame.axis = {0, 1};
    local.grid.spacing = {8, 8, 1};
    const auto coarse = field(true);
    const std::array<ProbeVolume, 2> volumes{coarse, local};
    const CaptureGi field(volumes);
    EnvironmentBakeSettings bias;
    bias.normal_bias = bias.view_bias = .5f;
    const Float3 p{-72, 32, 4};
    const auto expected = sample_probe_volumes(coarse, &local, p, {0, 0, 1}, .5f, {0, 0, .5f});
    CHECK(expected.has_value());
    CHECK(field.sample(p, {0, 0, 1}, {0, 0, 1}, bias) == expected->irradiance);
}
}  // namespace

int main() {
    uniform_field();
    retain_lighting_and_visibility_changes();
    retain_missing_evidence_and_odd_endpoints();
    compressed_density_workers();
    oriented_field_bins();
    return openrac::test::result();
}
