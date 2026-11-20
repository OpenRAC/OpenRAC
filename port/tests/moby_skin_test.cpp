// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// scene/moby_skin.h: the editor's skins.json read with its model, which animation plays for a
// class's sequence, and the clock that restarts it when the sequence changes.

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "check.h"
#include "scene/moby_skin.h"

namespace {

using namespace openrac;

std::string base64(const std::vector<unsigned char>& bytes) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (std::size_t i = 0; i < bytes.size(); i += 3) {
        const unsigned v = (bytes[i] << 16) | ((i + 1 < bytes.size() ? bytes[i + 1] : 0) << 8)
                           | (i + 2 < bytes.size() ? bytes[i + 2] : 0);
        out += table[(v >> 18) & 63];
        out += table[(v >> 12) & 63];
        out += i + 1 < bytes.size() ? table[(v >> 6) & 63] : '=';
        out += i + 2 < bytes.size() ? table[v & 63] : '=';
    }
    return out;
}

void put(std::vector<unsigned char>& bytes, std::initializer_list<float> values) {
    for (float v : values) {
        unsigned char b[4];
        std::memcpy(b, &v, 4);
        bytes.insert(bytes.end(), b, b + 4);
    }
}

// A triangle on two joints with two animations, "a" and "b", in DIR/skins/model.gltf.
void write_model(const std::filesystem::path& dir) {
    std::vector<unsigned char> buffer;
    put(buffer, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    put(buffer, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -1, 1});
    put(buffer, {0.0f, 1.0f});
    const float h = std::sqrt(0.5f);
    put(buffer, {0, 0, 0, 1, 0, 0, h, h});
    put(buffer, {0, 0, 0, 1, 0, 0, 0, 1, 0});
    for (int i = 0; i < 3; ++i) {
        buffer.insert(buffer.end(), {1, 0, 0, 0});
    }
    for (int i = 0; i < 3; ++i) {
        put(buffer, {1, 0, 0, 0});
    }
    const std::string json = R"({
 "asset": {"version": "2.0"},
 "buffers": [{"byteLength": 264, "uri": "data:application/octet-stream;base64,)"
                             + base64(buffer) + R"("}],
 "bufferViews": [
  {"buffer": 0, "byteOffset": 0, "byteLength": 128},
  {"buffer": 0, "byteOffset": 128, "byteLength": 8},
  {"buffer": 0, "byteOffset": 136, "byteLength": 32},
  {"buffer": 0, "byteOffset": 168, "byteLength": 36},
  {"buffer": 0, "byteOffset": 204, "byteLength": 12},
  {"buffer": 0, "byteOffset": 216, "byteLength": 48}],
 "accessors": [
  {"bufferView": 0, "componentType": 5126, "count": 2, "type": "MAT4"},
  {"bufferView": 1, "componentType": 5126, "count": 2, "type": "SCALAR", "min": [0], "max": [1]},
  {"bufferView": 2, "componentType": 5126, "count": 2, "type": "VEC4"},
  {"bufferView": 3, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]},
  {"bufferView": 4, "componentType": 5121, "count": 3, "type": "VEC4"},
  {"bufferView": 5, "componentType": 5126, "count": 3, "type": "VEC4"}],
 "meshes": [{"primitives": [{"attributes": {"POSITION": 3, "JOINTS_0": 4, "WEIGHTS_0": 5}}]}],
 "skins": [{"joints": [1, 2], "inverseBindMatrices": 0}],
 "nodes": [
  {"mesh": 0, "skin": 0},
  {"name": "joint_000", "children": [2]},
  {"name": "joint_001", "translation": [0, 0, 1]}],
 "animations": [
  {"name": "a", "samplers": [{"input": 1, "output": 2}],
   "channels": [{"sampler": 0, "target": {"node": 2, "path": "rotation"}}]},
  {"name": "b", "samplers": [{"input": 1, "output": 2}],
   "channels": [{"sampler": 0, "target": {"node": 1, "path": "rotation"}}]}],
 "scenes": [{"nodes": [0, 1]}]
})";
    std::filesystem::create_directories(dir / "skins");
    std::ofstream(dir / "skins" / "model.gltf") << json;
}

bool load(
    const std::filesystem::path& dir,
    const std::string& skins,
    scene::MobySkins& out,
    std::string& error
) {
    std::ofstream(dir / "skins.json") << skins;
    scene::LevelData level;
    std::vector<std::string> warnings;
    const bool read = scene::load_moby_skins(dir / "skins.json", level, out, error, warnings);
    for (const auto& warning : warnings) {
        std::fprintf(stderr, "warning: %s\n", warning.c_str());
    }
    return read;
}

}  // namespace

int main() {
    const auto dir = std::filesystem::temp_directory_path() / "openrac_moby_skin_test";
    std::filesystem::remove_all(dir);
    write_model(dir);

    std::string error;
    scene::MobySkins skins;
    CHECK(load(
        dir,
        R"({"format": 1, "skins": [
        {"spawn_id": 7, "model": "skins/model.gltf", "scale": 2, "animations": [1, -1, 5]},
        {"spawn_id": 9, "model": "skins/model.gltf", "scale": 0.5}]})",
        skins,
        error
    ));
    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    CHECK(skins.by_spawn_id.size() == 2);
    const auto& skin = skins.by_spawn_id[7];
    CHECK(skin.scale == 2.0f && skin.rig.animations.size() == 2);
    CHECK(skins.by_spawn_id[9].model == skin.model);  // one model for both
    // Sequence 0 plays the chosen 1; 1 (-1) its own number; 2 (5, not in the rig) and 3 (none,
    // past the rig's) the first.
    CHECK(scene::moby_skin_animation(skin, 0) == 1);
    CHECK(scene::moby_skin_animation(skin, 1) == 1);
    CHECK(scene::moby_skin_animation(skin, 2) == 0);
    CHECK(scene::moby_skin_animation(skin, 3) == 0);
    CHECK(scene::moby_skin_animation(skins.by_spawn_id[9], 1) == 1);

    // The clock: a tick a call, from 0 again when the sequence changes, by moby.
    CHECK(scene::moby_skin_seconds(skins, 0x1000, 4) == 0.0f);
    CHECK(scene::moby_skin_seconds(skins, 0x1000, 4) == 1.0f / 60.0f);
    CHECK(scene::moby_skin_seconds(skins, 0x2000, 4) == 0.0f);
    CHECK(scene::moby_skin_seconds(skins, 0x1000, 4) == 2.0f / 60.0f);
    CHECK(scene::moby_skin_seconds(skins, 0x1000, 5) == 0.0f);

    // A model outside the pack, a broken entry, a model that is not there: refused or left out.
    scene::MobySkins outside;
    CHECK(!load(
        dir, R"({"format": 1, "skins": [{"spawn_id": 1, "model": "../x.glb"}]})", outside, error
    ));
    scene::MobySkins bad;
    CHECK(!load(
        dir,
        R"({"format": 1, "skins": [{"spawn_id": -3, "model": "skins/model.gltf"}]})",
        bad,
        error
    ));
    scene::MobySkins missing;
    error.clear();
    CHECK(load(
        dir,
        R"({"format": 1, "skins": [{"spawn_id": 1, "model": "skins/none.glb"}]})",
        missing,
        error
    ));
    CHECK(missing.empty());
    std::filesystem::remove_all(dir);
    return openrac::test::result();
}
