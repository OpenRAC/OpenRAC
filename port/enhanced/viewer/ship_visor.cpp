// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Builds a pane from the exported canopy ribs so it follows the original joint animation.
#include "enhanced/viewer/ship_visor.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>

#include "enhanced/renderer/reflection_material.h"
#include "scene/level.h"

namespace openrac::lighting {
namespace {
// Three ribs sampled across the Veldin ship's canopy joint, in the base primitive's index order.
constexpr std::array kVeldinRibs{
    std::array{1703, 1697, 1691, 1679, 1673, 1667, 1655, 1649, 1643},
    std::array{1808, 1802, 1796, 1790, 1784, 1778, 1772, 1766, 1760},
    std::array{1860, 1871, 1943, 1931, 1919, 1907, 1895, 1737, 1730},
};

// The green ship's front and side windows sit between the upper and lower rails of joint 18.
constexpr std::array kGreenShipRails{
    std::array{
        11978,
        12323,
        12317,
        12311,
        12305,
        12299,
        12293,
        12287,
        12281,
        12275,
        12269,
        12263,
        12257,
        12251,
        11959
    },
    std::array{
        11976,
        3180,
        3174,
        3168,
        3162,
        3156,
        3150,
        3152,
        12197,
        12191,
        12185,
        12179,
        12173,
        12167,
        11958
    },
};

// Tangents come from neighbouring rib points, with one-sided differences at the pane edges.
renderer::Vec3 difference(const scene::Vertex& end, const scene::Vertex& start) {
    return {
        end.position[0] - start.position[0],
        end.position[1] - start.position[1],
        end.position[2] - start.position[2]
    };
}

// Identity of a verified NTSC-U canopy layout.
struct ShipLayout {
    int class_id;             // Ship model class.
    unsigned index_count;     // Size of the sole hull primitive.
    unsigned joint;           // Joint shared by the pane anchors.
    std::uint64_t signature;  // Ordered anchor positions in the exported bind pose.
};

// Class, index count, joint and anchor signature from the NTSC-U ship exports.
constexpr ShipLayout kVeldinShip{530, 12306, 6, 0x0e2369860a69769full};
constexpr ShipLayout kGreenShip{531, 15042, 18, 0x30a33792a887a3cdull};

// Fits a pane only when the selected offsets still describe the reviewed frame.
template <std::size_t Rows, std::size_t Columns>
void fit_pane(
    scene::LevelData& level,
    const ShipLayout& layout,
    const std::array<std::array<int, Columns>, Rows>& ribs
) {
    // Other regional exports and replacement models need their own verified fit.
    const auto ship = level.moby_classes.find(layout.class_id);
    if (ship == level.moby_classes.end() || ship->second.second) {
        return;
    }
    auto& model = level.models[ship->second.first];
    if (model.primitives.size() != 1
        || model.primitives.front().index_count != layout.index_count) {
        return;
    }
    const auto start = model.primitives.front().first_index;
    std::array<std::array<scene::Vertex, Columns>, Rows> pane;
    // Fold FP32 words in row order with the 64-bit FNV offset and prime.
    std::uint64_t signature = 14695981039346656037ull;
    constexpr std::uint64_t kFnvPrime = 1099511628211ull;
    for (std::size_t r = 0; r < pane.size(); ++r) {
        for (std::size_t c = 0; c < Columns; ++c) {
            const auto& anchor = level.vertices[level.indices[start + unsigned(ribs[r][c])]];
            // All anchors must belong entirely to the moving canopy joint.
            if (anchor.joints[0] != layout.joint || anchor.weights[0] != 1.f) {
                return;
            }
            for (const float coordinate : anchor.position) {
                // Treat signed zero equally across exporters and host transforms.
                const auto bits = coordinate == 0 ? 0u : std::bit_cast<std::uint32_t>(coordinate);
                signature = (signature ^ bits) * kFnvPrime;
            }
            pane[r][c] = anchor;
        }
    }
    // Matching counts and skin weights alone do not establish the frame geometry.
    if (signature != layout.signature) {
        return;
    }

    const auto vertex_start = static_cast<std::uint32_t>(level.vertices.size());
    for (std::size_t r = 0; r < pane.size(); ++r) {
        for (std::size_t c = 0; c < Columns; ++c) {
            auto vertex = pane[r][c];
            const auto across =
                difference(pane[r][std::min(c + 1, Columns - 1)], pane[r][c - (c > 0)]);
            const auto along =
                difference(pane[std::min(r + 1, pane.size() - 1)][c], pane[r - (r > 0)][c]);
            const auto normal = renderer::normalize(renderer::cross(along, across));
            std::copy(normal.begin(), normal.end(), vertex.normal);
            level.vertices.push_back(vertex);
        }
    }
    scene::Primitive primitive;
    primitive.first_index = static_cast<std::uint32_t>(level.indices.size());
    for (unsigned r = 0; r + 1 < pane.size(); ++r) {
        for (unsigned c = 0; c + 1 < Columns; ++c) {
            const auto a = vertex_start + r * unsigned(Columns) + c;
            const auto b = a + unsigned(Columns);
            level.indices.insert(level.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    primitive.index_count =
        static_cast<std::uint32_t>(level.indices.size()) - primitive.first_index;
    primitive.material = static_cast<int>(level.materials.size());
    scene::Material material;
    material.name = enhanced::kShipVisorMaterial;
    material.metal = true;
    // The pane follows the hull's one authored slot, independent of global material order.
    material.source_slot = 1;
    level.materials.push_back(std::move(material));
    model.primitives.push_back(primitive);
}
}  // namespace

void add_ship_visor(scene::LevelData& level) {
    // Only the RAC1 exports carry these two verified canopy layouts.
    if (level.game != "rac1") {
        return;
    }
    fit_pane(level, kVeldinShip, kVeldinRibs);
    fit_pane(level, kGreenShip, kGreenShipRails);
}
}  // namespace openrac::lighting
