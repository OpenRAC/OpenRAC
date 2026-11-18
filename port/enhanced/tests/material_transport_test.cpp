// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material bake-cache identity checks using synthetic metadata and transport, without a GPU or game assets.
#include <cstdio>
#include <exception>

#include "common/staged_file.h"
#include "enhanced/tools/baker/sources/level_scene.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"
#include "scene/material_overrides.h"
#include "tests/check.h"

namespace {
using namespace openrac;

// A static synthetic surface used to separate bake radiance from visible emission and roughness.
std::shared_ptr<scene::MaterialAssets> assets() {
    auto result = std::make_shared<scene::MaterialAssets>();
    scene::Material original;
    original.emission = {1, 1, 1};
    original.bake_intensity = 4;
    original.image = 0;
    original.emission_image = 0;
    result->originals.push_back(original);
    materials::Identity identity;
    identity.layer = "shrub";
    result->sources.push_back({0, 0, "fixture", identity, true});
    return result;
}

// Writes valid metadata while keeping the same selector and transport contribution.
materials::Catalog catalog(std::string_view property) {
    const auto result = materials::parse_catalog(
        std::string(R"({"version":1,"overrides":[{"id":"surface","match":{"layer":"shrub"},"set":)")
        + std::string(property) + "}]}"
    );
    CHECK(result);
    return result.value();
}

// Runtime-only JSON values and formatting preserve preparation identity; authored bake changes invalidate it.
void test_preparation_identity(const std::filesystem::path& directory) {
    const auto baseline = lighting::probe_input_hash(directory);
    const auto runtime =
        catalog(R"({"surface":{"roughness":0.75},"wetness":{"reflectivity":0.04}})");
    CHECK(materials::write_catalog(directory / "material-overrides.json", runtime));
    CHECK(lighting::probe_input_hash(directory) == baseline);
    const auto baked = catalog(R"({"emission":{"bake_intensity":8}})");
    CHECK(materials::write_catalog(directory / "material-overrides.json", baked));
    CHECK(lighting::probe_input_hash(directory) != baseline);
    const auto sparse_rgb = catalog(R"({"emission":{"bake_mesh_rgb":[2,4,8]}})");
    CHECK(materials::write_catalog(directory / "material-overrides.json", sparse_rgb));
    CHECK(lighting::probe_input_hash(directory) != baseline);
    std::error_code error;
    std::filesystem::remove(directory / "material-overrides.json", error);
    CHECK(!error);
}

void test_glass_preparation_identity(const std::filesystem::path& directory) {
    const auto baseline = lighting::probe_input_hash(directory);
    const auto glass = catalog(R"({"type":"glass-lit"})");
    CHECK(materials::write_catalog(directory / "material-overrides.json", glass));
    const auto glass_hash = lighting::probe_input_hash(directory);
    CHECK(glass_hash != baseline);
    const auto tuned =
        catalog(R"({"type":"glass-lit","glass":{"transmission":0.5},"surface":{"roughness":0.7}})");
    CHECK(materials::write_catalog(directory / "material-overrides.json", tuned));
    CHECK(lighting::probe_input_hash(directory) == glass_hash);
    const auto lit = catalog(R"({"type":"lit"})");
    CHECK(materials::write_catalog(directory / "material-overrides.json", lit));
    CHECK(lighting::probe_input_hash(directory) == baseline);
    materials::Catalog generated;
    generated.presets["lit-default"] = {{"type", std::string("glass-lit")}};
    CHECK(materials::write_catalog(directory / "material-overrides.json", generated));
    CHECK(lighting::probe_input_hash(directory) != baseline);
    std::filesystem::remove(directory / "material-overrides.json");
}

// Original suppresses later sparse contributions, so it must survive transport projection.
void test_original_preparation_identity(const std::filesystem::path& directory) {
    auto lit = materials::parse_catalog(
        R"({"version":1,"presets":{"paint":{"type":"lit"}},
      "overrides":[{"id":"assign","match":{"layer":"shrub"},"material":"paint"},
      {"id":"tint","match":{"layer":"shrub","class_id":7},"set":{"surface":{"tint":[0.5,1,1]}}}]})"
    );
    CHECK(lit);
    const auto source = assets();
    source->sources[0].identity.class_id = 7;
    scene::LevelData level;
    level.dir = directory;
    level.materials = source->originals;
    const lighting::BakeSceneExport scene;
    level.material_bindings = scene::resolve_materials(source, *lit).value();
    CHECK(level.material_bindings->bindings[0].surface().colour[0] == .5f);
    const auto lit_activation = lighting::probe_scene_hash(level, scene, 0, false);
    CHECK(materials::write_catalog(directory / "material-overrides.json", *lit));
    const auto lit_input = lighting::probe_input_hash(directory);
    auto original = *lit;
    original.presets["paint"]["type"] = std::string("original");
    level.material_bindings = scene::resolve_materials(source, original).value();
    CHECK(level.material_bindings->bindings[0].surface().colour[0] == 1.f);
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) != lit_activation);
    CHECK(materials::write_catalog(directory / "material-overrides.json", original));
    const auto original_input = lighting::probe_input_hash(directory);
    CHECK(original_input != lit_input);
    const auto projected =
        materials::parse_catalog(materials::transport_catalog_stamp(original).value());
    CHECK(projected);
    CHECK(
        materials::text_value(
            materials::resolve(*projected, source->sources[0].identity)->properties, "type"
        )
        == "original"
    );
    original.presets["paint"]["surface.roughness"] = .7;
    CHECK(materials::write_catalog(directory / "material-overrides.json", original));
    CHECK(lighting::probe_input_hash(directory) == original_input);
    std::filesystem::remove(directory / "material-overrides.json");
}

// Migrated identical values and visible glow edits preserve existing bakes when baked intensity remains fixed.
void test_activation_identity(const std::filesystem::path& directory) {
    scene::LevelData level;
    level.dir = directory;
    const auto source = assets();
    level.materials = source->originals;
    const lighting::BakeSceneExport scene;
    const auto baseline = lighting::probe_scene_hash(level, scene, 0, false);
    const auto migrated = catalog(R"({"emission":{"runtime_intensity":1,"bake_intensity":4}})");
    level.material_bindings = scene::resolve_materials(source, migrated).value();
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) == baseline);
    const auto visible =
        catalog(R"({"emission":{"runtime_intensity":2},"surface":{"roughness":0.8}})");
    level.material_bindings = scene::resolve_materials(source, visible).value();
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) == baseline);
    const auto baked = catalog(R"({"emission":{"bake_intensity":8}})");
    level.material_bindings = scene::resolve_materials(source, baked).value();
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) != baseline);
    const auto averaged = catalog(R"({"emission":{"average_for_bake":true}})");
    level.material_bindings = scene::resolve_materials(source, averaged).value();
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) == baseline);
    const auto point_sampled = catalog(R"({"emission":{"average_for_bake":false}})");
    level.material_bindings = scene::resolve_materials(source, point_sampled).value();
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) != baseline);

    const auto texture = catalog(R"({"emission":{"bake_mode":"texture","average_for_bake":true}})");
    const auto texture_surface =
        scene::resolve_materials(source, texture).value()->bindings[0].surface();
    CHECK(!texture_surface.bake_mesh_rgb);
    CHECK(texture_surface.bake_use_cutout);
    CHECK(!texture_surface.average_emission_for_bake);

    const auto mesh = catalog(R"({"emission":{"bake_mode":"mesh","bake_mesh_rgb":[2,4,8]}})");
    level.material_bindings = scene::resolve_materials(source, mesh).value();
    const auto& mesh_surface = level.material_bindings->bindings[0].surface();
    CHECK(mesh_surface.bake_mesh_rgb.value() == (scene::Vec3{2, 4, 8}));
    CHECK(!mesh_surface.bake_use_cutout);
    const auto mesh_hash = lighting::probe_scene_hash(level, scene, 0, false);
    CHECK(mesh_hash != baseline);
    const auto masked = catalog(
        R"({"emission":{"bake_mode":"mesh","bake_mesh_rgb":[2,4,8],"bake_use_cutout":true}})"
    );
    level.material_bindings = scene::resolve_materials(source, masked).value();
    CHECK(level.material_bindings->bindings[0].surface().bake_use_cutout);
    CHECK(lighting::probe_scene_hash(level, scene, 0, false) != mesh_hash);
    const auto incomplete = catalog(R"({"emission":{"bake_mode":"mesh"}})");
    CHECK(!scene::resolve_materials(source, incomplete));
}
}  // namespace

// Each run owns an exclusive folder and removes only its generated metadata.
int main() {
    try {
        auto staged = StagedFile::create(
            std::filesystem::temp_directory_path() / "openrac-material-transport"
        );
        CHECK(staged);
        if (!staged) {
            return 1;
        }
        const auto directory = staged->path().parent_path() / "level_fixture";
        std::error_code error;
        CHECK(std::filesystem::create_directory(directory, error));
        CHECK(!error);
        test_preparation_identity(directory);
        test_glass_preparation_identity(directory);
        test_original_preparation_identity(directory);
        test_activation_identity(directory);
        std::filesystem::remove(directory, error);
        CHECK(!error);
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
