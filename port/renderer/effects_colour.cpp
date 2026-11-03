// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// EffectRenderer's graph recording: scene snapshots, ordered colour and retained draw metadata.
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <vector>

#include "renderer/colour_graph.h"
#include "renderer/effects.h"
#include "renderer/surface_chain.h"

namespace openrac::renderer {
// ---- Snapshot and colour ----

Result<void> EffectRenderer::record_colour(const FrameInput& input, RenderState& state) {
    // Unknown providers keep the original bucket boundary and immediate preparation.
    if (!prepares_colour_chain()) {
        return BucketRenderer::record_colour(input, state);
    }

    // Effect preparation failures follow the same Result boundary as ordinary colour declarations.
    try {
        render(input, state);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"unknown effect colour preparation failure"});
    }
}

void EffectRenderer::record_colour(
    const FrameInput& input,
    RenderState& state,
    const std::shared_ptr<const PreparedEffects>& prepared
) {
    // Known providers retain simulation and geometry setup outside this snapshot/draw scope.
    const bool shared = surface_chain_enabled() && colour_graph_enabled()
                        && (!m_material || m_material->prepares_colour_chain());
    // The enclosing render call owns draw metadata and frame state through synchronous execution.
    const auto record = [this, &input, &state, prepared, shared] {
        // A snapshot is needed only when an accepted strip owns a replacement mesh.
        if (m_material
            && std::any_of(prepared->meshes.begin(), prepared->meshes.end(), [](const auto& mesh) {
                   return mesh.has_value();
               })) {
            m_material->prepare_scene(input, state.target);
        }

        // Unknown material callbacks retain their original immediate recording contract.
        if (!shared) {
            draw_prepared(input, state, *prepared);
            return;
        }

        const auto textures = m_material ? m_material->scene_textures() : std::array<unsigned, 2>{};
        std::vector<unsigned> reads;

        // Absent snapshots are represented by zero rather than an imported dummy texture.
        for (unsigned texture : textures) {
            // Only allocated snapshots participate in the graph.
            if (texture != 0) {
                reads.push_back(texture);
            }
        }

        const auto drawn = colour_pass(
            name() + " colour",
            state.target,
            [this, &input, &state, prepared] { draw_prepared(input, state, *prepared); },
            ColourAttachments::ColourDepth,
            {{}, reads, ColourRecording::Ordered}
        );

        // Invalid snapshot dependencies reject the scope before any copy or draw executes.
        if (!drawn) {
            throw std::runtime_error(drawn.error().message);
        }
    };

    // The same recording callback serves the original and shared execution paths.
    if (!shared || collecting_surface_chain()) {
        record();
        return;
    }

    const auto result = run_surface_chain(record);

    // Failed preparation cannot silently leave an incomplete effects pass.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

}  // namespace openrac::renderer
