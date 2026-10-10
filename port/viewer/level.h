// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One level as the editor's port export writes it (editor/port.py):
//
//   manifest.json      format 1: the level, its files, the classes
//   placements.json    ties, shrubs and mobys: class, 4 x 4 matrix (game
//                      axes, column-major) and the fields the game stores
//   terrain.glb        the terrain fragments
//   ties/, shrubs/, mobys/   one GLB per class (mobys in their bind pose)
//   sky.glb            the sky shells, in drawing order (vertex colours)
//   textures/*.png     shared by every mesh
//
// Loading is all CPU work (no GL), so it is tested without a context: every
// mesh goes into one vertex and index array, every PNG into an RGBA8 image.

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "renderer/math.h"
#include "renderer/texture.h"

namespace openrac::viewer {

using renderer::Mat4;
using renderer::Vec3;

inline constexpr int kManifestFormat = 1;

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float colour[4];
};

struct Material {
    int image = -1;       // index into LevelData::images; -1 draws white
    bool cutout = false;  // glTF alphaMode MASK: alpha test at 0.5
    bool blend = false;   // glTF alphaMode BLEND
    std::array<float, 4> colour{1.0f, 1.0f, 1.0f, 1.0f};
};

struct Primitive {
    std::uint32_t first_index = 0;
    std::uint32_t index_count = 0;
    int material = 0;
};

struct Model {
    std::string name;
    std::vector<Primitive> primitives;
    Vec3 min{};
    Vec3 max{};
};

struct Instance {
    std::uint32_t model = 0;
    Mat4 matrix = renderer::identity();
    std::array<float, 4> tint{1.0f, 1.0f, 1.0f, 1.0f};
};

// What the viewer draws, in the game's order.
enum class Layer : std::uint8_t {
    Sky,
    Terrain,
    Ties,
    Shrubs,
    Mobys,
    Count
};

inline constexpr std::size_t kLayerCount = static_cast<std::size_t>(Layer::Count);

const char* layer_name(Layer layer);

struct LevelData {
    std::filesystem::path dir;
    int level = -1;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Model> models;
    std::vector<Material> materials;
    std::vector<renderer::Rgba8Image> images;
    std::vector<std::string> image_paths;
    std::array<std::vector<Instance>, kLayerCount> instances;
    // Moby classes: model and whether it is the stand-in box, for placing live mobys.
    std::map<int, std::pair<std::uint32_t, bool>> moby_classes;
    std::array<float, 3> background{0.0f, 0.0f, 0.0f};  // where no sky shell covers
    Vec3 bounds_min{};
    Vec3 bounds_max{};
    int missing_images = 0;
};

// Load a level directory. Returns false with a reason if the manifest or a
// mesh cannot be read; a missing texture only counts in missing_images.
bool load_level(const std::filesystem::path& dir, LevelData& out, std::string& error);

// Load one glTF or GLB file as one model (every mesh node, in its node's
// transform, except skinned meshes, which are drawn in their bind pose).
// Returns the model's index.
std::optional<std::uint32_t> load_model(
    const std::filesystem::path& path, LevelData& level, std::string& error
);

// A unit box, 0.5 on a side, standing on its base: what the viewer draws for
// a moby class without a mesh.
std::uint32_t add_box_model(LevelData& level);

}  // namespace openrac::viewer
