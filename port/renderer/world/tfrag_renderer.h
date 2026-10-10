// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tfrag_render.rs,
// tfrag_lod.rs and tfrag_light.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The terrain (tfrag) renderer, replacing the placeholder of
// renderer/subsystems.h: DrawTfrag and TfragProc (PAL func_002346C0,
// func_002352C8), VU1 programs 55907 and 903379 (RENDERER.md section 2).
//
// Every triangle of all three strip lists is uploaded once per level
// (TfragDrawData). Each frame the CPU replays TfragProc for every tfrag
// (tfrag_lod.h: occlusion, culling, the draw mode) into a record buffer the
// vertex shader reads, with each tfrag's point-light list; the shader drops
// the lists the mode does not draw and replays the VU1 morph and collapse
// passes. The colours are the slots' baked LightTfrags results (computed when
// the level loads, by the asset side); the point lights are added in the
// shader. Each batch is drawn once (As = 0x80 everywhere) or twice (the
// alpha-test split, gs_pass.h).

#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "renderer/world/gpu.h"
#include "renderer/world/tfrag_lod.h"
#include "renderer/world/world_renderer.h"

namespace openrac::renderer::world {

class TfragRenderer final : public WorldRenderer {
public:
    explicit TfragRenderer(std::shared_ptr<WorldContext> context)
        : WorldRenderer("tfrag", Bucket::Terrain, std::move(context)) {}

    bool init(RenderState& state, std::string& error) override;
    void render(const FrameInput& input, RenderState& state) override;
    void release() override;

    // Upload a level's terrain (the GL context must be current).
    bool load(const TfragDrawData& data, std::string& error);
    void unload();

    // This frame's draw mode per tfrag (tfrag_lod.h).
    std::span<const std::uint32_t> modes() const { return m_modes; }

private:
    struct Batch {
        Mesh mesh;
        std::uint32_t texture = 0;
        SamplerState sampler;
        std::vector<GsDraw> draws;
    };

    Shader m_shader;
    float m_lod_base = 0.0f;
    WorldConstants m_constants;
    std::vector<TfragCull> m_tfrags;
    std::vector<WorldTexture> m_textures;
    std::vector<Batch> m_batches;
    RecordBuffer m_slots;
    RecordBuffer m_infos;
    RecordBuffer m_frame;
    std::vector<std::uint32_t> m_modes;
    std::vector<std::uint32_t> m_frame_words;
};

}  // namespace openrac::renderer::world
