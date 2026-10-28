// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/renderer.h"
#include "scene/level.h"

namespace openrac::scene {
struct PickHit {
    Layer layer = Layer::Terrain;
    unsigned instance = 0, model = 0, primitive = 0, triangle = 0;
    int material = -1, class_id = -1;
    std::uint64_t identity = 0;
    float distance = 0;
    Vec3 position{}, object_position{};
    std::array<float, 3> barycentric{};
    std::array<Vec3, 3> face{};
    std::array<Vec3, 8> bounds{};
    std::string name, texture;
};

struct PickResult {
    bool sampled = false;
    int level = -1;
    std::uint64_t tick = 0;
    std::optional<PickHit> hit;
};

std::string picker_report_json(
    const PickResult& result, bool enabled, std::string_view game, std::string_view class_name = {}
);

// Debug-only CPU mesh ray cast. Static model BVHs are built once on first use.
// Queries use the exact interpolated instance transforms and skinning palette.
class ObjectPicker {
public:
    void clear();
    std::optional<PickHit> cast(
        const LevelData& level,
        std::span<const Instance> live,
        std::span<const Mat4> palette,
        const renderer::Camera& camera,
        unsigned layers = 0x1e,
        std::array<float, 2> ndc = {0, 0}
    );
    bool refresh(
        const LevelData& level,
        std::span<const Instance> live,
        std::span<const Mat4> palette,
        const renderer::Camera& camera,
        PickHit& hit
    );

private:
    struct Bounds {
        Vec3 low{}, high{};
    };

    struct Triangle {
        std::array<unsigned, 3> vertices{};
        unsigned primitive = 0, number = 0;
    };

    struct Node {
        Bounds bounds;
        unsigned first = 0, count = 0, left = 0, right = 0;
    };

    struct Mesh {
        std::vector<Triangle> triangles;
        std::vector<Node> nodes;
    };

    std::vector<Mesh> m_meshes;
    void load(const LevelData& level);
    Vec3 posed_vertex(
        const LevelData& level,
        const Instance& instance,
        std::span<const Mat4> palette,
        unsigned vertex
    ) const;
    unsigned build(Mesh& mesh, const LevelData& level, unsigned begin, unsigned end);
};
}  // namespace openrac::scene
