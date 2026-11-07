// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// EffectRenderer's fused strip/quad recorder, preserving shared batch state and material order.
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <source_location>
#include <stdexcept>

#include "common/stats.h"
#include "renderer/effects.h"
#include "renderer/gl.h"
#include "renderer/gs.h"
#include "renderer/render_opt.h"

namespace openrac::renderer {
using namespace gl;

namespace {

// Key only the sampler and blend controls that EffectRenderer::apply reads.
void normalize_quad_state(std::uint64_t& tex1, std::uint64_t& clamp, std::uint64_t& alpha) {
    const auto filter = gs::Tex1::decode(tex1);
    const bool linear_min = filter.mmin == 1 || filter.mmin == 4 || filter.mmin == 5;
    tex1 = (std::uint64_t(filter.mmag) << 5) | (std::uint64_t(linear_min) << 6);

    const auto wrap = gs::Clamp::decode(clamp);
    clamp = std::uint64_t(wrap.wms != gs::Wrap::Repeat)
            | (std::uint64_t(wrap.wmt != gs::Wrap::Repeat) << 2);

    // FIX only participates in the blend when C selects it.
    alpha &= gs::Alpha::decode(alpha).c == 2 ? 0xFF000000FFull : 0xFFull;
}

}  // namespace

// ---- Ordered effect draws ----

void EffectRenderer::draw_prepared(
    const FrameInput& input, RenderState& state, const PreparedEffects& prepared
) {
    const bool quads = m_part != EffectPart::Liquids && !input.effects.empty();
    const auto& strip_counts = prepared.strip_counts;
    const auto& material_meshes = prepared.meshes;
    const std::size_t quads_from = prepared.quads_from;
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

    static const bool& upload_reference = stats::probe(
        "OPENRAC_VK_EFFECT_UPLOAD_REUSE_OFF", "Reference: copy and hash every effect texture upload"
    );
    const bool reuse_uploads = gl::is_vulkan() && !upload_reference;
    std::uint64_t reused_uploads = 0, reused_bytes = 0;
    const auto upload = [&](const ImageUpload& image) {
        const profiling::Zone trace("Effects texture upload");

        // Vulkan may reuse uploads whose content already matches.
        if (reuse_uploads) {
            // Count only uploads that avoided a new transfer.
            if (state.textures.upload_cached(image)) {
                ++reused_uploads;
                reused_bytes += image.data.size();
            }
        } else {
            state.textures.upload(image);
        }
    };

    // The draw being gathered: its first vertex and count, and the GS state it was set up with.
    struct Batch {
        GLint first = 0;
        GLsizei count = 0;
    } batch;

    struct State {
        std::uint64_t tex0, tex1, clamp, alpha;
        std::uint32_t tbp, cbp, surface;
        EffectUvMotion motion{};
        bool fogged = false;
        std::array<float, 4> fog{};
        std::uint32_t fog_colour = 0;
        // A quad's TEST from the list (EffectQuad::registers; 0 none) and whether it writes Z.
        std::uint64_t test = 0;
        bool z_write = false;
        // A strip sent before the mobys that writes no Z (before_mobys_writes_z: its ZBUF masks
        // it, its TEST keeps only the colour, or its list gives no sign of a surface): colour only.
        bool z_masked = false;
        bool operator==(const State&) const = default;
    };

    // The GS fog of a strip whose program applies it (EffectStrip::fogged); none for the others.
    bool fog_on = false;
    const auto set_fog = [&](bool fogged, const std::array<float, 4>& line, std::uint32_t colour) {
        // Unfogged batches need no repeated fog uniforms.
        if (!fogged && !fog_on) {
            return;
        }

        fog_on = fogged;
        glUniform1i(m_fog_enable, fogged ? 1 : 0);
        glUniform4f(m_fog_line, line[0], line[1], line[2], line[3]);
        glUniform3f(
            m_fog_colour,
            static_cast<float>(colour & 0xFF),
            static_cast<float>((colour >> 8) & 0xFF),
            static_cast<float>((colour >> 16) & 0xFF)
        );
    };

    std::optional<State> set;
    bool quad_indices = false;
    const auto draw_batch = [&] {
        if (quad_indices) {
            const auto offset = static_cast<std::uintptr_t>(batch.first) * sizeof(std::uint32_t);
            glDrawElements(
                GL_TRIANGLES, batch.count, GL_UNSIGNED_INT, reinterpret_cast<const void*>(offset)
            );
        } else {
            glDrawArrays(GL_TRIANGLES, batch.first, batch.count);
        }
        state.stats.draw_calls++;
        state.stats.triangles += static_cast<std::uint64_t>(batch.count / 3);
    };
    const auto flush = [&] {
        // TEST and depth-writing batches may require separate alpha-pass and alpha-fail draws.
        if (batch.count > 0 && set && (set->test != 0 || set->z_write)) {
            // A quad drawn with the list's TEST and ZBUF, as the chip draws it: the pixels that
            // pass the alpha test with every write (Z unless ZMSK), then, as OpenGOAL does, those
            // that fail with what AFAIL still writes. Going Commando's sparks and flames: GEQUAL
            // 0x60, the failing pixels colour only.
            const profiling::Zone trace("Effects draw batch");
            const gs::Test t = gs::Test::decode(set->test);
            const auto call = [&](int test, bool keep_failing, bool colour, bool alpha, bool z) {
                glUniform1i(m_alpha_test, test);
                glUniform1f(m_alpha_ref, static_cast<float>(t.aref));
                glUniform1i(m_alpha_keep_failing, keep_failing ? 1 : 0);
                glColorMask(colour, colour, colour, alpha);
                glDepthMask(z ? GL_TRUE : GL_FALSE);
                draw_batch();
            };
            const bool fail_colour =
                t.afail == gs::AlphaFail::FbOnly || t.afail == gs::AlphaFail::RgbOnly;
            const bool fail_alpha = t.afail == gs::AlphaFail::FbOnly;
            const bool fail_z = t.afail == gs::AlphaFail::ZbOnly && set->z_write;

            // ATST NEVER needs no shader's alpha test: every pixel fails, and only what AFAIL keeps
            // is written. Going Commando's nanotech orb draws its glow and the crate's glass faces
            // so (TEST 0x53001: NEVER, RGB_ONLY, ZBUF without ZMSK): colour only, no Z. Taken as
            // passing, with Z, the glass faces hid the orb's sparks inside the crate.
            if (t.ate && t.atst == gs::AlphaTest::Never) {
                if (t.afail != gs::AlphaFail::Keep) {
                    call(0, false, fail_colour, fail_alpha, fail_z);
                }
            } else if (!t.ate || t.atst == gs::AlphaTest::Always
                       || m_active_shader != &m_test_shader) {
                // An untested shader writes every covered pixel once.
                call(0, false, true, true, set->z_write);
            } else {
                call(static_cast<int>(t.atst), false, true, true, set->z_write);

                // AFAIL preserves selected channels for pixels rejected by the alpha test.
                if (t.afail != gs::AlphaFail::Keep) {
                    call(static_cast<int>(t.atst), true, fail_colour, fail_alpha, fail_z);
                }
            }

            glUniform1i(m_alpha_test, 0);
            glUniform1i(m_alpha_keep_failing, 0);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_FALSE);
        } else if (batch.count > 0) {
            const profiling::Zone trace("Effects draw batch");
            draw_batch();
        }

        batch.count = 0;
    };
    // Adds `count` vertices from `first` to the draw, with this state; false when the state has
    // no texture in the pool (apply).
    bool drawable = true;
    const auto add = [&](const State& wanted, GLint first, GLsizei count, bool must_have_texture) {
        // State changes flush earlier geometry before installing the next material.
        if (!set || !(*set == wanted)) {
            flush();
            drawable = apply(
                wanted.tex0, wanted.tex1, wanted.clamp, wanted.alpha, state, wanted.tbp, wanted.cbp
            );

            // Material bindings precede the shared descriptor read declaration.
            if (m_material) {
                // Plain variants retain the original surface-zero material behavior.
                if (m_active_shader == &m_quad_shader
                    || (m_active_shader == &m_test_shader && m_test_plain)) {
                    m_material->bind_quads(*m_active_shader, input);
                } else {
                    m_material->bind(*m_active_shader, wanted.surface, input, wanted.motion);
                }
            }

            prepare_colour_reads();
            set_fog(wanted.fogged, wanted.fog, wanted.fog_colour);
            if (m_part == EffectPart::Liquids) {
                glDepthMask(wanted.z_masked ? GL_FALSE : GL_TRUE);
            }
            set = wanted;
        }

        // Quads without an uploaded texture must remain absent.
        if (must_have_texture && !drawable) {
            return;
        }

        // A stream gap cannot be included in the current contiguous draw.
        if (batch.count > 0 && batch.first + batch.count != first) {
            flush();
        }

        // The first accepted range establishes the next draw offset.
        if (batch.count == 0) {
            batch.first = first;
        }

        batch.count += count;
    };

    std::optional<stats::Scope> strip_timing;

    // Empty strip lists need no GPU timer.
    if (!strip_counts.empty()) {
        strip_timing.emplace("Effect strips", stats::kGpu, std::source_location::current());
    }

    GLint at = 0;
    unsigned plain_strips = 0, strip_switches = 0;
    std::size_t strip_index = 0;

    // Accepted strips retain authored draw and texture-upload order.
    for (const EffectStrip& strip : input.strips) {
        // Malformed or excluded strips have no prepared geometry entry.
        if (strip.count < 3
            || static_cast<std::size_t>(strip.first) + strip.count > input.strip_vertices.size()
            || !draws(strip)) {
            continue;
        }

        // An image put in the pool may replace what an earlier strip of the draw samples: that one
        // is drawn first, and the state is set again after.
        if (strip.uploads >= 0 && strip.upload_count > 0) {
            flush();
            set.reset();

            // Each strip may carry several texture uploads before its draw.
            for (int u = 0; u < strip.upload_count; ++u) {
                const std::size_t i = static_cast<std::size_t>(strip.uploads + u);

                // Malformed upload references cannot index beyond the captured frame.
                if (i < input.effect_uploads.size()) {
                    upload(input.effect_uploads[i]);
                }
            }
        }

        const auto mesh = material_meshes[strip_index];
        const auto count = strip_counts[strip_index++];
        const bool plain_strip = !mesh && strip.surface == 0 && !strip.uv_motion.enabled
                                 && m_quad_shader.id() && m_material->plain_strips_supported()
                                 && render_opt::enabled(render_opt::Opt::EffectStripShader);
        auto& wanted_shader = plain_strip ? m_quad_shader : m_shader;

        // Shader changes flush the current batch before switching uniforms.
        if (m_active_shader != &wanted_shader) {
            flush();
            use_shader(wanted_shader, view_projection, input.camera.view);
            set.reset();
            ++strip_switches;
        }

        plain_strips += plain_strip ? 1u : 0u;

        // Keep the original strip order and texture uploads around each material draw.
        if (mesh) {
            flush();
            set.reset();
            apply(strip.tex0, strip.tex1, strip.clamp, strip.alpha, state);
            m_material->bind(m_shader, strip.surface, input, strip.uv_motion);
            prepare_colour_reads();
            set_fog(strip.fogged, strip.fog, strip.fog_colour);
            {
                const profiling::Zone trace("Effects material draw");
                m_material->draw_mesh(m_shader, *mesh);
            }
            state.stats.draw_calls += mesh->draws;
            state.stats.triangles += mesh->vertices / 3;
            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            continue;
        }

        add({strip.tex0,
             strip.tex1,
             strip.clamp,
             strip.alpha,
             0,
             0,
             strip.surface,
             strip.uv_motion,
             strip.fogged,
             strip.fog,
             strip.fog_colour,
             0,
             false,
             strip.before_mobys && !before_mobys_writes_z(strip)},
            at,
            count,
            false);
        at += count;
    }

    flush();
    strip_timing.reset();
    stats::gauge("effect plain strips", plain_strips);
    stats::gauge("effect strip shader switches", strip_switches);
    std::optional<stats::Scope> quad_timing;

    // Only nonempty quad lists contribute a timed GPU section.
    if (quads) {
        quad_timing.emplace("Effect quads", stats::kGpu, std::source_location::current());
    }

    const bool plain =
        quads && m_quad_shader.id() && render_opt::enabled(render_opt::Opt::EffectQuadShader);

    // Quad specialization selects the same source-zero material behavior.
    if (plain || (quads && m_active_shader != &m_shader)) {
        use_shader(plain ? m_quad_shader : m_shader, view_projection, input.camera.view);
    }

    stats::gauge("effect quad shader", quads && m_active_shader == &m_quad_shader ? 1 : 0);

    // Every quad's texture before any quad is drawn. A texture's upload rides on the first quad that
    // named it (frontend::particle_quads hands out the pool keys per frame, in pool order), but the
    // quads are drawn back to front: one drawn before that quad sampled what the key held last frame,
    // another texture, and the nanotech crates' orbs came out as opaque white quads.
    // Refactor note: the uploads could travel as a list of their own instead of riding on quads.
    for (const EffectQuad& q : quads ? input.effects : std::span<const EffectQuad>{}) {
        // A complete texture/palette pair must exist before either is uploaded.
        if (q.uploads >= 0
            && static_cast<std::size_t>(q.uploads) + 1 < input.effect_uploads.size()) {
            upload(input.effect_uploads[static_cast<std::size_t>(q.uploads)]);
            upload(input.effect_uploads[static_cast<std::size_t>(q.uploads) + 1]);
        }
    }

    // Quads are not cut at the far plane (the GS has none): Drek's Fleet's backdrop lies ~7000
    // units away, past it.
    const bool clamp_quads = quads && !input.depth_clamp && gl::depth_clamp_available();
    if (clamp_quads) {
        glEnable(GL_DEPTH_CLAMP);
    }

    set.reset();
    quad_indices = quads && m_quad_indices != 0;
    if (quad_indices) {
        bind_vertex_stream(m_quad_vao, quads_from);
    }
    at = quad_indices ? 0 : static_cast<GLint>(quads_from);
    Shader* const quad_shader = m_active_shader;
    unsigned tested_quads = 0;
    unsigned texture_splits = 0, sampler_splits = 0, blend_splits = 0;

    // Captured quad order preserves back-to-front blending across shader switches.
    for (const EffectQuad& q : quads ? input.effects : std::span<const EffectQuad>{}) {
        State wanted{q.tex0, q.tex1, q.clamp, q.alpha, q.tbp, q.cbp, 0};
        if (quad_indices) {
            normalize_quad_state(wanted.tex1, wanted.clamp, wanted.alpha);
        }
        Shader* shader = quad_shader;

        // Captured list registers override quad alpha testing and depth writes.
        if (q.registers) {
            // The list's TEST and ZBUF (EffectQuad::test, zbuf). An alpha test needs the shader
            // that keeps it; without that shader the quad is drawn as before, every pixel, no Z.
            const gs::Test t = gs::Test::decode(q.test);
            const bool testing =
                t.ate && t.atst != gs::AlphaTest::Always && t.atst != gs::AlphaTest::Never;

            // Missing optional test shaders retain the original untested path.
            if (!testing || m_test_shader.id()) {
                wanted.test = q.test;
                wanted.z_write = !gs::Zbuf::decode(q.zbuf).zmsk;

                // Only nontrivial alpha tests need the dedicated tested variant.
                if (testing) {
                    shader = &m_test_shader;
                    ++tested_quads;
                }
            }
        }

        // List TEST changes must finish the preceding quad batch first.
        if (m_active_shader != shader) {
            flush();
            use_shader(*shader, view_projection, input.camera.view);
            set.reset();
        }

        if (batch.count > 0 && set) {
            texture_splits +=
                wanted.tex0 != set->tex0 || wanted.tbp != set->tbp || wanted.cbp != set->cbp;
            sampler_splits += wanted.tex1 != set->tex1 || wanted.clamp != set->clamp;
            blend_splits += wanted.alpha != set->alpha;
        }
        add(wanted, at, 6, true);
        at += 6;
    }

    flush();
    if (clamp_quads) {
        glDisable(GL_DEPTH_CLAMP);
    }
    stats::gauge("effect quads alpha tested", tested_quads);
    if (quads && gl::is_vulkan()) {
        stats::gauge("effect quad texture splits", texture_splits);
        stats::gauge("effect quad sampler splits", sampler_splits);
        stats::gauge("effect quad blend splits", blend_splits);
    }
    stats::gauge("effects uploads reused", double(reused_uploads));

    // The material publishes counters after its final draw.
    if (m_material) {
        m_material->finish_pass();
    }

    stats::gauge("effects upload bytes reused", double(reused_bytes));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

}  // namespace openrac::renderer
