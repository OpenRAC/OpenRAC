// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared runtime/bake material bindings with synthetic classes, colours and emission only.
#include <cstdio>
#include <exception>

#include "scene/material_overrides.h"
#include "tests/check.h"

namespace {
using namespace openrac;

// Two object types share one original texture/material; only an explicit selector can distinguish them.
std::shared_ptr<scene::MaterialAssets> assets() {
    auto result = std::make_shared<scene::MaterialAssets>();
    scene::Material original;
    original.image = 0;
    original.cutout = true;
    original.colour = {.8f, .6f, .4f, .75f};
    result->originals.push_back(original);
    materials::Identity identity;
    identity.layer = "shrub";
    identity.class_id = 7;
    result->sources.push_back({0, 0, "shrub_7", identity, true});
    identity.class_id = 8;
    result->sources.push_back({1, 0, "shrub_8", identity, true});
    return result;
}

// Resolves catalog fixtures without reading a disc or creating a rendering context.
materials::Catalog catalog(std::string_view text) {
    const auto parsed = materials::parse_catalog(text);
    CHECK(parsed);
    return parsed ? *parsed : materials::Catalog{};
}

// Object-specific edits retain alpha and packet flags, and do not leak through a shared image index.
void test_isolation_and_original_coverage() {
    const auto source = assets();
    const auto rules = catalog(
        R"({"version":1,"overrides":[{"id":"leaf","match":{"layer":"shrub","class_id":7},"set":{"surface":{"tint":[0.5,1,1],"diffuse_scale":0.25}}}]})"
    );
    const auto compiled = scene::resolve_materials(source, rules);
    CHECK(compiled);
    const auto* selected = (*compiled)->find(0, 0);
    const auto* ordinary = (*compiled)->find(1, 0);
    CHECK(selected && ordinary);
    CHECK(selected->surface().colour[0] == .4f && ordinary->surface().colour[0] == .8f);
    CHECK(
        selected->surface().colour[3] == .75f && selected->surface().cutout
        && !selected->surface().blend
    );
    CHECK(source->originals[0].diffuse_scale == 1 && selected->surface().diffuse_scale == .25f);
    CHECK(selected->variant() != ordinary->variant());
}

// Pure surface-lighting changes preserve transport identities; genuine baked-energy edits change them.
void test_transport_identity() {
    auto source = assets();
    source->originals[0].colour[3] = 1;
    source->originals[0].emission = {1, 1, 1};
    source->originals[0].bake_intensity = 4;
    source->originals[0].emission_image = 0;
    const auto runtime = catalog(
        R"({"version":1,"overrides":[{"id":"leaf","match":{"layer":"shrub"},"set":{"emission":{"runtime_intensity":2},"surface":{"roughness":0.8}}}]})"
    );
    const auto visible = scene::resolve_materials(source, runtime);
    CHECK(visible && (*visible)->transport_stamp.empty());
    const auto bake = catalog(
        R"({"version":1,"overrides":[{"id":"leaf","match":{"layer":"shrub"},"set":{"emission":{"bake_intensity":8}}}]})"
    );
    const auto changed = scene::resolve_materials(source, bake);
    CHECK(changed && !(*changed)->transport_stamp.empty());
    const auto same = catalog(
        R"({"version":1,"overrides":[{"id":"leaf","match":{"layer":"shrub"},"set":{"emission":{"runtime_intensity":1,"bake_intensity":4}}}]})"
    );
    CHECK(scene::resolve_materials(source, same).value()->transport_stamp.empty());
}

// An explicitly referenced emission image contributes its content identity, not only its unchanged filename.
void test_emission_image_content_identity() {
    auto source = assets();
    source->originals[0].colour[3] = 1;
    source->images["textures/glow.png"] = 1;
    source->image_hashes[1] = "first synthetic image";
    const auto rules = catalog(R"({"version":1,"overrides":[{"id":"glow","match":{"layer":"shrub"},
      "set":{"emission":{"runtime_intensity":1,"texture":"textures/glow.png"}}}]})");
    const auto first = scene::resolve_materials(source, rules);
    CHECK(first && !(*first)->transport_stamp.empty());
    source->image_hashes[1] = "second synthetic image";
    const auto second = scene::resolve_materials(source, rules);
    CHECK(second && (*first)->transport_stamp != (*second)->transport_stamp);
}

// Identical effective records share a variant so model changes alone cannot split material batches.
void test_identical_variants_share_bindings() {
    const auto source = assets();
    const auto compiled = scene::resolve_materials(source, catalog(R"({"version":1})"));
    CHECK(compiled && (*compiled)->find(0, 0)->variant() == (*compiled)->find(1, 0)->variant());
    CHECK((*compiled)->records.size() == 1);
    CHECK((*compiled)->find(0, 0)->record == (*compiled)->find(1, 0)->record);
    CHECK((*compiled)->find(2, 0) == nullptr);
}

void test_legacy_sky_tint() {
    auto source = assets();
    source->sources[0].identity.layer = "sky";
    source->sources[1].identity.layer = "sky";
    const auto tinted = scene::resolve_materials(
        source,
        catalog(
            R"({"version":1,"overrides":[
      {"id":"sky-tint","match":{"layer":"sky","class_id":7},"set":{"surface":{"tint":[0.5,1,1]}}}]})"
        )
    );
    CHECK(tinted);
    CHECK((*tinted)->find(0, 0)->surface().colour[0] == .4f);
    CHECK(materials::text_value((*tinted)->find(0, 0)->values.properties, "type") == "lit");
    CHECK(materials::text_value((*tinted)->find(1, 0)->values.properties, "type") == "original");
}
}  // namespace

// No geometry, game addresses or GPU resources participate in these material-cache checks.
int main() {
    try {
        test_isolation_and_original_coverage();
        test_transport_identity();
        test_emission_image_content_identity();
        test_identical_variants_share_bindings();
        test_legacy_sky_tint();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
