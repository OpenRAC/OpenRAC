// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/tie_render.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/tie_renderer.h"

#include <algorithm>
#include <cstddef>

#include "renderer/world/gl_world.h"
#include "world_shaders.h"

namespace openrac::renderer::world {

using namespace openrac::renderer::world::gl;

namespace {

struct GpuVertex {
    float position[3];
    float delta[3];
    float uv[2];
    std::uint32_t info;
    std::uint32_t morph;
};

constexpr std::array<GsPass, 3> kListPass{
    GsPass::Opaque,
    GsPass::OpaqueTested,
    GsPass::ColorOnlyLowAlpha,
};

std::uint8_t draw_bits(const std::vector<GsDraw>& draws) {
    std::uint8_t bits = 0;
    for (const GsDraw& d : draws) {
        for (std::size_t i = 0; i < kListPass.size(); ++i) {
            if (d.pass == kListPass[i]) {
                bits = static_cast<std::uint8_t>(bits | (1u << i));
            }
        }
    }
    return bits;
}

std::uint32_t pack_normal_xy(const std::array<std::int16_t, 3>& n) {
    return static_cast<std::uint16_t>(n[0]) | (std::uint32_t{static_cast<std::uint16_t>(n[1])} << 16);
}

}  // namespace

bool TieRenderer::init(RenderState& state, std::string& error) {
    (void)state;
    if (!context().init(error)) {
        return false;
    }
    return build_world_shader(
        m_shader,
        "tie",
        shaders::tie_vert,
        shaders::tie_frag,
        {shaders::world_common_glsl, shaders::world_lights_glsl, shaders::display_blend_glsl},
        error
    );
}

bool TieRenderer::load(const TieDrawData& data, std::string& error) {
    unload();
    m_constants = data.constants;
    m_instances = data.instances;
    m_textures = data.textures;

    static constexpr std::array<Mesh::Attribute, 5> kAttributes{{
        {0, 3, GL_FLOAT, false, offsetof(GpuVertex, position)},
        {1, 3, GL_FLOAT, false, offsetof(GpuVertex, delta)},
        {2, 2, GL_FLOAT, false, offsetof(GpuVertex, uv)},
        {3, 1, GL_UNSIGNED_INT, true, offsetof(GpuVertex, info)},
        {4, 1, GL_UNSIGNED_INT, true, offsetof(GpuVertex, morph)},
    }};
    for (const TieClass& c : data.classes) {
        Class cls;
        cls.distances = c.lod_distances;
        for (const TiePart& p : c.parts) {
            std::vector<GpuVertex> vertices;
            vertices.reserve(p.vertices.size());
            for (const TieVertex& v : p.vertices) {
                const auto k12 = std::uint32_t{static_cast<std::uint16_t>(v.lod_k)} & 0xfffu;
                vertices.push_back({
                    {v.position[0], v.position[1], v.position[2]},
                    {v.delta[0], v.delta[1], v.delta[2]},
                    {v.s, v.t},
                    (v.slot & 63u) | (k12 << 20),
                    (v.morph_slot1 & 63u) | ((v.morph_slot2 & 63u) << 6)
                        | (std::uint32_t{v.fat} << 12) | ((v.lod & 3u) << 13),
                });
            }
            Part part;
            part.mesh.create(bytes_of(vertices), sizeof(GpuVertex), kAttributes, p.indices);
            part.texture = p.texture;
            part.sampler = p.sampler;
            part.slots = p.slots_used;
            cls.parts.push_back(std::move(part));
        }
        m_classes.push_back(std::move(cls));
    }

    Words records;
    m_part_draws.assign(m_instances.size(), {});
    for (std::size_t ii = 0; ii < m_instances.size(); ++ii) {
        const TieInstance& inst = m_instances[ii];
        records.mat(inst.matrix)
            .f({inst.sphere[0], inst.sphere[1], inst.sphere[2], inst.draw_distance})
            .f({inst.sphere[3], 0.0f, 0.0f, 0.0f});
        for (const std::uint32_t c : inst.colours) {
            records.u(c);
        }
        for (const auto& n : inst.normals) {
            records.u({pack_normal_xy(n), static_cast<std::uint16_t>(n[2])});
        }
        if (inst.tie_class >= m_classes.size()) {
            continue;
        }
        // Af of every slot a part reads (a fat vertex's colours blend two more).
        const Class& cls = m_classes[inst.tie_class];
        for (const Part& part : cls.parts) {
            std::vector<std::uint8_t> alphas;
            for (std::size_t s = 0; s < 64; ++s) {
                if ((part.slots >> s) & 1u) {
                    alphas.push_back(static_cast<std::uint8_t>(inst.colours[s] >> 24));
                }
            }
            const AlphaRange texel =
                part.texture < m_textures.size() ? m_textures[part.texture].alpha : AlphaRange::opaque();
            m_part_draws[ii].push_back(draw_bits(draws(kArefWorld, texel, AlphaRange::of(alphas))));
        }
    }
    m_instance_records.upload(records.data(), RecordBuffer::Format::Rgba32ui, false);
    m_records.assign(m_instances.size() * 4, 0);
    (void)error;
    return true;
}

void TieRenderer::unload() {
    for (Class& c : m_classes) {
        for (Part& p : c.parts) {
            p.mesh.release();
        }
    }
    m_classes.clear();
    m_instances.clear();
    m_textures.clear();
    m_part_draws.clear();
    m_records.clear();
}

void TieRenderer::render(const FrameInput& input, RenderState& state) {
    if (m_classes.empty() || m_instances.empty()) {
        return;
    }
    WorldContext& ctx = context();
    ctx.prepare(input, state.target);
    const WorldFrame& frame = ctx.frame();
    const WorldCamera& cam = ctx.camera();

    for (Class& c : m_classes) {
        for (Part& p : c.parts) {
            for (auto& list : p.lists) {
                list.clear();
            }
        }
    }
    std::vector<std::uint32_t> words(m_instances.size() * 8, 0);
    for (std::size_t ii = 0; ii < m_instances.size(); ++ii) {
        const TieInstance& inst = m_instances[ii];
        std::array<std::uint32_t, 4> rec{kTieCulled, 0, 0, 0};
        if (inst.tie_class < m_classes.size() && inst.occlusion.visible(frame.occlusion)) {
            // Instance +0x04 is an s32, converted at load; TieProc caps it.
            const float distance = std::min(inst.draw_distance, m_constants.tie_draw_cap);
            const Vec3 c = cam.to_camera({inst.sphere[0], inst.sphere[1], inst.sphere[2]});
            rec = tie_record(
                c,
                inst.sphere[3],
                distance,
                m_classes[inst.tie_class].distances,
                frame.fog,
                cam.tan_x,
                cam.tan_y
            );
        }
        std::copy(rec.begin(), rec.end(), m_records.begin() + static_cast<std::ptrdiff_t>(ii * 4));
        std::copy(rec.begin(), rec.end(), words.begin() + static_cast<std::ptrdiff_t>(ii * 8));
        words[ii * 8 + 4] = ii < frame.tie_lights.size() ? frame.tie_lights[ii] : kNoLights;
        if ((rec[0] & 0xffu) == kTieCulled) {
            continue;
        }
        Class& cls = m_classes[inst.tie_class];
        for (std::size_t pi = 0; pi < cls.parts.size(); ++pi) {
            const std::uint8_t bits = m_part_draws[ii][pi];
            for (std::size_t l = 0; l < 3; ++l) {
                if ((bits >> l) & 1u) {
                    cls.parts[pi].lists[l].push_back(static_cast<std::uint32_t>(ii));
                }
            }
        }
    }
    m_frame.upload(words, RecordBuffer::Format::Rgba32ui, true);

    m_shader.use();
    m_instance_records.bind(1);
    m_frame.bind(2);
    // The GS does not cull, and instance matrices can mirror.
    glDisable(GL_CULL_FACE);
    const int max_level_uniform = m_shader.uniform("u_max_level");
    for (Class& cls : m_classes) {
        for (Part& p : cls.parts) {
            if (p.lists[0].empty() && p.lists[1].empty() && p.lists[2].empty()) {
                continue;
            }
            const WorldTexture empty;
            const WorldTexture& t = p.texture < m_textures.size() ? m_textures[p.texture] : empty;
            ctx.bind_texture(state.textures, t, p.sampler, 0);
            glUniform1f(max_level_uniform, static_cast<float>(t.mip_levels.size()));
            for (std::size_t l = 0; l < 3; ++l) {
                if (p.lists[l].empty()) {
                    continue;
                }
                p.mesh.set_instance_ids(5, p.lists[l]);
                p.mesh.bind();
                set_gs_draw(m_shader, {kListPass[l], kArefWorld});
                p.mesh.draw_instanced(static_cast<int>(p.lists[l].size()));
                count_draw(state, p.mesh.index_count() * p.lists[l].size());
            }
        }
    }
    reset_state();
}

void TieRenderer::release() {
    unload();
    m_instance_records.release();
    m_frame.release();
    m_shader.release();
    context().release();
}

}  // namespace openrac::renderer::world
