// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material validation, IEEE half words and synthetic texture selection, without retail assets or a GPU.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "common/json.h"
#include "enhanced/renderer/foliage.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"
#include "tests/check.h"

namespace {
using namespace openrac;

// Creates and removes only a newly owned temporary directory, independent of time and randomness.
class Temporary {
public:
    Temporary() {
        // An already owned candidate is skipped rather than reused or removed.
        for (unsigned attempt = 0; attempt < 1000; ++attempt) {
            m_path = std::filesystem::temp_directory_path()
                     / ("openrac-foliage-" + std::to_string(attempt));
            if (std::filesystem::create_directory(m_path)) {
                return;
            }
        }
        throw std::runtime_error("cannot create foliage test directory");
    }

    // Removes only this instance's newly created fixture.
    ~Temporary() {
        std::error_code ignored;
        std::filesystem::remove_all(m_path, ignored);
    }

    Temporary(const Temporary&) = delete;
    Temporary& operator=(const Temporary&) = delete;
    Temporary(Temporary&&) = delete;
    Temporary& operator=(Temporary&&) = delete;

    // Returns the owned fixture root.
    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;  // Owned, newly created fixture root.
};

// Parses a small control object independently of profile matching.
Result<enhanced::FoliageMaterial> material(std::string_view text) {
    json::Value value;
    std::string error;
    CHECK(json::parse(text, value, error));
    return enhanced::read_foliage_material(value);
}

// Optical controls cannot admit negative energy, invalid roughness or nonnumeric fields.
void test_parameter_validation() {
    const auto defaults = material("{}");
    CHECK(defaults && defaults->transmission == .35f && defaults->roughness == .65f);
    CHECK(material(R"({"transmission":0,"thickness":4,"normal_bend":0,"roughness":1})"));
    // Every invalid field is rejected instead of silently selecting a default.
    for (std::string_view input :
         {"[]",
          "0",
          R"({"transmission":-1})",
          R"({"thickness":5})",
          R"({"normal_bend":2})",
          R"({"roughness":0})",
          R"({"roughness":".5"})",
          R"({"thickness":1e99})"}) {
        CHECK(!material(input));
    }
    // Positive IEEE binary16 one is 0x3C00 in each half of the two spare material words.
    const auto words = enhanced::pack_foliage_material({1, 1, 1, 1});
    CHECK(words[0] == 0x3C003C00u && words[1] == 0x3C003C00u);
}

// Both texture pixels and dimensions participate in identity, including transparent pixels.
void test_image_identity() {
    renderer::Rgba8Image first(1, 4), other(4, 1);
    first.pixels = other.pixels;
    CHECK(enhanced::foliage_texture_hash(first) != enhanced::foliage_texture_hash(other));
    other = first;
    CHECK(enhanced::foliage_texture_hash(first) == enhanced::foliage_texture_hash(other));
    other.pixels[3] = 1;
    CHECK(enhanced::foliage_texture_hash(first) != enhanced::foliage_texture_hash(other));
    CHECK(enhanced::foliage_texture_hash({}).empty());
}

// Writes only synthetic metadata into the owned fixture.
void save(const std::filesystem::path& path, const std::string& text) {
    std::ofstream output(path);
    output << text;
    output.close();
    CHECK(!output.fail());
}

// Reviewed matches opt in, and all non-leaf or disabled paths return exact zero controls.
void test_profile_selection() {
    Temporary temporary;
    scene::LevelData level;
    level.dir = temporary.path();
    level.images.emplace_back(1, 1);
    level.images[0].pixels = {20, 80, 40, 255};
    // Independent SHA-256 of little-endian 1x1 dimensions followed by RGBA bytes 20,80,40,255.
    CHECK(
        enhanced::foliage_texture_hash(level.images[0])
        == "a59c05317aeeb8780ebe19e0f924bee51965e046a1a83d00ae059abb882b4861"
    );
    scene::Material leaf;
    leaf.image = 0;
    leaf.cutout = true;
    level.materials.push_back(leaf);
    const std::string profile = R"({"version":1,"materials":[{"texture_rgba_sha256":")"
                                + enhanced::foliage_texture_hash(level.images[0])
                                + R"(","transmission":0.5}]})";
    const auto path = level.dir / "foliage-materials.json";
    save(path, profile);
    enhanced::FoliageMaterials foliage;
    CHECK(foliage.load(level) && foliage.size() == 1);
    CHECK(foliage.parameters(leaf, true)[1] != 0);
    CHECK(foliage.parameters(leaf, false)[1] == 0);
    // The same texture on a trunk, metal packet or blended effect is not a leaf material.
    leaf.cutout = false;
    CHECK(foliage.parameters(leaf, true)[1] == 0);
    leaf.cutout = true;
    leaf.metal = true;
    CHECK(foliage.parameters(leaf, true)[1] == 0);
    leaf.metal = false;
    leaf.blend = true;
    CHECK(foliage.parameters(leaf, true)[1] == 0);
    // Replacement artwork opts out without relying on filenames, green colour or a shrub class.
    level.images[0].pixels[0] = 21;
    CHECK(foliage.load(level) && foliage.size() == 0);
    save(path, R"({"version":1,"materials":[],"debug":0.5})");
    CHECK(!foliage.load(level) && foliage.size() == 0);
    save(path, R"({"version":1,"materials":[{"texture_rgba_sha256":"bad"}]})");
    CHECK(!foliage.load(level) && foliage.size() == 0);
}

// Live edits repack matched images atomically and reset to the loaded per-texture profile values.
void test_live_controls_and_reset() {
    Temporary temporary;
    scene::LevelData level;
    level.dir = temporary.path();
    level.images.emplace_back(1, 1);
    level.images[0].pixels = {20, 80, 40, 255};
    scene::Material leaf;
    leaf.image = 0;
    leaf.cutout = true;
    level.materials.push_back(leaf);
    save(
        level.dir / "foliage-materials.json",
        R"({"version":1,"materials":[{"name":"Test leaf","texture_rgba_sha256":")"
            + enhanced::foliage_texture_hash(level.images[0])
            + R"(","normal_bend":0,"transmission":0.5}]})"
    );
    enhanced::FoliageMaterials foliage;
    CHECK(foliage.load(level));
    const auto original = foliage.parameters(leaf, true);
    auto controls = foliage.controls();
    CHECK(controls.materials.size() == 1);
    CHECK(controls.materials[0].name == "Test leaf" && controls.materials[0].matched_images == 1);
    controls.materials[0].material.transmission = 0;
    CHECK(foliage.parameters(leaf, true) == original);
    CHECK(foliage.set_controls(controls));
    CHECK(foliage.parameters(leaf, true) != original);
    // One invalid component rejects the master-switch edit too.
    controls.enabled = false;
    controls.materials[0].material.roughness = .1f;
    CHECK(!foliage.set_controls(controls) && foliage.controls().enabled);
    controls = foliage.controls();
    controls.materials[0].texture_hash = "stale";
    CHECK(!foliage.set_controls(controls));
    controls = foliage.controls();
    controls.enabled = false;
    controls.debug = 2;
    CHECK(foliage.set_controls(controls) && foliage.parameters(leaf, true)[1] == 0);
    foliage.reset_controls();
    CHECK(foliage.parameters(leaf, true) == original && foliage.controls().debug == 0);
    CHECK(foliage.controls().materials[0].material.normal_bend == 0);
}

// Runtime leaf controls cannot invalidate baked opaque transport; original artwork still does.
void test_foliage_controls_do_not_invalidate_probes() {
    Temporary temporary;
    save(temporary.path() / "manifest.json", "{}");
    const std::uint64_t original = lighting::probe_input_hash(temporary.path());
    save(temporary.path() / "foliage-materials.json", R"({"enabled":false})");
    CHECK(lighting::probe_input_hash(temporary.path()) == original);
    save(temporary.path() / "foliage-materials.json", R"({"enabled":true,"debug":3})");
    CHECK(lighting::probe_input_hash(temporary.path()) == original);
    save(temporary.path() / "manifest.json", R"({"changed":true})");
    CHECK(lighting::probe_input_hash(temporary.path()) != original);
}

// A higher-priority disabled profile overrides a lower-priority enabled material catalog.
void test_profile_priority_and_master_switch() {
    Temporary temporary;
    scene::LevelData level;
    level.dir = temporary.path();
    level.overrides = level.dir / "override";
    std::error_code error;
    CHECK(std::filesystem::create_directory(level.overrides, error) && !error);
    level.images.emplace_back(1, 1);
    level.images[0].pixels = {20, 80, 40, 255};
    scene::Material leaf;
    leaf.image = 0;
    leaf.cutout = true;
    level.materials.push_back(leaf);
    const std::string profile = R"({"version":1,"materials":[{"texture_rgba_sha256":")"
                                + enhanced::foliage_texture_hash(level.images[0]) + R"("}]})";
    save(level.dir / "foliage-materials.json", profile);
    save(
        level.overrides / "foliage-materials.json",
        R"({"version":1,"enabled":false,"materials":[]})"
    );
    enhanced::FoliageMaterials foliage;
    CHECK(foliage.load(level) && foliage.parameters(leaf, true)[1] == 0);
    level.mod_roots.push_back(level.dir);
    CHECK(foliage.load(level) && foliage.parameters(leaf, true)[1] != 0);
    foliage.clear();
    CHECK(foliage.size() == 0 && foliage.parameters(leaf, true)[1] == 0);
}
}  // namespace

// All inputs are synthetic; no graphics context or disc is needed.
int main() {
    try {
        test_parameter_validation();
        test_image_identity();
        test_live_controls_and_reset();
        test_foliage_controls_do_not_invalidate_probes();
        test_profile_selection();
        test_profile_priority_and_master_switch();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
