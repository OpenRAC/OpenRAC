// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/effects.h"

#include <cstddef>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/colour_graph.h"
#include "renderer/direct.h"
#include "renderer/gl.h"
#include "renderer/gs.h"
#include "renderer/render_opt.h"
#include "renderer/surface_chain.h"
#include "renderer_shaders.h"

namespace openrac::renderer {

using namespace gl;

bool liquid_strip(std::uint64_t alpha, std::uint64_t test) {
    // A TEST whose alpha test never passes and keeps only the colour writes no Z: a layer over
    // the picture, not a surface (Oltanis's ice, the first of its passes, drawn from callback
    // list 1 after the mobys).
    const gs::Test t = gs::Test::decode(test);
    const bool writes_z =
        !(t.ate && t.atst == gs::AlphaTest::Never
          && (t.afail == gs::AlphaFail::FbOnly || t.afail == gs::AlphaFail::RgbOnly));
    return (alpha & 0xFF) == 0x64 && ((alpha >> 32) & 0xFF) > 0x60 && writes_z;
}

bool EffectRenderer::draws(const EffectStrip& strip) const {
    switch (m_part) {
        case EffectPart::Liquids:
            return liquid_strip(strip.alpha, strip.test);
        case EffectPart::Rest:
            return !liquid_strip(strip.alpha, strip.test);
        case EffectPart::All:
            break;
    }
    return true;
}

bool EffectRenderer::init(RenderState& state, std::string& error) {
    (void)state;
    m_quad_shader.release();
    m_test_shader.release();
    m_test_plain = false;
    m_active_shader = nullptr;
    std::string fragment(shaders::direct_frag);
    fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_UPSCALED_TEXTURE_COLOURS 1\n");
    const std::string tested_fragment = fragment;  // the alpha test kept (m_test_shader)
    // Effect packets skip HUD distance fields and alpha tests; authored strip fog remains dynamic.
    // Keep texture functions/tcc dynamic: they can differ between effect strips.
    if (render_opt::enabled(render_opt::Opt::ShaderVariants)
        && render_opt::enabled(render_opt::Opt::EffectStaticControls)) {
        fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_EFFECT_FIXED 1\n");
        fragment = shader_constants(
            std::move(fragment), {{"textured", 1}, {"distance_field", 0}, {"alpha_test", 0}}
        );
    }
    const std::string original_fragment = fragment;
    std::string vertex_source(shaders::effect_vert);
    if (m_material) {
        if (!m_material->init(error)) {
            log::warn("Optional effect material unavailable: {}", error);
            m_material->release();
            m_material.reset();
            error.clear();
        } else {
            const auto extension = m_material->vertex_source();
            if (!extension.empty()) {
                vertex_source.insert(vertex_source.find("void main()"), extension);
                vertex_source
                    .insert(vertex_source.find('\n') + 1, "#define OPENRAC_EFFECT_POSITION 1\n");
            }
            fragment.insert(fragment.find("void main()"), m_material->fragment_source());
            fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_EFFECT_MATERIAL 1\n");
        }
    }
    if (!m_shader.build("effect", vertex_source, fragment, error)) {
        if (!m_material) {
            return false;
        }
        log::warn("Optional effect shader unavailable: {}", error);
        m_material->release();
        m_material.reset();
        if (!m_shader.build("effect", shaders::effect_vert, original_fragment, error)) {
            return false;
        }
        error.clear();
    }
    if (m_material) {
        const auto plain = m_material->quad_fragment_source();
        if (plain
            && (*plain != m_material->fragment_source() || !m_material->vertex_source().empty())) {
            std::string quad_fragment = original_fragment;
            quad_fragment.insert(quad_fragment.find("void main()"), *plain);
            quad_fragment
                .insert(quad_fragment.find('\n') + 1, "#define OPENRAC_EFFECT_MATERIAL 1\n");
            if (!m_quad_shader.build("effect quads", shaders::effect_vert, quad_fragment, error)) {
                log::warn("Original effect quad shader retained: {}", error);
                error.clear();
            }
        }
    }
    // The quads with an alpha test from the list's TEST: as the quads are drawn otherwise (the
    // material's plain quad source, else its full one), with the test.
    {
        std::string tested = tested_fragment;
        std::string tested_vertex(shaders::effect_vert);
        if (m_material) {
            const auto plain = m_material->quad_fragment_source();
            m_test_plain = plain && m_quad_shader.id();
            if (!m_test_plain) {
                const auto extension = m_material->vertex_source();
                if (!extension.empty()) {
                    tested_vertex.insert(tested_vertex.find("void main()"), extension);
                    tested_vertex.insert(
                        tested_vertex.find('\n') + 1, "#define OPENRAC_EFFECT_POSITION 1\n"
                    );
                }
            }
            tested.insert(
                tested.find("void main()"), m_test_plain ? *plain : m_material->fragment_source()
            );
            tested.insert(tested.find('\n') + 1, "#define OPENRAC_EFFECT_MATERIAL 1\n");
        }
        if (!m_test_shader.build("effect tested quads", tested_vertex, tested, error)) {
            log::warn("Effect quads keep no alpha test: {}", error);
            m_test_shader.release();
            m_test_plain = false;
            error.clear();
        }
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    constexpr auto stride = static_cast<GLsizei>(sizeof(Vertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, x))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(Vertex, s))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, reinterpret_cast<const void*>(offsetof(Vertex, r))
    );
    glBindVertexArray(0);
    return true;
}

void EffectRenderer::use_shader(Shader& shader, const Mat4& view_projection, const Mat4& view) {
    m_active_shader = &shader;
    m_colour_reads_ready = false;
    shader.use();
    m_view_projection = shader.uniform("view_projection");
    m_textured = shader.uniform("textured");
    m_tcc = shader.uniform("tcc");
    m_tfx = shader.uniform("tfx");
    m_tex_alpha_scale = shader.uniform("tex_alpha_scale");
    m_fog_enable = shader.uniform("fog_enable");
    m_view = shader.uniform("view");
    m_fog_colour = shader.uniform("fog_colour");
    m_fog_line = shader.uniform("fog_line");
    m_source_scale = shader.uniform("source_scale");
    m_source_fix = shader.uniform("source_fix");
    m_alpha_test = shader.uniform("alpha_test");
    m_alpha_ref = shader.uniform("alpha_ref");
    m_alpha_keep_failing = shader.uniform("alpha_keep_failing");
    glUniformMatrix4fv(m_view_projection, 1, GL_FALSE, view_projection.data());
    glUniformMatrix4fv(m_view, 1, GL_FALSE, view.data());
    glUniform1i(m_fog_enable, 0);
    glUniform1i(m_alpha_test, 0);
    glUniform1i(m_textured, 1);
    m_applied = AppliedState{};
}

void EffectRenderer::prepare_colour_reads() {
    // Per-material textures stay backend-owned; shared lighting and refraction inputs stay fixed
    // for this shader phase.
    if (m_colour_reads_ready) {
        return;
    }
    const auto reads = graphics_shader_reads("effect colour inputs");
    if (!reads) {
        throw std::runtime_error(reads.error().message);
    }
    m_colour_reads_ready = true;
}

// Collect material meshes before drawing effects so providers can snapshot the untouched scene.
void EffectRenderer::render(const FrameInput& input, RenderState& state) {
    const bool quads = m_part != EffectPart::Liquids && !input.effects.empty();
    bool any = quads;
    for (const EffectStrip& strip : input.strips) {
        any = any || draws(strip);
    }
    if (m_material) {
        const profiling::Zone prepare("Effects material prepare");
        const auto setup = run_surface_setup([this, &input] { m_material->prepare(input); });

        // Simulation may borrow stack-owned transfer callbacks and must complete synchronously.
        if (!setup) {
            throw std::runtime_error(setup.error().message);
        }
    }
    if (!any) {
        if (m_material) {
            m_material->finish_pass();
        }
        return;
    }
    m_texture_filter = input.texture_filter;
    const Mat4 view_projection = multiply(input.camera.projection, input.camera.view);
    use_shader(m_shader, view_projection, input.camera.view);
    glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
    glViewport(0, 0, state.target.width, state.target.height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);  // reversed depth
    glDepthMask(m_part == EffectPart::Liquids ? GL_TRUE : GL_FALSE);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glActiveTexture(GL_TEXTURE0);

    // Every strip and quad as triangles in one buffer, sent once; consecutive ones with the same GS
    // state are then one draw. A frame has hundreds to thousands of them (particles, glows), and
    // a draw each, with its own buffer and state, cost more than the rest of the frame on a phone.
    // The triangles keep their order, so what is blended over what does not change.
    std::optional<profiling::Zone> geometry;
    geometry.emplace("Effects geometry setup");
    m_vertices.clear();
    const auto vertex = [](const EffectVertex& e) {
        return Vertex{
            e.x,
            e.y,
            e.z,
            e.s,
            e.t,
            static_cast<std::uint8_t>(e.rgba & 0xFF),
            static_cast<std::uint8_t>((e.rgba >> 8) & 0xFF),
            static_cast<std::uint8_t>((e.rgba >> 16) & 0xFF),
            static_cast<std::uint8_t>(e.rgba >> 24)
        };
    };
    std::vector<GLsizei> strip_counts;
    std::vector<std::optional<EffectMesh>> material_meshes;
    for (const EffectStrip& strip : input.strips) {
        if (strip.count < 3
            || static_cast<std::size_t>(strip.first) + strip.count > input.strip_vertices.size()
            || !draws(strip)) {
            continue;
        }
        const auto start = m_vertices.size();
        auto mesh =
            m_material
                ? m_material->mesh(
                      strip, std::span(input.strip_vertices).subspan(strip.first, strip.count)
                  )
                : std::nullopt;
        // Resident material meshes never enter the core's expanded vertex upload.
        if (!mesh) {
            for (std::uint32_t k = 0; k + 2 < strip.count; ++k) {
                for (std::uint32_t c = 0; c < 3; ++c) {
                    m_vertices.push_back(vertex(input.strip_vertices[strip.first + k + c]));
                }
            }
        }
        material_meshes.push_back(mesh);
        strip_counts.push_back(static_cast<GLsizei>(m_vertices.size() - start));
    }
    if (m_material) {
        m_material->upload_meshes();
        m_shader.use();
    }
    const std::size_t quads_from = m_vertices.size();
    for (const EffectQuad& q : quads ? input.effects : std::span<const EffectQuad>{}) {
        // The strip's two triangles, 0-1-2 and 1-2-3 (the faces are not culled).
        for (const int k : {0, 1, 2, 1, 2, 3}) {
            m_vertices.push_back(
                {q.corner[k][0],
                 q.corner[k][1],
                 q.corner[k][2],
                 q.st[k][0],
                 q.st[k][1],
                 static_cast<std::uint8_t>(q.rgba[k] & 0xFF),
                 static_cast<std::uint8_t>((q.rgba[k] >> 8) & 0xFF),
                 static_cast<std::uint8_t>((q.rgba[k] >> 16) & 0xFF),
                 static_cast<std::uint8_t>(q.rgba[k] >> 24)}
            );
        }
    }
    // Optional material uploads may bind their own ARRAY_BUFFER. Restore the
    // core stream before writing the expanded strips/quads into it.
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_STREAM_DRAW
    );

    geometry.reset();

    auto prepared = std::make_shared<const PreparedEffects>(
        PreparedEffects{std::move(strip_counts), std::move(material_meshes), quads_from}
    );
    record_colour(input, state, prepared);
}

bool EffectRenderer::apply(
    std::uint64_t tex0_bits,
    std::uint64_t tex1_bits,
    std::uint64_t clamp_bits,
    std::uint64_t alpha_bits,
    RenderState& state,
    std::uint32_t tbp,
    std::uint32_t cbp
) {
    const profiling::Zone trace("Effects state setup");
    const gs::Texa texa{0, false, 0x80};
    gs::Tex0 tex0 = gs::Tex0::decode(tex0_bits);
    if (tbp != 0) {
        tex0.tbp0 = tbp;
        tex0.cbp = cbp;
    }
    TextureHandle texture;
    {
        const profiling::Zone resolve("Effects texture resolve");
        texture = state.textures.resolve(tex0, texa);
    }
    if (log::level() <= log::Level::Debug) {
        static std::set<std::uint64_t> seen;
        if (seen.insert(tex0_bits).second) {
            const Rgba8Image& image = state.textures.image(texture);
            log::debug(
                "effect texture {:#x}: block {:#x}, {}x{}, {}{}",
                tex0_bits,
                tex0.tbp0,
                image.width,
                image.height,
                state.textures.name(texture),
                texture == state.textures.placeholder() ? " (placeholder)" : ""
            );
        }
    }
    // The filter and the wrap are the bound texture's own state (glTexParameteri): set again for
    // each texture bound, not only when the bits change. A second texture with the same bits kept
    // the filter it had: the sea's second layer was magnified pixel by pixel (Pokitaru). The wrap
    // is set by the bind (TexturePool::set_wrap), the filter here.
    const bool rebound = texture != m_applied.texture || clamp_bits != m_applied.clamp_bits;
    if (rebound) {
        const auto wrap = gs::Clamp::decode(clamp_bits);
        state.textures.bind_upscaled_texture_colours(
            texture,
            *m_active_shader,
            m_texture_filter,
            wrap.wms == gs::Wrap::Repeat,
            wrap.wmt == gs::Wrap::Repeat
        );
    }
    m_applied.texture = texture;
    if (rebound || tex1_bits != m_applied.tex1_bits) {
        const gs::Tex1 tex1 = gs::Tex1::decode(tex1_bits);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            static_cast<GLint>(tex1.mmag ? GL_LINEAR : GL_NEAREST)
        );
        const bool linear_min = tex1.mmin == 1 || tex1.mmin == 4 || tex1.mmin == 5;
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            static_cast<GLint>(linear_min ? GL_LINEAR : GL_NEAREST)
        );
        m_applied.tex1_bits = tex1_bits;
        m_applied.clamp_bits = clamp_bits;
    }
    const int tcc_val = tex0.tcc ? 1 : 0;
    if (tcc_val != m_applied.tcc) {
        glUniform1i(m_tcc, tcc_val);
        m_applied.tcc = tcc_val;
    }
    const int tfx_val = static_cast<GLint>(tex0.tfx);
    if (tfx_val != m_applied.tfx) {
        glUniform1i(m_tfx, tfx_val);
        m_applied.tfx = tfx_val;
    }
    const float scale = state.textures.alpha_scale(texture) == AlphaScale::Full ? 0.5f : 1.0f;
    if (scale != m_applied.alpha_scale) {
        glUniform1f(m_tex_alpha_scale, scale);
        m_applied.alpha_scale = scale;
    }
    if (alpha_bits != m_applied.alpha_bits) {
        const GlBlend b = translate_blend(gs::Alpha::decode(alpha_bits));
        // Only source-over blends add the medium's in-scattering. Additive/subtractive
        // effects attenuate their own radiance without injecting fog on every overdraw.
        glUniform1i(
            m_active_shader->uniform("atmosphere_inscatter"), (alpha_bits & 0xCFu) == 0x44u ? 1 : 0
        );
        glEnable(GL_BLEND);
        glBlendEquation(b.equation);
        glBlendFuncSeparate(b.src, b.dst, GL_ONE, GL_ZERO);
        glBlendColor(0.0f, 0.0f, 0.0f, b.constant);
        glUniform1i(m_source_scale, static_cast<GLint>(b.source_scale));
        glUniform1f(m_source_fix, b.fix);
        m_applied.alpha_bits = alpha_bits;
    }
    return texture != state.textures.placeholder();
}

void EffectRenderer::release() {
    if (m_material) {
        m_material->release();
    }
    m_shader.release();
    m_quad_shader.release();
    m_active_shader = nullptr;
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

}  // namespace openrac::renderer
