// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "mod_overlay.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "frontend.h"
#include "mods.h"
#include "openrac/openrac_mod.h"
#include "renderer/ui/overlay_draw.h"
#include "renderer/ui/overlay_list.h"

namespace openrac::frontend::mod_overlay {

namespace {

// The frame a callback draws on.
struct Frame {
    const renderer::Camera* camera = nullptr;
    renderer::ui::OverlayList* list = nullptr;
    // The mod whose callback last drew, for the blend it set: each callback starts with alpha.
    const openrac_mod* drawer = nullptr;
};

// The list of the frame, for the mod whose callback is running: its blend starts as alpha, not as
// the one the callback before it left (the list's blend is the frame's, the callbacks run in turn).
renderer::ui::OverlayList& list_for_caller(Frame& frame) {
    const openrac_mod* caller = mods::overlay_mod();
    if (caller != frame.drawer) {
        frame.drawer = caller;
        frame.list->set_blend(OPENRAC_OVERLAY_BLEND_ALPHA);
    }
    return *frame.list;
}

int project(void* context, const float world[3], float* sx, float* sy) {
    const auto* frame = static_cast<const Frame*>(context);
    float x = 0;
    float y = 0;
    if (frame->camera == nullptr || world == nullptr || sx == nullptr || sy == nullptr
        || !renderer::ui::overlay_project(
            frame->camera->view, frame->camera->projection, {world[0], world[1], world[2]}, x, y
        )) {
        return 0;
    }
    *sx = x;
    *sy = y;
    return 1;
}

void rect(void* context, float x, float y, float w, float h, std::uint32_t rgba) {
    list_for_caller(*static_cast<Frame*>(context)).rect(x, y, w, h, rgba);
}

void text(void* context, float x, float y, float height, std::uint32_t rgba, const char* line) {
    if (line != nullptr) {
        list_for_caller(*static_cast<Frame*>(context)).text(x, y, height, rgba, line);
    }
}

// How this callback's rectangles and text that follow are laid over the frame.
int set_blend(void* context, std::uint32_t blend) {
    return list_for_caller(*static_cast<Frame*>(context)).set_blend(blend) ? 1 : 0;
}

float fade(void*, float distance, float near_distance, float far_distance) {
    return renderer::ui::overlay_fade(distance, near_distance, far_distance);
}

float text_width(void* context, float height, const char* line) {
    return line != nullptr ? static_cast<Frame*>(context)->list->text_width(height, line) : 0.0f;
}

struct State {
    renderer::ui::OverlayList list;
    renderer::ui::OverlayDraw draw;
};

State s;

}  // namespace

void draw(
    const renderer::Camera& camera,
    bool has_camera,
    bool world_under_menu,
    int phase,
    unsigned framebuffer,
    int width,
    int height,
    float aspect
) {
    if (!mods::overlay_wanted(phase)) {
        return;
    }
    // A level in play is drawn over only with its world, as on_overlay always was.
    if (phase == OPENRAC_PLAY_PLAYING && !has_camera) {
        return;
    }
    // A menu's frame has a camera that matches its picture only where the world is drawn live under
    // it (the pause menu's live picture): the vendor's and the others show a picture the game took,
    // or a camera of the menu's own. Without one `project` says no and the rest draws as it does.
    const bool projects = has_camera && (phase != OPENRAC_PLAY_MENU || world_under_menu);
    s.list.begin(aspect > 0.0f ? aspect : static_cast<float>(width) / static_cast<float>(height));
    Frame frame{projects ? &camera : nullptr, &s.list};
    openrac_overlay overlay{};
    overlay.size = sizeof overlay;
    overlay.phase = mods::overlay_bit(phase);
    overlay.context = &frame;
    overlay.project = project;
    overlay.rect = rect;
    overlay.text = text;
    overlay.fade = fade;
    overlay.text_width = text_width;
    overlay.set_blend = set_blend;
    mods::run_overlays(overlay);
    run_graphics([&] {
        s.draw.draw(framebuffer, width, height, s.list.vertices(), s.list.batches());
    });
}

void release() {
    s.draw.release();
}

}  // namespace openrac::frontend::mod_overlay
