// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/effects.h"

#include <algorithm>
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
namespace {

// The GL texture of the off-screen target the 2D path drew at TEX0's block (g_offscreen_textures),
// or 0.
unsigned offscreen_texture(std::uint64_t tex0_bits, std::uint32_t tbp) {
    if (!g_quads_sample_targets) {
        return 0;
    }
    const std::uint32_t block = tbp != 0 ? tbp : gs::Tex0::decode(tex0_bits).tbp0;
    for (const OffscreenTexture& o : g_offscreen_textures) {
        if (o.block == block) {
            return o.texture;
        }
    }
    return 0;
}

}  // namespace

using namespace gl;

bool test_writes_z(std::uint64_t test) {
    // A TEST whose alpha test never passes and keeps only the colour writes no Z: a layer over
    // the picture, not a surface (Oltanis's ice, the first of its passes, drawn from callback
    // list 1 after the mobys).
    const gs::Test t = gs::Test::decode(test);
    return !(
        t.ate && t.atst == gs::AlphaTest::Never
        && (t.afail == gs::AlphaFail::FbOnly || t.afail == gs::AlphaFail::RgbOnly)
    );
}

bool liquid_strip(std::uint64_t alpha, std::uint64_t test) {
    return (alpha & 0xFF) == 0x64 && ((alpha >> 32) & 0xFF) > 0x60 && test_writes_z(test);
}

bool before_mobys_writes_z(const EffectStrip& strip) {
    if (strip.z_masked || !test_writes_z(strip.test)) {
        return false;
    }
    if (liquid_strip(strip.alpha, strip.test) || strip.z_written) {
        return true;
    }
    // An alpha test that keeps some pixels: a surface drawn where its texture is (Dobbo's water,
    // GEQUAL 0x10). A layer with no test and no ZBUF of its own is a glow over the surface under
    // it, which writes the depth the surface has already.
    const gs::Test t = gs::Test::decode(strip.test);
    return t.ate && t.atst != gs::AlphaTest::Never && t.atst != gs::AlphaTest::Always;
}

bool liquid_pass_strip(const EffectStrip& strip) {
    // A strip sent before the world's mobys is drawn where the game drew it, with the liquids:
    // Dobbo's water (ALPHA 0x44, TEST 0x5310B, from the draw callback list after the ties), drawn
    // after the mobys, covered the Dynamo's platforms over it.
    return strip.before_mobys || liquid_strip(strip.alpha, strip.test);
}

namespace {

// A draw can establish early depth only if every covered sample writes both colour and depth.
bool opaque_effect(std::uint64_t alpha, std::uint64_t test, bool full_vertex_alpha) {
    const auto t = gs::Test::decode(test);
    if (!t.zte || t.ztst != gs::DepthTest::Gequal || t.date
        || (t.ate && t.atst != gs::AlphaTest::Always
            && !(full_vertex_alpha && t.atst == gs::AlphaTest::Gequal && t.aref <= 0x80))) {
        return false;
    }
    const auto b = gs::Alpha::decode(alpha);
    return (b.a == b.b && b.d == 0)
           || (b.a == 0 && b.b == 1 && b.d == 1
               && ((b.c == 2 && b.fix == 0x80) || (b.c == 0 && full_vertex_alpha)));
}

// A draw can move earlier only when frame transfers leave its sampled pages unchanged.
bool uses_frame_uploads(const gs::Tex0& texture, const FrameInput& input) {
    // Content keys refer to RGBA textures placed before rendering, outside GS memory.
    if (texture.tbp0 > 0x3FFF) {
        return false;
    }
    return std::ranges::any_of(input.effect_uploads, [&](const ImageUpload& upload) {
        const auto end = upload.end_block();
        const auto covers = [&](std::uint32_t block) {
            return upload.dbp <= block && block < end;
        };
        return covers(texture.tbp0) || (palette_entries(texture.psm) != 0 && covers(texture.cbp));
    });
}

}  // namespace

bool opaque_effect_strip(const EffectStrip& strip, std::span<const EffectVertex> vertices) {
    if (strip.surface != 0 || strip.z_masked || strip.count < 3 || strip.first > vertices.size()
        || strip.count > vertices.size() - strip.first
        || (!strip.z_written && !liquid_strip(strip.alpha, strip.test))) {
        return false;
    }
    const auto alpha = strip.blend_disabled ? 0 : strip.alpha;
    if (opaque_effect(alpha, strip.test, false)) {
        return true;
    }
    const bool full_vertex_alpha = !gs::Tex0::decode(strip.tex0).tcc
                                   && std::ranges::all_of(
                                       vertices.subspan(strip.first, strip.count),
                                       [](const EffectVertex& v) { return (v.rgba >> 24) == 0x80; }
                                   );
    return full_vertex_alpha && opaque_effect(alpha, strip.test, true);
}

bool opaque_effect_quad(const EffectQuad& quad) {
    const bool full_vertex_alpha = !gs::Tex0::decode(quad.tex0).tcc
                                   && std::ranges::all_of(quad.rgba, [](std::uint32_t colour) {
                                          return (colour >> 24) == 0x80;
                                      });
    return quad.registers && !gs::Zbuf::decode(quad.zbuf).zmsk
           && opaque_effect(quad.alpha, quad.test, full_vertex_alpha);
}

bool EffectRenderer::draws(const EffectStrip& strip, const FrameInput& input) const {
    const bool uploads = strip.uploads >= 0 && strip.upload_count > 0;
    if (input.opaque_effects && (m_split_opaque || m_part == EffectPart::Opaque)
        && opaque_effect_strip(strip, input.strip_vertices) && !uploads
        && !uses_frame_uploads(gs::Tex0::decode(strip.tex0), input)) {
        // Moving an upload changes earlier consumers too. Keep all transfers and their
        // dependent draws in the original sequence, including interior blocks and palettes.
        return m_part == EffectPart::Opaque;
    }
    const bool liquid = liquid_pass_strip(strip);
    switch (m_part) {
        case EffectPart::Opaque:
            return false;
        case EffectPart::Liquids:
            return liquid;
        case EffectPart::Rest:
            return !liquid;
        case EffectPart::All:
            break;
    }
    return true;
}

bool EffectRenderer::draws(const EffectQuad& quad, const FrameInput& input) const {
    if (input.opaque_effects && (m_split_opaque || m_part == EffectPart::Opaque)
        && opaque_effect_quad(quad) && offscreen_texture(quad.tex0, quad.tbp) == 0) {
        // Content keys are placed before rendering. Raw GS addresses must stay with the original
        // upload sequence if this frame can replace their texture or palette.
        auto tex = gs::Tex0::decode(quad.tex0);
        const auto pixels = quad.tbp != 0 ? quad.tbp : tex.tbp0;
        if (quad.tbp != 0) {
            tex.tbp0 = quad.tbp;
            tex.cbp = quad.cbp;
        }
        if (pixels > 0x3FFF || (quad.uploads < 0 && !uses_frame_uploads(tex, input))) {
            return m_part == EffectPart::Opaque;
        }
    }
    return m_part == EffectPart::All || m_part == EffectPart::Rest;
}

bool EffectRenderer::init(RenderState& state, std::string& error) {
    (void)state;
    m_quad_shader.release();
    m_test_shader.release();
    m_depth_shader.release();
    if (m_part == EffectPart::Opaque
        && !m_depth_shader.build(
            "opaque effects depth", shaders::effect_vert, shaders::effect_depth_frag, error
        )) {
        return false;
    }
    m_test_plain = false;
    m_active_shader = nullptr;
    std::string fragment(shaders::direct_frag);
    fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_UPSCALED_TEXTURE_COLOURS 1\n");
    const std::string tested_fragment = fragment;  // the alpha test kept (m_test_shader)
    // Effect packets skip HUD distance fields and alpha tests; authored strip fog remains dynamic.
    // Keep texture functions/tcc dynamic: they can differ between effect strips.
    if (render_opt::enabled(render_opt::Opt::EffectStaticControls)) {
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
    bind_vertex_stream(m_vao, 0);
    if (gl::is_vulkan()) {
        glGenVertexArrays(1, &m_quad_vao);
        glGenBuffers(1, &m_quad_indices);
    }
    glBindVertexArray(0);
    return true;
}

void EffectRenderer::bind_vertex_stream(unsigned vao, std::size_t first) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    constexpr auto stride = static_cast<GLsizei>(sizeof(Vertex));
    const auto offset = first * sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offset + offsetof(Vertex, x))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offset + offsetof(Vertex, s))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<const void*>(offset + offsetof(Vertex, r))
    );
}

void EffectRenderer::reserve_quad_indices(std::size_t quads) {
    if (quads <= m_quad_capacity) {
        return;
    }
    const auto capacity = std::max(quads, m_quad_capacity * 2);
    std::vector<std::uint32_t> indices;
    indices.reserve(capacity * 6);
    for (std::size_t quad = 0; quad < capacity; ++quad) {
        const auto first = static_cast<std::uint32_t>(quad * 4);
        for (unsigned corner : {0u, 1u, 2u, 1u, 2u, 3u}) {
            indices.push_back(first + corner);
        }
    }
    glBindVertexArray(m_quad_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quad_indices);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(indices[0])),
        indices.data(),
        GL_STATIC_DRAW
    );
    m_quad_capacity = capacity;
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
    const bool quads = m_part == EffectPart::Opaque
                           ? std::ranges::any_of(
                                 input.effects, [&](const EffectQuad& q) { return draws(q, input); }
                             )
                           : m_part != EffectPart::Liquids && !input.effects.empty();
    const bool any = quads || std::ranges::any_of(input.strips, [&](const EffectStrip& strip) {
                         return draws(strip, input);
                     });
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
            || !draws(strip, input)) {
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
    if (quads) {
        stats::gauge(
            "effect quad vertex bytes",
            double(input.effects.size() * (m_quad_indices ? 4 : 6) * sizeof(Vertex))
        );
    }
    constexpr std::array corners{0, 1, 2, 1, 2, 3};
    constexpr std::array indexed_corners{0, 1, 2, 3};
    const std::span<const int> order =
        m_quad_indices ? std::span<const int>(indexed_corners) : std::span<const int>(corners);
    for (const EffectQuad& q : quads ? input.effects : std::span<const EffectQuad>{}) {
        if (!draws(q, input)) {
            continue;
        }
        // A quad that samples an off-screen target the 2D path drew (g_offscreen_textures): its
        // coordinates over the TEX0's size become the target texture's, which spans
        // kOffscreenPixels chip pixels with its first row on top.
        float s_scale = 1.0f, t_scale = 1.0f, t_from = 0.0f;
        if (offscreen_texture(q.tex0, q.tbp) != 0) {
            const gs::Tex0 tex0 = gs::Tex0::decode(q.tex0);
            s_scale = static_cast<float>(tex0.width()) / static_cast<float>(kOffscreenPixels);
            t_scale = -static_cast<float>(tex0.height()) / static_cast<float>(kOffscreenPixels);
            t_from = 1.0f;
        }
        // The strip's two triangles, 0-1-2 and 1-2-3 (the faces are not culled).
        for (const int k : order) {
            m_vertices.push_back(
                {q.corner[k][0],
                 q.corner[k][1],
                 q.corner[k][2],
                 q.st[k][0] * s_scale,
                 t_from + q.st[k][1] * t_scale,
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
    if (quads && m_quad_indices) {
        reserve_quad_indices(input.effects.size());
    }

    geometry.reset();

    auto prepared = std::make_shared<const PreparedEffects>(
        PreparedEffects{std::move(strip_counts), std::move(material_meshes), quads_from, quads}
    );
    record_colour(input, state, prepared);
}

void EffectRenderer::render_depth(const FrameInput& input, RenderState& state) {
    if (m_part != EffectPart::Opaque || !input.opaque_effects) {
        return;
    }
    m_vertices.clear();
    for (const EffectStrip& strip : input.strips) {
        if (!draws(strip, input)) {
            continue;
        }
        for (std::uint32_t k = 0; k + 2 < strip.count; ++k) {
            for (const auto& v : input.strip_vertices.subspan(strip.first + k, 3)) {
                m_vertices.push_back({v.x, v.y, v.z, 0, 0, 0, 0, 0, 0});
            }
        }
    }
    const auto quad_start = static_cast<GLint>(m_vertices.size());
    for (const EffectQuad& q : input.effects) {
        if (!draws(q, input)) {
            continue;
        }
        auto tex = gs::Tex0::decode(q.tex0);
        if (q.tbp != 0) {
            tex.tbp0 = q.tbp;
            tex.cbp = q.cbp;
        }
        // Colour skips unresolved quads. A page transferred during colour is only safe to
        // prepass once the texture is resident.
        if (state.textures.resolve(tex, {0, false, 0x80}) == state.textures.placeholder()) {
            continue;
        }
        for (int corner : {0, 1, 2, 1, 2, 3}) {
            const auto& v = q.corner[corner];
            m_vertices.push_back({v[0], v[1], v[2], 0, 0, 0, 0, 0, 0});
        }
    }
    if (m_vertices.empty()) {
        return;
    }
    const Mat4 view_projection = multiply(input.camera.projection, input.camera.view);
    m_depth_shader.use();
    glUniformMatrix4fv(
        m_depth_shader.uniform("view_projection"), 1, GL_FALSE, view_projection.data()
    );
    glUniformMatrix4fv(m_depth_shader.uniform("view"), 1, GL_FALSE, input.camera.view.data());
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_STREAM_DRAW
    );
    const auto draw = [&](GLint first, GLsizei count) {
        glDrawArrays(GL_TRIANGLES, first, count);
        ++state.stats.draw_calls;
        state.stats.triangles += count / 3;
    };
    if (quad_start != 0) {
        draw(0, quad_start);
    }
    if (quad_start < static_cast<GLint>(m_vertices.size())) {
        const bool clamp = !input.depth_clamp && gl::depth_clamp_available();
        if (clamp) {
            glEnable(GL_DEPTH_CLAMP);
        }
        draw(quad_start, static_cast<GLsizei>(m_vertices.size()) - quad_start);
        if (clamp) {
            glDisable(GL_DEPTH_CLAMP);
        }
    }
    glBindVertexArray(0);
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
    // An off-screen target the 2D path drew, sampled from the texture it was drawn in.
    if (const unsigned target = offscreen_texture(tex0_bits, tbp); target != 0) {
        if (m_applied.texture != kOffscreenHandle || m_applied.offscreen != target
            || clamp_bits != m_applied.clamp_bits) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, target);
            glUniform1i(m_active_shader->uniform("upscaled_texture_colours_ready"), 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            m_applied.texture = kOffscreenHandle;
            m_applied.offscreen = target;
            m_applied.clamp_bits = clamp_bits;
            m_applied.tex1_bits = tex1_bits;
        }
        apply_colour_state(tex0, 0.5f, alpha_bits);
        return true;
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
    apply_colour_state(
        tex0, state.textures.alpha_scale(texture) == AlphaScale::Full ? 0.5f : 1.0f, alpha_bits
    );
    return texture != state.textures.placeholder();
}

void EffectRenderer::apply_colour_state(
    const gs::Tex0& tex0, float scale, std::uint64_t alpha_bits
) {
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
        if (m_part == EffectPart::Opaque) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
        }
        glBlendEquation(b.equation);
        glBlendFuncSeparate(b.src, b.dst, GL_ONE, GL_ZERO);
        glBlendColor(0.0f, 0.0f, 0.0f, b.constant);
        glUniform1i(m_source_scale, static_cast<GLint>(b.source_scale));
        glUniform1f(m_source_fix, b.fix);
        m_applied.alpha_bits = alpha_bits;
    }
}

void EffectRenderer::release() {
    if (m_material) {
        m_material->release();
    }
    m_shader.release();
    m_depth_shader.release();
    m_quad_shader.release();
    m_active_shader = nullptr;
    if (m_quad_indices) {
        glDeleteVertexArrays(1, &m_quad_vao);
        glDeleteBuffers(1, &m_quad_indices);
        m_quad_indices = m_quad_vao = 0;
        m_quad_capacity = 0;
    }
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
