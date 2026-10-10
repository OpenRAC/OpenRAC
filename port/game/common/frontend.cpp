// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "frontend.h"

#include <SDL3/SDL.h>

#include <format>
#include <memory>
#include <optional>
#include <vector>

#include "common/log.h"
#include "platform/input.h"
#include "platform/window.h"
#include "renderer/direct.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "viewer/game_state.h"
#include "viewer/level.h"
#include "viewer/level_renderer.h"
#include "viewer/screenshot.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int openrac_game_loaded_overlay(void);

namespace openrac::frontend {

namespace {

struct State {
    std::string game;
    std::filesystem::path levels;
    std::unique_ptr<platform::Platform> sdl;
    std::unique_ptr<platform::Window> window;
    platform::Input input;
    std::unique_ptr<renderer::Renderer> renderer;
    renderer::FrameBuffer frame;
    viewer::LevelData level;
    viewer::LevelScene scene;
    int loaded = -2;  // the level whose geometry is uploaded; -2 none
    std::uint64_t index = 0;
};

std::unique_ptr<State> g;

std::uint32_t word_at(std::span<const std::uint8_t> ram, std::uint32_t at) {
    std::uint32_t w = 0;
    at &= 0x01FFFFFF;
    if (at + 4 <= ram.size()) {
        std::memcpy(&w, ram.data() + at, 4);
    }
    return w;
}

/*
 * The GIF data a frame's display list sends straight to the GS (VIF DIRECT): the 2D path (HUD,
 * menus, text, fades). The chain is followed as the DMA controller follows a source chain (the
 * tag's upper half goes to VIF, as the game's lists are sent with TTE), and the VIF codes that feed
 * VU1 (UNPACK, MPG and the rest) are stepped over by their sizes: what VU1 draws, the world, has
 * renderers of its own.
 */
std::vector<std::uint8_t> direct_packets(std::span<const std::uint8_t> ram, std::uint32_t chain) {
    std::vector<std::uint8_t> vif;
    std::uint32_t tag_at = chain & 0x01FFFFF0;
    std::uint32_t stack[2] = {0, 0};
    int depth = 0;
    auto append = [&](std::uint32_t at, std::uint32_t bytes) {
        at &= 0x01FFFFFF;
        if (at + bytes <= ram.size()) {
            vif.insert(vif.end(), ram.data() + at, ram.data() + at + bytes);
        }
    };
    for (int steps = 0; tag_at != 0 && steps < 20000; ++steps) {
        const std::uint32_t lo = word_at(ram, tag_at);
        const std::uint32_t addr = word_at(ram, tag_at + 4) & 0x7FFFFFF0;
        const std::uint32_t qwc = lo & 0xFFFF;
        const std::uint32_t id = (lo >> 28) & 7;
        append(tag_at + 8, 8);  // the two VIF codes in the tag
        const std::uint32_t after = tag_at + 16;
        if (addr & 0x80000000u) {
            break;  // the scratchpad: not followed
        }
        switch (id) {
            case 0:  // refe
                append(addr, qwc * 16);
                tag_at = 0;
                break;
            case 1:  // cnt
                append(after, qwc * 16);
                tag_at = after + qwc * 16;
                break;
            case 2:  // next
                append(after, qwc * 16);
                tag_at = addr;
                break;
            case 3:  // ref
            case 4:  // refs
                append(addr, qwc * 16);
                tag_at = after;
                break;
            case 5:  // call
                append(after, qwc * 16);
                if (depth < 2) {
                    stack[depth++] = after + qwc * 16;
                }
                tag_at = addr;
                break;
            case 6:  // ret
                append(after, qwc * 16);
                tag_at = depth > 0 ? stack[--depth] : 0;
                break;
            default:  // end
                append(after, qwc * 16);
                tag_at = 0;
                break;
        }
    }

    if (const char* dump = std::getenv("OPENRAC_DUMP_VIF")) {
        if (std::FILE* f = std::fopen(dump, "wb")) {
            std::fwrite(vif.data(), 1, vif.size(), f);
            std::fclose(f);
        }
    }
    std::vector<std::uint8_t> gif;
    std::size_t at = 0;
    while (at + 4 <= vif.size()) {
        std::uint32_t code;
        std::memcpy(&code, vif.data() + at, 4);
        at += 4;
        const std::uint32_t command = (code >> 24) & 0x7F;
        const std::uint32_t imm = code & 0xFFFF;
        const std::uint32_t num = (code >> 16) & 0xFF;
        if (command == 0x20) {
            at += 4;
        } else if (command == 0x30 || command == 0x31) {
            at += 16;
        } else if (command == 0x4A) {
            at += std::size_t{num == 0 ? 256u : num} * 8;
        } else if (command == 0x50 || command == 0x51) {
            const std::size_t size = std::size_t{imm == 0 ? 65536u : imm} * 16;
            if (at + size > vif.size()) {
                break;
            }
            gif.insert(gif.end(), vif.data() + at, vif.data() + at + size);
            at += size;
        } else if (command >= 0x60) {
            const std::uint32_t vn = (command >> 2) & 3;
            const std::uint32_t vl = command & 3;
            const std::uint32_t bits = vl == 0 ? 32 : vl == 1 ? 16 : vl == 2 ? 8 : 16;
            const std::uint32_t count = num == 0 ? 256 : num;
            const std::uint32_t components = vl == 3 ? 1 : vn + 1;
            at += ((count * components * bits + 31) / 32) * 4;
        }
    }
    return gif;
}

// Uploads a level's geometry; the boot program's title world is drawn with level 0's for now.
void use_level(int number) {
    const int wanted = number < 0 ? 0 : number;
    if (wanted == g->loaded) {
        return;
    }
    g->loaded = wanted;
    g->renderer.reset();
    g->scene.release();
    g->scene = viewer::LevelScene();
    g->level = viewer::LevelData();
    g->renderer = std::make_unique<renderer::Renderer>();
    std::string error;
    const auto dir = g->levels / std::format("level_{:02d}", wanted);
    if (!viewer::load_level(dir, g->level, error)) {
        log::warn("level {}: {} (extract it with editor/extract.py port)", wanted, error);
        g->renderer->add(std::make_unique<renderer::DirectRenderer>("hud", renderer::Bucket::Hud), error);
        g->renderer->init(error);
        return;
    }
    if (!g->scene.upload(g->level, g->renderer->textures(), error)) {
        log::error("level {}: {}", wanted, error);
        return;
    }
    for (std::size_t layer = 0; layer < viewer::kLayerCount; ++layer) {
        g->renderer->add(
            std::make_unique<viewer::LayerRenderer>(g->scene, static_cast<viewer::Layer>(layer)),
            error
        );
    }
    g->renderer->add(std::make_unique<renderer::DirectRenderer>("hud", renderer::Bucket::Hud), error);
    g->renderer->init(error);
    log::info("drawing level {} natively ({} models)", wanted, g->level.models.size());
}

}  // namespace

bool open(const std::string& game_id, const std::filesystem::path& levels, std::string& error) {
    g = std::make_unique<State>();
    g->game = game_id;
    g->levels = levels;
    g->sdl = std::make_unique<platform::Platform>(platform::kVideo | platform::kGamepad);
    if (!g->sdl->ok()) {
        error = g->sdl->error();
        return false;
    }
    platform::WindowConfig config;
    config.title = "OpenRAC: Ratchet & Clank (native)";
    config.width = 1280;
    config.height = 960;
    g->window = platform::Window::open(config, error);
    if (!g->window) {
        return false;
    }
    std::string missing;
    if (!gl::load(platform::Window::gl_loader(), missing)) {
        error = "OpenGL functions missing: " + missing;
        return false;
    }
    return true;
}

bool frame(std::span<const std::uint8_t> ram, std::uint32_t chain) {
    if (!g) {
        return true;
    }
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT
            || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
            return false;
        }
        g->input.handle_event(event);
    }
    g->input.update();

    // The level program loaded now (-1: the boot program, title and menus), as the port knows it.
    const int level = openrac_game_loaded_overlay();
    const viewer::GameAddresses& a =
        level >= 0 && level < 19 ? viewer::kRac1PalLevels[level] : viewer::kRac1Pal;
    use_level(level >= 0 && level < 19 ? level : -1);
    const viewer::GameState state = viewer::read_game_state(ram, a);

    if (g->index % 100 == 0) {
        log::info(
            "frame {}: level {}, camera {:.1f} {:.1f} {:.1f}, forward {:.2f} {:.2f} {:.2f}, fov {:.3f}, {} mobys",
            g->index, level, state.camera_position[0], state.camera_position[1],
            state.camera_position[2], state.forward[0], state.forward[1], state.forward[2],
            state.tan_half_fov_y, state.mobys.size()
        );
    }

    // The live mobys, by class.
    std::vector<viewer::Instance> live;
    for (const viewer::LiveMoby& m : state.mobys) {
        auto cls = g->level.moby_classes.find(m.class_id);
        if (cls != g->level.moby_classes.end()) {
            live.push_back({cls->second.first, m.matrix, {1, 1, 1, 1}});
        }
    }
    if (!g->level.models.empty()) {
        g->scene.set_instances(viewer::Layer::Mobys, live);
    }

    int width = 0;
    int height = 0;
    g->window->drawable_size(width, height);
    std::string error;
    if (width > 0 && height > 0 && (g->frame.width() != width || g->frame.height() != height)) {
        g->frame.create(width, height, error);
    }
    renderer::FrameInput input;
    input.clear_colour = {g->level.background[0], g->level.background[1], g->level.background[2], 1};
    input.camera.view = state.view();
    input.camera.projection = state.projection(
        static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1), 0.05f, 2000.0f
    );
    input.camera.position = state.camera_position;
    input.frame = g->index++;
    // The 2D path: the frame's direct GIF data, drawn by the direct renderer.
    if (g->index == 300) {
        log::info("chain: bases {:#x} {:#x}, building {}, shown {:#x}, cursor {:#x}",
                  word_at(ram, a.chain_bases), word_at(ram, a.chain_bases + 4),
                  word_at(ram, a.chain_index), viewer::shown_chain(ram, a), word_at(ram, 0x00161000));
        setenv("OPENRAC_DUMP_VIF", "/tmp/openrac_vif_300.bin", 1);
    } else {
        unsetenv("OPENRAC_DUMP_VIF");
    }
    (void)chain;
    const std::vector<std::uint8_t> packets = direct_packets(ram, viewer::shown_chain(ram, a));
    input.packets[static_cast<std::size_t>(renderer::Bucket::Hud)] = packets;
    if (g->renderer) {
        g->renderer->render(input, {g->frame.id(), width, height});
    }
    // OPENRAC_SHOT=FRAME:FILE.png writes that frame (for checking without looking at the screen).
    if (const char* shot = std::getenv("OPENRAC_SHOT")) {
        const char* colon = std::strchr(shot, ':');
        if (colon && static_cast<std::uint64_t>(std::atoll(shot)) == input.frame) {
            viewer::write_png(colon + 1, width, height, g->frame.read_rgba());
            log::info("frame {} written to {}", input.frame, colon + 1);
        }
    }
    g->frame.blit_to(0, width, height);
    g->window->swap();
    return true;
}

bool pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]) {
    if (!g || port < 0 || port >= platform::kPadPorts) {
        return false;
    }
    const platform::Pad& p = g->input.pad(port);
    *buttons = p.buttons;
    // OPENRAC_PRESS=FRAME:MASK:FRAMES,... holds buttons (mask bits as the pad reports them, 0x4000
    // cross, 0x8 start) for scripted checks.
    if (const char* script = port == 0 ? std::getenv("OPENRAC_PRESS") : nullptr) {
        for (const char* at = script; at && *at;) {
            unsigned long frame = 0, mask = 0, length = 5;
            if (std::sscanf(at, "%lu:%lx:%lu", &frame, &mask, &length) >= 2 && g->index >= frame
                && g->index < frame + length) {
                *buttons = static_cast<std::uint16_t>(*buttons & ~mask);
            }
            at = std::strchr(at, ',');
            at = at ? at + 1 : nullptr;
        }
    }
    analog[0] = p.right_x;
    analog[1] = p.right_y;
    analog[2] = p.left_x;
    analog[3] = p.left_y;
    return true;
}

void close() {
    if (g) {
        g->frame.release();
        g->renderer.reset();
        g->scene.release();
        g.reset();
    }
}

}  // namespace openrac::frontend
