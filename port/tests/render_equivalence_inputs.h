// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Immutable CPU inputs for the equivalence scenario matrix.

#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "tests/render_fixture.h"

namespace openrac::test {

// One immutable presentation input shared by independent renderer runs.
struct FixtureFrame {
    openrac::renderer::Camera camera;
    render::MobyFrame mobys;
};

// All scenario inputs in registry order, owned by one test selection.
using FixtureInputs = std::array<std::vector<FixtureFrame>, render::kScenarios.size()>;

// Computes cameras, poses and churn once without caching mutable renderer state.
inline FixtureInputs prepare_inputs(
    const openrac::scene::LevelData& level,
    const render::SceneSize& size,
    int width,
    int height,
    std::uint64_t frames
) {
    FixtureInputs inputs;
    const auto walk = render::walk_over(level);
    for (std::size_t scenario_index = 0; scenario_index < render::kScenarios.size();
         ++scenario_index) {
        const auto& scenario = render::kScenarios[scenario_index];
        const double ticks = static_cast<double>(frames) / (scenario.between + 1);
        for (std::uint64_t index = 0; index < frames; ++index) {
            const auto shown = render::picture(index, scenario.between);
            const double tick = static_cast<double>(shown.tick - 1) + shown.alpha;
            auto mobys = render::mobys_at(level, size, shown);
            if (scenario.churn) {
                mobys = render::churned(std::move(mobys), shown.tick);
            }
            inputs[scenario_index].push_back(
                {render::camera_at(
                     walk, scenario.camera, tick, ticks, static_cast<float>(width) / height
                 ),
                 std::move(mobys)}
            );
        }
    }
    return inputs;
}

}  // namespace openrac::test
