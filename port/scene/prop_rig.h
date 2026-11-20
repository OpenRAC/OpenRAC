// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A mod's prop mesh (.glb) with a skeleton and animations (openrac_api.prop_animate): its joints'
// tree, their inverse binds and the animations' channels, and the skinning palette at a time into
// one. load_model keeps the mesh in its bind pose with the joints numbered as here (joint_NNN, else
// the joint's index in its skin), so the palette's entry for a joint is its matrix.

#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace openrac::scene {

struct PropRig {
    struct Node {
        int parent = -1;
        std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
        std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};  // x, y, z, w
        std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
    };

    struct Joint {
        int node = 0;
        int number = 0;                        // its palette entry
        std::array<float, 16> inverse_bind{};  // column-major
    };
    enum class Path {
        Translation,
        Rotation,
        Scale
    };
    enum class Interpolation {
        Linear,
        Step,
        Cubic
    };

    struct Channel {
        int node = 0;
        Path path = Path::Translation;
        Interpolation interpolation = Interpolation::Linear;
        std::vector<float> times;
        std::vector<float> values;  // width a key (three of them for a cubic one)
    };

    struct Animation {
        std::string name;
        float duration = 0.0f;
        std::vector<Channel> channels;
    };

    std::vector<Node> nodes;
    std::vector<Joint> joints;
    int palette_size = 0;
    std::vector<Animation> animations;
};

// The rig of the .glb at `path`; nothing when it cannot be read or has no skin. `error` says why
// when it cannot be read.
std::optional<PropRig> load_prop_rig(const std::filesystem::path& path, std::string& error);

// The skinning palette (column-major matrices, entry = joint number) of `animation` at `seconds`
// (looped over its length, else held at its ends); empty for an animation the rig does not have.
std::vector<std::array<float, 16>> prop_rig_palette(
    const PropRig& rig, int animation, float seconds, bool loop
);

}  // namespace openrac::scene
