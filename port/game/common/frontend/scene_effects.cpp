// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Binds captured effect geometry (scene_frame_internal.h) using owned scene history.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "game/common/frontend/effect_attachment.h"
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/scene_frame_internal.h"

namespace openrac::frontend {
namespace {

// Captured and displayed transforms of explicit anchors, indexed by game address.
using EffectParents = std::unordered_map<std::uint32_t, EffectParent>;

// Current and displayed transforms of callback owners with co-located rendered bodies.
using CallbackParents =
    std::unordered_map<std::uint32_t, std::pair<renderer::Mat4, renderer::Mat4>>;

// Collects only physical attachment anchors, excluding generic callback owners.
std::unordered_set<std::uint32_t> effect_attachment_addresses(const Kept& scene) {
    std::unordered_set<std::uint32_t> wanted;

    // Callback ownership does not attach world-space trails or beams.
    for (const auto& quad : scene.effects) {
        if (quad.attachment != 0) {
            wanted.insert(quad.attachment);
        }
    }

    // Delayed strips need rebasing even when the display is exactly on a simulation tick.
    for (const auto& strip : scene.strips) {
        if (strip.previous_parent != 0) {
            wanted.insert(strip.previous_parent);
        }
    }

    return wanted;
}

// Resolves only explicitly attached effects against the same transforms as the visible mobys.
EffectParents effect_parents(const SceneFrame& frame) {
    const Kept& scene = frame.scene;

    // Loading and camera cuts have no adjacent parent snapshot to interpolate.
    if (!scene.has_prev_state) {
        return {};
    }

    const auto wanted = effect_attachment_addresses(scene);

    EffectParents parents;

    // Avoid scanning the mobys when the scene has no explicitly attached effects.
    if (wanted.empty()) {
        return parents;
    }

    // Only parents retained across adjacent ticks have a usable source transform.
    for (std::size_t n = 0; n < scene.live.size() && n < scene.addresses.size(); ++n) {
        const std::uint32_t address = scene.addresses[n];

        if (!wanted.contains(address)) {
            continue;
        }

        const auto previous = scene.prev_matrices.find(address);
        const renderer::Mat4& current = scene.live[n].matrix;

        if (previous == scene.prev_matrices.end() || !same_moby(previous->second, current)) {
            continue;
        }

        parents.emplace(
            address,
            EffectParent{
                previous->second,
                current,
                blend_matrix(previous->second, current, frame.output.alpha)
            }
        );
    }

    // Callback-only objects still have captured transforms even when no mesh was selected.
    for (const std::uint32_t address : wanted) {
        if (parents.contains(address)) {
            continue;
        }

        if (const auto parent = captured_effect_parent(
                scene.prev_state.mobys, scene.state.mobys, address, frame.output.alpha
            )) {
            parents.emplace(address, *parent);
        }
    }

    return parents;
}

// Moves delayed, explicitly attached strips to their parents' displayed transforms.
void set_attached_strips(SceneFrame& frame, const EffectParents& parents) {
    const Kept& scene = frame.scene;

    // Most scenes have no attachments; leave their vertex buffer shared with the snapshot.
    if (!std::any_of(scene.strips.begin(), scene.strips.end(), [&](const auto& strip) {
            return parents.contains(strip.previous_parent);
        })) {
        return;
    }

    frame.strip_vertices = scene.strip_vertices;

    // Each captured strip owns a distinct vertex range in the snapshot.
    for (const auto& strip : scene.strips) {
        const auto parent = parents.find(strip.previous_parent);

        if (parent == parents.end()) {
            continue;
        }

        const std::size_t first = strip.first;
        const std::size_t count = strip.count;

        // Malformed packet ranges cannot address the scratch buffer.
        if (first > frame.strip_vertices.size() || count > frame.strip_vertices.size() - first) {
            continue;
        }

        for (auto& vertex : std::span(frame.strip_vertices).subspan(first, count)) {
            float point[3] = {vertex.x, vertex.y, vertex.z};
            follow_strip_parent(point, parent->second, strip.previous_parent_position_only);
            vertex.x = point[0];
            vertex.y = point[1];
            vertex.z = point[2];
        }
    }

    frame.input.strip_vertices = frame.strip_vertices;
}

// The moves of the mobys that carry their effect quads (EffectQuad::parent), by address: this
// tick's matrix and the one this frame draws the moby with, part way from the last tick's
// (blend_matrix). Only an owner that is not drawn itself but has a drawn moby at its very place,
// its body: level 5's ship, 0x018C7400 in the challenge, whose flame func_L05_004186D8 places by
// its rotation +0xC0 and position +0x10, while its body is drawn as other mobys at its place
// (classes 4305 to 4325). Its quads move as that body does. Others' quads stay where the tick has
// them: a callback's argument is often a moby whose quads are not on it (a Lancer shot's trail
// reaching back along its path, beams to far points), and turning those with the moby tore them
// into stray lines.
CallbackParents owner_moves(const Kept& scene, float alpha) {
    CallbackParents moves;

    if (alpha >= 1.0f || !scene.has_prev_state) {
        return moves;
    }

    std::unordered_set<std::uint32_t> drawn_here(scene.addresses.begin(), scene.addresses.end());
    std::unordered_set<std::uint32_t> owners;

    for (const renderer::EffectQuad& q : scene.effects) {
        if (q.parent != 0 && !drawn_here.contains(q.parent)) {
            owners.insert(q.parent);
        }
    }

    for (const host::LiveMoby& m : scene.state.mobys) {
        if (!owners.contains(m.address)) {
            continue;
        }

        // The drawn moby at its place this tick, drawn the tick before too.
        for (std::size_t n = 0; n < scene.live.size() && n < scene.addresses.size(); ++n) {
            const renderer::Mat4& now = scene.live[n].matrix;
            const float dx = now[12] - m.matrix[12], dy = now[13] - m.matrix[13],
                        dz = now[14] - m.matrix[14];
            const auto last = scene.prev_matrices.find(scene.addresses[n]);

            if (dx * dx + dy * dy + dz * dz < 1e-6f && last != scene.prev_matrices.end()) {
                moves.emplace(m.address, std::pair{now, blend_matrix(last->second, now, alpha)});
                break;
            }
        }
    }

    return moves;
}

// Follows a callback's co-located rendered body when no explicit anchor handled the quad.
bool follow_callback_parent(renderer::EffectQuad& quad, const CallbackParents& parents) {
    const auto parent = parents.find(quad.parent);

    // Generic callback arguments can describe world-space effects without a matching body.
    if (parent == parents.end()) {
        return false;
    }

    for (auto& corner : quad.corner) {
        follow_moby(parent->second.first, parent->second.second, corner);
    }

    return true;
}

// Interpolates current quads from explicit anchors, callback bodies or matching slot history.
void set_effect_quads(SceneFrame& frame, const EffectParents& attachments) {
    const Kept& scene = frame.scene;
    const float alpha = frame.output.alpha;

    // Current-tick quads already occupy their final positions at the endpoint.
    if (alpha >= 1.0f || scene.effects.empty()) {
        return;
    }

    const bool owned =
        std::any_of(scene.effects.begin(), scene.effects.end(), [](const auto& quad) {
            return quad.parent != 0 || quad.attachment != 0;
        });

    // World-space effects with no retained slot or owner stay at their captured positions.
    if (!owned && scene.prev_effect_corners.empty()) {
        return;
    }

    // Resolved explicit attachments do not need the more restrictive callback-body search.
    const bool callback_owned =
        std::any_of(scene.effects.begin(), scene.effects.end(), [&](const auto& quad) {
            return quad.parent != 0 && !attachments.contains(quad.attachment);
        });
    const auto parents = owner_moves(scene, callback_owned ? alpha : 1.0f);
    frame.effects = scene.effects;

    for (auto& quad : frame.effects) {
        const auto attachment = attachments.find(quad.attachment);

        // Explicit anchors also cover new particles whose slots have no history yet.
        if (attachment != attachments.end()) {
            follow_effect_parent(quad, attachment->second);
            continue;
        }

        // Legacy callback effects can follow their separately rendered bodies.
        if (follow_callback_parent(quad, parents)) {
            continue;
        }

        const auto previous = scene.prev_effect_corners.find(quad.id);

        // Unidentified quads and newly allocated world-space slots have no interpolation source.
        if (quad.id != 0 && previous != scene.prev_effect_corners.end()) {
            blend_effect_quad(quad, previous->second, alpha);
        }
    }

    frame.input.effects = frame.effects;
}

}  // namespace

void set_effects(SceneFrame& frame) {
    frame.input.strip_vertices = frame.scene.strip_vertices;
    frame.input.strips = frame.scene.strips;
    frame.input.effects = frame.scene.effects;
    const auto attachments = effect_parents(frame);
    set_attached_strips(frame, attachments);
    set_effect_quads(frame, attachments);
}

}  // namespace openrac::frontend
