// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material catalog parsing, source precedence and serialization using synthetic identities only.
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <exception>
#include <limits>

#include "common/json.h"
#include "common/staged_file.h"
#include "materials/catalog.h"
#include "tests/check.h"

namespace {
using namespace openrac;

// Parses independent authored fixtures, reporting syntax failures before accessing their values.
materials::Catalog catalog(std::string_view text, std::string source = "fixture.json") {
    const auto parsed = materials::parse_catalog(text, std::move(source));
    CHECK(parsed);
    return parsed ? *parsed : materials::Catalog{};
}

// Existing JSON readers keep last-key semantics while authored catalogs explicitly reject duplicates.
void test_json_reader_compatibility() {
    json::Value value;
    std::string error;
    CHECK(json::parse(R"({"a":1,"a":2})", value, error));
    CHECK(value["a"].number() == 2);
    CHECK(!json::parse(R"({"a":1,"a":2})", value, error, json::DuplicateKeys::Reject));
}

// Validation happens before publication; an invalid edit cannot replace the last readable profile.
void test_atomic_catalog_save() {
    auto fixture =
        StagedFile::create(std::filesystem::temp_directory_path() / "openrac-material-catalog");
    CHECK(fixture);
    if (!fixture) {
        return;
    }
    const auto first =
        catalog(R"({"version":1,"presets":{"surface":{"surface":{"roughness":0.5}}}})");
    CHECK(materials::write_catalog(fixture->path(), first));
    auto second = first;
    second.presets["surface"]["surface.roughness"] = .8;
    CHECK(materials::write_catalog(fixture->path(), second));
    CHECK(materials::read_catalog(fixture->path())->presets == second.presets);
    second.presets["surface"]["surface.roughness"] = std::numeric_limits<double>::quiet_NaN();
    CHECK(!materials::write_catalog(fixture->path(), second));
    CHECK(
        std::get<double>(
            materials::read_catalog(fixture->path())->presets.at("surface").at("surface.roughness")
        )
        == .8
    );
}

// Both metadata parsing and FP32 conversion are canonical under the game's toward-zero environment.
void test_rounding_independence() {
    const int original = std::fegetround();
    const auto nearest = catalog(R"({"version":1,"presets":{"a":{"fur":{"scattering":0.33}}}})");
    CHECK(std::fesetround(FE_TOWARDZERO) == 0);
    const auto toward_zero =
        catalog(R"({"version":1,"presets":{"a":{"fur":{"scattering":0.33}}}})");
    CHECK(std::fegetround() == FE_TOWARDZERO);
    CHECK(nearest.presets == toward_zero.presets);
    CHECK(materials::number_value(toward_zero.presets.at("a"), "fur.scattering", 0) == .33f);
    CHECK(std::fesetround(original) == 0);
}

// Strict metadata cannot hide misspellings, duplicate keys, nonnumeric controls or ambiguous ids.
void test_validation() {
    CHECK(materials::parse_catalog(R"({"version":1})"));
    for (std::string_view text :
         {R"({"version":2})",
          R"({"version":1,"typo":{}})",
          R"({"version":1,"presets":{"leaf":{},"leaf":{}}})",
          R"({"version":1,"presets":{"leaf":{"foliage":{"transmission":2}}}})",
          R"({"version":1,"presets":{"leaf":{"fur":{"enabled":1}}}})",
          R"({"version":1,"presets":{"leaf":{"surface":{"tint":[1,2]}}}})",
          R"({"version":1,"presets":{"leaf":{"emission":{"texture":"../outside.png"}}}})",
          R"({"version":1,"presets":{"leaf":{"shading":"unsupported"}}})",
          R"({"version":1,"overrides":[{"id":"leaf","match":{}}]})",
          R"({"version":1,"overrides":[{"id":"leaf","match":{"class_id":0.5}}]})",
          R"({"version":1,"overrides":[{"id":"leaf","match":{"texture_rgba_sha256":"bad"}}]})",
          R"({"version":1,"overrides":[{"id":"a","enabled":false},{"id":"a","enabled":false}]})"}) {
        CHECK(!materials::parse_catalog(text));
    }
    CHECK(!materials::validate({{"surface.roughness", std::numeric_limits<double>::quiet_NaN()}}));
    const auto error = materials::parse_catalog(
        R"({"version":1,"presets":{"leaf":{"fur":{"typo":0}}}})", "bad.json"
    );
    CHECK(
        !error
        && error.error().message.find("bad.json: $.presets.leaf.fur.typo") != std::string::npos
    );
}

// Object-specific settings affect every matching instance without leaking to another layer or class.
void test_identity_and_conflicts() {
    const auto source =
        catalog(R"({"version":1,"presets":{"leaf":{"surface":{"roughness":0.7}}},"overrides":[
        {"id":"leaf","match":{"layer":"shrub","class_id":7},"preset":"leaf"}]})");
    materials::Identity identity;
    identity.layer = "shrub";
    identity.class_id = 7;
    const auto selected = materials::resolve(source, identity);
    CHECK(selected && std::get<double>(selected->properties.at("surface.roughness")) == .7);
    identity.class_id = 8;
    CHECK(materials::resolve(source, identity)->properties.empty());
    identity.class_id = 7;
    identity.layer = "moby";
    CHECK(materials::resolve(source, identity)->properties.empty());
    const auto ambiguous = catalog(R"({"version":1,"overrides":[
        {"id":"a","match":{"layer":"moby"},"set":{"surface":{"roughness":0.4}}},
        {"id":"b","match":{"layer":"moby"},"set":{"surface":{"roughness":0.8}}}]})");
    CHECK(!materials::resolve(ambiguous, identity));
    const auto missing = catalog(
        R"({"version":1,"overrides":[{"id":"a","match":{"layer":"moby"},"preset":"unknown"}]})"
    );
    CHECK(!materials::resolve(missing, identity));
}

// A sparse higher-source edit promotes only the changed fields, preserving independent lower rules.
void test_sparse_source_precedence() {
    materials::Catalog merged;
    materials::merge_catalog(
        merged,
        catalog(
            R"({"version":1,"overrides":[
      {"id":"broad","match":{"layer":"shrub"},"set":{"foliage":{"transmission":0.3},"surface":{"roughness":0.5}}},
      {"id":"specific","match":{"layer":"shrub","class_id":7},"set":{"foliage":{"transmission":0.8}}}]})",
            "base.json"
        ),
        0
    );
    materials::merge_catalog(
        merged,
        catalog(
            R"({"version":1,"overrides":[
      {"id":"broad","match":{"layer":"shrub"},"set":{"surface":{"roughness":0.9}}}]})",
            "mod.json"
        ),
        1
    );
    materials::Identity identity;
    identity.layer = "shrub";
    identity.class_id = 7;
    const auto result = materials::resolve(merged, identity);
    CHECK(result);
    CHECK(std::get<double>(result->properties.at("foliage.transmission")) == .8);
    CHECK(std::get<double>(result->properties.at("surface.roughness")) == .9);
    CHECK(result->sources.at("foliage.transmission") == "base.json");
    CHECK(result->sources.at("surface.roughness") == "mod.json");
    materials::merge_catalog(
        merged, catalog(R"({"version":1,"overrides":[{"id":"specific","enabled":false}]})"), 2
    );
    CHECK(
        std::get<double>(
            materials::resolve(merged, identity)->properties.at("foliage.transmission")
        )
        == .3
    );
}

// A higher-source preset field competes at its actual layer, rather than the layer that first referenced it.
void test_preset_field_precedence() {
    auto base = catalog(
        R"({"version":1,"presets":{"coat":{"surface":{"roughness":0.5}}},"overrides":[
      {"id":"preset","match":{"layer":"moby"},"preset":"coat"},
      {"id":"specific","match":{"layer":"moby","class_id":7},"set":{"surface":{"roughness":0.6}}}]})"
    );
    const auto overlay =
        catalog(R"({"version":1,"presets":{"coat":{"surface":{"roughness":0.8}}}})", "mod.json");
    materials::merge_catalog(base, overlay, 1);
    materials::Identity identity;
    identity.layer = "moby";
    identity.class_id = 7;
    const auto resolved = materials::resolve(base, identity);
    CHECK(resolved && materials::number_value(resolved->properties, "surface.roughness", 0) == .8f);
    CHECK(resolved->sources.at("surface.roughness") == "mod.json");
}

// Escaped author labels, presets and booleans survive a stable save/read cycle.
void test_round_trip_and_bake_projection() {
    const auto source = catalog(
        R"({"version":1,"presets":{"leaf":{"shading":"foliage","surface":{"roughness":0.65},"foliage":{"thickness":0.7}}},"overrides":[
      {"id":"leaf","name":"Quote \" and newline\n","match":{"layer":"shrub","cutout_only":true},"preset":"leaf"}]})"
    );
    const auto text = materials::catalog_text(source);
    CHECK(text);
    const auto reread = materials::parse_catalog(*text);
    CHECK(reread && reread->rules.at("leaf").name == source.rules.at("leaf").name);
    CHECK(materials::catalog_text(*reread).value() == *text);
    const materials::Properties properties{
        {"surface.roughness", .5},
        {"foliage.thickness", .7},
        {"emission.runtime_intensity", 4.0},
        {"emission.bake_intensity", 2.0}
    };
    const auto transport = materials::transport_properties(properties);
    CHECK(transport.size() == 1 && std::get<double>(transport.at("emission.bake_intensity")) == 2);
}
}  // namespace

// Synthetic catalog fixtures need neither a disc, a window nor a GPU.
int main() {
    try {
        test_validation();
        test_rounding_independence();
        test_preset_field_precedence();
        test_json_reader_compatibility();
        test_atomic_catalog_save();
        test_identity_and_conflicts();
        test_sparse_source_precedence();
        test_round_trip_and_bake_projection();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
