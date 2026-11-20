// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A mod's material on a moby as the renderer carries it (interface 1.23): packed with the shine
// distance into the instance's `lights` (scene::pack_moby_lights) and read back as mesh.vert reads
// it, for every shine distance a moby can have; a lit tie's `lights` untouched.
#include <cmath>
#include <cstdint>

#include "check.h"
#include "scene/level.h"

using namespace openrac;

namespace {

// OPENRAC_MOBY_MATERIAL_NONE, _TINT, _PLATINUM and _OBSIDIAN (sdk/include/openrac/openrac_mod.h).
constexpr std::uint8_t kNone = 0;
constexpr std::uint8_t kTint = 1;
constexpr std::uint8_t kPlatinum = 2;
constexpr std::uint8_t kObsidian = 3;

// What mesh.vert makes of `lights`: the mod's material and the shine distance (0: none).
struct Read {
    int material = 0;
    int distance = 0;
};

// Returns `lights` read as mesh.vert reads it, in floats as the shader has them.
Read read_lights(int packed) {
    const float lights = static_cast<float>(packed);
    const float code = lights < -0.5f ? -lights - 1.0f : 0.0f;
    const float material =
        std::floor(code / static_cast<float>(scene::kMobyMaterialLightsStep) + 0.001f);
    const float distance = code - static_cast<float>(scene::kMobyMaterialLightsStep) * material;
    return {static_cast<int>(material + 0.5f), static_cast<int>(distance + 0.5f)};
}

}  // namespace

int main() {
    // A moby's shine distance is its byte at +0x73: none (-1) or 1 to 255, -1 - distance.
    constexpr int kDistances = 256;

    // Every material with every distance comes back as it went in.
    for (const std::uint8_t material : {kNone, kTint, kPlatinum, kObsidian}) {
        // The distances 0 (none) to 255.
        for (int distance = 0; distance < kDistances; ++distance) {
            const Read r = read_lights(scene::pack_moby_lights(-1 - distance, material));
            CHECK(r.material == material && r.distance == distance);
        }
    }

    // No material leaves `lights` as it was.
    CHECK(scene::pack_moby_lights(-25, 0) == -25);
    // A lit tie's first light colour is never a material.
    CHECK(scene::pack_moby_lights(7, kPlatinum) == 7);
    CHECK(read_lights(7).material == 0);
    // An instance carries none unless the frontend gives it one.
    CHECK(scene::Instance{}.mod_material == 0);
    return test::result();
}
