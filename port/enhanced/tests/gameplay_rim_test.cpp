// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks GI-coloured rims through the real prepass, early lighting and instanced mesh draws.

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/viewer/early_lighting.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/framebuffer.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;

// Draws two instances of one model so eligibility cannot bleed across an instanced group.
void check_rim(bool skinned, float albedo, bool chrome = false) {
    scene::LevelData level;
    level.light_sets.resize(256);
    for (const auto& point :
         {renderer::Vec3{-.6f, -.6f, 0},
          renderer::Vec3{.6f, -.6f, 0},
          renderer::Vec3{.6f, .6f, 0},
          renderer::Vec3{-.6f, .6f, 0}}) {
        scene::Vertex vertex{};
        std::copy(point.begin(), point.end(), vertex.position);
        vertex.normal[0] = std::sqrt(.96f);
        vertex.normal[2] = .2f;
        std::fill_n(vertex.colour, 4, 1.0f);
        vertex.light_slot = -1;
        vertex.weights[0] = 1;
        level.vertices.push_back(vertex);
    }
    level.indices = {0, 1, 2, 0, 2, 3};
    level.materials.emplace_back();
    level.materials.back().colour = {albedo, albedo, albedo, 1};
    scene::Model model;
    model.min = {-.6f, -.6f, 0};
    model.max = {.6f, .6f, 0};
    model.primitives.push_back({0, 6, 0});
    if (chrome) {
        level.materials.emplace_back();
        level.materials.back().metal = true;
        model.primitives.push_back({0, 6, 1});
    }
    model.joints = skinned ? 1 : 0;
    level.models.push_back(model);
    std::vector<scene::Instance> instances(2);
    for (unsigned i = 0; i < instances.size(); ++i) {
        instances[i].matrix = renderer::translation({i == 0 ? -.8f : .8f, 0, -4});
        instances[i].palette = skinned ? 0 : -1;
        instances[i].moby_light = {0, 0, 0, 1};
    }
    level.instances[static_cast<std::size_t>(scene::Layer::Mobys)] = instances;
    renderer::Renderer renderer;
    scene::LevelScene scene;
    lighting::use_scene_lighting(scene);
    std::string error;
    CHECK(scene.upload(level, renderer.textures(), error));
    if (skinned) {
        scene.set_palette({renderer::identity()});
    }
    scene.set_probe_lighting(true, 1);
    scene.set_probe_bias(0, 0);
    scene.set_gameplay_rim_lift(false, .09f, 7.1f);
    // The terrain bucket owns the shared early-lighting dispatch, even for a moby-only fixture.
    CHECK(
        renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Terrain), error)
    );
    CHECK(renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Mobys), error));
    CHECK(renderer.init(error));
    renderer::FrameBuffer frame;
    CHECK(frame.create(64, 64, error));
    renderer::FrameInput input;
    input.camera.view = renderer::identity();
    input.camera.projection = renderer::perspective_reversed(1.0f, 1, .1f, 100);
    input.clear_colour = {0, 0, 0, 1};
    lighting::ProbeGrid grid;
    grid.origin = {-8, -8, -8};
    grid.spacing = {16, 16, 16};
    auto volume = lighting::ProbeVolume::allocate(grid);
    for (auto& probe : volume.probes) {
        probe.valid = true;
    }
    for (auto& moments : volume.distance_moments) {
        moments = {1024, 1024 * 1024};
    }
    const auto draw = [&](bool left, bool right) {
        instances[0].gameplay_essential = left;
        instances[1].gameplay_essential = right;
        scene.set_instances(scene::Layer::Mobys, instances);
        ++input.frame;
        renderer.render(input, {frame.id(), 64, 64});
        return frame.read_rgba();
    };
    // Pure red and pure blue GI make any white or fixed-colour outline observable.
    for (unsigned channel : {0u, 2u}) {
        for (auto& texel : volume.irradiance) {
            texel = {};
            texel[channel] = std::numbers::pi_v<float> * .3f;
        }
        CHECK(lighting::scene_lighting(scene)->set_probe_volume(test::gpu_probes(volume), error));
        const auto baseline = draw(false, false);
        const auto left = draw(true, false);
        const auto right = draw(false, true);
        unsigned changed_left = 0, changed_right = 0;
        for (unsigned y = 0; y < 64; ++y) {
            for (unsigned x = 0; x < 64; ++x) {
                const unsigned at = (y * 64 + x) * 4;
                for (unsigned c = 0; c < 4; ++c) {
                    CHECK(left[at + c] >= baseline[at + c]);
                    CHECK(right[at + c] >= baseline[at + c]);
                    if (c != channel || x >= 32) {
                        CHECK(left[at + c] == baseline[at + c]);
                    }
                    if (c != channel || x < 32) {
                        CHECK(right[at + c] == baseline[at + c]);
                    }
                }
                changed_left += left[at + channel] > baseline[at + channel];
                changed_right += right[at + channel] > baseline[at + channel];
            }
        }
        CHECK(changed_left > 0 && changed_right > 0);
        scene.set_gameplay_rim(false, .30f, 6.46f);
        CHECK(draw(true, true) == baseline);
        scene.set_gameplay_rim(true, 0, 6.46f);
        CHECK(draw(true, true) == baseline);
        scene.set_gameplay_rim(true, .60f, 6.46f);
        const auto stronger = draw(true, false);
        scene.set_gameplay_rim(true, .30f, 8);
        const auto narrower = draw(true, false);
        unsigned brighter = 0, dimmer = 0;
        for (std::size_t at = 0; at < left.size(); ++at) {
            CHECK(stronger[at] >= left[at]);
            CHECK(narrower[at] <= left[at]);
            brighter += stronger[at] > left[at];
            dimmer += narrower[at] < left[at];
        }
        CHECK(brighter > 0 && dimmer > 0);
        scene.set_gameplay_rim(true, .30f, 6.46f);
        for (int mode : {2, 3, 4}) {
            scene.set_probe_view(mode);
            CHECK(draw(false, false) == draw(true, true));
        }
        scene.set_probe_view(1);
    }
    if (albedo == 0 && !chrome) {
        // Mixed, dim GI must brighten without changing its RGB ratios on a dark skinned object.
        for (auto& texel : volume.irradiance) {
            texel =
                {.006f * std::numbers::pi_v<float>,
                 .012f * std::numbers::pi_v<float>,
                 .024f * std::numbers::pi_v<float>};
        }
        CHECK(lighting::scene_lighting(scene)->set_probe_volume(test::gpu_probes(volume), error));
        scene.set_gameplay_rim(true, 1, 1);
        scene.set_gameplay_rim_lift(false, .5f, 4);
        const auto unlifted = draw(true, false);
        scene.set_gameplay_rim_lift(true, 1, 4);
        CHECK(draw(true, false) == unlifted);
        scene.set_gameplay_rim_lift(true, .5f, 1);
        CHECK(draw(true, false) == unlifted);
        scene.set_gameplay_rim_lift(true, .5f, 2);
        const auto capped = draw(true, false);
        scene.set_gameplay_rim_lift(true, .5f, 4);
        const auto lifted = draw(true, false);
        unsigned brighter = 0, hue_checks = 0;
        const auto linear = [](unsigned byte) {
            const float value = static_cast<float>(byte) / 255;
            return value <= .04045f ? value / 12.92f : std::pow((value + .055f) / 1.055f, 2.4f);
        };
        for (std::size_t at = 0; at < lifted.size(); at += 4) {
            for (unsigned c = 0; c < 4; ++c) {
                CHECK(lifted[at + c] >= capped[at + c]);
                CHECK(capped[at + c] >= unlifted[at + c]);
                brighter += lifted[at + c] > capped[at + c];
                // The gain cap applies before the shared peak compression and display encoding.
                CHECK(linear(capped[at + c]) <= 2 * linear(unlifted[at + c]) + .003f);
            }
            if (lifted[at + 2] > 16) {
                const float red = linear(lifted[at]), green = linear(lifted[at + 1]);
                const float blue = linear(lifted[at + 2]), total = red + green + blue;
                CHECK(std::abs(red / total - 1.f / 7) < .025f);
                CHECK(std::abs(green / total - 2.f / 7) < .025f);
                ++hue_checks;
            }
        }
        CHECK(brighter > 0 && hue_checks > 0);
        // Above the reference luminance, the optional transform leaves the output unchanged.
        for (auto& texel : volume.irradiance) {
            texel = {1, 1, 1};
        }
        CHECK(lighting::scene_lighting(scene)->set_probe_volume(test::gpu_probes(volume), error));
        const auto bright = draw(true, false);
        scene.set_gameplay_rim_lift(false, .5f, 4);
        CHECK(draw(true, false) == bright);
    }
    scene.set_gameplay_rim_lift(true, .5f, 4);
    // No rim when probes are disabled, or when the GI is black, even with shadow lift.
    scene.set_probe_lighting(false, 1);
    CHECK(draw(false, false) == draw(true, true));
    scene.set_probe_lighting(true, 1);
    for (auto& texel : volume.irradiance) {
        texel = {};
    }
    CHECK(lighting::scene_lighting(scene)->set_probe_volume(test::gpu_probes(volume), error));
    CHECK(draw(false, false) == draw(true, true));
    renderer.clear_renderers();
    scene.release();
    frame.release();
}
}  // namespace

int main() {
    openrac::test::GlContext context;
    std::string why;
    if (!context.open(why) || !openrac::lighting::EarlyLighting::supported()) {
        return openrac::test::kSkip;
    }
    check_rim(false, 1);
    check_rim(true, 1);
    check_rim(true, 0);
    // A fully opaque chrome overlay must retain the rim on Clank and other metallic equipment.
    check_rim(true, 0, true);
    return openrac::test::result();
}
