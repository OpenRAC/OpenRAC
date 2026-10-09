// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tfrag_render.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/tfrag_renderer.h"

#include <array>

#include "renderer/world/gl_world.h"
#include "world_shaders.h"

namespace openrac::renderer::world {

using namespace openrac::renderer::world::gl;

namespace {

struct GpuVertex {
    std::uint32_t info;
    std::uint32_t ref;  // tfrag | list << 16 | TEX1 K (s12) << 18
};

constexpr std::uint32_t tier_code(TfragTier tier) {
    return static_cast<std::uint32_t>(tier);
}

}  // namespace

bool TfragRenderer::init(RenderState& state, std::string& error) {
    (void)state;
    if (!context().init(error)) {
        return false;
    }
    return build_world_shader(
        m_shader,
        "tfrag",
        shaders::tfrag_vert,
        shaders::tfrag_frag,
        {shaders::world_common_glsl, shaders::world_lights_glsl, shaders::display_blend_glsl},
        error
    );
}

bool TfragRenderer::load(const TfragDrawData& data, std::string& error) {
    unload();
    m_lod_base = data.lod_base;
    m_constants = data.constants;
    m_tfrags = data.tfrags;
    m_textures = data.textures;

    Words slots;
    for (const TfragSlot& s : data.slots) {
        const std::uint32_t normal =
            s.has_normal ? (s.azimuth | (std::uint32_t{s.elevation} << 8) | (1u << 16)) : 0u;
        slots.f({s.position[0], s.position[1], s.position[2]})
            .u(s.rgba)
            .u({s.parent1, s.parent2, tier_code(s.tier), normal});
    }
    m_slots.upload(slots.data(), RecordBuffer::Format::Rgba32ui, false);

    Words infos;
    for (const TfragVertexInfo& e : data.infos) {
        infos.f({e.s, e.t}).u({e.slot, tier_code(e.tier), e.parent1_info, e.parent2_slot, 0, 0});
    }
    m_infos.upload(infos.data(), RecordBuffer::Format::Rgba32ui, false);

    static constexpr std::array<Mesh::Attribute, 1> kAttributes{{{0, 2, GL_UNSIGNED_INT, true, 0}}};
    for (const TfragBatch& b : data.batches) {
        if (b.texture >= m_textures.size() && !m_textures.empty()) {
            error = "tfrag batch names a texture past the table";
            unload();
            return false;
        }
        std::vector<GpuVertex> vertices;
        vertices.reserve(b.vertices.size());
        for (const TfragVertex& v : b.vertices) {
            const auto k12 = static_cast<std::uint32_t>(static_cast<std::uint16_t>(v.lod_k)) & 0xfffu;
            vertices.push_back(
                {v.info, std::uint32_t{v.tfrag} | (std::uint32_t{v.list & 3u} << 16) | (k12 << 18)}
            );
        }
        Batch batch;
        batch.mesh.create(bytes_of(vertices), sizeof(GpuVertex), kAttributes, b.indices);
        batch.texture = b.texture;
        batch.sampler = b.sampler;
        const AlphaRange texel =
            b.texture < m_textures.size() ? m_textures[b.texture].alpha : AlphaRange::opaque();
        batch.draws = draws(kArefWorld, texel, b.vertex_alpha);
        m_batches.push_back(std::move(batch));
    }
    m_modes.assign(m_tfrags.size(), kTfragCulled);
    return true;
}

void TfragRenderer::unload() {
    for (Batch& b : m_batches) {
        b.mesh.release();
    }
    m_batches.clear();
    m_tfrags.clear();
    m_textures.clear();
    m_modes.clear();
}

void TfragRenderer::render(const FrameInput& input, RenderState& state) {
    if (m_batches.empty()) {
        return;
    }
    WorldContext& ctx = context();
    ctx.prepare(input, state.target);
    const WorldFrame& frame = ctx.frame();

    tfrag_modes(
        m_tfrags, m_lod_base, ctx.camera(), frame.occlusion, m_constants.tfrag_far_cull, m_modes
    );
    m_frame_words.assign(m_tfrags.size() * 4, 0);
    for (std::size_t i = 0; i < m_tfrags.size(); ++i) {
        m_frame_words[i * 4] = m_modes[i];
        m_frame_words[i * 4 + 1] = i < frame.tfrag_lights.size() ? frame.tfrag_lights[i] : kNoLights;
    }
    m_frame.upload(m_frame_words, RecordBuffer::Format::Rgba32ui, true);

    const TfragLodConstants k = tfrag_lod_constants(m_lod_base, frame.fog);
    m_shader.use();
    glUniform4f(m_shader.uniform("u_k666"), k.k666[0], k.k666[1], k.k666[2], k.k666[3]);
    glUniform4f(m_shader.uniform("u_k667"), k.k667[0], k.k667[1], k.k667[2], k.k667[3]);
    glUniform4f(m_shader.uniform("u_k668"), k.k668[0], k.k668[1], k.k668[2], k.k668[3]);
    glUniform4f(m_shader.uniform("u_k669"), k.k669[0], k.k669[1], k.k669[2], k.k669[3]);
    glUniform1f(m_shader.uniform("u_w_slope"), k.w_slope);
    m_slots.bind(1);
    m_infos.bind(2);
    m_frame.bind(3);
    // The GS draws both faces: the program emits every strip triangle.
    glDisable(GL_CULL_FACE);
    const int max_level_uniform = m_shader.uniform("u_max_level");
    for (const Batch& b : m_batches) {
        const WorldTexture empty;
        const WorldTexture& t = b.texture < m_textures.size() ? m_textures[b.texture] : empty;
        ctx.bind_texture(state.textures, t, b.sampler, 0);
        glUniform1f(max_level_uniform, static_cast<float>(t.mip_levels.size()));
        b.mesh.bind();
        for (const GsDraw& d : b.draws) {
            set_gs_draw(m_shader, d);
            b.mesh.draw();
            count_draw(state, b.mesh.index_count());
        }
    }
    reset_state();
}

void TfragRenderer::release() {
    unload();
    m_slots.release();
    m_infos.release();
    m_frame.release();
    m_shader.release();
    context().release();
}

}  // namespace openrac::renderer::world
