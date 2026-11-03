// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements renderer.h's opaque batching and ordered world-colour callbacks with per-node frame flags.
#include <algorithm>
#include <optional>

#include "common/breadcrumbs.h"
#include "common/stats.h"
#include "renderer/colour_graph.h"
#include "renderer/renderer.h"
#include "renderer/surface_chain.h"

namespace openrac::renderer {
// ---- Opaque batches ----

std::size_t Renderer::opaque_colour_end(std::size_t first) const {
    std::size_t end = first;

    // A final layer contributes opaque geometry before its own blended passes resume.
    for (; end < m_renderers.size(); ++end) {
        const auto& next = m_renderers[end];

        // Disabled layers contribute neither geometry nor a blend boundary.
        if (!next->enabled) {
            continue;
        }

        // Non-batched work must remain between the surrounding opaque batches.
        if (!next->batches_opaque_colour()) {
            break;
        }

        // Include this layer, then retain its original translucent/effect ordering.
        if (next->opaque_colour_barrier()) {
            ++end;
            break;
        }
    }

    return end;
}

std::vector<unsigned> Renderer::opaque_colour_buffers(
    std::size_t first, std::size_t end, const FrameInput& input, const RenderState& state
) const {
    std::vector<unsigned> buffers;

    // Sharing a cache across layers adds one dependency per allocation, not one per draw.
    for (std::size_t n = first; n < end; ++n) {
        const auto& next = m_renderers[n];

        // Disabled layers do not require a lighting provider.
        if (!next->enabled) {
            continue;
        }

        const auto reads = next->opaque_colour_buffers(input, state);

        // Unknown providers must execute after the shared plan rather than guess their inputs.
        if (reads.empty()) {
            return {};
        }

        buffers.insert(buffers.end(), reads.begin(), reads.end());
    }

    std::sort(buffers.begin(), buffers.end());
    buffers.erase(std::unique(buffers.begin(), buffers.end()), buffers.end());

    return buffers;
}

void Renderer::draw_opaque_colour(
    std::size_t first, std::size_t end, const FrameInput& input, RenderState& state
) {
    crash::breadcrumb("world opaque colour");
    // Static solids precede alpha-capable geometry across this contiguous batch.
    constexpr unsigned kOpaquePhases = 2;

    // Phase order preserves solid-before-cutout batching across participating layers.
    for (unsigned phase = 0; phase < kOpaquePhases; ++phase) {
        // Each layer contributes only its geometry for the current phase.
        for (std::size_t n = first; n < end; ++n) {
            const auto& next = m_renderers[n];

            // Disabled layers must not regain geometry through batch recording.
            if (next->enabled && next->batches_opaque_colour()) {
                next->render_opaque_colour(phase, input, state);
            }
        }
    }
}

Result<void> Renderer::record_opaque_colour(
    std::size_t first,
    std::size_t end,
    const FrameInput& input,
    RenderState& state,
    std::span<const unsigned> buffers
) {
    const bool shared = collecting_surface_chain();
    // The enclosing frame owns state and providers until this render-thread scope completes.
    return colour_pass(
        "world opaque colour",
        state.target,
        [this, first, end, &input, &state, shared] {
            std::optional<stats::Scope> timing;

            // Shared execution already times this node; immediate recording needs its own scope.
            if (!shared) {
                timing.emplace("world opaque colour", stats::kGpu);
            }
            draw_opaque_colour(first, end, input, state);
        },
        ColourAttachments::ColourDepth,
        {buffers, {}, ColourRecording::Ordered}
    );
}

// ---- Ordered world colour ----

bool Renderer::can_prepare_world_tail(
    std::size_t first, std::size_t hud, const FrameInput& input
) const {
    // Unknown temporal callbacks retain their synchronous boundaries.
    if (first >= hud || hud >= m_renderers.size() || !input.prepares_temporal_chain
        || !input.on_resolve_world) {
        return false;
    }

    // Disabled particle buckets still carry the opaque snapshot hook.
    for (std::size_t n = first; n < hud; ++n) {
        const auto& next = m_renderers[n];
        const auto bucket = next->bucket();

        // UI and external providers cannot be deferred into a world/temporal section.
        if (bucket < Bucket::Terrain || bucket > Bucket::Particles
            || (next->enabled && !next->prepares_colour_chain())) {
            return false;
        }
    }

    return true;
}

Result<void> Renderer::record_world_tail(
    std::size_t first,
    std::size_t hud,
    const FrameInput& input,
    RenderState& state,
    std::vector<bool>& opaque_prepared
) {
    bool particles_started = false;

    // The enclosing scope retains frame state and every provider until temporal recording finishes.
    for (std::size_t n = first; n < hud; ++n) {
        const auto& next = m_renderers[n];

        // Snapshot opaque colour before the first particle bucket, even when that bucket is disabled.
        if (!particles_started && next->bucket() == Bucket::Particles
            && input.on_before_particles) {
            input.on_before_particles(state.target);
            particles_started = true;
        }

        // Enabled world layers preserve their opaque and blended ordering before effects.
        if (next->bucket() <= Bucket::Mobys) {
            const std::size_t end = std::min(world_colour_end(n), hud);
            const auto recorded = record_world_colour(n, end, input, state, opaque_prepared);

            // Invalid inputs reject all pending work before recording starts.
            if (!recorded) {
                return unexpected(recorded.error());
            }

            n = end - 1;
            continue;
        }

        // Effect preparation owns its draw metadata through the enclosing synchronous scope.
        if (next->enabled) {
            const auto recorded = record_bucket_colour(n, input, state, false);

            // A failed effect declaration cannot publish an incomplete reconstructed image.
            if (!recorded) {
                return unexpected(recorded.error());
            }
        }
    }

    crash::breadcrumb("temporal resolve");
    input.on_resolve_world(state.target);
    return {};
}

std::size_t Renderer::world_colour_end(std::size_t first) const {
    std::size_t end = first;

    // World-only sequences stop before every external hook or unknown renderer.
    for (; end < m_renderers.size(); ++end) {
        const auto& next = m_renderers[end];
        const auto bucket = next->bucket();

        // Even disabled UI/particle buckets may carry frame callbacks.
        if (bucket < Bucket::Terrain || bucket > Bucket::Mobys) {
            break;
        }

        // Unknown enabled renderers retain immediate recording at their original position.
        if (next->enabled && !next->prepares_colour_chain()) {
            break;
        }
    }

    return end;
}

Result<void> Renderer::record_world_colour(
    std::size_t first,
    std::size_t end,
    const FrameInput& input,
    RenderState& state,
    std::vector<bool>& opaque_prepared
) {
    // Batch boundaries preserve every layer's original opaque-before-blended ordering.
    for (std::size_t n = first; n < end; ++n) {
        const auto& next = m_renderers[n];

        // Disabled layers contribute neither colour nor material dependencies.
        if (!next->enabled) {
            continue;
        }

        // Later opaque rounds stay after any preceding layer's blended work.
        if (!opaque_prepared[n] && state.reuse_prepass_depth && next->batches_opaque_colour()) {
            const std::size_t batch_end = std::min(opaque_colour_end(n), end);
            const auto buffers = opaque_colour_buffers(n, batch_end, input, state);
            const auto recorded = record_opaque_colour(n, batch_end, input, state, buffers);

            // A failed declaration must reject the whole sequence before execution.
            if (!recorded) {
                return unexpected(recorded.error());
            }

            std::fill(opaque_prepared.begin() + n, opaque_prepared.begin() + batch_end, true);
        }

        const auto recorded = record_bucket_colour(n, input, state, opaque_prepared[n]);

        // Never skip a rejected layer and publish an incomplete world image.
        if (!recorded) {
            return unexpected(recorded.error());
        }
    }

    return {};
}

Result<void> Renderer::record_bucket_colour(
    std::size_t index, const FrameInput& input, RenderState& state, bool opaque_batched
) {
    state.opaque_colour_batched = opaque_batched;
    return m_renderers[index]->record_colour(input, state);
}

Result<void> BucketRenderer::record_colour(const FrameInput& input, RenderState& state) {
    const auto buffers = opaque_colour_buffers(input, state);
    const bool opaque_batched = state.opaque_colour_batched;
    // The enclosing frame owns the bucket and state until this render-thread scope completes.
    return colour_pass(
        name(),
        state.target,
        [this, &input, &state, opaque_batched] {
            state.opaque_colour_batched = opaque_batched;
            crash::breadcrumb(name());
            render(input, state);
        },
        ColourAttachments::ColourDepth,
        {buffers, {}, ColourRecording::Ordered}
    );
}
}  // namespace openrac::renderer
