// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic production-pass coverage for glass cutouts and reversible retail chrome materials.
#include "enhanced/tests/material_surface_gpu.h"

#include <algorithm>
#include <array>
#include <memory>

#include "enhanced/renderer/gtao.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "scene/level_renderer.h"
#include "scene/object_preview.h"
#include "tests/check.h"

namespace openrac::test {
using lighting::Float3;
using namespace gl;

// Both the ordinary transparent pass and the moby metal pass retain authored cutout coverage.
void test_glass_cutout_coverage() {
    for (const auto layer : {scene::Layer::Ties, scene::Layer::Mobys}) {
        scene::LevelData level;
        level.light_sets.resize(256);
        const std::array<Float3, 4> quad_positions{
            {{-1, -1, .5f}, {1, -1, .5f}, {1, 1, .5f}, {-1, 1, .5f}}
        };
        for (const auto& p : quad_positions) {
            scene::Vertex corner{};
            std::copy(p.begin(), p.end(), corner.position);
            corner.normal[2] = 1;
            std::fill_n(corner.colour, 4, 1.f);
            corner.uv[0] = (p[0] + 1) * .5f;
            corner.uv[1] = (p[1] + 1) * .5f;
            corner.light_slot = -1;
            level.vertices.push_back(corner);
        }
        level.indices = {0, 1, 2, 0, 2, 3};
        level.images.emplace_back(3, 1);
        level.image_paths.push_back("/textures/moby_glass_cutout.png");
        level.images.back().pixels = {255, 255, 255, 0, 255, 255, 255, 128, 255, 255, 255, 255};
        level.materials.emplace_back();
        level.materials[0].image = 0;
        level.materials[0].cutout = true;
        level.materials[0].clamp_s = level.materials[0].clamp_t = true;
        scene::Model model;
        model.name = layer == scene::Layer::Ties ? "tie_7.glb" : "moby_7.glb";
        model.min = {-1, -1, .5f};
        model.max = {1, 1, .5f};
        model.primitives.push_back({0, 6, 0});
        level.models.push_back(model);
        level.instances[static_cast<std::size_t>(layer)].emplace_back();
        const auto catalog = materials::parse_catalog(R"({"version":1,"presets":{
          "glass":{"type":"glass-lit","glass":{"transmission":0,"tint":[1,1,1]},
                   "reflection":{"strength":0}}},"overrides":[
          {"id":"glass","match":{"class_id":7},"material":"glass"}]})");
        CHECK(catalog);
        if (!catalog) {
            return;
        }
        const auto assets = scene::identify_materials(level, *catalog);
        CHECK(assets);
        level.material_bindings = scene::resolve_materials(*assets, *catalog).value();
        renderer::Renderer renderer;
        renderer.set_ambient_occlusion(std::make_unique<renderer::Gtao>());
        scene::LevelScene scene;
        lighting::use_scene_lighting(scene);
        std::string error;
        CHECK(scene.upload(level, renderer.textures(), error));
        CHECK(renderer.add(std::make_unique<scene::LayerRenderer>(scene, layer), error));
        CHECK(renderer.init(error));
        renderer::FrameBuffer frame;
        CHECK(frame.create(32, 16, error));
        renderer::FrameInput input;
        input.camera.view = input.camera.projection = renderer::identity();
        input.clear_colour = {0, 0, 0, 1};
        renderer.render(input, {frame.id(), 32, 16});
        const auto pixels = frame.read_rgba();
        const auto green = [&](unsigned x) {
            return pixels[(8 * 32 + x) * 4 + 1];
        };
        std::printf(
            "Glass cutout layer %d: %u %u %u\n", int(layer), green(2), green(16), green(29)
        );
        CHECK(green(2) == 0);
        CHECK(green(16) > 40 && green(16) < 200);
        CHECK(green(29) > 230);
        CHECK(glGetError() == GL_NO_ERROR);
    }
}

// Original restores the retail texture only on the selected part of a shared chrome packet.
void test_original_chrome_coverage() {
    scene::LevelData level;
    level.light_sets.resize(256);
    level.models.emplace_back();
    auto& model = level.models.back();
    model.name = "moby_7.glb";
    model.min = {-1, -1, .5f};
    model.max = {1, 1, .5f};
    for (int half = 0; half < 2; ++half) {
        const float low = float(half) - 1, high = float(half);
        const std::array<Float3, 4> corners{
            {{low, -1, .5f}, {high, -1, .5f}, {high, 1, .5f}, {low, 1, .5f}}
        };
        for (const auto& p : corners) {
            scene::Vertex v{};
            std::copy(p.begin(), p.end(), v.position);
            v.normal[2] = 1;
            std::fill_n(v.colour, 4, 1.f);
            v.light_slot = -1;
            level.vertices.push_back(v);
        }
        const auto base = static_cast<std::uint32_t>(half * 4);
        for (const auto index : {0u, 1u, 2u, 0u, 2u, 3u}) {
            level.indices.push_back(base + index);
        }
    }
    level.materials.emplace_back();
    level.materials[0].colour = {0, 1, 0, 1};
    scene::Material chrome;
    chrome.name = "metal_chrome";
    chrome.metal = true;
    chrome.image = 0;
    level.materials.push_back(chrome);
    level.images.emplace_back(1, 1);
    level.images.back().pixels = {255, 0, 0, 255};
    level.image_paths.push_back("/textures/moby_chrome.png");
    model.primitives.push_back({0, 6, 0});
    model.primitives.push_back({0, 12, 1});
    level.instances[static_cast<std::size_t>(scene::Layer::Mobys)].emplace_back();
    scene::prepare_chrome_geometry(level);
    const auto& replacement = model.primitives.back();
    const int replacement_slot = replacement.material;
    CHECK(replacement.enhanced_only);
    const auto assets = scene::identify_materials(level, {});
    CHECK(assets);
    level.material_bindings = scene::resolve_materials(*assets, {}).value();
    renderer::Renderer renderer;
    renderer.set_ambient_occlusion(std::make_unique<renderer::Gtao>());
    scene::LevelScene scene;
    lighting::use_scene_lighting(scene);
    std::string error;
    CHECK(scene.upload(level, renderer.textures(), error));
    CHECK(renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Mobys), error));
    CHECK(renderer.init(error));
    renderer::FrameBuffer frame;
    CHECK(frame.create(32, 16, error));
    renderer::FrameInput input;
    input.camera.view = input.camera.projection = renderer::identity();
    input.clear_colour = {0, 0, 0, 1};
    auto draw = [&] {
        renderer.render(input, {frame.id(), 32, 16});
        return frame.read_rgba();
    };
    const auto lit = draw();
    const auto left = (8 * 32 + 8) * 4, right = (8 * 32 + 24) * 4;
    CHECK(lit[left] == 0 && lit[left + 1] > 100);
    CHECK(lit[right] > 100 && lit[right + 1] > 100);
    auto& runtime = lighting::scene_lighting(scene)->materials();
    const auto index = runtime.editor()->bindings->lookup.at({0, replacement_slot});
    const std::array selection{index};
    CHECK(runtime.assign(selection, "original-chrome", {{"type", std::string("original")}}));
    const auto original = draw();
    CHECK(original[right] > 100 && original[right + 1] == 0 && original[right + 2] == 0);
    CHECK(original[left] == lit[left] && original[left + 1] == lit[left + 1]);
    scene::PickHit hit;
    hit.layer = scene::Layer::Mobys;
    scene::Instance instance;
    const auto preview =
        scene::preview_mesh(level, hit, instance, {}, runtime.editor()->bindings.get());
    CHECK(preview && preview->parts.size() == 2);
    CHECK(preview && std::ranges::any_of(preview->parts, [](const auto& part) {
              return part.material.metal && part.material.image >= 0;
          }));
    lighting::scene_lighting(scene)->set_material_overrides(false);
    const auto classic = draw();
    CHECK(classic[right] == original[right] && classic[right + 1] == original[right + 1]);
    lighting::scene_lighting(scene)->set_material_overrides(true);
    runtime.reset();
    CHECK(draw() == lit);
    CHECK(glGetError() == GL_NO_ERROR);
}

}  // namespace openrac::test
