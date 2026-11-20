// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// scene/prop_rig.h: a skinned prop's skeleton and animation, from a small glTF made here (two
// joints, the second turning a quarter about z in one second).

#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/math.h"
#include "scene/prop_rig.h"

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

std::filesystem::path write_gltf(bool broken = false) {
    std::vector<unsigned char> buffer;
    put(buffer, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});   // root: no bind offset
    put(buffer, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -1, 1});  // child, bound one up
    put(buffer, {0.0f, 1.0f});                                       // times
    const float h = std::sqrt(0.5f);
    put(buffer, {0, 0, 0, 1, 0, 0, h, h});     // none, a quarter about z
    put(buffer, {0, 0, 0, 1, 0, 0, 0, 1, 0});  // a triangle
    const std::string json = R"({
 "asset": {"version": "2.0"},
 "buffers": [{"byteLength": 204, "uri": "data:application/octet-stream;base64,)"
                             + base64(buffer) + R"("}],
 "bufferViews": [
  {"buffer": 0, "byteOffset": 0, "byteLength": 128},
  {"buffer": 0, "byteOffset": 128, "byteLength": 8},
  {"buffer": 0, "byteOffset": 136, "byteLength": 32},
  {"buffer": 0, "byteOffset": 168, "byteLength": 36}],
 "accessors": [
  {"bufferView": 0, "componentType": 5126, "count": )"
                             + std::string(broken ? "9" : "2") + R"(, "type": "MAT4"},
  {"bufferView": 1, "componentType": 5126, "count": 2, "type": "SCALAR", "min": [0], "max": [1]},
  {"bufferView": 2, "componentType": 5126, "count": 2, "type": "VEC4"},
  {"bufferView": 3, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]}],
 "meshes": [{"primitives": [{"attributes": {"POSITION": 3}}]}],
 "skins": [{"joints": [1, 2], "inverseBindMatrices": 0}],
 "nodes": [
  {"mesh": 0, "skin": 0},
  {"name": "root", "children": [2]},
  {"name": "tip", "translation": [0, 0, 1]}],
 "animations": [{"name": "turn", "samplers": [{"input": 1, "output": 2}],
   "channels": [{"sampler": 0, "target": {"node": 2, "path": "rotation"}}]}],
 "scenes": [{"nodes": [0, 1]}]
})";
    const auto path = std::filesystem::temp_directory_path()
                      / (broken ? "openrac_prop_rig_broken.gltf" : "openrac_prop_rig_test.gltf");
    std::ofstream(path) << json;
    return path;
}

bool near(const renderer::Vec3& a, const renderer::Vec3& b) {
    return std::fabs(a[0] - b[0]) < 1e-4f && std::fabs(a[1] - b[1]) < 1e-4f
           && std::fabs(a[2] - b[2]) < 1e-4f;
}

}  // namespace

int main() {
    std::string error;
    const auto rig = scene::load_prop_rig(write_gltf(), error);
    CHECK(rig.has_value());
    if (!rig) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return openrac::test::result();
    }
    CHECK(rig->palette_size == 2 && rig->animations.size() == 1);
    CHECK(rig->animations[0].name == "turn" && rig->animations[0].duration == 1.0f);
    // A point on the tip, bound one up: turned with it about the tip's joint.
    const renderer::Vec3 point{1.0f, 0.0f, 1.0f};
    auto at = [&](float seconds, bool loop) {
        const auto palette = scene::prop_rig_palette(*rig, 0, seconds, loop);
        return palette.size() == 2 ? renderer::transform_point(palette[1], point)
                                   : renderer::Vec3{};
    };
    CHECK(near(at(0.0f, false), {1.0f, 0.0f, 1.0f}));
    CHECK(near(at(1.0f, false), {0.0f, 1.0f, 1.0f}));
    CHECK(near(at(5.0f, false), {0.0f, 1.0f, 1.0f}));  // held at its end
    const float h = std::sqrt(0.5f);
    CHECK(near(at(2.5f, true), {h, h, 1.0f}));  // looped: half way
    // The root does not move; an animation the rig has not: nothing.
    const auto palette = scene::prop_rig_palette(*rig, 0, 0.5f, false);
    CHECK(palette.size() == 2 && near(renderer::transform_point(palette[0], point), point));
    CHECK(scene::prop_rig_palette(*rig, 1, 0.0f, false).empty());
    // An accessor longer than its buffer: no rig, nothing read past it.
    std::string broken_error;
    CHECK(
        !scene::load_prop_rig(write_gltf(true), broken_error).has_value() && !broken_error.empty()
    );
    return openrac::test::result();
}
