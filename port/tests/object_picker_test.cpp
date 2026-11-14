// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cmath>

#include "common/json.h"
#include "scene/object_picker.h"
#include "scene/material_overrides.h"
#include "tests/check.h"
using namespace openrac;

static void check_chrome_replacement(
    scene::LevelData level, const renderer::Camera& camera, scene::Instance moby
) {
    // The chrome packet repeats the colour face with different indices and adds a face to its right.
    const auto body = level.vertices;
    level.vertices.insert(level.vertices.end(), body.begin(), body.end());
    for (auto vertex : body) {
        vertex.position[0] += 2;
        level.vertices.push_back(vertex);
    }
    level.indices = {0, 1, 2, 5, 4, 3, 6, 7, 8};
    scene::Material chrome;
    chrome.name = "metal_chrome";
    chrome.metal = true;
    chrome.image = 0;
    level.materials.push_back(chrome);
    auto& primitives = level.models[0].primitives;
    primitives.push_back({3, 6, 1});
    scene::prepare_chrome_geometry(level);
    CHECK(primitives.size() == 4);
    if (primitives.size() != 4) {
        return;
    }
    const auto& replacement = primitives.back();
    CHECK(replacement.enhanced_only && replacement.index_count == 3);
    CHECK(level.indices[replacement.first_index] == 6);
    CHECK(!level.materials[replacement.material].metal && level.materials[replacement.material].image == -1);
    scene::ObjectPicker picker;
    constexpr auto mobys = 1u << unsigned(scene::Layer::Mobys);
    moby.matrix = renderer::translation({0, 0, -5});
    const std::array live{moby};
    const auto base = picker.cast(level, live, {}, camera, mobys, {}, true);
    CHECK(base && base->material == 0);
    const std::array ndc{2.f / (5.f * std::tan(.5f)), 0.f};
    const auto classic = picker.cast(level, live, {}, camera, mobys, ndc);
    CHECK(classic && classic->material == 1);
    const auto enhanced = picker.cast(level, live, {}, camera, mobys, ndc, true);
    CHECK(enhanced && enhanced->material == replacement.material);
    const auto assets = scene::identify_materials(level, {});
    CHECK(assets);
    materials::Catalog catalog;
    materials::Rule rule;
    rule.id = "restore";
    rule.match.model = level.models[0].name;
    rule.match.material_slot = replacement.material;
    rule.properties["type"] = std::string("original");
    catalog.rules.emplace(rule.id, rule);
    const auto bindings = scene::resolve_materials(*assets, catalog);
    CHECK(bindings);
    if (!bindings) return;
    const auto restored = picker.cast(level, live, {}, camera, mobys, ndc, true, bindings->get());
    CHECK(restored && restored->material == replacement.material);
    CHECK(restored && level.models[0].primitives[restored->primitive].material == 1);
    const auto kept = picker.cast(level, live, {}, camera, mobys, {}, true, bindings->get());
    CHECK(kept && level.models[0].primitives[kept->primitive].material == 0);
    CHECK(!scene::material_pass_visible(replacement, level.materials[replacement.material], true, true));
    CHECK(scene::material_pass_visible(replacement, level.materials[replacement.material], true, false));
    // The snapshot used for live picking must not change the classic selection.
    const auto still_classic = picker.cast(level, live, {}, camera, mobys, ndc, false, bindings->get());
    CHECK(still_classic && still_classic->material == 1);
}

int main() {
    scene::LevelData level;
    for (unsigned i = 0; i < 3; ++i) {
        scene::Vertex v{};
        v.position[0] = i == 0 ? -1.f : (i == 1 ? 1.f : 0.f);
        v.position[1] = i == 2 ? 1.f : -1.f;
        level.vertices.push_back(v);
    }
    level.indices = {0, 1, 2};
    level.materials.resize(1);
    level.materials[0].image = 0;
    level.image_paths = {"texture\"with\nquotes.png"};
    level.models.resize(1);
    level.models[0].name = "test triangle";
    level.models[0].primitives.push_back({0, 3, 0});
    level.moby_classes[42] = {0, false};
    scene::Instance terrain;
    terrain.matrix = renderer::translation({0, 0, -10});
    level.instances[unsigned(scene::Layer::Terrain)] = {terrain};
    scene::Instance tie;
    tie.matrix = renderer::translation({0, 0, -5});
    level.instances[unsigned(scene::Layer::Ties)] = {tie};
    renderer::Camera camera;
    camera.view = renderer::identity();
    camera.projection = renderer::perspective_reversed(1, 1, .05f, 2000);
    check_chrome_replacement(level, camera, tie);
    scene::ObjectPicker picker;
    auto hit = picker.cast(level, {}, {}, camera);
    CHECK(hit && hit->layer == scene::Layer::Ties && std::abs(hit->distance - 5) < 1e-5);
    auto miss = picker.cast(level, {}, {}, camera, 0x1e, {.9f, .9f});
    CHECK(!miss);
    auto ground = picker.cast(level, {}, {}, camera, 1u << unsigned(scene::Layer::Terrain));
    CHECK(ground && ground->distance == 10);
    level.instances[unsigned(scene::Layer::Ties)][0].matrix[12] = 2;
    auto off_centre = picker.cast(level, {}, {}, camera, 0x1e, {2.f / (5.f * std::tan(.5f)), 0});
    CHECK(off_centre && off_centre->layer == scene::Layer::Ties);
    CHECK(off_centre && std::abs(off_centre->position[0] - 2) < 1e-5);
    level.instances[unsigned(scene::Layer::Ties)][0].matrix[12] = 0;
    // Scale must not corrupt world-space hit distances; singular transforms cannot be hit.
    level.instances[unsigned(scene::Layer::Ties)][0].matrix[0] = 2;
    hit = picker.cast(level, {}, {}, camera);
    CHECK(hit && std::abs(hit->distance - 5) < 1e-5);
    level.instances[unsigned(scene::Layer::Ties)][0].matrix[10] = 0;
    hit = picker.cast(level, {}, {}, camera);
    CHECK(hit && hit->layer == scene::Layer::Terrain);
    // Animated geometry must be picked at its posed position, not its bind pose.
    std::vector<scene::Instance> live(1);
    live[0].matrix = renderer::translation({0, 0, -3});
    live[0].palette = 0;
    live[0].temporal_id = 0x123456;
    for (auto& v : level.vertices) {
        v.weights[0] = 1;
    }
    std::vector<renderer::Mat4> palette{renderer::translation({2, 0, 0})};
    hit = picker.cast(level, live, palette, camera);
    CHECK(hit && hit->layer == scene::Layer::Terrain);
    palette[0] = renderer::identity();
    hit = picker.cast(level, live, palette, camera);
    CHECK(
        hit && hit->layer == scene::Layer::Mobys && hit->class_id == 42 && hit->identity == 0x123456
    );
    if (hit) {
        live[0].matrix[12] = 1;
        CHECK(picker.refresh(level, live, palette, camera, *hit));
        CHECK(std::abs(hit->position[0] - 1) < 1e-5);
        auto saved = *hit;
        live[0].temporal_id = 7;
        CHECK(!picker.refresh(level, live, palette, camera, *hit));
        scene::PickResult result{true, 1, 77, saved};
        auto report = scene::picker_report_json(result, true, "rac1\nntsc", "class\"42");
        json::Value parsed;
        std::string error;
        CHECK(json::parse(report, parsed, error));
        CHECK(parsed["hit"]["texture"].string() == level.image_paths[0]);
        CHECK(parsed["hit"]["identity"].number() == 0x123456);
        CHECK(parsed["level"].number() == 1);
        CHECK(json::parse(scene::picker_report_json(result, false, "rac1"), parsed, error));
        CHECK(parsed["hit"].is_null());
    }
    // Multiple BVH leaves preserve source triangle IDs and camera translation.
    auto dense = level;
    for (unsigned n = 1; n <= 20; ++n) {
        for (unsigned i = 0; i < 3; ++i) {
            auto vertex = level.vertices[i];
            vertex.position[0] += 4.f * float(n);
            dense.indices.push_back(unsigned(dense.vertices.size()));
            dense.vertices.push_back(vertex);
        }
    }
    dense.models[0].primitives[0].index_count = unsigned(dense.indices.size());
    picker.clear();
    camera.position = {80, 0, 0};
    camera.view = renderer::look_along(camera.position, {0, 0, -1}, {0, 1, 0});
    hit = picker.cast(dense, {}, {}, camera);
    CHECK(hit && hit->triangle == 20 && hit->layer == scene::Layer::Terrain);
    camera.position = {};
    camera.view = renderer::identity();
    // Geometry replacement invalidates the cached acceleration structure explicitly.
    picker.clear();
    level.instances[unsigned(scene::Layer::Terrain)].clear();
    level.instances[unsigned(scene::Layer::Ties)].clear();
    CHECK(!picker.cast(level, {}, {}, camera));
    return test::result();
}
