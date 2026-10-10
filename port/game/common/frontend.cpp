// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "frontend.h"

#include <SDL3/SDL.h>

#include <format>
#include <memory>
#include <optional>
#include <vector>

#include "common/log.h"
#include "openrac/guest.h"
#include "platform/input.h"
#include "platform/window.h"
#include "renderer/direct.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "viewer/game_state.h"
#include "viewer/level.h"
#include "viewer/level_renderer.h"
#include "viewer/moby_pose.h"
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
    // The camera the game's world renderers drew with this frame (world_drawn), if they ran.
    std::optional<viewer::GameState> world_camera;
    // Mobys drawn this frame with a camera of their own (mobys_drawn): address, that camera.
    std::vector<std::pair<std::uint32_t, viewer::GameState>> moby_cameras;
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

// The game's frame as the TV showed it: 4:3 (the PS2's 512x448 PAL / 512x416 NTSC buffer), the
// largest such box in the window, centred; the rest of the window is black. As ReRAC's game frame
// (crates/rc-engine/src/display.rs; ISC License, Copyright (c) 2026 ReRAC contributors): the frame
// keeps the game's framing whatever the window's shape.
struct Box {
    int x, y, width, height;
};

Box frame_box(int window_width, int window_height) {
    int width = window_width;
    int height = width * 3 / 4;
    if (height > window_height) {
        height = window_height;
        width = height * 4 / 3;
    }
    return {(window_width - width) / 2, (window_height - height) / 2, width, height};
}

// Draws `framebuffer` (width x height) into the window's box, black around it; `flip`: its first
// row is the top (an uploaded picture) rather than the bottom (a rendered frame).
void present(unsigned framebuffer, int width, int height, int window_width, int window_height, bool flip) {
    using namespace gl;
    const Box b = frame_box(window_width, window_height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    const int y0 = flip ? b.y + b.height : b.y;
    const int y1 = flip ? b.y : b.y + b.height;
    glBlitFramebuffer(0, 0, width, height, b.x, y0, b.x + b.width, y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

// The inverse of a view matrix (a rotation and a translation, column-major).
renderer::Mat4 rigid_inverse(const renderer::Mat4& m) {
    renderer::Mat4 r{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r[4 * j + i] = m[4 * i + j];
        }
    }
    for (int i = 0; i < 3; ++i) {
        r[12 + i] = -(r[i] * m[12] + r[4 + i] * m[13] + r[8 + i] * m[14]);
    }
    r[15] = 1.0f;
    return r;
}

// The extracted level the title world is written as (editor/level.py load_title).
constexpr int kTitleWorld = 99;
// The flight between planets' sky, written as a level of its own (editor/level.py load_flight).
constexpr int kFlightWorld = 98;
// The game mode word (0x15F6E8); 6 in the boot program is the flight between planets.
constexpr std::uint32_t kGameMode = 0x0015F6E8;
// The directional light bank, in the executable's terms (relocated to the loaded program's copy).
constexpr gaddr kLightBank = 0x0019BEC0;

// Uploads a level's geometry; while the boot program runs (the title and the main menu), the title
// world's, or level 0's when the title world was not extracted.
void use_level(int number) {
    int wanted = number;
    if (number == -2 && std::filesystem::exists(g->levels / std::format("level_{:02d}", kFlightWorld))) {
        wanted = kFlightWorld;
    } else if (number < 0) {
        wanted = std::filesystem::exists(g->levels / std::format("level_{:02d}", kTitleWorld)) ? kTitleWorld : 0;
    }
    if (wanted == g->loaded) {
        return;
    }
    g->loaded = wanted;
    // One renderer for the whole run: a new level replaces the subsystem renderers, but its texture
    // pool is the GS memory, which keeps what the game uploaded (a card's text uploaded through the
    // display list, the fonts) across the change.
    const bool fresh = !g->renderer;
    if (fresh) {
        g->renderer = std::make_unique<renderer::Renderer>();
        for (const renderer::ImageUpload& image : g->images) {
            g->renderer->textures().upload(image);
        }
    } else {
        g->renderer->clear_renderers();
    }
    g->scene.release();
    g->scene = viewer::LevelScene();
    g->level = viewer::LevelData();
    std::string error;
    const auto dir = g->levels / std::format("level_{:02d}", wanted);
    if (!viewer::load_level(dir, g->level, error)) {
        log::warn("level {}: {} (extract it with editor/extract.py port)", wanted, error);
        g->renderer->add(std::make_unique<renderer::DirectRenderer>("hud", renderer::Bucket::Hud), error);
        if (fresh) {
            g->renderer->init(error);
        }
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
    if (fresh) {
        g->renderer->init(error);
    }
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
    // The game paces itself (main.cpp: its frame rate, 50 Hz for PAL); a swap that also waited for
    // the display's refresh (60 or 120 Hz) would make two clocks fight and the frames stagger.
    config.vsync = false;
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

void mobys_drawn(std::span<const std::uint8_t> ram, std::uint32_t first, int count) {
    if (!g) {
        return;
    }
    const int level = openrac_game_loaded_overlay();
    const viewer::GameAddresses& a =
        level >= 0 && level < 19 ? viewer::kRac1PalLevels[level] : viewer::kRac1Pal;
    const viewer::GameState camera = viewer::read_game_state(ram, a);
    for (int i = 0; i < count; ++i) {
        g->moby_cameras.emplace_back(first + static_cast<std::uint32_t>(i) * 0x100u, camera);
    }
}

void world_drawn(std::span<const std::uint8_t> ram) {
    if (!g || g->world_camera) {
        return;
    }
    const int level = openrac_game_loaded_overlay();
    const viewer::GameAddresses& a =
        level >= 0 && level < 19 ? viewer::kRac1PalLevels[level] : viewer::kRac1Pal;
    g->world_camera = viewer::read_game_state(ram, a);
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
    present(g->picture.id(), width, height, window_width, window_height, true);
    g->window->swap();
    return true;
}

bool frame(std::span<const std::uint8_t> ram, std::uint32_t chain, std::uint32_t draws) {
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
    // The boot program's worlds: the flight between planets (game mode 6), else the title world.
    use_level(level >= 0 && level < 19 ? level : word_at(ram, kGameMode) == 6 ? -2 : -1);
    viewer::GameState state = viewer::read_game_state(ram, a);
    // The world is drawn from the camera the game's world renderers used, not from whatever the
    // camera globals hold at the end of the frame (the page menu sets its own camera, at
    // (256, 256, 64) facing +x, to draw its frame objects, as ReRAC notes).
    if (g->world_camera) {
        state.camera_position = g->world_camera->camera_position;
        state.forward = g->world_camera->forward;
        state.left = g->world_camera->left;
        state.up = g->world_camera->up;
        state.tan_half_fov_x = g->world_camera->tan_half_fov_x;
        state.tan_half_fov_y = g->world_camera->tan_half_fov_y;
        state.fog_colour = g->world_camera->fog_colour;
        state.fog_near = g->world_camera->fog_near;
        state.fog_far = g->world_camera->fog_far;
        state.fog_near_f = g->world_camera->fog_near_f;
        state.fog_far_f = g->world_camera->fog_far_f;
        g->world_camera.reset();
    }

    // OPENRAC_DEBUG: the game's state each 100 frames, for bring-up.
    static const bool debug = std::getenv("OPENRAC_DEBUG") != nullptr;
    if (debug && g->index % 100 == 0) {
        log::info(
            "frame {}: level {}, camera {:.1f} {:.1f} {:.1f}, forward {:.2f} {:.2f} {:.2f}, fov {:.3f}, {} mobys, layers drawn {:#x}",
            g->index, level, state.camera_position[0], state.camera_position[1],
            state.camera_position[2], state.forward[0], state.forward[1], state.forward[2],
            state.tan_half_fov_y, state.mobys.size(), draws
        );
        log::info("  mode {} dialog kind {} step {} level word {} title exit {} pad {:#x}",
                  static_cast<int>(word_at(ram, 0x0015F6E8)), static_cast<int>(word_at(ram, 0x00193400)),
                  static_cast<int>(word_at(ram, 0x00193400 + 0x1C)), static_cast<int>(word_at(ram, 0x0015EE84)),
                  static_cast<int>(word_at(ram, 0x0015F690)), word_at(ram, 0x0013CBE4));
        log::info("  fog colour {:.0f} {:.0f} {:.0f}, depth {:.0f} to {:.0f}, F {:.0f} to {:.0f}",
                  state.fog_colour[0] * 255, state.fog_colour[1] * 255, state.fog_colour[2] * 255,
                  state.fog_near, state.fog_far, state.fog_near_f, state.fog_far_f);
        log::info("  load stage {:#x} retries {} snd state {:#x}", word_at(ram, 0x0015EF48),
                  word_at(ram, 0x0015EFBC), word_at(ram, word_at(ram, 0x001517D0) + 8) & 0xFFFF);
    }

    // The live mobys, by class, each in the pose its animation fields give (viewer/moby_pose.h):
    // its joint palette goes to the scene, which skins the class's mesh with it. OPENRAC_ANIM=0
    // draws every moby in its bind pose.
    static const bool animate = [] {
        const char* v = std::getenv("OPENRAC_ANIM");
        return v == nullptr || std::strcmp(v, "0") != 0;
    }();
    std::vector<viewer::Instance> live;
    std::vector<viewer::LevelScene::JointColumns> palette;
    for (const viewer::LiveMoby& m : state.mobys) {
        auto cls = g->level.moby_classes.find(m.class_id);
        if (cls == g->level.moby_classes.end()) {
            continue;
        }
        viewer::Instance instance{cls->second.first, m.matrix, {1, 1, 1, 1}};
        // Its light word (+0x38: set 0, set 1, the cross-fade) and ambient colour (+0x3C), as
        // MobyProc reads them to light it.
        if (!g->level.light_sets.empty() && m.address + 0x100 <= ram.size()) {
            const std::uint8_t* b = ram.data() + m.address;
            instance.moby_light = {static_cast<float>(b[0x38]), static_cast<float>(b[0x39]),
                                   static_cast<float>(b[0x3A]) / 256.0f, 1.0f};
            // w: with mode bit 0x10, 1 + its glow word's RGB (+0x90), which MobyProc's glow list
            // draws its glow packets in; 0 without (a 24-bit integer is exact in a float).
            const std::uint32_t glow = static_cast<std::uint32_t>(b[0x90] | b[0x91] << 8 | b[0x92] << 16);
            instance.moby_ambient = {b[0x3C] / 128.0f, b[0x3D] / 128.0f, b[0x3E] / 128.0f,
                                     (b[0x34] & 0x10) != 0 ? static_cast<float>(glow) + 1.0f : 0.0f};
        }
        // Drawn by the game with a camera of its own: placed so the world's camera sees it where
        // that camera did (M' = V_world^-1 V_own M), as ReRAC draws the menu's frame objects.
        for (const auto& [address, camera] : g->moby_cameras) {
            if (address == m.address
                && (camera.camera_position != state.camera_position || camera.forward != state.forward)) {
                instance.matrix = renderer::multiply(
                    rigid_inverse(state.view()), renderer::multiply(camera.view(), instance.matrix)
                );
                break;
            }
        }
        const int joints = g->level.models[instance.model].joints;
        if (debug && g->index % 100 == 0 && m.class_id == 0) {
            // The hero, for bring-up: where he is and what his animation fields and pose say.
            const auto pose = viewer::moby_palette(ram, m.address);
            const std::uint8_t* b = ram.data() + m.address;
            float t = 0.0f;
            std::memcpy(&t, b + 0x54, 4);
            bool finite = !pose.empty();
            for (const auto& f : pose) {
                for (const auto& row : f) {
                    for (float x : row) {
                        finite = finite && std::isfinite(x) && std::fabs(x) < 1.0e6f;
                    }
                }
            }
            log::info("  hero at {:#x}: position {:.2f} {:.2f} {:.2f}, scale {:.3f}, keys {}:{} -> {}:{} t {:.3f}, "
                      "{} joints posed ({} in the mesh), {}, mode {:#x}",
                      m.address, m.matrix[12], m.matrix[13], m.matrix[14], m.matrix[0], b[0x52], b[0x50],
                      b[0x53], b[0x51], t, pose.size(), joints, finite ? "finite" : "NOT finite",
                      static_cast<unsigned>(b[0x34] | b[0x35] << 8));
        }
        if (animate && joints > 0) {
            const auto pose = viewer::moby_palette(ram, m.address);
            if (!pose.empty()) {
                instance.palette = static_cast<int>(palette.size());
                for (int j = 0; j < joints; ++j) {
                    // Columns: the images of the axes, then the translation, which the palette
                    // has in packed units (the mesh is in packed units / 1024).
                    viewer::LevelScene::JointColumns c{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
                    if (static_cast<std::size_t>(j) < pose.size()) {
                        const auto& f = pose[static_cast<std::size_t>(j)];
                        for (int i = 0; i < 3; ++i) {
                            c[4 * i + 0] = f[i][0];
                            c[4 * i + 1] = f[i][1];
                            c[4 * i + 2] = f[i][2];
                            c[4 * i + 3] = 0.0f;
                        }
                        c[12] = f[3][0] / 1024.0f;
                        c[13] = f[3][1] / 1024.0f;
                        c[14] = f[3][2] / 1024.0f;
                        c[15] = 1.0f;
                    }
                    palette.push_back(c);
                }
            }
        }
        live.push_back(instance);
    }
    g->moby_cameras.clear();
    g->scene.set_fog(state.fog_colour, state.fog_near, state.fog_far, state.fog_near_f,
                     state.fog_far_f);
    // The light bank as the game holds it now (16 sets of 0x40 bytes; the executable's 0x19BEC0, each
    // level's copy elsewhere): the level loader fills it from the gameplay file, and the game changes
    // sets at run time (the page menus write set 14 for their frame mobys, fog zones the hero's).
    {
        const gaddr bank = openrac_relocate_data(kLightBank);
        if (bank + 16 * 0x40 <= ram.size()) {
            std::vector<float> sets(16 * 16);
            std::memcpy(sets.data(), ram.data() + bank, sets.size() * sizeof(float));
            g->scene.set_light_sets(sets);
        }
    }
    if (!g->level.models.empty()) {
        g->scene.set_palette(palette);
        g->scene.set_instances(viewer::Layer::Mobys, live);
    }

    int window_width = 0;
    int window_height = 0;
    g->window->drawable_size(window_width, window_height);
    const Box box = frame_box(window_width, window_height);
    const int width = box.width;
    const int height = box.height;
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
            r->enabled = has_camera && (draws & (1u << static_cast<unsigned>(r->bucket()))) != 0;
        }
    }
    input.camera.view = state.view();
    // The game's frustum: its horizontal and vertical tangents as the game set them (the frame is
    // the TV's 4:3, which they were made for), not the window's shape.
    input.camera.projection = state.projection(
        state.tan_half_fov_y > 0.0f ? state.tan_half_fov_x / state.tan_half_fov_y : 4.0f / 3.0f, 0.05f, 2000.0f
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
    present(g->frame.id(), width, height, window_width, window_height, false);
    g->window->swap();
    return true;
}

bool pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]) {
    if (!g || port < 0 || port >= platform::kPadPorts) {
        return false;
    }
    const platform::Pad& p = g->input.pad(port);
    *buttons = p.buttons;
    analog[0] = p.right_x;
    analog[1] = p.right_y;
    analog[2] = p.left_x;
    analog[3] = p.left_y;
    // OPENRAC_PRESS=FRAME:MASK:FRAMES[:LX:LY],... holds buttons (mask bits as the pad reports them,
    // 0x4000 cross, 0x8 start) and optionally the left stick (0-255, 128 centred; 0 is left or
    // up) for scripted checks. FRAME counts the window's frames (world, 2D and movie frames).
    if (const char* script = port == 0 ? std::getenv("OPENRAC_PRESS") : nullptr) {
        for (const char* at = script; at && *at;) {
            unsigned long frame = 0, mask = 0, length = 5;
            unsigned int lx = 128, ly = 128;
            const int read = std::sscanf(at, "%lu:%lx:%lu:%u:%u", &frame, &mask, &length, &lx, &ly);
            if (read >= 2 && g->index >= frame && g->index < frame + length) {
                *buttons = static_cast<std::uint16_t>(*buttons & ~mask);
                if (read >= 5) {
                    analog[2] = static_cast<std::uint8_t>(lx);
                    analog[3] = static_cast<std::uint8_t>(ly);
                }
            }
            at = std::strchr(at, ',');
            at = at ? at + 1 : nullptr;
        }
    }
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
