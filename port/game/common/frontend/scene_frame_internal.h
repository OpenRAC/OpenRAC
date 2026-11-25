// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private inputs shared by world recording, scene interpolation and captured HUD callbacks.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "game/common/frontend/state.h"

namespace openrac::frontend {

// The inputs and temporary views of one owned snapshot; callbacks end before this context expires.
struct SceneFrame {
    SceneRenderState& owner;             // Pinned by the frontend until the handoff completes.
    const Kept& scene;                   // Owned by the accepted render job.
    const settings::Settings& graphics;  // Settings copied with that job.
    FramePresentation& output;           // Shared result, published by the worker handoff mutex.
    renderer::FrameInput input{};        // Views into scene and the shadow storage below.
    std::vector<renderer::ShadowCapsule> shadows{};  // Interpolated capsule storage for input.
    std::vector<renderer::ShadowSet> shadow_sets{};  // Interpolated further casters for input.
    bool temporal = false;    // Temporal reconstruction is active for this scene.
    float picture_scale = 1;  // Width of the original picture inside the wider frame.
    bool has_camera = false;  // The world is drawn: the tick has a camera and set_camera kept one.
    std::vector<renderer::EffectQuad> effects{};  // Effect quads moved between ticks, for input.
    // Attached strip vertices moved from the display list's tick to the displayed parent.
    std::vector<renderer::EffectVertex> strip_vertices{};
    // The moby layer as uploaded when split-screen views draw mobys of their own (Kept::live_views):
    // each view's world takes its own, then this one goes back (split_views.cpp).
    std::optional<std::vector<scene::Instance>> mobys{};
};

// The camera part way from the last tick's to this one's (a cut is not blended). `pace`: the eye's
// move the tick before, when known (Kept::camera_pace, geometry.h blend_pose).
host::GameState blend_camera(
    const host::GameState& from,
    const host::GameState& to,
    float t,
    const renderer::Vec3* pace = nullptr
);

// The move the moby at `address` made the tick before the last, when known (Kept::prev_paces).
inline const renderer::Vec3* moby_pace(const Kept& scene, std::uint32_t address) {
    const auto it = scene.prev_paces.find(address);
    return it == scene.prev_paces.end() ? nullptr : &it->second;
}

// The eye's move the tick before the last, when known (Kept::camera_pace).
inline const renderer::Vec3* camera_pace(const Kept& scene) {
    return scene.camera_pace ? &*scene.camera_pace : nullptr;
}

// Uploads the interpolated transforms and poses used by world and motion rendering.
void upload_instances(SceneFrame& frame);

// Interpolates capsule shadows only while their source moby retains its identity.
void blend_shadows(SceneFrame& frame);

// Binds the owned packets, effects and HUD quads for this scene.
void set_packets(SceneFrame& frame);

// Binds captured strip and quad geometry, including interpolated effect attachments.
void set_effects(SceneFrame& frame);

// Registers the native HUD callback, consumed synchronously before the owned snapshot is released.
void set_target_mobys(SceneFrame& frame);

// Retains the last world where the original menu reads its background.
void retain_background(SceneFrame& frame);

// ---- Split screen (frontend/split_views.cpp, openrac/frame.h's openrac_frame_view_begin) ----
// Whether the tick recorded split-screen views: its world is then drawn per view.
bool split_screen(const Kept& scene);
// Draws each view the tick recorded into its kept picture, with its own camera; forgets them all
// when the tick recorded none.
void draw_split_views(SceneFrame& frame);
// The pictures the 2D path draws where it reaches their view's World (FrameInput::view_pictures):
// kept pictures of the views whose World the tick's splices reach, at the rectangle drawn there;
// `in_stream` their bits.
std::array<unsigned, renderer::kFrameViews> stream_view_pictures(
    SceneRenderState& owner, const Kept& scene, std::uint32_t& in_stream
);
// Puts every other kept view's picture (not in `in_stream`) on `target` (a framebuffer of `width` x
// `height`) at its rectangle: the frame's world, under its 2D.
void put_split_views(
    SceneRenderState& owner, unsigned target, int width, int height, std::uint32_t in_stream
);
// Before the graphics context goes.
void release_split_views(SceneRenderState& owner);

// ---- A mod's views (frontend/views.cpp, frontend.h's set_view_source) ----

// True while a mod has registered views, including before the window opens.
bool views_wanted();

// Asks the mod for this frame's views on the game thread.
void begin_views();

// Draws each requested view after the window's picture, from the same
// scene (`world`: whether this frame drew its world anew, else the last pictures are given again),
// then copies them where the mod said.
void draw_views(SceneFrame& frame, bool world);

// Tells the mod the frame that drew its views is submitted (after the window's swap).
void finish_views();

// Before the graphics context goes: the mod lets go of what it made with it, then the views'
// pictures and framebuffers go.
void release_views(SceneRenderState& owner);

}  // namespace openrac::frontend
