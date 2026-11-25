// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Split screen (openrac/frame.h's openrac_frame_view_begin / _end, docs/port/FRAME_RECORDER.md):
// the game draws each local player's view of the world into a part of its screen, with that
// player's camera. Here each view the tick recorded is drawn with the camera the game drew it with
// into a picture of its own (the world alone: no 2D, which the frame draws over all views), and
// every view's last picture goes on the frame at its rectangle, in place of the one world a
// full-screen frame draws. A game that draws one view a tick keeps the others on screen, as the
// console's frame buffer kept them; one that draws all of them in a tick refreshes all.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/log.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "game/common/frontend/view_mobys.h"
#include "renderer/frame/colour_transfer.h"
#include "renderer/renderer.h"

namespace openrac::frontend {
namespace {

// The far plane, as the window's (scene_frame.cpp's set_camera).
constexpr float kFar = 2000.0f;

// A view's rectangle in a target's pixels, from the top left. The world fills the target's width,
// as a full-screen frame's does on a target wider than the game's picture.
struct ViewPixels {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;

    int width() const { return x1 - x0; }

    int height() const { return y1 - y0; }
};

// Converts screen fractions to bounded pixels without rounding an unbounded floating-point value.
ViewPixels view_pixels(const std::array<float, 4>& share, int width, int height) {
    const auto at = [](float s, int size) {
        return static_cast<int>(std::lround(std::clamp(s, 0.0f, 1.0f) * static_cast<float>(size)));
    };
    return {at(share[0], width), at(share[1], height), at(share[2], width), at(share[3], height)};
}

// The camera of view `v`: the game's for that view (the tick's when the view ended with none),
// its vertical field of view the game's, opened to the view's shape in the target.
renderer::Camera view_camera(const SceneFrame& frame, const renderer::FrameView& v, float aspect) {
    host::GameState state = frame.scene.state;
    if (v.has_camera) {
        state.camera_position = v.camera.position;
        state.forward = v.camera.forward;
        state.left = v.camera.left;
        state.up = v.camera.up;
        if (v.camera.tan_half_fov_y > 0.0f) {
            state.tan_half_fov_x = v.camera.tan_half_fov_x;
            state.tan_half_fov_y = v.camera.tan_half_fov_y;
        }
    }
    renderer::Camera camera;
    camera.view = state.view();
    camera.position = state.camera_position;
    camera.projection = state.projection(aspect, frame.input.depth_clamp ? 0.25f : 0.05f, kFar);
    return camera;
}

// Draws the world of view `v` into `picture` (`size` pixels) with its camera.
void render_view(SceneFrame& frame, const renderer::FrameView& v, renderer::FrameBuffer& picture) {
    SceneRenderState& owner = frame.owner;
    renderer::FrameInput input = frame.input;
    // The frame's own hand-offs (its read-back copy, the temporal history, the probes' choice by
    // its depth) are the frame's, not a view's; its 2D is drawn over all views on the frame.
    input.before_hud = {};
    input.on_before_particles = {};
    input.pre_particle_colour_texture = 0;
    input.on_resolve_world = {};
    input.after_world_depth = {};
    input.prepares_world_depth = false;
    input.prepares_temporal_chain = false;
    input.keep_colour = false;
    input.gtao = false;
    input.direct_before_world = {};
    input.recording = nullptr;
    input.splices = {};
    input.splices_before_world = 0;
    input.culling_camera.reset();
    // The views take turns: no view's visibility is the last view's.
    input.keep_pvs_history = false;
    const float aspect = static_cast<float>(picture.width()) / static_cast<float>(picture.height());
    input.camera = view_camera(frame, v, aspect);
    owner.scene.choose_scenery(
        frame.graphics.draw_distance_scale(), input.camera.position, input.camera.projection
    );
    owner.scene.update_probe_regions(input.camera);

    renderer::RenderTarget target{picture.id(), picture.width(), picture.height(), 1.0f};
    target.depth_texture = picture.depth_texture();

    // The world alone, with only the layers this player recorded; the 2D bucket is the frame's.
    std::vector<std::pair<renderer::BucketRenderer*, bool>> kept;
    for (const auto& bucket : owner.renderer->renderers()) {
        kept.emplace_back(bucket.get(), bucket->enabled);
        if (bucket->bucket() == renderer::Bucket::Hud) {
            bucket->enabled = false;
        } else if (bucket->bucket() < renderer::Bucket::Particles) {
            // One player's World calls must not enable another player's undrawn layers.
            bucket->enabled = bucket->enabled
                              && (v.layers & (1u << static_cast<unsigned>(bucket->bucket()))) != 0;
        }
    }
    // A split screen drawn in one capture: the mobys this view's camera draws (view_mobys.h).
    if (frame.mobys) {
        owner.scene.set_instances(
            scene::Layer::Mobys, drawn_in_view(*frame.mobys, frame.scene.live_views, v.view)
        );
    }
    owner.renderer->render(input, target);
    for (const auto& [bucket, enabled] : kept) {
        bucket->enabled = enabled;
    }
}

// Runs the submitted recording's scheduled post effects of each view on that player's picture,
// before any composition, only while the picture holds that same recording's world and once
// (renderer::pair_view_effects). The worlds come from the recording closed last and the submitted
// one can be a send behind (Up Your Arsenal's split-screen maps): a player's effects then reach its
// picture a tick after its world, and a picture since redrawn from another recording never takes
// them. Either way the main 2D path skips them (FrameCommand::view_effect_prepared).
void apply_view_effects(SceneFrame& frame) {
    SceneRenderState& owner = frame.owner;
    const renderer::FrameRecording& submitted = frame.scene.recording;
    for (const renderer::FrameView& v : submitted.views) {
        if (v.screen_effects.empty() || !v.valid()) {
            continue;
        }
        SceneRenderState::SplitView& kept = owner.split_views[v.view];
        const std::uint64_t world = kept.held ? kept.world : 0;
        const renderer::ViewEffectPairing pairing =
            renderer::pair_view_effects(submitted.serial, world, kept.effects);
        if (pairing == renderer::ViewEffectPairing::Applied) {
            continue;
        }
        // Not run over another recording's world nor the composite: reported, never guessed.
        if (pairing == renderer::ViewEffectPairing::Unpaired) {
            static log::Capped told(4);
            if (told()) {
                log::warn(
                    "split screen: view {}: {} post effect(s) of recording {} not run: its picture "
                    "holds recording {}'s world",
                    v.view,
                    v.screen_effects.size(),
                    submitted.serial,
                    world
                );
            }
            continue;
        }
        renderer::FrameBuffer& picture = kept.picture;
        renderer::RenderTarget target{picture.id(), picture.width(), picture.height(), 1.0f};
        target.depth_texture = picture.depth_texture();
        // Each validated post pass copies and writes only this view's target.
        for (const std::uint32_t index : v.screen_effects) {
            owner.split_screen_effects
                .apply(submitted.screen_effects.at(index), target, picture.resolved());
        }
        kept.effects = submitted.serial;
    }
}

void forget(SceneRenderState& owner) {
    for (SceneRenderState::SplitView& v : owner.split_views) {
        v.held = false;
    }
}

// Forgets old pictures before drawing any view whose rectangle replaces the retained layout.
void forget_changed_layout(SceneRenderState& owner, std::span<const renderer::FrameView> views) {
    // The game may refresh all players or only one, so missing views alone do not change layout.
    for (const renderer::FrameView& v : views) {
        // The game also selects layouts while preparing a frame, before drawing any world.
        if (!v.has_world) {
            continue;
        }

        const bool changed =
            std::any_of(owner.split_views.begin(), owner.split_views.end(), [&v](const auto& kept) {
                return kept.held && !v.compatible_with(kept.layout);
            });

        // Forget once before drawing any replacement, so later views keep the new pictures.
        if (changed) {
            forget(owner);
            return;
        }
    }
}

// Rejects ordering that cannot yet be represented by an isolated native world post pass.
void require_effect_context(const SceneFrame& frame) {
    const auto& graphics = frame.graphics;
    const bool finishing =
        graphics.enhanced_graphics && frame.owner.enhanced_graphics_available()
        && ((graphics.bloom.enabled && graphics.bloom.strength > 0.0f) || graphics.agx_preview);
    const Result<void> supported = frame.scene.recording.validate_view_effect_context(
        {frame.has_camera, !frame.scene.packets.empty(), finishing}
    );

    // A rejected context is a visible worker failure, never a fallback to whole-composite processing.
    if (!supported) {
        throw std::runtime_error(supported.error().message);
    }
}

}  // namespace

bool split_screen(const Kept& scene) {
    // The latest recording's worlds, or a submitted split recording whose cut list refreshed none
    // of them: the retained pictures stay rather than a new full-screen world.
    return scene.recording.has_split_world()
           || std::any_of(scene.views.begin(), scene.views.end(), [](const auto& v) {
                  return v.has_world;
              });
}

void draw_split_views(SceneFrame& frame) {
    SceneRenderState& owner = frame.owner;
    if (!split_screen(frame.scene)) {
        forget(owner);
        return;
    }

    require_effect_context(frame);
    if (!owner.renderer || !frame.has_camera) {
        return;
    }
    const int width = frame.output.width;
    const int height = frame.output.height;
    const bool sampleable = owner.enhanced_graphics_available() && frame.graphics.enhanced_graphics;

    forget_changed_layout(owner, frame.scene.views);

    // Only a recorded world refreshes a player's retained picture.
    for (const renderer::FrameView& v : frame.scene.views) {
        // Layout-only calls and malformed coordinates do not describe a drawable view.
        if (!v.valid() || !v.has_world) {
            continue;
        }
        SceneRenderState::SplitView& kept = owner.split_views[v.view];
        kept.layout = v;
        const ViewPixels p = view_pixels(v.relative_rect(), width, height);
        if (p.width() < 1 || p.height() < 1) {
            kept.held = false;
            continue;
        }
        renderer::FrameBuffer& picture = kept.picture;
        if (picture.width() != p.width() || picture.height() != p.height()
            || (picture.depth_texture() != 0) != sampleable
            || picture.float_colour() != owner.frame.float_colour()) {
            std::string error;
            if (!picture.create(
                    p.width(), p.height(), error, 1, sampleable, owner.frame.float_colour()
                )) {
                static log::Capped told(4);
                if (told()) {
                    log::warn("split screen: view {}: {}", v.view, error);
                }
                kept.held = false;
                continue;
            }
        }
        render_view(frame, v, picture);
        kept.held = true;
        // A fresh world of the latest recording: none of its post effects has run on it yet.
        kept.world = frame.scene.views_serial;
        kept.effects = 0;
    }
    apply_view_effects(frame);
    // The frame's own mobys and scenery choice again, for what else reads them this frame.
    if (frame.mobys) {
        owner.scene.set_instances(scene::Layer::Mobys, *frame.mobys);
    }
    owner.scene.choose_scenery(
        frame.graphics.draw_distance_scale(),
        frame.input.camera.position,
        frame.input.camera.projection
    );
}

std::array<unsigned, renderer::kFrameViews> stream_view_pictures(
    SceneRenderState& owner, const Kept& scene, std::uint32_t& in_stream
) {
    std::array<unsigned, renderer::kFrameViews> pictures{};
    in_stream = 0;
    const renderer::FrameRecording& r = scene.recording;
    for (const renderer::FrameSplice& splice : scene.splices) {
        if (splice.command >= r.commands.size()) {
            continue;
        }
        const renderer::FrameCommand& c = r.commands[splice.command];
        // A World of a view that the 2D path reaches: that view's picture is drawn there.
        if (c.op != renderer::FrameOp::World || c.view >= r.views.size()
            || !r.views[c.view].valid()) {
            continue;
        }
        const renderer::FrameView& v = r.views[c.view];
        SceneRenderState::SplitView& kept = owner.split_views[v.view];
        // Only a picture of that rectangle: one of another layout is put whole, before the 2D.
        if (!kept.held || kept.layout.relative_rect() != v.relative_rect()) {
            continue;
        }
        static_cast<void>(kept.picture.resolved());
        pictures[v.view] = kept.picture.colour_texture();
        in_stream |= 1u << v.view;
    }
    return pictures;
}

void put_split_views(
    SceneRenderState& owner, unsigned target, int width, int height, std::uint32_t in_stream
) {
    for (std::uint32_t n = 0; n < renderer::kFrameViews; ++n) {
        SceneRenderState::SplitView& v = owner.split_views[n];
        // The 2D path draws those at their World (stream_view_pictures).
        if (!v.held || ((in_stream >> n) & 1u) != 0) {
            continue;
        }
        const ViewPixels p = view_pixels(v.layout.relative_rect(), width, height);
        // A window resized since the view was drawn: it waits for the next.
        if (p.width() != v.picture.width() || p.height() != v.picture.height()) {
            continue;
        }
        // GL's rows run bottom to top.
        renderer::blit_colour(
            "split-screen view",
            v.picture.resolved(),
            target,
            {{0, 0}, {p.width(), p.height()}, {p.x0, height - p.y1}, {p.x1, height - p.y0}}
        );
    }
}

void release_split_views(SceneRenderState& owner) {
    owner.split_screen_effects.release();
    for (SceneRenderState::SplitView& v : owner.split_views) {
        v.picture.release();
        v.held = false;
    }
}

}  // namespace openrac::frontend
