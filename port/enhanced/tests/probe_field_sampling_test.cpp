// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Analytic directional fields verify serial/parallel receiver evidence, rounding and shared ownership.
#include <array>
#include <cfenv>
#include <cstdio>
#include <exception>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "common/joining_thread.h"
#include "enhanced/lighting/environment_gi.h"
#include "enhanced/lighting/probe_field_sampling.h"
#include "tests/check.h"

namespace {
using openrac::lighting::Float3;
using openrac::lighting::ProbeFieldSamples;
using openrac::lighting::ProbeVolume;
using openrac::lighting::bake_detail::ProbeDensityReceiver;

// Builds bounded coarse/local fields with changing RGB values and unobstructed distance moments.
std::vector<ProbeVolume> fields() {
    std::vector<ProbeVolume> volumes;

    // Two grids exercise both coarse-only and blended receiver positions.
    for (unsigned index = 0; index < 2; ++index) {
        openrac::lighting::ProbeGrid grid;
        grid.origin = index == 0 ? Float3{-8, -8, -8} : Float3{-2, -2, -2};
        grid.spacing = index == 0 ? Float3{16, 16, 16} : Float3{2, 2, 2};
        grid.dimensions =
            index == 0 ? std::array<unsigned, 3>{2, 2, 2} : std::array<unsigned, 3>{3, 3, 3};
        auto volume = ProbeVolume::allocate(grid);

        // Every allocated probe supplies valid support in this analytic fixture.
        for (auto& probe : volume.probes) {
            probe.valid = true;
        }

        // Binary fractions vary spatially and directionally without authored data.
        for (std::size_t texel = 0; texel < volume.irradiance.size(); ++texel) {
            const float value = static_cast<float>(texel % 17) / 16;
            volume.irradiance[texel] = {value, 0.25f + value / 2, 1 - value / 4};
            volume.distance_moments[texel] =
                {grid.max_distance, grid.max_distance * grid.max_distance};
        }

        volumes.push_back(std::move(volume));
    }

    return volumes;
}

// Includes more than one comparison chunk and points beyond every grid's support.
std::vector<ProbeDensityReceiver> receivers() {
    std::vector<ProbeDensityReceiver> values;
    // More than 512 receivers exercise multiple claim and comparison chunk boundaries.
    constexpr unsigned kReceiverCount = 1057;
    values.reserve(kReceiverCount);

    // Receiver positions exercise multiple chunks and repeated coarse/local boundary crossings.
    for (unsigned index = 0; index < kReceiverCount; ++index) {
        const float x = static_cast<float>(index % 17) * 0.5f - 4;
        const float y = static_cast<float>((index / 17) % 9) * 0.5f - 2;
        // Periodic distant receivers exercise explicit unsupported samples.
        const float z = index % 31 == 0 ? 64.f : .125f;
        values.push_back({{x, y, z}, {0, 0, 1}});
    }

    return values;
}

// Compares the five view samples without a tolerance that could hide arithmetic changes.
void check_views(const ProbeFieldSamples& actual, const ProbeFieldSamples& expected) {
    // Each view must retain both support classification and exact RGB evidence.
    for (unsigned view = 0; view < actual.size(); ++view) {
        CHECK(actual[view].supported == expected[view].supported);
        CHECK(actual[view].irradiance == expected[view].irradiance);
    }
}

// Requires exact evidence equality in receiver order after workers have drained.
void check_samples(
    std::span<const ProbeFieldSamples> actual, std::span<const ProbeFieldSamples> expected
) {
    CHECK(actual.size() == expected.size());

    // Receiver order must remain independent of task completion order.
    for (std::size_t index = 0; index < actual.size(); ++index) {
        check_views(actual[index], expected[index]);
    }
}

// Parallel view queries preserve the controller's rounding mode and serial evidence.
void test_rounding_and_worker_budgets() {
    const auto volumes = fields();
    const openrac::lighting::bake_detail::CaptureGi field(volumes);
    const auto points = receivers();
    openrac::lighting::BakeThreadPool pool(4);
    const int original = std::fegetround();

    // Successive calls reuse workers across different directed rounding modes.
    for (const int mode : {FE_TONEAREST, FE_TOWARDZERO, FE_DOWNWARD, FE_TONEAREST}) {
        CHECK(std::fesetround(mode) == 0);
        std::vector<ProbeFieldSamples> expected(points.size());
        CHECK(openrac::lighting::sample_probe_field({field, points, expected}));
        CHECK(!expected.front().front().supported);
        CHECK(expected[1].front().supported);

        // Every requested worker budget must reproduce the same serial sample evidence.
        for (const unsigned threads : {1u, 2u, 4u}) {
            std::vector<ProbeFieldSamples> actual(points.size());
            const auto result =
                openrac::lighting::sample_probe_field({field, points, actual}, {threads, &pool});
            // Keep CHECK's internal branches outside this already nested worker-budget loop.
            check_samples(actual, expected);

            // A changed status or controller rounding mode is a test failure.
            if (!result || std::fegetround() != mode) {
                throw std::runtime_error("parallel field sampling changed status or rounding");
            }
        }
    }

    CHECK(std::fesetround(original) == 0);
}

// Concurrent controllers share one execution budget while retaining separate sample buffers.
void test_concurrent_controllers() {
    const auto volumes = fields();
    const openrac::lighting::bake_detail::CaptureGi field(volumes);
    const auto points = receivers();
    std::vector<ProbeFieldSamples> expected(points.size());
    CHECK(openrac::lighting::sample_probe_field({field, points, expected}));

    // Both single-thread and parallel pools must support concurrent controllers.
    for (const unsigned threads : {1u, 4u}) {
        openrac::lighting::BakeThreadPool pool(threads);
        std::vector<ProbeFieldSamples> first(points.size());
        std::vector<ProbeFieldSamples> second(points.size());
        bool first_ok = false;
        bool second_ok = false;
        {
            // Both controllers join before their borrowed field and output buffers leave scope.
            openrac::JoiningThread first_controller([&] {
                first_ok =
                    bool(openrac::lighting::sample_probe_field({field, points, first}, {0, &pool}));
            });
            openrac::JoiningThread second_controller([&] {
                second_ok = bool(
                    openrac::lighting::sample_probe_field({field, points, second}, {0, &pool})
                );
            });
        }
        CHECK(first_ok && second_ok);
        check_samples(first, expected);
        check_samples(second, expected);
    }
}

// Invalid output sizes are rejected before partial publication, and empty batches are ordinary success.
void test_buffer_validation() {
    const auto volumes = fields();
    const openrac::lighting::bake_detail::CaptureGi field(volumes);
    const auto points = receivers();
    std::vector<ProbeFieldSamples> output(points.size() - 1);
    openrac::lighting::BakeThreadPool pool(2);
    CHECK(!openrac::lighting::sample_probe_field({field, points, output}, {2, &pool}));
    CHECK(openrac::lighting::sample_probe_field({field, {}, {}}, {2, &pool}));
}
}  // namespace

int main() {
    // Task and allocation errors must report test failure after all borrowed state has drained.
    try {
        test_rounding_and_worker_budgets();
        test_concurrent_controllers();
        test_buffer_validation();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
