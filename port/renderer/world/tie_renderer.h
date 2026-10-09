// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tie_render.rs,
// tie_lod.rs and tie_light.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The tie renderer (instanced scenery), replacing the placeholder of
// renderer/subsystems.h: DrawTies and TieProc (PAL func_00236BE0,
// func_00236F00), VU1 programs 13507 and 224979.
//
// Per class, one mesh per (texture, wrap state) holding the triangles of all
// three LODs (TieDrawData). Each frame the CPU replays TieProc for every
// instance (tie_lod.h) into a record buffer: the LOD, the morph factor and
// colour weights, the instance's fog. Instances it draws are drawn with one
// instanced draw per part and GS state; the vertex shader drops the vertices
// of the other LODs, morphs the fat vertices and blends their colours with
// the console's float arithmetic. The slot colours are the baked LightTies
// results; the point lights are added in the shader.

#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "renderer/world/gpu.h"
#include "renderer/world/tie_lod.h"
#include "renderer/world/world_renderer.h"

namespace openrac::renderer::world {

class TieRenderer final : public WorldRenderer {
public:
    explicit TieRenderer(std::shared_ptr<WorldContext> context)
        : WorldRenderer("tie", Bucket::Ties, std::move(context)) {}

    bool init(RenderState& state, std::string& error) override;
    void render(const FrameInput& input, RenderState& state) override;
    void release() override;

    bool load(const TieDrawData& data, std::string& error);
    void unload();

    // This frame's TieProc record per instance (tie_lod.h tie_record).
    std::span<const std::uint32_t> records() const { return m_records; }

private:
    struct Part {
        Mesh mesh;
        std::uint32_t texture = 0;
        SamplerState sampler;
        std::uint64_t slots = 0;
        // This frame's instances per draw: Opaque, OpaqueTested, ColorOnlyLowAlpha.
        std::array<std::vector<std::uint32_t>, 3> lists;
    };

    struct Class {
        std::array<float, 3> distances{};
        std::vector<Part> parts;
    };

    Shader m_shader;
    WorldConstants m_constants;
    std::vector<Class> m_classes;
    std::vector<TieInstance> m_instances;
    std::vector<WorldTexture> m_textures;
    // Per instance and part of its class: the draws it needs (bit 0 Opaque,
    // 1 OpaqueTested, 2 ColorOnlyLowAlpha), from its slot alphas.
    std::vector<std::vector<std::uint8_t>> m_part_draws;
    RecordBuffer m_instance_records;
    RecordBuffer m_frame;
    std::vector<std::uint32_t> m_records;
};

}  // namespace openrac::renderer::world
