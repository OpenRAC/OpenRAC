// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Unified material preview, source isolation and atomic authoring using synthetic objects only.
#include <cstdio>
#include <exception>
#include <fstream>

#include "common/staged_file.h"
#include "enhanced/renderer/material_runtime.h"
#include "tests/check.h"

namespace {
using namespace openrac;

// Two synthetic object types share the same original surface; a preset supplies their common roughness.
scene::LevelData level(const std::filesystem::path& directory) {
    scene::LevelData result;
    result.dir = directory;
    auto assets = std::make_shared<scene::MaterialAssets>();
    assets->originals.emplace_back();
    assets->originals[0].image = 0;
    materials::Identity first;
    first.layer = "moby";
    first.class_id = 7;
    first.material_name = "body";
    first.model = "moby_7.glb";
    first.material_slot = 0;
    assets->sources.push_back({0, 0, "moby_7.glb", first, true});
    first.class_id = 8;
    first.model = "moby_8.glb";
    assets->sources.push_back({1, 0, "moby_8.glb", first, true});
    const auto catalog = materials::parse_catalog(
        R"({"version":1,"presets":{"shared":{"surface":{"roughness":0.5}}},"overrides":[{"id":"base","match":{"layer":"moby"},"preset":"shared"}]})"
    );
    CHECK(catalog);
    result.material_assets = assets;
    result.material_bindings = scene::resolve_materials(assets, *catalog).value();
    return result;
}

// Sparse object edits do not affect a second type or overwrite unrelated preset fields.
void test_transactional_preview(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    CHECK(runtime.editor() && runtime.editor()->error.empty());
    CHECK(runtime.patch(0, "surface.roughness", .75));
    CHECK(runtime.response(0, 0)->reflection.roughness == .75f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .5f);
    const auto previous = runtime.editor();
    CHECK(!runtime.patch(0, "surface.roughness", 0.0));
    CHECK(runtime.editor() == previous);
    CHECK(runtime.patch_preset("shared", "surface.roughness", .9));
    CHECK(runtime.response(0, 0)->reflection.roughness == .75f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .9f);
    CHECK(!runtime.editor()->transport_dirty);
    CHECK(runtime.undo(0, "surface.roughness"));
    CHECK(runtime.response(0, 0)->reflection.roughness == .9f);
    runtime.reset();
    CHECK(runtime.response(0, 0)->reflection.roughness == .5f && !runtime.editor()->dirty);
}

// Independent cached optical records reset inherited values instead of leaking the previous object's edits.
void test_optical_override_isolation(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    CHECK(runtime.patch(0, "wetness.diffuse_scale", .1));
    CHECK(runtime.patch(0, "fur.strength", .7));
    CHECK(runtime.response(0, 0)->wetness.diffuse_scale == .1f);
    CHECK(
        runtime.response(1, 0)->wetness.diffuse_scale
        == enhanced::WetnessMaterialSettings{}.diffuse_scale
    );
    CHECK(runtime.response(0, 0)->fur.strength == .7f);
    CHECK(runtime.response(1, 0)->fur.strength == enhanced::FurMaterialSettings{}.strength);
}

// Saved JSON roundtrips exactly and never contains the loaded shared catalog or unrelated original defaults.
void test_sparse_atomic_save(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    CHECK(runtime.patch(0, "surface.diffuse_scale", .25));
    CHECK(runtime.editor()->transport_dirty);
    CHECK(runtime.save());
    CHECK(!runtime.editor()->dirty && runtime.editor()->transport_dirty);
    const auto saved = materials::read_catalog(directory / "material-overrides.json");
    CHECK(saved && saved->rules.size() == 1 && saved->presets.empty());
    CHECK(saved->rules.begin()->second.properties.size() == 1);
    auto catalog = level(directory).material_bindings->catalog;
    materials::merge_catalog(catalog, *saved, 1);
    const auto compiled = scene::resolve_materials(level(directory).material_assets, catalog);
    CHECK(compiled && (*compiled)->find(0, 0)->surface.diffuse_scale == .25f);
    CHECK((*compiled)->find(1, 0)->surface.diffuse_scale == 1);
    CHECK(runtime.patch(0, "surface.diffuse_scale", .5));
    runtime.reset();
    CHECK(runtime.find(0, 0)->surface.diffuse_scale == .25f);
}

// Preview and Save preserve same-file specificity and reject conflicting rules before editing.
void test_preview_and_save_share_author_priority(const std::filesystem::path& directory) {
    const auto path = directory / "material-overrides.json";
    const auto author = materials::parse_catalog(
        R"({"version":1,"overrides":[{"id":"authored","match":{"layer":"moby","model":"moby_7.glb","material_slot":0,"pass":"colour"},"set":{"surface":{"roughness":0.5}}}]})"
    );
    CHECK(author && materials::write_catalog(path, *author));
    auto source = level(directory);
    auto catalog = source.material_bindings->catalog;
    materials::merge_catalog(catalog, *author, 1);
    source.material_bindings = scene::resolve_materials(source.material_assets, catalog).value();
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(source));
    CHECK(!runtime.patch(0, "surface.roughness", .7));
    CHECK(runtime.save());
    CHECK(
        materials::catalog_text(materials::read_catalog(path).value()).value()
        == materials::catalog_text(*author).value()
    );
    CHECK(!runtime.editor()->dirty && runtime.response(0, 0)->reflection.roughness == .5f);
    CHECK(runtime.patch_preset("shared", "surface.roughness", .9));
    CHECK(runtime.response(0, 0)->reflection.roughness == .5f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .9f);
    CHECK(runtime.save());
    CHECK(runtime.response(0, 0)->reflection.roughness == .5f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .9f);
    std::error_code error;
    std::filesystem::remove(path, error);
    CHECK(!error);
}

// External edits are never silently overwritten by a stale authoring window.
void test_external_save_conflict(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    CHECK(runtime.patch(0, "surface.roughness", .7));
    materials::Catalog outside;
    outside.presets["external"]["surface.roughness"] = .9;
    CHECK(materials::write_catalog(directory / "material-overrides.json", outside));
    CHECK(!runtime.save());
    CHECK(
        materials::read_catalog(directory / "material-overrides.json")->presets.contains("external")
    );
}

// Same-name and unnamed slots survive preview and JSON round-trip as separate identities.
void test_exact_slots(const std::filesystem::path& directory) {
    scene::LevelData source;
    source.dir = directory;
    source.materials.resize(4);
    source.materials[0].name = source.materials[1].name = "body";
    scene::Model model;
    model.name = "moby_7.glb";
    for (int slot = 0; slot < 4; ++slot) {
        source.materials[static_cast<std::size_t>(slot)].source_slot = slot;
        scene::Primitive primitive;
        primitive.material = slot;
        model.primitives.push_back(primitive);
    }
    source.models.push_back(model);
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(source));
    CHECK(runtime.patch(0, "surface.diffuse_scale", .5));
    CHECK(runtime.patch(2, "surface.diffuse_scale", .25));
    CHECK(runtime.save());
    scene::prepare_material_overrides(source);
    CHECK(runtime.load(source));
    CHECK(runtime.find(0, 0)->surface.diffuse_scale == .5f);
    CHECK(runtime.find(0, 1)->surface.diffuse_scale == 1.f);
    CHECK(runtime.find(0, 2)->surface.diffuse_scale == .25f);
    CHECK(runtime.find(0, 3)->surface.diffuse_scale == 1.f);
    CHECK(std::filesystem::remove(directory / "material-overrides.json"));
}

// A family selected without a preset still uses its established optical defaults.
void test_family_defaults() {
    scene::MaterialSource source;
    source.identity.metal = true;
    const auto response = enhanced::material_response(source, {{"shading", std::string("glass")}});
    CHECK(response.reflection.roughness == .08f && response.reflection.reflectivity == .04f);
}

// Model filename extensions cannot suppress type selectors, and sky is never classified as terrain.
void test_export_identity() {
    scene::LevelData source;
    source.materials.emplace_back();
    scene::Model object;
    object.name = "moby_7.glb";
    scene::Primitive primitive;
    primitive.material = 0;
    object.primitives.push_back(primitive);
    source.models.push_back(object);
    object.name = "sky.glb";
    source.models.push_back(object);
    source.moby_classes[99] = {0, true};
    const auto assets = scene::identify_materials(source, {});
    CHECK(assets && (*assets)->sources[0].identity.class_id == 7);
    CHECK((*assets)->sources[1].identity.layer == "sky");
}
}  // namespace

int main() {
    try {
        auto staged =
            StagedFile::create(std::filesystem::temp_directory_path() / "openrac-material-runtime");
        CHECK(staged);
        if (!staged) {
            return 1;
        }
        const auto directory = staged->path().parent_path();
        test_exact_slots(directory);
        test_preview_and_save_share_author_priority(directory);
        test_transactional_preview(directory);
        test_optical_override_isolation(directory);
        test_sparse_atomic_save(directory);
        test_export_identity();
        test_family_defaults();
        test_external_save_conflict(directory);
        std::error_code error;
        std::filesystem::remove(directory / "material-overrides.json", error);
        CHECK(!error);
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
