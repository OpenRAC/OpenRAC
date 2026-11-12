// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/renderer.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include "common/breadcrumbs.h"
#include "common/log.h"
#include "common/stats.h"
#include "renderer/direct/direct.h"
#include "renderer/frame/colour_graph.h"
#include "renderer/frame/subsystems.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/gl.h"

namespace openrac::renderer {

using namespace openrac::gl;

const char* bucket_name(Bucket bucket) {
    switch (bucket) {
        case Bucket::Sky:
            return "sky";
        case Bucket::Terrain:
            return "terrain";
        case Bucket::Ties:
            return "ties";
        case Bucket::Shrubs:
            return "shrubs";
        case Bucket::Mobys:
            return "mobys";
        case Bucket::Particles:
            return "particles";
        case Bucket::Hud:
            return "hud";
        case Bucket::Count:
            break;
    }
    return "?";
}

unsigned readable(const RenderTarget& target) {
    if (target.resolve_framebuffer == 0) {
        return target.framebuffer;
    }
    if (target.resolves_itself) {
        return target.resolve_framebuffer;
    }
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.resolve_framebuffer);
    glBlitFramebuffer(
        0,
        0,
        target.width,
        target.height,
        0,
        0,
        target.width,
        target.height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    return target.resolve_framebuffer;
}

Renderer::~Renderer() = default;

bool Renderer::begin_surface_prepass(RenderState& state) {
    const RenderTarget& target = state.target;
    GLint samples = 1;
    glGetIntegerv(GL_SAMPLES, &samples);
    samples = std::max(1, samples);
    std::string error;

    // The shared graph orders depth sampling before opaque rendering reattaches the image.
    const bool share_depth =
        samples == 1 && target.depth_texture != 0 && collecting_surface_chain();

    // Matching sample counts preserve the colour rounds' per-sample depth coverage.
    if ((!m_surface_prepass.id() || m_surface_prepass.width() != target.width
         || m_surface_prepass.height() != target.height || m_surface_prepass.samples() != samples
         || m_surface_prepass_shared_depth != share_depth)
        && !(
            share_depth ? m_surface_prepass.create_colour(target.width, target.height, error)
                        : m_surface_prepass
                              .create(target.width, target.height, error, samples, samples == 1)
        )) {
        log::warn("surface normals: {}", error);
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_surface_prepass.id());
    m_surface_prepass_shared_depth = share_depth;
    if (share_depth) {
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, target.depth_texture, 0
        );
    } else if (samples == 1) {
        state.target.depth_texture = m_surface_prepass.depth_texture();
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    state.target.normal_texture = m_surface_prepass.colour_texture();
    return true;
}

void Renderer::finish_surface_prepass(const RenderTarget& target) {
    if (m_surface_prepass_shared_depth) {
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        return;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_surface_prepass.id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.framebuffer);
    const auto queued = queue_surface_copy(
        m_surface_prepass.id(),
        target.framebuffer,
        {target.width, target.height},
        SurfaceCopy::DepthStencil
    );
    // A rejected shared copy must not publish an incomplete depth prepass.
    if (!queued) {
        throw std::runtime_error(queued.error().message);
    }
    // Frames outside the shared scope keep the original native copy.
    if (!*queued) {
        glBlitFramebuffer(
            0,
            0,
            target.width,
            target.height,
            0,
            0,
            target.width,
            target.height,
            GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
            GL_NEAREST
        );
    }
    // Resolve the normal colour independently of the existing sky and background.
    const auto normals = m_surface_prepass.resolve_target()
                             ? queue_surface_copy(
                                   m_surface_prepass.id(),
                                   m_surface_prepass.resolve_target(),
                                   {target.width, target.height},
                                   SurfaceCopy::Colour
                               )
                             : Result<bool>{false};
    // Rejected resolves cannot supply normal data to the lighting consumers.
    if (!normals) {
        throw std::runtime_error(normals.error().message);
    }
    // The reference path and implicit single-sample targets retain their existing behavior.
    if (!*normals) {
        m_surface_prepass.resolved();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
}

bool Renderer::init(std::string& error) {
    RenderState state{m_textures, {}, {}};
    for (auto& r : m_renderers) {
        if (!r->init(state, error)) {
            return false;
        }
    }
    m_initialised = true;
    return true;
}

BucketRenderer* Renderer::add(std::unique_ptr<BucketRenderer> renderer, std::string& error) {
    if (m_initialised) {
        RenderState state{m_textures, {}, {}};
        if (!renderer->init(state, error)) {
            return nullptr;
        }
    }
    m_renderers.push_back(std::move(renderer));
    return m_renderers.back().get();
}

bool Renderer::add_game_renderers(std::string& error) {
    return add(std::make_unique<SkyRenderer>(), error) != nullptr
           && add(std::make_unique<TfragRenderer>(), error) != nullptr
           && add(std::make_unique<TieRenderer>(), error) != nullptr
           && add(std::make_unique<ShrubRenderer>(), error) != nullptr
           && add(std::make_unique<MobyRenderer>(), error) != nullptr
           && add(std::make_unique<ParticleRenderer>(), error) != nullptr
           && add(std::make_unique<DirectRenderer>("hud", Bucket::Hud), error) != nullptr;
}

void Renderer::render(const FrameInput& input, const RenderTarget& source) {
    RenderTarget target = source;
    // Animated texture pack replacements show the frame of the game's tick, not of the clock.
    m_textures.set_tick(input.frame);
    // [debug tools] The frame's target set and cleared, timed on the CPU only: on Vulkan the clear
    // is recorded with the first pass that draws.
    {
        const stats::Scope clear("frame clear");
        crash::breadcrumb("frame clear");
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        glViewport(0, 0, target.width, target.height);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glClearColor(
            input.clear_colour[0],
            input.clear_colour[1],
            input.clear_colour[2],
            input.clear_colour[3]
        );
        // Reversed depth: nothing is in front of 0, and the tests are GEQUAL.
        glClearDepth(0.0);
        glClear(
            (input.keep_colour ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT
            | GL_STENCIL_BUFFER_BIT
        );
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
    }
    // The world's passes clamp depth when the frame asks (FrameInput::depth_clamp), up to the HUD.
    const bool depth_clamp = input.depth_clamp && gl::depth_clamp_available();
    if (depth_clamp) {
        glEnable(GL_DEPTH_CLAMP);
    }

    RenderState state{m_textures, target, {}};
    const bool enhanced_available =
        std::any_of(m_renderers.begin(), m_renderers.end(), [](const auto& r) {
            return r->enabled && r->enhanced_lighting_available();
        });
    // Drawn when the frame asks, a renderer lights with it and a program registered one.
    const bool gtao = input.gtao && enhanced_available && m_ambient_occlusion != nullptr;
    bool scoped_ao = false, needs_indirect_ao = false;
    for (const auto& r : m_renderers) {
        if (!r->enabled || r->bucket() < Bucket::Terrain || r->bucket() > Bucket::Mobys) {
            continue;
        }
        const auto mode = r->ao_lighting();
        scoped_ao |= mode != AoLighting::Legacy;
        needs_indirect_ao |= mode == AoLighting::Indirect;
    }
    const bool early_lighting =
        std::any_of(m_renderers.begin(), m_renderers.end(), [&](const auto& r) {
            return r->enabled && r->needs_early_lighting(input);
        });
    const bool prepass_needed =
        early_lighting || input.after_world_depth || (gtao && input.gtao_settings.strength > 0);
    static const bool& discard_prepass = stats::probe(
        "OPENRAC_DEPTH_REUSE_OFF",
        "Profiling reference: discard GTAO prepass depth before world colour"
    );
    const auto world_prepass = [&] {
        crash::breadcrumb("world depth prepass");
        // Early background/UI and sky have already drawn. Depth now belongs
        // exclusively to the world and remains intact through its colour passes.
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        glViewport(0, 0, target.width, target.height);
        const bool surface_prepass = (early_lighting || gtao) && begin_surface_prepass(state);
        render_surface_prepass(input, state, surface_prepass);
        // Reuse the prepass depth without overwriting previously drawn sky or background.
        if (surface_prepass) {
            finish_surface_prepass(target);
        }
        glClearColor(
            input.clear_colour[0],
            input.clear_colour[1],
            input.clear_colour[2],
            input.clear_colour[3]
        );
        state.reuse_prepass_depth = true;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        unsigned resolved_depth =
            surface_prepass && m_surface_prepass.samples() == 1 ? state.target.depth_texture : 0;
        if (gtao && needs_indirect_ao
            && m_ambient_occlusion->render(input, state.target, false, early_lighting)) {
            if (early_lighting) {
                state.gtao_raw = m_ambient_occlusion->raw_texture();
                state.gtao_scale = input.gtao_settings.half_resolution ? 2 : 1;
                state.gtao_strength = input.gtao_settings.strength;
            } else {
                state.gtao_visibility = m_ambient_occlusion->visibility_texture();
            }
            state.stats.draw_calls += early_lighting ? 0 : 1;
            state.stats.triangles += early_lighting ? 0 : 1;
            resolved_depth = m_ambient_occlusion->depth_texture();
        }
        if (input.after_world_depth || early_lighting) {
            crash::breadcrumb("probe coverage selection");
            const stats::Scope selection("probe coverage selection", stats::kGpu);
            std::string error;
            if (!resolved_depth
                && ((m_selection_depth.width() == target.width
                     && m_selection_depth.height() == target.height)
                    || m_selection_depth.create(target.width, target.height, error, 1, true))) {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebuffer);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_selection_depth.id());
                glBlitFramebuffer(
                    0,
                    0,
                    target.width,
                    target.height,
                    0,
                    0,
                    target.width,
                    target.height,
                    GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                    GL_NEAREST
                );
                resolved_depth = m_selection_depth.depth_texture();
            }
            if (resolved_depth && input.after_world_depth) {
                input.after_world_depth(resolved_depth, target.width, target.height);
            }
        }
        if (resolved_depth && early_lighting) {
            crash::breadcrumb("early lighting");
            for (const auto& r : m_renderers) {
                if (r->enabled && r->needs_early_lighting(input)) {
                    r->render_early_lighting(resolved_depth, input, state);
                }
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        glViewport(0, 0, target.width, target.height);
        glDepthMask(GL_TRUE);
        if (discard_prepass) {
            glClearDepth(0);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            state.reuse_prepass_depth = false;
        }
        stats::gauge("world depth reused", state.reuse_prepass_depth ? 1 : 0);
    };
    // The HUD's renderer is the last of its bucket (the part of the 2D path before the world is
    // drawn by one ahead of the world's).
    std::size_t hud = m_renderers.size();
    std::size_t ao_before = 0;
    for (std::size_t i = 0; i < m_renderers.size(); ++i) {
        const Bucket bucket = m_renderers[i]->bucket();
        // DirectBackground has a HUD bucket but runs before the world. AO must
        // follow the last world pass, not the first pass labelled as UI.
        if (bucket >= Bucket::Terrain && bucket <= Bucket::Mobys) {
            ao_before = i + 1;
        }
        if (m_renderers[i]->bucket() == Bucket::Hud) {
            hud = i;
        }
    }
    bool ao_applied = false;
    bool prepass_done = false;
    bool particles_started = false;
    bool world_resolved = false;
    std::array<std::uint8_t, 32> local_flags{};
    std::vector<std::uint8_t> extra_flags;
    std::span<std::uint8_t> opaque_prepared;
    if (m_renderers.size() <= local_flags.size()) {
        opaque_prepared = std::span(local_flags).first(m_renderers.size());
    } else {
        extra_flags.resize(m_renderers.size());
        opaque_prepared = extra_flags;
    }
    for (std::size_t i = 0; i < m_renderers.size(); ++i) {
        const auto bucket = m_renderers[i]->bucket();
        const bool world_bucket =
            m_renderers[i]->enabled && bucket >= Bucket::Terrain && bucket <= Bucket::Mobys;
        const bool colour_chain = world_bucket && surface_chain_enabled() && colour_graph_enabled()
                                  && (scoped_ao || !gtao)
                                  && m_renderers[i]->prepares_colour_chain();
        const bool needs_prepass = prepass_needed && !prepass_done && world_bucket;
        const bool shared_prepass = needs_prepass && early_lighting && gtao && needs_indirect_ao
                                    && input.gtao_settings.strength > 0 && !discard_prepass
                                    && can_prepare_surface_chain(input, state);
        std::size_t colour_end = i;
        bool resolves_world = false;
        const auto prepare_colour = [&] {
            resolves_world = can_prepare_world_tail(i, hud, input);
            colour_end = resolves_world ? hud : world_colour_end(i);
            const auto recorded =
                resolves_world ? record_world_tail(i, hud, input, state, opaque_prepared)
                               : record_world_colour(i, colour_end, input, state, opaque_prepared);
            if (!recorded) {
                throw std::runtime_error(recorded.error().message);
            }
        };
        if (needs_prepass) {
            // Compose supported fused lighting, including explicit multisample resolves.
            if (shared_prepass) {
                // The scope prepares on this thread and executes before these frame locals expire.
                const auto prepare = [&] {
                    world_prepass();

                    // Unsupported colour paths retain the original batch boundary.
                    if (!colour_graph_enabled() || !state.reuse_prepass_depth
                        || !m_renderers[i]->batches_opaque_colour()) {
                        return;
                    }

                    const std::size_t end = opaque_colour_end(i);
                    const auto buffers = opaque_colour_buffers(i, end, input, state);

                    // Every participating layer must identify its current lighting inputs.
                    if (buffers.empty()) {
                        return;
                    }

                    const auto recorded = record_opaque_colour(i, end, input, state, buffers);

                    // Invalid cache reads reject the plan before deferred writers.
                    if (!recorded) {
                        throw std::runtime_error(recorded.error().message);
                    }

                    std::fill(opaque_prepared.begin() + i, opaque_prepared.begin() + end, true);

                    // The early-lighting inputs and the following colour providers are all
                    // declared. Compile them in order before publishing either section.
                    if (colour_chain) {
                        prepare_colour();
                    }
                };
                const auto result = run_surface_chain(std::cref(prepare));
                // An invalid plan cannot supply world depth or lighting to colour draws.
                if (!result) {
                    throw std::runtime_error(result.error().message);
                }
            } else {
                world_prepass();
            }
            prepass_done = true;
        }
        // A provider without declared early inputs still starts colour after the prepass finishes.
        if (colour_chain && colour_end == i) {
            const auto recorded = run_surface_chain(std::cref(prepare_colour));
            if (!recorded) {
                throw std::runtime_error(recorded.error().message);
            }
        }
        if (colour_end != i) {
            if (resolves_world) {
                target = state.target;
                world_resolved = true;
                ao_applied = true;
            }
            i = colour_end - 1;
            continue;
        }
        if (!opaque_prepared[i] && state.reuse_prepass_depth && m_renderers[i]->enabled
            && m_renderers[i]->batches_opaque_colour()) {
            const std::size_t end = opaque_colour_end(i);
            const auto recorded = record_opaque_colour(i, end, input, state);
            if (!recorded) {
                throw std::runtime_error(recorded.error().message);
            }
            std::fill(opaque_prepared.begin() + i, opaque_prepared.begin() + end, true);
        }
        state.opaque_colour_batched = opaque_prepared[i];
        if (ao_before != 0 && i == ao_before) {
            if (!scoped_ao && gtao
                && m_ambient_occlusion->render(input, state.target, true, false)) {
                state.stats.draw_calls += 1;
                state.stats.triangles += 1;
            }
            ao_applied = true;
        }
        // Opaque colour is retained once, even when the particle bucket is empty or disabled.
        if (!particles_started && bucket == Bucket::Particles && input.on_before_particles) {
            input.on_before_particles(target);
            state.pre_particle_colour_texture =
                m_renderers[i]->enabled ? input.pre_particle_colour_texture : 0;
            particles_started = true;
            glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
            glViewport(0, 0, target.width, target.height);
        }

        // Temporal reconstruction finishes before any HUD geometry uses display-resolution coordinates.
        if (i == hud && input.on_resolve_world) {
            crash::breadcrumb("temporal resolve");
            // Joined colour/temporal sections have already resolved before this HUD boundary.
            if (!world_resolved) {
                input.on_resolve_world(target);
            }
            state.target = target;
            glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
            glViewport(0, 0, target.width, target.height);
            glDisable(GL_SCISSOR_TEST);
            glDepthMask(GL_TRUE);
            glClearDepth(0);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        if (i == hud && depth_clamp) {
            glDisable(GL_DEPTH_CLAMP);
        }
        if (i == hud && input.before_hud) {
            input.before_hud();
            glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
            glViewport(0, 0, target.width, target.height);
        }
        auto& r = m_renderers[i];
        if (r->enabled) {
            // One marker per subsystem pass, never per mesh, draw or particle.
            crash::breadcrumb(r->name());
            // [debug tools] The pass, timed and counted for the overlay and the dump (common/stats.h).
            const stats::Scope pass(r->name(), r->owns_gpu_timing() ? 0 : stats::kGpu);
            // World buckets preserve their attachments; UI may redirect its target and keeps its
            // own path.
            if (bucket <= Bucket::Particles) {
                const auto recorded = record_bucket_colour(i, input, state, opaque_prepared[i]);
                if (!recorded) {
                    throw std::runtime_error(recorded.error().message);
                }
            } else {
                r->render(input, state);
            }
            if (bucket == Bucket::Particles) {
                state.pre_particle_colour_texture = 0;
            }
        }
    }
    if (depth_clamp) {
        glDisable(GL_DEPTH_CLAMP);
    }
    if (!scoped_ao && !ao_applied && ao_before != 0 && gtao
        && m_ambient_occlusion->render(input, state.target, true, false)) {
        state.stats.draw_calls += 1;
        state.stats.triangles += 1;
    }
    // Nothing reads the frame's depth and stencil after it (the next frame clears them): a tiled
    // GPU (a phone's) then does not write them back to memory at the end of the pass, which it does
    // for every attachment it is not told to drop (gl.h, OPENRAC_GL_ES_OPTIONAL_FUNCTIONS).
    if (glInvalidateFramebuffer != nullptr && target.framebuffer != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        const GLenum dropped[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, dropped);
    }
    stats::gauge(
        "opaque colour batching",
        std::any_of(
            opaque_prepared.begin(), opaque_prepared.end(), [](bool prepared) { return prepared; }
        )
            ? 1
            : 0
    );
    m_stats = state.stats;
    // [debug tools] The textures no pass drew with for a while let go, which walks every entry.
    const stats::Scope retire("texture retire");
    m_textures.end_frame();
}

void Renderer::clear_renderers() {
    for (auto& r : m_renderers) {
        r->release();
    }
    m_renderers.clear();
}

void Renderer::release() {
    m_surface_prepass.release();
    m_selection_depth.release();
    // Only a registered ambient occlusion has GL objects to free.
    if (m_ambient_occlusion != nullptr) {
        m_ambient_occlusion->release();
    }
    for (auto& r : m_renderers) {
        r->release();
    }
    m_textures.release_gl();
}

void Renderer::set_ambient_occlusion(std::unique_ptr<AmbientOcclusion> ambient_occlusion) {
    m_ambient_occlusion = std::move(ambient_occlusion);
}

}  // namespace openrac::renderer
