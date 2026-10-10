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
    renderer::FrameBuffer picture;  // show_picture's: a movie frame, a boot still
    viewer::LevelData level;
    viewer::LevelScene scene;
    int loaded = -2;  // the level whose geometry is uploaded; -2 none
    std::uint64_t index = 0;
    // Every image the game sent outside the display list, oldest first, the latest per
    // rectangle: given again to each renderer made (use_level makes a new one per level).
    std::vector<renderer::ImageUpload> images;
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
    for (const renderer::ImageUpload& image : g->images) {
        g->renderer->textures().upload(image);
    }
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
    g->sdl = std::make_unique<platform::Platform>(platform::kVideo | platform::kGamepad | platform::kAudio);
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

void upload_image(
    std::uint32_t base,
    std::uint32_t width_units,
    std::uint8_t psm,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint8_t> pixels
) {
    if (!g) {
        return;
    }
    log::debug(
        "library image to block {:#x} width {} psm {:#x} at {},{} size {}x{}", base, width_units, psm, x,
        y, width, height
    );
    renderer::ImageUpload image;
    image.dbp = base;
    image.dbw = width_units;
    image.dpsm = psm;
    image.x = x;
    image.y = y;
    image.width = width;
    image.height = height;
    image.data.assign(pixels.begin(), pixels.end());
    // A later image to the same rectangle replaces the earlier one.
    std::erase_if(g->images, [&](const renderer::ImageUpload& old) {
        return old.dbp == base && old.dpsm == psm && old.x == x && old.y == y && old.width == width
               && old.height == height;
    });
    if (g->renderer) {
        g->renderer->textures().upload(image);
    }
    g->images.push_back(std::move(image));
}

bool show_picture(const std::uint8_t* rgba, int width, int height, float black) {
    using namespace gl;
    if (!g || width <= 0 || height <= 0) {
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

    // The fade, on the bytes as the chip's black quad blends them: C * (1 - coverage).
    std::vector<std::uint8_t> pixels(rgba, rgba + static_cast<std::size_t>(width) * height * 4);
    if (black > 0.0f) {
        const float keep = black >= 1.0f ? 0.0f : 1.0f - black;
        for (std::size_t i = 0; i < pixels.size(); i += 4) {
            for (int c = 0; c < 3; ++c) {
                pixels[i + c] = static_cast<std::uint8_t>(static_cast<float>(pixels[i + c]) * keep);
            }
        }
    }
    std::string error;
    if (g->picture.width() != width || g->picture.height() != height) {
        g->picture.create(width, height, error);
    }
    glBindTexture(GL_TEXTURE_2D, g->picture.colour_texture());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    int window_width = 0;
    int window_height = 0;
    g->window->drawable_size(window_width, window_height);
    const std::uint64_t index = g->index++;
    if (const char* shot = std::getenv("OPENRAC_SHOT")) {
        const char* colon = std::strchr(shot, ':');
        const std::uint64_t every = static_cast<std::uint64_t>(std::atoll(shot));
        if (colon && every > 0 && index % every == 0) {
            const std::string path = std::string(colon + 1) + std::to_string(index) + ".png";
            viewer::write_png(path, width, height, pixels);
            log::info("frame {} written to {}", index, path);
        }
    }
    // The picture fills the frame the game's own frames fill; its first row is the top.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g->picture.id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width, height, 0, window_height, window_width, 0, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    g->window->swap();
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

    // OPENRAC_DEBUG: the game's state each 100 frames, for bring-up.
    static const bool debug = std::getenv("OPENRAC_DEBUG") != nullptr;
    if (debug && g->index % 100 == 0) {
        log::info(
            "frame {}: level {}, camera {:.1f} {:.1f} {:.1f}, forward {:.2f} {:.2f} {:.2f}, fov {:.3f}, {} mobys",
            g->index, level, state.camera_position[0], state.camera_position[1],
            state.camera_position[2], state.forward[0], state.forward[1], state.forward[2],
            state.tan_half_fov_y, state.mobys.size()
        );
        log::info("  mode {} dialog kind {} step {} level word {} title exit {} pad {:#x}",
                  static_cast<int>(word_at(ram, 0x0015F6E8)), static_cast<int>(word_at(ram, 0x00193400)),
                  static_cast<int>(word_at(ram, 0x00193400 + 0x1C)), static_cast<int>(word_at(ram, 0x0015EE84)),
                  static_cast<int>(word_at(ram, 0x0015F690)), word_at(ram, 0x0013CBE4));
        log::info("  load stage {:#x} retries {} snd state {:#x}", word_at(ram, 0x0015EF48),
                  word_at(ram, 0x0015EFBC), word_at(ram, word_at(ram, 0x001517D0) + 8) & 0xFFFF);
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
    // No camera yet (loading, fades between programs): a black frame, not the level's background.
    const bool has_camera = state.forward[0] != 0.0f || state.forward[1] != 0.0f || state.forward[2] != 0.0f;
    input.clear_colour = has_camera
        ? std::array<float, 4>{g->level.background[0], g->level.background[1], g->level.background[2], 1}
        : std::array<float, 4>{0, 0, 0, 1};
    for (auto& r : g->renderer ? g->renderer->renderers() : std::span<const std::unique_ptr<renderer::BucketRenderer>>{}) {
        if (r->bucket() != renderer::Bucket::Hud) {
            r->enabled = has_camera;
        }
    }
    input.camera.view = state.view();
    input.camera.projection = state.projection(
        static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1), 0.05f, 2000.0f
    );
    input.camera.position = state.camera_position;
    // OPENRAC_DUMP_DRAWS=N or N-M: the frames whose 2D draws are logged.
    static const char* dump_at = std::getenv("OPENRAC_DUMP_DRAWS");
    if (dump_at) {
        char* end = nullptr;
        const std::uint64_t first = std::strtoull(dump_at, &end, 10);
        const std::uint64_t last = *end == '-' ? std::strtoull(end + 1, nullptr, 10) : first;
        renderer::g_dump_draws = g->index >= first && g->index <= last;
    }
    if (renderer::g_dump_draws) {
        log::info("frame {}: the 2D path's draws", g->index);
    }
    input.frame = g->index++;
    // The 2D path: the frame's direct GIF data, drawn by the direct renderer.
    (void)chain;
    const std::vector<std::uint8_t> packets = direct_packets(ram, viewer::shown_chain(ram, a));
    input.packets[static_cast<std::size_t>(renderer::Bucket::Hud)] = packets;
    if (g->renderer) {
        g->renderer->render(input, {g->frame.id(), width, height});
    }
    // OPENRAC_SHOT=FRAME:FILE.png writes that frame (for checking without looking at the screen).
    if (const char* shot = std::getenv("OPENRAC_SHOT")) {
        const char* colon = std::strchr(shot, ':');
        const std::uint64_t every = static_cast<std::uint64_t>(std::atoll(shot));
        if (colon && every > 0 && input.frame % every == 0) {
            const std::string path = std::string(colon + 1) + std::to_string(input.frame) + ".png";
            viewer::write_png(path, width, height, g->frame.read_rgba());
            log::info("frame {} written to {}", input.frame, path);
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
        g->picture.release();
        g->renderer.reset();
        g->scene.release();
        g.reset();
    }
}

}  // namespace openrac::frontend
