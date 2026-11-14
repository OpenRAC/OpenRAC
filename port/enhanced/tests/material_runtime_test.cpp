// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Unified material preview, source isolation and atomic authoring using synthetic objects only.
#include <cstdio>
#include <exception>
#include <fstream>
#include <limits>

#include "common/staged_file.h"
#include "enhanced/renderer/material_runtime.h"
#include "enhanced/renderer/water.h"
#include "game/common/debug/material_presets.h"
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
    const auto resolved = scene::resolve_materials(assets, *catalog);
    if (!resolved) {
        throw std::runtime_error(resolved.error().message);
    }
    result.material_bindings = *resolved;
    return result;
}

// Switching finishes replaces the previous metal response without affecting another model.
void test_library_finishes(const std::filesystem::path& directory) {
    using namespace frontend::debug_overlay;
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    const MaterialPreset* metal = nullptr;
    for (const auto& preset : kMaterialPresets) {
        CHECK(materials::validate(preset_properties(preset)));
        if (preset.surface == PresetSurface::Metal) {
            metal = &preset;
        }
    }
    CHECK(metal);
    if (!metal) {
        return;
    }
    for (const auto& preset : kMaterialPresets) {
        if (preset.surface != PresetSurface::Opaque && preset.surface != PresetSurface::Foliage) {
            continue;
        }
        CHECK(runtime.patch(0, preset_properties(*metal)));
        CHECK(runtime.response(0, 0)->metallic == 1.f);
        CHECK(runtime.patch(0, preset_properties(preset)));
        const auto* response = runtime.response(0, 0);
        CHECK(response->metallic == 0.f);
        CHECK(response->dielectric_specular);
        CHECK(response->reflection.roughness == float(preset.roughness));
        CHECK(response->reflection.tint == (std::array<float, 3>{1, 1, 1}));
        CHECK(runtime.response(1, 0)->metallic == 0.f);
    }
}

// Assignments share edits, detach explicitly, and replace saved slot overrides after a reload.
void test_shared_materials(const std::filesystem::path& directory) {
    const auto path = directory / "shared-material-test";
    std::filesystem::create_directories(path);
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(path)));
    const std::array<std::size_t, 2> both{0, 1};
    const std::array<std::size_t, 1> first{0};
    CHECK(runtime.patch(0, "surface.roughness", .9));
    CHECK(runtime.save());
    CHECK(runtime.assign(
        both,
        "paint",
        {{"shading", std::string("standard")}, {"surface.roughness", .3}, {"surface.metallic", 0.}}
    ));
    CHECK(runtime.find(0, 0)->values.material == "paint");
    CHECK(runtime.find(1, 0)->values.material == "paint");
    CHECK(runtime.find(0, 0)->record == runtime.find(1, 0)->record);
    CHECK(runtime.response(0, 0)->reflection.roughness == .3f);
    CHECK(runtime.patch_preset("paint", "surface.roughness", .6));
    CHECK(runtime.response(0, 0)->reflection.roughness == .6f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .6f);
    const auto previous = runtime.editor();
    const std::array<std::size_t, 2> invalid{0, 99};
    CHECK(!runtime.assign(invalid, "paint"));
    CHECK(!runtime.assign(first, "missing"));
    CHECK(!runtime.assign(first, "invalid name", {{"surface.roughness", .4}}));
    CHECK(runtime.editor() == previous);
    const auto unique = runtime.make_unique(first);
    CHECK(unique && *unique != "paint");
    CHECK(runtime.find(0, 0)->values.material == *unique);
    CHECK(runtime.patch_preset(*unique, "surface.roughness", .2));
    CHECK(runtime.response(0, 0)->reflection.roughness == .2f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .6f);
    CHECK(runtime.save());
    // Synthetic level inputs retain their supplied assets; resolve the persisted author layer explicitly.
    const auto author = materials::read_catalog(path / "material-overrides.json");
    CHECK(author);
    enhanced::MaterialRuntime next;
    auto baseline = level(path);
    auto catalog = baseline.material_bindings->source_catalog;
    materials::merge_catalog(catalog, *author, std::numeric_limits<int>::max());
    baseline.material_bindings =
        scene::resolve_materials(baseline.material_assets, catalog).value();
    CHECK(next.load(baseline));
    CHECK(next.response(0, 0)->reflection.roughness == .2f);
    CHECK(next.response(1, 0)->reflection.roughness == .6f);
    CHECK(next.assign(first, "paint"));
    CHECK(next.response(0, 0)->reflection.roughness == .6f);
    CHECK(next.find(0, 0)->record == next.find(1, 0)->record);
    next.reset();
    CHECK(next.find(0, 0)->values.material == *unique);
    std::filesystem::remove(path / "material-overrides.json");
    std::filesystem::remove(path);
}

// Saved optical edits and shared assignments can be removed without changing another slot or its definition.
void test_clear_saved_slots(const std::filesystem::path& directory) {
    const auto path = directory / "clear-saved-slot-test";
    std::filesystem::create_directories(path);
    const auto fixture = [&] {
        scene::LevelData source;
        source.dir = path;
        source.materials.resize(2);
        scene::Model model;
        model.name = "tie_530.glb";
        for (int slot = 0; slot < 2; ++slot) {
            source.materials[slot].name = slot == 0 ? "tie_0062" : "tie_0061";
            source.materials[slot].source_slot = slot;
            scene::Primitive primitive;
            primitive.material = slot;
            model.primitives.push_back(primitive);
        }
        source.models.push_back(model);
        scene::prepare_material_overrides(source);
        return source;
    };
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(fixture()));
    const std::array<std::size_t, 1> first{0}, second{1};
    const std::array<std::size_t, 2> both{0, 1}, invalid{0, 999};
    CHECK(runtime.patch(0, {{"surface.metallic", .133}, {"surface.roughness", .22}}));
    CHECK(runtime.patch(1, "surface.roughness", .77));
    CHECK(runtime.save());
    CHECK(runtime.patch(0, "surface.roughness", .6));
    CHECK(runtime.undo(0, "surface.roughness"));
    CHECK(runtime.response(0, 0)->reflection.roughness == .22f);  // Undo retains saved values.
    const auto previous = runtime.editor();
    CHECK(!runtime.clear_slots(invalid));
    CHECK(runtime.editor() == previous);
    CHECK(runtime.clear_slots(first));
    CHECK(runtime.response(0, 0)->metallic == 0.f);
    CHECK(runtime.response(0, 0)->dielectric_specular);
    CHECK(runtime.response(0, 1)->reflection.roughness == .77f);
    CHECK(materials::read_catalog(path / "material-overrides.json")->rules.size() == 2);
    runtime.reset();
    CHECK(runtime.response(0, 0)->metallic == .133f);
    CHECK(runtime.assign(both, "paint", {{"surface.roughness", .45}}));
    CHECK(runtime.save());
    CHECK(runtime.clear_slots(first));
    CHECK(!runtime.find(0, 0)->values.material.empty());
    CHECK(runtime.find(0, 1)->values.material == "paint");
    CHECK(runtime.patch_preset("paint", "surface.roughness", .65));
    CHECK(runtime.response(0, 1)->reflection.roughness == .65f);
    CHECK(runtime.response(0, 0)->dielectric_specular);
    CHECK(runtime.save());
    enhanced::MaterialRuntime reloaded;
    CHECK(reloaded.load(fixture()));
    CHECK(reloaded.response(0, 0)->dielectric_specular);
    CHECK(reloaded.find(0, 1)->values.material == "paint");
    CHECK(reloaded.clear_slots(second));
    CHECK(reloaded.save());
    const auto persisted = materials::read_catalog(path / "material-overrides.json");
    CHECK(persisted && persisted->rules.empty() && persisted->presets.contains("paint"));
    std::filesystem::remove(path / "material-overrides.json");
    std::filesystem::remove(path);
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
    CHECK(!runtime.patch(0, {{"surface.roughness", .3}, {"surface.reflectivity", 2.}}));
    CHECK(runtime.editor() == previous);
    CHECK(runtime.patch(0, {{"surface.roughness", .75}, {"surface.reflectivity", .045}}));
    CHECK(runtime.response(0, 0)->reflection.reflectivity == .045f);
    CHECK(runtime.response(1, 0)->reflection.reflectivity == .012f);
    CHECK(runtime.patch_preset("shared", "surface.roughness", .9));
    CHECK(runtime.response(0, 0)->reflection.roughness == .75f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .9f);
    CHECK(!runtime.editor()->transport_dirty);
    CHECK(runtime.undo(0, "surface.roughness"));
    CHECK(runtime.response(0, 0)->reflection.roughness == .9f);
    runtime.reset();
    CHECK(runtime.response(0, 0)->reflection.roughness == .5f && !runtime.editor()->dirty);
}

void test_ocean_preset(const std::filesystem::path& directory) {
    scene::LevelData source;
    source.dir = directory;
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(source));
    CHECK(runtime.editor()->bindings->assets->sources[0].identity.material_name == "@water");
    enhanced::reset_ocean_material();
    const auto profile = enhanced::ocean_material();
    CHECK(runtime.patch(0, {{"ocean.roughness", .16}, {"ocean.reflectivity", .020}}));
    auto live = enhanced::ocean_material();
    CHECK(live.roughness == .16f && live.reflectivity == .020f);
    live.roughness = .31f;
    live.reflectivity = .06f;
    live.colour[0] = .13f;
    enhanced::set_ocean_material(live);
    CHECK(enhanced::ocean_material().roughness == .31f);
    CHECK(enhanced::ocean_material().reflectivity == .06f);
    CHECK(runtime.patch(0, {{"ocean.roughness", .16}, {"ocean.reflectivity", .020}}));
    const auto reapplied = enhanced::ocean_material();
    CHECK(reapplied.roughness == .16f && reapplied.reflectivity == .020f);
    CHECK(reapplied.colour == live.colour);
    enhanced::set_ocean_material(live);
    CHECK(runtime.patch(0, "ocean.roughness", .21));
    live = enhanced::ocean_material();
    CHECK(live.roughness == .21f && live.reflectivity == .06f && live.colour[0] == .13f);
    enhanced::reset_ocean_material();
    live = enhanced::ocean_material();
    CHECK(live.roughness == .21f && live.reflectivity == .020f && live.colour == profile.colour);
    CHECK(
        enhanced::reflection_material_settings(enhanced::ReflectionSurface::Water).roughness == .22f
    );
    CHECK(!runtime.editor()->transport_dirty);
    runtime.reset();
    live = enhanced::ocean_material();
    CHECK(live.roughness == profile.roughness && live.reflectivity == profile.reflectivity);
}

// Independent cached optical records reset inherited values instead of leaking the previous object's edits.
void test_optical_override_isolation(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    CHECK(runtime.patch(0, "wetness.diffuse_scale", .1));
    CHECK(runtime.patch(0, "fur.strength", .7));
    CHECK(runtime.patch(0, "glass.transmission", .65));
    CHECK(runtime.patch(0, "glass.tint", (std::array<double, 3>{.2, .3, .4})));
    CHECK(runtime.response(0, 0)->wetness.diffuse_scale == .1f);
    CHECK(
        runtime.response(1, 0)->wetness.diffuse_scale
        == enhanced::WetnessMaterialSettings{}.diffuse_scale
    );
    CHECK(runtime.response(0, 0)->fur.strength == .7f);
    CHECK(runtime.response(1, 0)->fur.strength == enhanced::FurMaterialSettings{}.strength);
    CHECK(runtime.response(0, 0)->glass.transmission == .65f);
    CHECK((runtime.response(0, 0)->glass.tint == std::array{.2f, .3f, .4f}));
    CHECK(runtime.response(1, 0)->glass.transmission == enhanced::ThinGlassSettings{}.transmission);
    CHECK(!runtime.editor()->transport_dirty);
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
    auto catalog = level(directory).material_bindings->source_catalog;
    materials::merge_catalog(catalog, *saved, 1);
    const auto compiled = scene::resolve_materials(level(directory).material_assets, catalog);
    CHECK(compiled && (*compiled)->find(0, 0)->surface().diffuse_scale == .25f);
    CHECK((*compiled)->find(1, 0)->surface().diffuse_scale == 1);
    CHECK(runtime.patch(0, "surface.diffuse_scale", .5));
    runtime.reset();
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == .25f);

    const auto id = saved->rules.begin()->first;
    CHECK(!runtime.delete_override("base"));
    CHECK(runtime.delete_override(id));
    CHECK(runtime.editor()->dirty && !runtime.editor()->local_rules.contains(id));
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == 1.f);
    CHECK(materials::read_catalog(directory / "material-overrides.json")->rules.contains(id));
    runtime.reset();
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == .25f);
    CHECK(runtime.delete_override(id));
    CHECK(runtime.save());
    CHECK(materials::read_catalog(directory / "material-overrides.json")->rules.empty());
}

// Preview and Save preserve same-file specificity and reject conflicting rules before editing.
void test_preview_and_save_share_author_priority(const std::filesystem::path& directory) {
    const auto path = directory / "material-overrides.json";
    const auto author = materials::parse_catalog(
        R"({"version":1,"overrides":[{"id":"authored","match":{"layer":"moby","model":"moby_7.glb","material_slot":0,"pass":"colour"},"set":{"surface":{"roughness":0.5}}}]})"
    );
    CHECK(author && materials::write_catalog(path, *author));
    auto source = level(directory);
    auto catalog = source.material_bindings->source_catalog;
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
    const std::array<std::size_t, 2> slots{0, 2};
    CHECK(runtime.patch(slots, {{"surface.diffuse_scale", .75}}));
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == .75f);
    CHECK(runtime.find(0, 2)->surface().diffuse_scale == .75f);
    const std::array<std::size_t, 2> invalid_slots{0, 999};
    const auto previous = runtime.editor();
    CHECK(!runtime.patch(invalid_slots, {{"surface.diffuse_scale", .5}}));
    CHECK(runtime.editor() == previous);
    CHECK(runtime.undo(slots, "surface.diffuse_scale"));
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == 1.f);
    CHECK(runtime.find(0, 2)->surface().diffuse_scale == 1.f);
    CHECK(runtime.patch(0, "surface.diffuse_scale", .5));
    CHECK(runtime.patch(2, "surface.diffuse_scale", .25));
    CHECK(runtime.save());
    scene::prepare_material_overrides(source);
    CHECK(runtime.load(source));
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == .5f);
    CHECK(runtime.find(0, 1)->surface().diffuse_scale == 1.f);
    CHECK(runtime.find(0, 2)->surface().diffuse_scale == .25f);
    CHECK(runtime.find(0, 3)->surface().diffuse_scale == 1.f);
    CHECK(std::filesystem::remove(directory / "material-overrides.json"));
}

// A family selected without a preset still uses its established optical defaults.
void test_family_defaults() {
    scene::MaterialSource source;
    source.identity.metal = true;
    const auto response = enhanced::material_response(source, {{"shading", std::string("glass")}});
    CHECK(response.reflection.roughness == .08f && response.reflection.reflectivity == .04f);
    CHECK(!response.dielectric_specular);

    source.identity.material_name = "metal_chrome";
    source.identity.class_id = 87;
    const auto chrome = enhanced::material_response(source, {});
    CHECK(!chrome.dielectric_specular);
    CHECK(chrome.reflection.roughness == .26f && chrome.reflection.reflectivity == .65f);

    source = {};
    CHECK(!enhanced::material_response(source, {}).dielectric_specular);
    const auto dielectric = enhanced::material_response(source, {{"surface.roughness", .32}});
    CHECK(dielectric.dielectric_specular);
    CHECK(dielectric.reflection.roughness == .32f && dielectric.reflection.reflectivity == .04f);
    const auto metallic = enhanced::material_response(source, {{"surface.metallic", .5}});
    CHECK(metallic.dielectric_specular && metallic.metallic == .5f);
    CHECK(chrome.metallic < 0.f);
    const auto leaf = enhanced::material_response(
        source,
        {{"shading", std::string("foliage")},
         {"reflection.strength", 0.},
         {"reflection.tint", std::array<double, 3>{.5, .25, 1.}}}
    );
    CHECK(leaf.dielectric_specular && leaf.reflection.strength == 0.f);
    CHECK(leaf.reflection.tint == (std::array<float, 3>{.5f, .25f, 1.f}));
    CHECK(!materials::validate({{"surface.metallic", 1.01}}));
}

// Untouched surfaces share editable materials; type switches, detach and save/reload use that same path.
void test_all_material_types(const std::filesystem::path& directory) {
    const auto path = directory / "material-types-test";
    std::filesystem::create_directories(path);
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(path)));
    const auto name = runtime.find(0, 0)->values.material;
    CHECK(!name.empty() && runtime.find(1, 0)->values.material == name);
    CHECK(runtime.find(0, 0)->record == runtime.find(1, 0)->record);
    CHECK(runtime.patch_preset(name, "surface.roughness", .23));
    CHECK(runtime.response(0, 0)->reflection.roughness == .23f);
    CHECK(runtime.response(1, 0)->reflection.roughness == .23f);
    for (const auto& type : materials::kMaterialTypes) {
        CHECK(runtime.patch_preset(name, "type", std::string(type.id)));
        CHECK(materials::text_value(runtime.find(0, 0)->values.properties, "type") == type.id);
        CHECK(runtime.find(0, 0)->values.material == name);
        CHECK(runtime.response(0, 0)->original == (type.id == "original"));
        CHECK(runtime.response(0, 0)->glass_surface == (type.id == "glass-lit"));
        CHECK(
            runtime.response(0, 0)->fur.enabled
            == (type.id == "lit-fur" || type.id == "lit-wet-fur")
        );
        CHECK(runtime.response(0, 0)->wet_fur == (type.id == "lit-wet-fur"));
        CHECK((runtime.response(0, 0)->foliage[1] != 0) == (type.id == "foliage"));
    }
    CHECK(runtime.patch_preset(name, "surface.diffuse_scale", .1));
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == 1.f);
    CHECK(!runtime.editor()->transport_dirty);  // Original ignores retained enhanced properties.
    CHECK(runtime.save());
    auto next_level = level(path);
    auto catalog = next_level.material_bindings->source_catalog;
    const auto saved = materials::read_catalog(path / "material-overrides.json");
    CHECK(saved && saved->presets.contains(name));
    materials::merge_catalog(catalog, *saved, 1);
    next_level.material_bindings =
        scene::resolve_materials(next_level.material_assets, catalog).value();
    enhanced::MaterialRuntime next;
    CHECK(next.load(next_level));
    CHECK(next.response(0, 0)->original && next.response(1, 0)->original);
    CHECK(next.find(0, 0)->values.material == name);
    const std::array<std::size_t, 1> first{0};
    const auto copy = next.make_unique(first);
    CHECK(copy && *copy != name);
    CHECK(next.patch_preset(*copy, "type", std::string("lit")));
    CHECK(!next.response(0, 0)->original && next.response(1, 0)->original);
    CHECK(next.assign(first, name));
    CHECK(next.response(0, 0)->original);
    std::filesystem::remove(path / "material-overrides.json");
    std::filesystem::remove(path);
}

void test_revert_created_material(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    const std::array<std::size_t, 1> first{0};
    CHECK(runtime.assign(
        first, "revert-created", {{"type", std::string("lit")}, {"surface.roughness", .27}}
    ));
    for (double roughness : {.88, .66}) {
        CHECK(runtime.patch_preset("revert-created", "surface.roughness", roughness));
        CHECK(runtime.undo_preset("revert-created", "surface.roughness"));
        CHECK(runtime.response(0, 0)->reflection.roughness == .27f);
    }
    CHECK(runtime.patch_preset("revert-created", "surface.diffuse_scale", .4));
    CHECK(runtime.undo_preset("revert-created", "surface.diffuse_scale"));
    CHECK(runtime.find(0, 0)->surface().diffuse_scale == 1.f);
    const auto copy = runtime.make_unique(first);
    CHECK(copy);
    CHECK(runtime.patch_preset(*copy, "surface.roughness", .75));
    CHECK(runtime.undo_preset(*copy, "surface.roughness"));
    CHECK(runtime.response(0, 0)->reflection.roughness == .27f);
    runtime.reset();
    CHECK(runtime.assign(first, "revert-created", {{"surface.roughness", .43}}));
    CHECK(runtime.patch_preset("revert-created", "surface.roughness", .75));
    CHECK(runtime.undo_preset("revert-created", "surface.roughness"));
    CHECK(runtime.response(0, 0)->reflection.roughness == .43f);
}

void test_material_control_defaults(const std::filesystem::path& directory) {
    using frontend::debug_overlay::material_control_properties;
    const auto definition = materials::material_definition({});
    scene::Material inherited;
    inherited.diffuse_scale = .25f;
    inherited.emission = {2, 1, 0};
    inherited.bake_intensity = 4;
    inherited.emission_contrast = 1.5f;
    const auto controls = material_control_properties(definition, inherited);
    CHECK(materials::validate(controls));
    CHECK(materials::colour_value(controls, "surface.tint", {}) == (std::array<float, 3>{1, 1, 1}));
    CHECK(materials::number_value(controls, "surface.diffuse_scale", 0) == .25f);
    CHECK(materials::number_value(controls, "emission.runtime_intensity", 0) == 2.f);
    CHECK(materials::number_value(controls, "emission.bake_intensity", 0) == 4.f);
    CHECK(
        !definition.contains("surface.tint") && !definition.contains("emission.runtime_intensity")
    );
    const auto explicit_controls =
        material_control_properties({{"surface.diffuse_scale", .6}}, inherited);
    CHECK(materials::number_value(explicit_controls, "surface.diffuse_scale", 0) == .6f);
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    const auto shared = runtime.find(0, 0)->values.material;
    CHECK(runtime.patch_preset(shared, "surface.tint", std::array<double, 3>{.5, 1, 1}));
    CHECK(runtime.find(0, 0)->surface().colour[0] == .5f);
    CHECK(runtime.find(1, 0)->surface().colour[0] == .5f);
    CHECK(runtime.undo_preset(shared, "surface.tint"));
    CHECK(runtime.find(0, 0)->surface().colour[0] == 1.f);
    CHECK(!runtime.editor()->bindings->catalog.presets.at(shared).contains("surface.tint"));
    CHECK(
        !runtime.editor()->bindings->catalog.presets.at(shared).contains("emission.bake_intensity")
    );
}

void test_glass_transport_dirty(const std::filesystem::path& directory) {
    enhanced::MaterialRuntime runtime;
    CHECK(runtime.load(level(directory)));
    const std::array<std::size_t, 1> first{0};
    CHECK(!runtime.editor()->transport_dirty);
    CHECK(runtime.assign(first, "transport-glass", {{"type", std::string("glass-lit")}}));
    CHECK(runtime.find(0, 0)->surface().blend && runtime.editor()->transport_dirty);
    CHECK(runtime.clear_slots(first));
    CHECK(!runtime.editor()->transport_dirty);
    runtime.reset();
    const auto shared = runtime.find(0, 0)->values.material;
    CHECK(runtime.patch_preset(shared, "type", std::string("glass-lit")));
    CHECK(runtime.editor()->transport_dirty);
    CHECK(runtime.undo_preset(shared, "type"));
    CHECK(!runtime.editor()->transport_dirty);
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
        test_shared_materials(directory);
        test_all_material_types(directory);
        test_revert_created_material(directory);
        test_glass_transport_dirty(directory);
        test_material_control_defaults(directory);
        test_clear_saved_slots(directory);
        test_library_finishes(directory);
        test_exact_slots(directory);
        test_preview_and_save_share_author_priority(directory);
        test_transactional_preview(directory);
        test_ocean_preset(directory);
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
