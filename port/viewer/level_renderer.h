// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The viewer's renderers: a loaded level on the GPU, drawn through the port's
// renderer (renderer/renderer.h) as one BucketRenderer per layer, in the
// game's order: the sky shells, then terrain, ties, shrubs and mobys.
//
// Every mesh shares one vertex and one index buffer; the instances of a class
// are drawn with one instanced call per material, their matrices in an
// instance buffer. Live mobys are skinned in the vertex shader with the joint
// palettes the game's animation fields give (moby_pose.h), from a float
// texture each instance indexes. Textures go through the renderer's texture pool. The
// conventions are the game's: Z up, reversed depth (cleared to 0, GEQUAL).
// Lighting is a fixed sun and ambient, only so shapes read: the game's own
// lights (tie and shrub light indices, the level's directional lights) are
// not applied yet.

#pragma once

#include <array>
#include <string>
#include <vector>

#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "viewer/level.h"

namespace openrac::viewer {

class LevelScene {
public:
    // Upload a level; the GL context must be current.
    bool upload(const LevelData& level, renderer::TexturePool& textures, std::string& error);
    void release();

    void draw_layer(Layer layer, const renderer::FrameInput& input, renderer::RenderState& state);

    // Replaces a layer's instances (live mobys, each frame).
    void set_instances(Layer layer, const std::vector<Instance>& instances);

    // One joint matrix of a palette, as the vertex shader takes it: four columns (the images of
    // the axes, then the translation, in the mesh's units), column-major.
    using JointColumns = std::array<float, 16>;

    // Replaces the joint palettes the mobys' instances index (Instance::palette), each frame.
    void set_palette(const std::vector<JointColumns>& matrices);

    bool lighting = true;

private:
    struct Group {
        std::uint32_t model = 0;
        unsigned vao = 0;
        unsigned instances = 0;  // instance buffer
        int count = 0;
    };

    void draw_sky(const renderer::FrameInput& input, renderer::RenderState& state);
    void build_groups(Layer layer, const std::vector<Instance>& instances);

    std::vector<Model> m_models;
    std::vector<Material> m_materials;
    std::vector<renderer::TextureHandle> m_textures;  // by image
    renderer::TextureHandle m_white = 0;
    std::array<std::vector<Group>, kLayerCount> m_groups;
    unsigned m_vertices = 0;
    unsigned m_indices = 0;
    unsigned m_palette = 0;  // RGBA32F texture, four texels per matrix
    std::vector<JointColumns> m_palette_data;
    renderer::Shader m_mesh;
    renderer::Shader m_sky;
};

// One layer of a LevelScene, as a bucket of the renderer.
class LayerRenderer : public renderer::BucketRenderer {
public:
    LayerRenderer(LevelScene& scene, Layer layer);

    void render(const renderer::FrameInput& input, renderer::RenderState& state) override;

private:
    LevelScene& m_scene;
    Layer m_layer;
};

}  // namespace openrac::viewer
