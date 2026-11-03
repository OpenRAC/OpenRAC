// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// MIS energy conservation and transparent rectangle visibility against analytic fixtures.
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"
#include "enhanced/tests/probe_bake_fixtures.h"

using namespace openrac::lighting;
using namespace openrac::lighting::bake_detail;

namespace {
// A large nearby ceiling approaches a constant-radiance hemisphere with irradiance pi times Le.
void test_hemisphere_energy(bool mesh) {
    BakeScene input;
    input.environment = {};
    const Float3 colour{1, 2, 3};
    // Large extent makes the finite ceiling's missing horizon negligible.
    constexpr float kExtent = 1000;

    // Both representations must approach the same emitting hemisphere.
    if (mesh) {
        auto faces = bake_test::box({-kExtent, -kExtent, 1}, {kExtent, kExtent, 2});

        // Only the lower face emits; other faces have zero albedo.
        for (std::size_t i = 0; i < 2; ++i) {
            faces[i].emission = colour;
        }

        input.triangles = std::move(faces);
    } else {
        input.area_emitters.push_back({{0, 0, 1}, {kExtent, 0, 0}, {0, -kExtent, 0}, colour});
    }

    Scene scene(input);
    BakeSettings settings;
    settings.bounces = 0;
    Random random{1};
    Random emission_random{17};
    Float3 sum{};
    constexpr unsigned kSamples = 8192;
    const double pdf = 1 / (4.0 * kPi);

    // Pair independent area samples with uniformly stratified sphere directions.
    for (unsigned i = 0; i < kSamples; ++i) {
        auto light = sample_emitter(scene, {}, settings.ray_bias, emission_random, settings);
        weight_emitter(light, pdf);
        const float z = 1 - 2 * (static_cast<float>(i) + .5f) / kSamples;
        // Golden-angle azimuth matches the primary sphere sequence.
        constexpr float kGoldenAngle = 2.39996323f;
        const float angle = static_cast<float>(i) * kGoldenAngle;
        const float radius = std::sqrt(1 - z * z);
        auto hit = scene.trace(
            {}, {radius * std::cos(angle), radius * std::sin(angle), z}, settings.ray_bias, INFINITY
        );
        const auto value = radiance(scene, settings, hit, random);

        // Both estimates include the receiving cosine, but only sphere samples need 4*pi.
        for (unsigned c = 0; c < 3; ++c) {
            sum[c] += (value[c] * std::max(0.f, z) * 4 * kPi
                       + light.radiance_weight[c] * std::max(0.f, light.direction[2]))
                      / kSamples;
        }
    }

    // Missing the horizon changes less than one percent at this ceiling extent.
    for (unsigned c = 0; c < 3; ++c) {
        CHECK(std::abs(sum[c] / (kPi * colour[c]) - 1) < .01f);
    }
}

// Rectangles emit independently without blocking each other or changing geometry distances.
void test_transparent_rectangles() {
    BakeScene input;
    input.area_emitters.push_back({{0, 0, 1}, {2, 0, 0}, {1, -2, 0}, {1, 2, 3}});
    input.area_emitters.push_back(input.area_emitters.front());
    Scene scene(input);
    BakeSettings settings;
    auto hit = scene.trace({}, {0, 0, 1}, settings.ray_bias, INFINITY);
    CHECK(hit.hit.geomID == RTC_INVALID_GEOMETRY_ID);
    CHECK(!scene.occluded({}, {0, 0, 1}, settings.ray_bias, INFINITY));
    const auto both = segment_emission(scene, settings, hit, 1);
    const double pdf = .5 / 16;
    const float weight = emission_mis_weight(1, pdf);
    CHECK(std::abs(both[0] - 2 * weight) < .00001f);
    hit.ray.tfar = .5f;
    CHECK((segment_emission(scene, settings, hit, 1) == Float3{}));
    hit = scene.trace({0, 0, 2}, {0, 0, -1}, settings.ray_bias, INFINITY);
    CHECK((segment_emission(scene, settings, hit, 1) == Float3{}));
}

// Stack scratch limits must switch to complete evaluation rather than omit visible emission.
void test_oversized_crossings() {
    // Exercise both the crossing-buffer limit and the visitation-bit limit.
    for (unsigned count : {65u, 4097u}) {
        BakeScene input;
        input.area_emitters.resize(count, {{0, 0, 1}, {2, 0, 0}, {0, -2, 0}, {1, 0, 0}});
        Scene scene(input);
        const auto hit = scene.trace({}, {0, 0, 1}, .002f, INFINITY);
        const auto value = segment_emission(scene, {}, hit, 1);
        const float expected =
            static_cast<float>(count) * emission_mis_weight(1, 1.0 / (16 * count));
        CHECK(std::abs(value[0] / expected - 1) < .00001f);
    }
}

// Power weights sum to one while discrete point sources retain their complete estimate.
void test_estimator_weights() {
    CHECK(
        std::abs(emission_mis_weight(1e-100, 1e100) + emission_mis_weight(1e100, 1e-100) - 1)
        < .00001f
    );
    CHECK(BakeSettings{}.rays_per_probe == 256);
    CHECK(emission_mis_weight(0, 1) == 0);
    CHECK(emission_mis_weight(1, 0) == 1);
    EmissionSample point{{0, 0, 1}, {1, 2, 3}, 0, true};
    weight_emitter(point, 1);
    CHECK((point.radiance_weight == Float3{1, 2, 3}));
}
}  // namespace

// Runs analytic cases without level assets or a GPU.
int main() {
    // Fixture setup failures must fail the test rather than escape its entry point.
    try {
        test_estimator_weights();
        test_transparent_rectangles();
        test_oversized_crossings();
        test_hemisphere_energy(false);
        test_hemisphere_energy(true);
    } catch (...) {
        return 1;
    }

    return openrac::test::result();
}
