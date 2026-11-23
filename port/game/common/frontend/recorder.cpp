// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame recorder's game-thread side (openrac/frame.h, docs/port/FRAME_RECORDER.md): each
// openrac_frame_* call appends a command to the open recording (State::recorder), with the records
// it carries; capture_2d_path (frame.cpp) closes it each tick and finds the recording the display
// list it walks was built with. Only the game's thread calls these, as the rest of the window's
// game-facing state (state.h).

#include <algorithm>
#include <cstring>
#include <memory>
#include <span>

#include "common/env.h"
#include "frontend.h"
#include "game/common/frontend/state.h"
#include "renderer/textures/texture.h"

namespace openrac::frontend::recorder {
namespace {

ImageResolver g_resolver;

renderer::BlendMode blend_mode(int blend) {
    const int mode = blend & 0xFF;
    return mode >= 0 && mode <= static_cast<int>(renderer::BlendMode::Multiply)
               ? static_cast<renderer::BlendMode>(mode)
               : renderer::BlendMode::Mix;
}

void set_rect(renderer::FrameCommand& c, const float* rect) {
    c.whole = rect == nullptr;
    if (rect != nullptr) {
        std::copy(rect, rect + 4, c.rect.begin());
    }
}

// The image `found` in the open recording's images, added once by its hash.
std::uint32_t keep_image(const renderer::UiImage& found) {
    auto& images = g->recorder.open().images;
    for (std::size_t i = 0; i < images.size(); ++i) {
        if (images[i].hash == found.hash && images[i].image == found.image) {
            return static_cast<std::uint32_t>(i);
        }
    }
    images.push_back(found);
    return static_cast<std::uint32_t>(images.size() - 1);
}

bool native() {
    // OPENRAC_UI_REPLAY draws the game's own packets instead, for comparing.
    static const bool on = !env::flag(env::Var::UiReplay);
    return g != nullptr && on;
}

}  // namespace

void set_image_resolver(ImageResolver resolver) {
    g_resolver = std::move(resolver);
}

void begin() {
    if (g) {
        g->recorder.close();
    }
}

void end() {
    if (g) {
        g->recorder.close();
    }
}

void anchor(gaddr list_cursor) {
    if (g) {
        g->recorder.set_anchor(list_cursor);
    }
}

void world(unsigned layers) {
    if (g) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::World;
        c.index = layers;
        g->recorder.push(c);
    }
}

void world_mobys() {
    if (g) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::WorldMobys;
        g->recorder.push(c);
    }
}

bool mobys(std::span<const std::uint8_t> ram, gaddr list, int count) {
    if (!g || count <= 0 || count > 0xFFFF) {
        return false;
    }
    mobys_drawn(ram, list, count);
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::Mobys;
    c.value = (static_cast<std::uint64_t>(count) & 0xFFFFu) << 32 | list;
    g->recorder.push(c);
    return true;
}

bool ui_item(std::span<const std::uint8_t> ram, const openrac_game_ui_item& item) {
    if (!native()) {
        return false;
    }
    renderer::UiItem out;
    std::memcpy(out.x, item.x, sizeof(out.x));
    std::memcpy(out.y, item.y, sizeof(out.y));
    std::memcpy(out.u, item.u, sizeof(out.u));
    std::memcpy(out.v, item.v, sizeof(out.v));
    std::memcpy(out.rgba, item.rgba, sizeof(out.rgba));
    std::memcpy(out.clip, item.clip, sizeof(out.clip));
    out.screen_width = item.screen_width > 0.0f ? item.screen_width : 512.0f;
    out.screen_height = item.screen_height > 0.0f ? item.screen_height : 448.0f;
    out.flags = item.flags;
    if ((item.flags & OPENRAC_UI_FLAT) == 0) {
        if (!g_resolver) {
            return false;
        }
        const renderer::UiImage found = g_resolver(ram, item.image);
        if (!found.image) {
            return false;
        }
        out.image = keep_image(found);
    }
    auto& items = g->recorder.open().ui_items;
    items.push_back(out);
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::UiItem;
    c.index = static_cast<std::uint32_t>(items.size() - 1);
    g->recorder.push(c);
    return true;
}

bool ui_quad(const openrac_game_ui_rect& quad) {
    if (!native()) {
        return false;
    }
    auto& quads = g->recorder.open().ui_quads;
    quads.push_back(ui_quad_record(quad));
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::UiQuad;
    c.index = static_cast<std::uint32_t>(quads.size() - 1);
    g->recorder.push(c);
    return true;
}

void effect_quad(std::uint32_t marker) {
    if (g && marker != 0) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::EffectQuad;
        c.index = marker;
        g->recorder.push(c);
    }
}

void fill(const float* rect, std::uint32_t rgba, int blend) {
    if (!g) {
        return;
    }
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::Fill;
    set_rect(c, rect);
    c.rgba = rgba;
    c.blend = blend_mode(blend);
    c.wide = (blend & OPENRAC_PICTURE_WHOLE) != 0 || rect == nullptr;
    g->recorder.push(c);
}

void clear(std::uint32_t rgba, int what) {
    if (!g || (what & (OPENRAC_CLEAR_COLOUR | OPENRAC_CLEAR_DEPTH)) == 0) {
        return;
    }
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::Clear;
    c.rgba = rgba;
    c.clear = static_cast<std::uint8_t>(what & 3);
    c.wide = true;
    g->recorder.push(c);
}

void picture(std::uint32_t id, const float* rect, std::uint32_t rgba, int blend) {
    if (!g) {
        return;
    }
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::Picture;
    c.index = id;
    // Pixels the host gave the picture go with the recording; otherwise it is a snapshot the back
    // end holds by id.
    if (const auto found = g->pictures.find(id); found != g->pictures.end()) {
        c.value = keep_image(found->second) + 1ull;
    }
    set_rect(c, rect);
    c.rgba = rgba;
    c.blend = blend_mode(blend);
    c.wide = (blend & OPENRAC_PICTURE_WHOLE) != 0;
    g->recorder.push(c);
}

void picture_image(
    std::span<const std::uint8_t> ram,
    const openrac_game_ui_image& image,
    const float* rect,
    std::uint32_t rgba,
    int blend
) {
    if (!g || !g_resolver) {
        return;
    }
    const renderer::UiImage found = g_resolver(ram, image);
    if (!found.image) {
        return;
    }
    renderer::FrameCommand c;
    c.op = renderer::FrameOp::Picture;
    c.index = renderer::kImagePicture;
    c.value = keep_image(found) + 1ull;
    set_rect(c, rect);
    c.rgba = rgba;
    c.blend = blend_mode(blend);
    c.wide = (blend & OPENRAC_PICTURE_WHOLE) != 0;
    g->recorder.push(c);
}

void picture_set(std::uint32_t id, const std::uint8_t* rgba, int width, int height) {
    if (!g || rgba == nullptr || width <= 0 || height <= 0) {
        return;
    }
    auto pixels = std::make_shared<renderer::Rgba8Image>(width, height);
    std::memcpy(pixels->pixels.data(), rgba, pixels->pixels.size());
    // The content names it, so that the pool's texture follows the pixels.
    std::uint64_t hash = 1469598103934665603ull ^ (static_cast<std::uint64_t>(id) << 32);
    for (const std::uint8_t b : pixels->pixels) {
        hash = (hash ^ b) * 1099511628211ull;
    }
    g->pictures[id] = {hash, std::move(pixels)};
}

void snapshot(std::uint32_t id) {
    if (g) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::Snapshot;
        c.index = id;
        g->pictures.erase(id);
        g->recorder.push(c);
    }
}

void target_begin(std::uint32_t id, int width, int height) {
    if (g && id >= renderer::kRecordedTargets) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::TargetBegin;
        c.index = id;
        c.width = static_cast<std::uint16_t>(std::clamp(width, 1, 0xFFFF));
        c.height = static_cast<std::uint16_t>(std::clamp(height, 1, 0xFFFF));
        g->recorder.push(c);
    }
}

void target_end() {
    if (g) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::TargetEnd;
        g->recorder.push(c);
    }
}

void scissor(const float* rect) {
    if (g) {
        renderer::FrameCommand c;
        c.op = renderer::FrameOp::Scissor;
        set_rect(c, rect);
        g->recorder.push(c);
    }
}

}  // namespace openrac::frontend::recorder
