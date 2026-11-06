// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "mod_overlay.h"

#include <cstdint>
#include <string_view>

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
};

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
    static_cast<Frame*>(context)->list->rect(x, y, w, h, rgba);
}

void text(void* context, float x, float y, float height, std::uint32_t rgba, const char* line) {
    if (line != nullptr) {
        static_cast<Frame*>(context)->list->text(x, y, height, rgba, line);
    }
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
    unsigned framebuffer,
    int width,
    int height,
    float aspect
) {
    if (!has_camera || !mods::overlay_wanted()) {
        return;
    }
    s.list.begin(aspect > 0.0f ? aspect : static_cast<float>(width) / static_cast<float>(height));
    Frame frame{&camera, &s.list};
    openrac_overlay overlay{};
    overlay.size = sizeof overlay;
    overlay.context = &frame;
    overlay.project = project;
    overlay.rect = rect;
    overlay.text = text;
    overlay.fade = fade;
    overlay.text_width = text_width;
    mods::run_overlays(overlay);
    s.draw.draw(framebuffer, width, height, s.list.vertices());
}

void release() {
    s.draw.release();
}

}  // namespace openrac::frontend::mod_overlay
