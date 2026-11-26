// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// EffectRenderer's fused strip/quad recorder, preserving shared batch state and material order.
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <source_location>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "common/stats.h"
#include "renderer/direct/direct.h"
#include "renderer/direct/gs.h"
#include "renderer/effects/effect_state.h"
#include "renderer/effects/effects.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#include "renderer/textures/texture_pack.h"

namespace openrac::renderer {
using namespace gl;

namespace {

// Key only the sampler and blend controls that EffectRenderer::apply reads.
void normalize_quad_state(std::uint64_t& tex1, std::uint64_t& clamp, std::uint64_t& alpha) {
    const auto filter = gs::Tex1::decode(tex1);
    const bool linear_min = filter.mmin == 1 || filter.mmin == 4 || filter.mmin == 5;
    tex1 = (std::uint64_t(filter.mmag) << 5) | (std::uint64_t(linear_min) << 6)
           | (std::uint64_t(filter.mipmapped()) << 7);

    const auto wrap = gs::Clamp::decode(clamp);
    clamp = std::uint64_t(wrap.wms != gs::Wrap::Repeat)
            | (std::uint64_t(wrap.wmt != gs::Wrap::Repeat) << 2);

    // FIX only participates in the blend when C selects it.
    alpha &= gs::Alpha::decode(alpha).c == 2 ? 0xFF000000FFull : 0xFFull;
}

}  // namespace

// ---- Ordered effect draws ----

void EffectRenderer::draw_prepared(
    const FrameInput& input,
    RenderState& state,
    const PreparedEffects& prepared,
    bool has_colour_pass
) {
    const bool quads = prepared.quads;
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
    m_depth_function = GL_GEQUAL;
    glDepthMask(m_part == EffectPart::Liquids || m_part == EffectPart::Opaque ? GL_TRUE : GL_FALSE);
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
        // An untextured quad (EffectQuad::untextured): drawn by m_untextured_shader, no texture.
        bool untextured = false;
        // A strip that samples the frame copy (EffectStrip::frame_copy): the pass's snapshot.
        bool frame_copy = false;
        // A pair run's second texture (PreparedEffects::quad_pairs; tbp is then its first), or 0.
        std::uint32_t pair = 0;
        // A native quad (EffectQuad::native): its state and its image's pool texture (0 none)
        // instead of the words above, which stay 0.
        bool native = false;
        EffectState material{};
        TextureHandle image = 0;
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
    const auto draw_batch_with_alpha_test = [&] {
        // TEST and depth-writing batches may require separate alpha-pass and alpha-fail draws; a
        // native batch is always drawn by its passes (its channels and depth write are its own).
        if (batch.count > 0 && set && (set->native || set->test != 0 || set->z_write)) {
            // A quad drawn with the list's TEST and ZBUF, as the chip draws it: the pixels that
            // pass the alpha test with every write (Z unless ZMSK), then, as OpenGOAL does, those
            // that fail with what AFAIL still writes. Going Commando's sparks and flames: GEQUAL
            // 0x60, the failing pixels colour only. ATST NEVER needs no shader's alpha test: every
            // pixel fails, and only what AFAIL keeps is written. Going Commando's nanotech orb
            // draws its glow and the crate's glass faces so (TEST 0x53001: NEVER, RGB_ONLY, ZBUF
            // without ZMSK): colour only, no Z. Taken as passing, with Z, the glass faces hid the
            // orb's sparks inside the crate. A native quad's material says the same in its terms.
            const profiling::Zone trace("Effects draw batch");
            const AlphaTest test = set->native ? set->material.alpha_test
                                               : alpha_test_from_gs(gs::Test::decode(set->test));
            const std::uint8_t writes = set->native ? set->material.colour_writes : kWriteColour;
            // The untextured shader keeps the alpha test in its source too (it is not built with
            // EffectStaticControls): a native untextured quad's test runs there.
            const bool tested = m_active_shader == &m_test_shader
                                || (set->native && m_active_shader == &m_untextured_shader);
            const EffectPasses passes = effect_passes(test, set->z_write, writes, tested);

            // Each planned draw takes its test, the pixels it keeps and its writes.
            for (std::size_t i = 0; i < passes.count; ++i) {
                const EffectPass& pass = passes.pass[i];
                glUniform1i(m_alpha_test, pass.alpha_test);
                glUniform1f(m_alpha_ref, static_cast<float>(test.reference));
                glUniform1i(m_alpha_keep_failing, pass.keep_failing ? 1 : 0);
                glColorMask(
                    (pass.colour_writes & kWriteRed) != 0,
                    (pass.colour_writes & kWriteGreen) != 0,
                    (pass.colour_writes & kWriteBlue) != 0,
                    (pass.colour_writes & kWriteAlpha) != 0
                );
                glDepthMask(pass.depth_write ? GL_TRUE : GL_FALSE);
                draw_batch();
            }

            glUniform1i(m_alpha_test, 0);
            glUniform1i(m_alpha_keep_failing, 0);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_FALSE);
        } else if (batch.count > 0) {
            const profiling::Zone trace("Effects draw batch");
            draw_batch();
        }
    };
    bool quads_declared = false;
    const auto flush = [&] {
        if (!batch.count) {
            return;
        }
        // Enhanced batches use their enclosing snapshot and geometry scopes.
        if (quads_declared || (has_colour_pass && enhanced_rendering_available())) {
            draw_batch_with_alpha_test();
        } else {
            RasterTexture texture{m_applied.offscreen};
            if (m_applied.texture && m_applied.texture != kOffscreenHandle) {
                const auto [name, mips] = state.textures.prepare_draw_texture(m_applied.texture);
                texture = {name, mips};
            }
            const RasterBuffer geometry[]{
                {m_vbo, m_vertices.size() * sizeof(Vertex)},
                {m_quad_indices, m_quad_capacity * 6 * sizeof(std::uint32_t)}
            };
            // A batch may have two alpha draws, but both borrow the same geometry and texture.
            const auto buffers = std::span(geometry).first(quad_indices ? 2 : 1);
            const auto attachments = m_part == EffectPart::Opaque
                                             || (m_part == EffectPart::Liquids && !set->z_masked)
                                             || set->z_write
                                         ? ColourAttachments::ColourDepth
                                         : ColourAttachments::ColourDepthRead;
            raster_pass(
                name(),
                state.target,
                {buffers, {&texture, 1}, attachments, m_material != nullptr},
                std::cref(draw_batch_with_alpha_test)
            );
        }
        batch.count = 0;
    };
    // The pair unit holds nothing outside a pair run's draws: a draw declares only its own textures.
    bool pair_bound = m_pair_shader;
    const auto unbind_pair = [&] {
        glActiveTexture(GL_TEXTURE0 + kPairUnit);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        pair_bound = false;
    };
    if (pair_bound) {
        unbind_pair();
    }
    // Adds `count` vertices from `first` to the draw, with this state; false when the state has
    // no texture in the pool (apply).
    bool drawable = true;
    const auto add = [&](const State& wanted, GLint first, GLsizei count, bool must_have_texture) {
        // State changes flush earlier geometry before installing the next material.
        if (!set || !(*set == wanted)) {
            flush();

            // A native state sets its own image, blend and depth comparison; an older untextured
            // state binds nothing (its shader does not sample), and the older states test depth as
            // the pass does.
            if (wanted.native) {
                if (pair_bound) {
                    unbind_pair();
                }
                drawable = apply_native(wanted.material, wanted.image, wanted.untextured, state);
            } else {
                set_depth_function(GL_GEQUAL);
                drawable = wanted.untextured
                           || apply(
                               wanted.tex0,
                               wanted.tex1,
                               wanted.clamp,
                               wanted.alpha,
                               state,
                               wanted.tbp,
                               wanted.cbp,
                               wanted.frame_copy
                           );
                if (drawable && wanted.pair != 0) {
                    bind_pair_texture(
                        state, wanted.tex0, wanted.tex1, wanted.clamp, wanted.pair, wanted.cbp
                    );
                    pair_bound = true;
                } else if (pair_bound) {
                    unbind_pair();
                }
            }

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

    // The pool textures of the tick's native images (FrameInput::effect_images), made the first
    // time a quad samples one: the pixels are the capture's own, never game memory.
    std::vector<TextureHandle> native_images(input.effect_images.size());
    const auto native_image_of = [&](bool untextured, std::uint32_t index) -> TextureHandle {
        // An untextured draw, or an index the capture did not give, samples nothing.
        if (untextured || index == 0 || index > native_images.size()) {
            return 0;
        }

        TextureHandle& handle = native_images[index - 1];
        const UiImage& image = input.effect_images[index - 1];

        // Made once a frame per image; an image without pixels stays without a texture.
        if (handle == 0 && image.image) {
            handle = state.textures.ui_image(image.hash, *image.image, image.indexed.get());
        }

        return handle;
    };
    const auto native_image = [&](const EffectQuad& q) -> TextureHandle {
        return native_image_of(q.untextured, q.image);
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
        if (strip.count < 3
            || static_cast<std::size_t>(strip.first) + strip.count > input.strip_vertices.size()
            || !draws(strip, input)) {
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

        if (strip.native) {
            // A native strip's state is its material's; it takes nothing from the list.
            if (strip.untextured && m_untextured_shader.id() == 0) {
                at += count;
                continue;
            }
            Shader& native_shader = strip.untextured ? m_untextured_shader : m_shader;
            if (m_active_shader != &native_shader) {
                flush();
                use_shader(native_shader, view_projection, input.camera.view);
                set.reset();
                ++strip_switches;
            }
            State native_state{};
            native_state.native = true;
            native_state.material = strip.state;
            native_state.image = native_image_of(strip.untextured, strip.image);
            native_state.untextured = strip.untextured;
            native_state.z_write = strip.state.depth_write;
            native_state.fogged = strip.fogged;
            native_state.fog = strip.fog;
            native_state.fog_colour = strip.fog_colour;
            native_state.z_masked = !strip.state.depth_write;
            add(native_state, at, count, !strip.untextured);
            at += count;
            continue;
        }

        State wanted{
            strip.tex0,
            strip.tex1,
            strip.clamp,
            m_part == EffectPart::Opaque && strip.blend_disabled ? 0 : strip.alpha,
            0,
            0,
            strip.surface,
            strip.uv_motion,
            strip.fogged,
            strip.fog,
            strip.fog_colour,
            0,
            false,
            strip.before_mobys && !before_mobys_writes_z(strip)
        };
        wanted.frame_copy = strip.frame_copy;
        // A frame-copy strip without the pass's snapshot is left out, not drawn from the chip's
        // copy or a placeholder (the black boxes under the dropship's thrusters).
        add(wanted, at, count, strip.frame_copy);
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
    // Only the last upload at each block is sent: all of them go in before any quad is drawn, so
    // an earlier one at the same block is replaced before anything samples it, and the last ones
    // keep their order. A field of quads paging two textures through one block in turn sent
    // (copied and hashed) one texture a quad, a thousand a frame.
    std::vector<std::size_t> quad_uploads;
    for (const EffectQuad& q :
         quads&& m_part != EffectPart::Opaque ? input.effects : std::span<const EffectQuad>{}) {
        // A complete texture/palette pair must exist before either is uploaded.
        if (q.uploads >= 0
            && static_cast<std::size_t>(q.uploads) + 1 < input.effect_uploads.size()) {
            quad_uploads.push_back(static_cast<std::size_t>(q.uploads));
            quad_uploads.push_back(static_cast<std::size_t>(q.uploads) + 1);
        }
    }
    std::vector<bool> superseded(quad_uploads.size());
    std::unordered_set<std::uint32_t> later_blocks;
    for (std::size_t i = quad_uploads.size(); i-- > 0;) {
        superseded[i] = !later_blocks.insert(input.effect_uploads[quad_uploads[i]].dbp).second;
    }
    for (std::size_t i = 0; i < quad_uploads.size(); ++i) {
        if (!superseded[i]) {
            upload(input.effect_uploads[quad_uploads[i]]);
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
    unsigned paired_quads = 0;

    const auto draw_quads = [&] {
        // Captured quad order preserves back-to-front blending across shader switches.
        for (const EffectQuad& q : quads ? input.effects : std::span<const EffectQuad>{}) {
            if (!draws(q, input)) {
                continue;
            }
            if (q.untextured && m_untextured_shader.id() == 0) {
                continue;
            }

            // A native quad's state is its material's; it takes nothing from the list.
            if (q.native) {
                State wanted{};
                wanted.native = true;
                wanted.material = q.state;
                wanted.image = native_image(q);
                wanted.untextured = q.untextured;
                wanted.z_write = q.state.depth_write;
                Shader* shader = q.untextured ? &m_untextured_shader : quad_shader;
                const Compare compare = q.state.alpha_test.compare;

                // Only a test that keeps some pixels needs the shader that keeps the test.
                if (!q.untextured && compare != Compare::Always && compare != Compare::Never
                    && m_test_shader.id()) {
                    shader = &m_test_shader;
                    ++tested_quads;
                }

                // A shader change finishes the quads drawn so far first.
                if (m_active_shader != shader) {
                    flush();
                    use_shader(*shader, view_projection, input.camera.view);
                    set.reset();
                }

                add(wanted, at, 6, !q.untextured);
                at += 6;
                continue;
            }

            State wanted{q.tex0, q.tex1, q.clamp, q.alpha, q.tbp, q.cbp, 0};
            wanted.untextured = q.untextured;
            // A quad of a pair run: the run's state, whichever of its two textures it samples.
            if (const std::size_t i = static_cast<std::size_t>(&q - input.effects.data());
                !prepared.quad_pairs.empty() && prepared.quad_pairs[i].second != 0) {
                const QuadPair& pair = prepared.quad_pairs[i];
                wanted.tex0 = pair.tex0;
                wanted.tbp = pair.first;
                wanted.pair = pair.second;
                paired_quads += q.tbp == pair.second;
            }
            if (quad_indices) {
                normalize_quad_state(wanted.tex1, wanted.clamp, wanted.alpha);
            }
            Shader* shader = q.untextured ? &m_untextured_shader : quad_shader;

            // Captured list registers override quad alpha testing and depth writes.
            if (q.registers && m_part != EffectPart::Opaque) {
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
            add(wanted, at, 6, !q.untextured);
            at += 6;
        }

        flush();
    };
    // Quad uploads are finished, so the texture set stays stable through every blend and TEST batch.
    if (quads_from < m_vertices.size() && !m_material && !enhanced_rendering_available()
        && !texture_pack::dumping()) {
        std::vector<RasterTexture> textures;
        std::set<std::tuple<std::uint64_t, std::uint32_t, std::uint32_t>> sources;
        for (const EffectQuad& quad : input.effects) {
            // A native quad's image is declared by its own texture, not by its zero words.
            if (quad.native) {
                // Only a drawn quad with an image has a texture to declare.
                if (const TextureHandle handle = draws(quad, input) ? native_image(quad) : 0) {
                    const auto [texture, mips] = state.textures.prepare_draw_texture(handle);
                    textures.push_back({texture, mips});
                }
                continue;
            }
            if (!draws(quad, input) || quad.untextured
                || !sources.emplace(quad.tex0, quad.tbp, quad.cbp).second) {
                continue;
            }
            if (const unsigned panel = offscreen_texture(quad.tex0, quad.tbp)) {
                textures.push_back({panel});
                continue;
            }
            auto tex0 = gs::Tex0::decode(quad.tex0);
            if (quad.tbp != 0) {
                tex0.tbp0 = quad.tbp;
                tex0.cbp = quad.cbp;
            }
            // The quad draw takes the same TEXA interpretation as the ordinary batch path.
            const auto handle = state.textures.resolve(tex0, {0, false, 0x80});
            const auto [texture, mips] = state.textures.prepare_draw_texture(handle);
            textures.push_back({texture, mips});
        }
        std::ranges::sort(textures, {}, &RasterTexture::name);
        const auto distinct = std::ranges::unique(textures, {}, &RasterTexture::name);
        textures.erase(distinct.begin(), distinct.end());
        const RasterBuffer geometry[]{
            {m_vbo, m_vertices.size() * sizeof(Vertex)},
            {m_quad_indices, m_quad_capacity * 6 * sizeof(std::uint32_t)}
        };
        // A newly prepared image can leave a different sampler binding than the strip batch did.
        m_applied = AppliedState{};
        quads_declared = true;
        raster_pass(
            "effect quads",
            state.target,
            {std::span(geometry).first(quad_indices ? 2 : 1),
             textures,
             ColourAttachments::ColourDepth},
            std::cref(draw_quads)
        );
    } else {
        draw_quads();
    }
    if (clamp_quads) {
        glDisable(GL_DEPTH_CLAMP);
    }
    if (pair_bound) {
        unbind_pair();
    }
    // A native quad may have changed the comparison: the next renderer finds the pass's.
    set_depth_function(GL_GEQUAL);
    stats::gauge("effect quads alpha tested", tested_quads);
    stats::gauge("effect quads on a pair's second texture", paired_quads);
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
