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
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "common/log.h"
#include "game/common/frontend/scene_frame_internal.h"
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

ViewPixels view_pixels(const std::array<float, 4>& share, int width, int height) {
    const auto at = [](float s, int size) {
        return std::clamp(static_cast<int>(std::lround(s * static_cast<float>(size))), 0, size);
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

    // The world alone: the 2D bucket is the frame's.
    std::vector<std::pair<renderer::BucketRenderer*, bool>> kept;
    for (const auto& bucket : owner.renderer->renderers()) {
        if (bucket->bucket() == renderer::Bucket::Hud) {
            kept.emplace_back(bucket.get(), bucket->enabled);
            bucket->enabled = false;
        }
    }
    owner.renderer->render(input, target);
    for (const auto& [bucket, enabled] : kept) {
        bucket->enabled = enabled;
    }
}

void forget(SceneRenderState& owner) {
    for (SceneRenderState::SplitView& v : owner.split_views) {
        v.held = false;
    }
}

}  // namespace

bool split_screen(const Kept& scene) {
    return !scene.recording.views.empty();
}

void draw_split_views(SceneFrame& frame) {
    SceneRenderState& owner = frame.owner;
    if (!split_screen(frame.scene)) {
        forget(owner);
        return;
    }
    if (!owner.renderer || !frame.has_camera) {
        return;
    }
    const int width = frame.output.width;
    const int height = frame.output.height;
    const bool sampleable = owner.enhanced_graphics_available() && frame.graphics.enhanced_graphics;
    for (const renderer::FrameView& v : frame.scene.recording.views) {
        if (v.view >= owner.split_views.size() || v.screen_width <= 0.0f
            || v.screen_height <= 0.0f) {
            continue;
        }
        SceneRenderState::SplitView& kept = owner.split_views[v.view];
        kept.share = {
            v.rect[0] / v.screen_width,
            v.rect[1] / v.screen_height,
            v.rect[2] / v.screen_width,
            v.rect[3] / v.screen_height,
        };
        const ViewPixels p = view_pixels(kept.share, width, height);
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
    }
    // The frame's own scenery choice again, for what else reads it this frame.
    owner.scene.choose_scenery(
        frame.graphics.draw_distance_scale(),
        frame.input.camera.position,
        frame.input.camera.projection
    );
}

void put_split_views(SceneRenderState& owner, unsigned target, int width, int height) {
    for (SceneRenderState::SplitView& v : owner.split_views) {
        if (!v.held) {
            continue;
        }
        const ViewPixels p = view_pixels(v.share, width, height);
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
    for (SceneRenderState::SplitView& v : owner.split_views) {
        v.picture.release();
        v.held = false;
    }
}

}  // namespace openrac::frontend
