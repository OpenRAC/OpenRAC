// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the game draws of its world, gathered for the next frame, a slice of frontend/state.h: the
// level's geometry and the renderers that draw it (use_level), the world effect quads
// (effect_quad), the sky's shells and sprites (sky_shell, sky_sprite, sky_sprite_quads), the
// particles (particle, particle_quads), the mobys' scripted colours (moby_colour) and the 2D
// layer's quads drawn natively (ui_quad); and what the game asks of the picture's shape while it
// draws (hud_side, picture_wider). frontend.cpp's frame() turns what is gathered here into the
// frame. The light bank's address is the game's C table's (openrac_front_end, light_bank_address);
// what else the frame reads of the game's memory, the game's profile names (game/common/profile.h).

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "common/breadcrumbs.h"
#include "common/c_boundary.h"
#include "common/env.h"
#include "common/log.h"
#include "common/stats.h"
#include "debug_overlay.h"
#include "enhanced/renderer/atmosphere.h"
#include "enhanced/renderer/gtao.h"
#include "enhanced/renderer/texture_upscale.h"
#include "enhanced/renderer/water.h"
#include "enhanced/viewer/scene_lighting.h"
#include "frontend.h"
#include "game/common/frontend/draw_coverage.h"
#include "game/common/frontend/effects_dump.h"
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/state.h"
#include "game/common/mods.h"
#include "game/common/particle_texture_cache.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "options_menu.h"
#include "program/program.h"
#include "renderer/direct.h"
#include "renderer/effects.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "renderer/shadows.h"
#include "renderer/texture.h"
#include "renderer/texture_pool.h"
#include "scene/level.h"
#include "scene/level_renderer.h"
#include "settings.h"

namespace openrac::frontend {

namespace {

// The extracted level the title world is written as (editor/level.py load_title).
constexpr int kTitleWorld = 99;
// The flight between planets' sky, written as a level of its own (editor/level.py load_flight).
constexpr int kFlightWorld = 98;
// Where the sky's textures are placed in the texture pool, past the GS's 14-bit blocks.
constexpr std::uint32_t kSkyTextureKey = 0x20000;
// Where the effect quads' textures are placed (effect_textures), past the particles' (0x30000).
constexpr std::uint32_t kEffectTextureKey = 0x40000;

// A level's files read on another thread before the level comes (preload_level), for use_level
// to take instead of reading them itself.
struct Preload {
    struct Read {
        bool ok = false;
        scene::LevelData level;
        std::string error;
    };

    int level = -1;
    unsigned generation = 0;
    std::future<Read> read;
};

Preload g_preload;

// Where a level's files are read from: its folder, a mod's edited placements, the mods' assets.
struct LevelSources {
    std::filesystem::path dir;
    std::filesystem::path edited;
    std::vector<std::filesystem::path> mod_roots;
};

LevelSources level_sources(int wanted) {
    LevelSources sources;
    sources.dir = g->levels / std::format("level_{:02d}", wanted);
    // An edited level's scenery (--mods): the editor's pack writes it beside its gameplay file.
    if (openrac_game_mods_dir != nullptr) {
        sources.edited = std::filesystem::path(openrac_game_mods_dir)
                         / std::format("level_{:02d}", wanted) / "placements.json";
    }
    // The loaded mods' assets for this level (MODS.md, section 3), searched before its own files.
    for (const std::filesystem::path& root : mods::asset_roots()) {
        sources.mod_roots.push_back(root / std::format("level_{:02d}", wanted));
    }
    return sources;
}

// Reads level `wanted` into g->level: the files read ahead for it (preload_level), waited for if
// they are still being read, or else read now.
bool read_level(int wanted, const LevelSources& sources, std::string& error) {
    if (g_preload.read.valid() && g_preload.level == wanted
        && g_preload.generation == openrac_game_mods_generation) {
        crash::level_breadcrumb("level data wait", wanted);
        Preload::Read read = g_preload.read.get();
        g_preload.level = -1;
        if (read.level.dir == sources.dir) {
            g->level = std::move(read.level);
            error = std::move(read.error);
            crash::level_breadcrumb(read.ok ? "level data ready" : "level data failed", wanted);
            return read.ok;
        }
    }
    crash::level_breadcrumb("level data read", wanted);
    const bool read =
        scene::load_level(sources.dir, g->level, error, sources.edited, sources.mod_roots);
    crash::level_breadcrumb(read ? "level data ready" : "level data failed", wanted);
    return read;
}

// Marks renderer initialisation without changing the existing caller's fallback handling.
void init_level_renderer(int level, std::string& error) {
    crash::level_breadcrumb("level renderer init", level);
    const bool ready = g->renderer->init(error);
    crash::level_breadcrumb(ready ? "level renderer ready" : "level renderer init failed", level);
}

}  // namespace

// Starts reading level `number`'s files on another thread, for when the game goes there (the
// flight between planets, whose few seconds would otherwise be followed by the whole read, the
// game stopped and its sound running ahead). Nothing for a level drawn or read ahead already.
void preload_level(int number) {
    if (!g || number < 0 || number == g->loaded || !g->profile->has_levels()) {
        return;
    }
    if (g_preload.level == number && g_preload.generation == openrac_game_mods_generation) {
        return;
    }
    // A read of another level still under way is waited for here (its future ends with it).
    crash::level_breadcrumb("level preload requested", number);
    g_preload.level = number;
    g_preload.generation = openrac_game_mods_generation;
    log::info("reading level {} ahead", number);
    g_preload.read = std::async(std::launch::async, [sources = level_sources(number)] {
        Preload::Read read;
        read.ok = scene::load_level(
            sources.dir, read.level, read.error, sources.edited, sources.mod_roots
        );
        return read;
    });
    crash::level_breadcrumb("level preload queued", number);
}

// The directional light bank, in the executable's terms (relocated to the loaded program's copy).
gaddr light_bank_address() {
    return openrac_game_front_end != nullptr ? openrac_game_front_end->light_bank : 0;
}

// Uploads a level's geometry; while the boot program runs (the title and the main menu), the title
// world's, or level 0's when the title world was not extracted.
void use_level(int number) {
    static unsigned loaded_generation;
    int wanted = number;
    if (number <= -2
        && std::filesystem::exists(
            g->levels / std::format("level_{:02d}", kFlightWorld + 2 + number)
        )) {
        wanted = kFlightWorld + 2 + number;
    } else if (number < 0) {
        wanted = std::filesystem::exists(g->levels / std::format("level_{:02d}", kTitleWorld))
                     ? kTitleWorld
                     : 0;
    }
    if (wanted == g->loaded && loaded_generation == openrac_game_mods_generation) {
        return;
    }
    crash::level_breadcrumb("level load started", wanted);
    loaded_generation = openrac_game_mods_generation;
    g->loaded = wanted;
    // One renderer for the whole run: a new level replaces the subsystem renderers, but its texture
    // pool is the GS memory, which keeps what the game uploaded (a card's text uploaded through the
    // display list, the fonts) across the change.
    const bool fresh = !g->renderer;
    g->hud = nullptr;
    if (fresh) {
        g->renderer = std::make_unique<renderer::Renderer>();
        // The enhanced renderer's ambient occlusion, drawn when the player's settings ask for it.
        g->renderer->set_ambient_occlusion(std::make_unique<renderer::Gtao>());
        for (const renderer::ImageUpload& image : g->images) {
            g->renderer->textures().upload(image);
        }
    } else {
        g->renderer->clear_renderers();
    }
    crash::level_breadcrumb("level scene reset", wanted);
    g->scene.release();
    g->scene = scene::LevelScene();
    // The enhanced renderer's probe and early lighting, drawn when the player's settings ask.
    lighting::use_scene_lighting(g->scene);
    g->level = scene::LevelData();
    g->picker.clear();
    std::string error;
    const LevelSources sources = level_sources(wanted);
    const auto& dir = sources.dir;
    debug_overlay::notice({});
    // Only Ratchet & Clank's levels are extracted for the port so far (its profile says so). The
    // other games (Going Commando, Up Your Arsenal, Deadlocked at their menus) have no scenery to
    // read: their whole picture is the 2D path, both its parts, and nothing is missing to tell the
    // player about.
    const bool has_levels = g->profile->has_levels();
    const bool read = read_level(wanted, sources, error);
    if (!has_levels && !read) {
        crash::level_breadcrumb("level scenery unavailable", wanted);
        log::info(
            "level {}: no scenery for this game yet ({}); the 2D path is drawn alone", wanted, error
        );
        auto hud = std::make_unique<renderer::DirectRenderer>(
            "hud",
            renderer::Bucket::Hud,
            renderer::DirectConfig{},
            renderer::DirectRenderer::Input::Vif
        );
        g->renderer->add(std::make_unique<renderer::DirectBackground>(*hud), error);
        g->renderer->add(std::move(hud), error);
        if (fresh) {
            init_level_renderer(wanted, error);
        }
        return;
    }
    if (has_levels && !read) {
        crash::level_breadcrumb("level load failed", wanted);
        log::warn("level {}: {} (extract it with editor/extract.py port)", wanted, error);
        // Said in the picture: the game goes on, its HUD over a black world, and a player takes
        // that for the game being broken.
        debug_overlay::notice(
            std::format(
                "This level's scenery could not be read, so the world is black.\n{}\n"
                "Close the game and run \"Check the levels\" in the launcher (the log says which "
                "file).",
                error
            )
        );
        g->renderer->add(
            std::make_unique<renderer::DirectRenderer>(
                "hud",
                renderer::Bucket::Hud,
                renderer::DirectConfig{},
                renderer::DirectRenderer::Input::Vif
            ),
            error
        );
        if (fresh) {
            init_level_renderer(wanted, error);
        }
        return;
    }
    enhanced::install_texture_upscaler(g->renderer->textures());
    crash::level_breadcrumb("level scene upload", wanted);
    if (!g->scene.upload(g->level, g->renderer->textures(), error)) {
        crash::level_breadcrumb("level upload failed", wanted);
        log::error("level {}: {}", wanted, error);
        return;
    }
    // The 2D path: its part before the world first, the rest after the world renderers.
    crash::level_breadcrumb("level renderers setup", wanted);
    auto hud = std::make_unique<renderer::DirectRenderer>(
        "hud", renderer::Bucket::Hud, renderer::DirectConfig{}, renderer::DirectRenderer::Input::Vif
    );
    g->hud = hud.get();
    g->renderer->add(std::make_unique<renderer::DirectBackground>(*hud), error);
    static const bool& hero_pixel_shadow = stats::probe(
        "OPENRAC_HERO_SHADOW",
        "Ratchet-only shadow map: sun direction grounded, downward platforming cue airborne"
    );
    const bool sun_shadow = enhanced_graphics_available() && openrac_game_has_level(wanted)
                            && env::flag(env::Var::SunShadows);
    if (!g->scene.enable_sun_shadow(sun_shadow, error)) {
        log::warn("Cascaded sun shadows unavailable: {}", error);
    }
    if (!g->scene.enable_hero_shadow(
            enhanced_graphics_available() && (hero_pixel_shadow || g->scene.sun_shadow_enabled()),
            error
        )) {
        log::warn("Ratchet shadow map unavailable: {}", error);
    }
    for (std::size_t layer = 0; layer < scene::kLayerCount; ++layer) {
        // The caster is drawn after its shadow, so its proxy cannot darken
        // Ratchet himself. Opaque scenery has already filled the depth buffer.
        if (g->profile->shadow_directions() != 0
            && layer == static_cast<std::size_t>(scene::Layer::Mobys)) {
            if (g->renderer->add(std::make_unique<renderer::ShadowRenderer>(), error) == nullptr) {
                log::error("shadow renderer: {}", error);
            }
        }
        g->renderer->add(
            std::make_unique<scene::LayerRenderer>(g->scene, static_cast<scene::Layer>(layer)),
            error
        );
        // The liquids (the seas and lava the game draws solid) after the ties, where retail's draw
        // callback list 3 draws them, before the shrubs and mobys (renderer/effects.h).
        if (static_cast<scene::Layer>(layer) == scene::Layer::Ties
            && g->renderer->add(
                   std::make_unique<renderer::EffectRenderer>(
                       "liquids",
                       renderer::EffectPart::Liquids,
                       renderer::Bucket::Ties,
                       enhanced::atmosphere_material(g->scene)
                   ),
                   error
               ) == nullptr) {
            log::error("liquid renderer: {}", error);
        }
    }
    if (g->renderer->add(
            std::make_unique<renderer::EffectRenderer>(
                "effects",
                renderer::EffectPart::Rest,
                renderer::Bucket::Particles,
                enhanced::atmosphere_material(
                    g->scene,
                    wanted == 1 && g->game.starts_with("rac1-")
                        ? enhanced::make_water(g->scene, dir)
                        : nullptr
                )
            ),
            error
        )
        == nullptr) {
        log::error("effect renderer: {}", error);
    }
    g->renderer->add(std::move(hud), error);
    if (fresh) {
        init_level_renderer(wanted, error);
    }
    // The sky's textures at pool keys of their own (kSkyTextureKey + number), for its sprites.
    crash::level_breadcrumb("level sky upload", wanted);
    const auto sky = scene::load_sky_textures(dir, g->level.overrides);
    for (std::size_t i = 0; i < sky.size(); ++i) {
        if (sky[i].width > 0) {
            const renderer::TextureHandle h = g->renderer->textures().add(
                sky[i], renderer::AlphaScale::Full, std::format("sky {} of level {}", i, wanted)
            );
            g->renderer->textures().place(kSkyTextureKey + static_cast<std::uint32_t>(i), h);
        }
    }
    crash::level_breadcrumb("level load finished", wanted);
    log::info("drawing level {} natively ({} models)", wanted, g->level.models.size());
}

void savestate_restored(std::uint64_t tick) {
    options_menu::savestate_restored();
    if (!g) {
        return;
    }
    // Join any in-flight render before discarding its view of future guest state. use_level() will
    // rebuild the scene and GPU renderers from the restored level on the next tick.
    g->render_worker.reset();
    g->pending_frame.reset();
    g->prepared_tick.reset();
    g->pending_images.clear();
    g->images_after_list.clear();
    g->images.clear();
    g->water_impulses.clear();
    g->kept = std::make_shared<Kept>();
    g->scene.release();
    g->renderer.reset();
    if (g->temporal) {
        g->temporal->reset();
    }
    g->scene = scene::LevelScene();
    g->level = scene::LevelData();
    g->loaded = -2;
    g->index = tick;
    g->world_camera.reset();
    g->moby_cameras.clear();
    g->effects.clear();
    g->effect_uploads.clear();
    g->sky_shell_mask = 0;
    g->sky_sprites.clear();
    g->particles.clear();
    g->particle_textures.clear();
    g->particle_textures_for = nullptr;
    g->effect_textures.clear();
    g->effect_textures_for = nullptr;
    g->effect_texture_ids.clear();
    g->effect_hashed.clear();
    g->ui_quads.clear();
    g->ui_kept = {};
    g->ui_generation = 0;
    g->last_world_tick = 0;
    g->last_tick_menu = false;
    g->hold_last_world = false;
    g->tick_pictures = TickPictures{};
    g->repeat_frame = false;
    g->overdraw = false;
    g->skipped = 0;
    g->agx.release();
    g->frame.release();
    g->picture.release();
    g->last_world.release();
    for (renderer::FrameBuffer& view : g->views) {
        view.release();
    }
    g->view_targets.clear();
    g->render_failed = false;
    g_route = {};
    pacing.tick = std::chrono::steady_clock::now();
    pacing.due = {};
    pacing.last_alpha = -1.0f;
    pacing.first_pending = false;
}

void water_explosion(const float* position, float radius, float strength) {
    if (!g || g->loaded < 0 || !position
        || g->water_impulses.size() >= renderer::kWaterImpulseLimit) {
        return;
    }
    renderer::WaterImpulse impulse{
        {position[0], position[1], position[2]},
        {position[0], position[1], position[2]},
        radius,
        strength
    };
    if (renderer::valid_water_impulse(impulse)) {
        g->water_impulses.push_back(impulse);
    }
}

void effect_quad(std::span<const std::uint8_t> ram, const openrac_game_quad& quad) {
    if (quad.source < g->effect_sources.size()) {
        ++g->effect_sources[quad.source];
    }
    renderer::EffectQuad q;
    std::memcpy(q.corner, quad.corner, sizeof(q.corner));
    std::memcpy(q.rgba, quad.rgba, sizeof(q.rgba));
    std::memcpy(q.st, quad.st, sizeof(q.st));
    q.clamp = quad.clamp;
    q.tex0 = quad.tex0;
    q.tex1 = quad.tex1;
    q.alpha = quad.alpha;
    const std::uint32_t pixels = quad.pixels & 0x01FFFFFF;
    const std::uint32_t clut = quad.clut & 0x01FFFFFF;
    const std::uint32_t tbp = static_cast<std::uint32_t>(quad.tex0 & 0x3FFF);
    const std::uint32_t cbp = static_cast<std::uint32_t>((quad.tex0 >> 37) & 0x3FFF);
    q.tbp = tbp;
    q.cbp = cbp;
    const std::uint32_t width = 1u << ((quad.tex0 >> 26) & 0xF);
    const std::uint32_t height = 1u << ((quad.tex0 >> 30) & 0xF);
    if (pixels != 0 && clut != 0 && pixels + width * height <= ram.size()
        && clut + 1024 <= ram.size()) {
        renderer::ImageUpload palette;
        palette.dbp = cbp;
        palette.dbw = 1;
        palette.dpsm = 0x00;  // PSMCT32
        palette.width = 16;
        palette.height = 16;
        palette.data.assign(ram.data() + clut, ram.data() + clut + 1024);
        renderer::ImageUpload image;
        image.dbp = tbp;
        image.dbw = std::max<std::uint32_t>(1, width / 64);
        image.dpsm = 0x13;  // PSMT8
        image.width = width;
        image.height = height;
        image.data.assign(ram.data() + pixels, ram.data() + pixels + width * height);
        q.uploads = static_cast<int>(g->effect_uploads.size());
        // An 8-bit texture with a 32-bit CLUT (the paging's own) is drawn from a texture of its own,
        // by content (effect_textures): its GS blocks are shared with other effect textures in the
        // frame, and the pool holds only the last upload at a block when the quads are drawn.
        const renderer::gs::Tex0 t = renderer::gs::Tex0::decode(quad.tex0);
        if (t.psm == 0x13 && t.cpsm == 0x00 && !t.csm) {
            const std::array<std::uint32_t, 3> source{pixels, clut, width << 16 | height};
            auto hashed =
                std::find_if(g->effect_hashed.begin(), g->effect_hashed.end(), [&](const auto& h) {
                    return h.first == source;
                });
            if (hashed == g->effect_hashed.end()) {
                g->effect_hashed.push_back(
                    {source,
                     {renderer::hash_bytes(image.data.data(), image.data.size()),
                      renderer::hash_bytes(palette.data.data(), palette.data.size())}}
                );
                hashed = std::prev(g->effect_hashed.end());
            }
            g->effect_texture_ids.push_back(
                {g->effects.size(), hashed->second.first, hashed->second.second}
            );
        }
        g->effect_uploads.push_back(std::move(palette));
        g->effect_uploads.push_back(std::move(image));
        g->effects.push_back(q);
    } else {
        // No paging record: a texture the game put in the GS itself (the flight's planet picture,
        // loaded with the transition's data), resolved from the pool at TEX0's blocks; the
        // renderer leaves the quad out when nothing is there.
        g->effects.push_back(q);
    }
}

void effect_textures() {
    // A new renderer (a level loaded): a new pool.
    if (g->effect_textures_for != g->renderer.get()) {
        g->effect_textures.clear();
        g->effect_textures_for = g->renderer.get();
    }
    const renderer::gs::Texa texa{0, false, 0x80};
    for (const State::EffectTextureId& e : g->effect_texture_ids) {
        if (!g->renderer || e.effect >= g->effects.size()) {
            continue;
        }
        renderer::EffectQuad& q = g->effects[e.effect];
        const auto at = static_cast<std::size_t>(q.uploads);
        if (q.uploads < 0 || at + 1 >= g->effect_uploads.size()) {
            continue;
        }
        const renderer::ImageUpload& palette = g->effect_uploads[at];
        const renderer::ImageUpload& image = g->effect_uploads[at + 1];
        const std::uint64_t id = e.pixels ^ (e.clut * 0x9E3779B97F4A7C15ull)
                                 ^ (static_cast<std::uint64_t>(image.width) << 48
                                    | static_cast<std::uint64_t>(image.height) << 32);
        std::uint32_t& key = g->effect_textures[id];
        if (key == 0) {
            // As the pool converts the two uploads (convert_upload): the pixels as they lie, the
            // CLUT in CSM1, in the chip's alpha units.
            const std::vector<std::uint32_t> colours =
                renderer::clut_csm1(palette.data, 0x00, 16, 256, texa);
            renderer::Rgba8Image texture = renderer::convert_t8(
                image.data,
                static_cast<int>(image.width),
                static_cast<int>(image.height),
                colours,
                renderer::AlphaScale::Gs
            );
            const renderer::TextureHandle h = g->renderer->textures().add(
                std::move(texture),
                renderer::AlphaScale::Gs,
                std::format("effect {:#x}/{:#x}", image.dbp, palette.dbp)
            );
            key = kEffectTextureKey + static_cast<std::uint32_t>(g->effect_textures.size() - 1);
            g->renderer->textures().place(key, h);
        }
        q.tbp = key;
        q.cbp = 0;
    }
    // For a capture: the effect quads by where their texture's source was found (effect_quad).
    if (stats::enabled()) {
        stats::gauge("effect quads without texture source", g->effect_sources[0]);
        stats::gauge("effect quads paged this frame", g->effect_sources[1]);
        stats::gauge("effect quads paged before", g->effect_sources[2]);
    }
    g->effect_texture_ids.clear();
    g->effect_hashed.clear();
    g->effect_sources = {};
}

void sky_shell(int shell, const float* matrix) {
    if (!g || shell < 0 || shell >= static_cast<int>(g->sky_shells.size())) {
        return;
    }
    std::memcpy(g->sky_shells[static_cast<std::size_t>(shell)].data(), matrix, sizeof(float) * 16);
    g->sky_shell_mask |= 1u << shell;
}

int hud_side() {
    if (!g || !g->window || settings::get().hud != settings::Hud::Edges) {
        return 0;
    }
    int window_width = 0;
    int window_height = 0;
    g->window->drawable_size(window_width, window_height);
    const Box box =
        frame_box(window_width, window_height, picture_aspect(window_width, window_height));
    if (box.height <= 0) {
        return 0;
    }
    // The picture is box.width / box.height wide where the game's is 4:3: each side gained half
    // the difference, in pixels of which the game's picture has 512.
    const float wider =
        static_cast<float>(box.width) / (kTvAspect * static_cast<float>(box.height));
    return wider > 1.0f ? static_cast<int>(256.0f * (wider - 1.0f)) : 0;
}

float picture_wider() {
    if (!g || !g->window) {
        return 1.0f;
    }
    int window_width = 0;
    int window_height = 0;
    g->window->drawable_size(window_width, window_height);
    const float aspect = picture_aspect(window_width, window_height);
    const Box box = frame_box(window_width, window_height, aspect);
    // As render() widens the frame's horizontal tangent: 1 / picture_scale.
    if (aspect <= kTvAspect || box.width <= 0 || box.height <= 0) {
        return 1.0f;
    }
    return std::max(
        1.0f, static_cast<float>(box.width) / (kTvAspect * static_cast<float>(box.height))
    );
}

void sky_sprite(const std::uint8_t* record, std::uint64_t tex0) {
    std::array<std::uint8_t, 0x20> r;
    std::memcpy(r.data(), record, r.size());
    g->sky_sprites.emplace_back(r, tex0);
}

// The sky sprites as effect quads, as SkySpriteProc draws them with the particle program (ReRAC's
// sky_stars.rs; ISC License, Copyright (c) 2026 ReRAC contributors): around the eye, facing it,
// the half-diagonal size * 832 / depth frame-buffer pixels, turned by the rotation, additive, behind
// everything. Drawn far out along their direction (kSkyDistance, inside the far plane) and scaled to
// match, so the world's depth hides them where it is in front of the sky.
void sky_sprite_quads(const host::GameState& state) {
    constexpr float kSkyDistance = 1900.0f;
    const std::array<float, 3> cam = state.camera_position;
    const std::array<float, 3> fwd = state.forward;
    const std::array<float, 3> right{-state.left[0], -state.left[1], -state.left[2]};
    const std::array<float, 3> down{-state.up[0], -state.up[1], -state.up[2]};
    const float tan_y = state.tan_half_fov_y > 0.0f ? state.tan_half_fov_y : 0.476f;
    std::vector<renderer::EffectQuad> quads;
    quads.reserve(g->sky_sprites.size());
    for (const auto& [r, tex0] : g->sky_sprites) {
        float pos[3];
        float theta;
        float size;
        std::uint32_t rgba;
        std::memcpy(pos, r.data() + 0x10, 12);
        std::memcpy(&theta, r.data() + 8, 4);
        std::memcpy(&size, r.data() + 0x1C, 4);
        std::memcpy(&rgba, r.data() + 4, 4);
        const float len = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
        const float z = pos[0] * fwd[0] + pos[1] * fwd[1] + pos[2] * fwd[2];
        if (!(len > 0.0f) || !(z > 0.0f) || !std::isfinite(size)) {
            continue;
        }
        const float k = kSkyDistance / len;
        const float h = 4.0f * size * tan_y * k;
        const float c = std::cos(theta);
        const float s = std::sin(theta);
        renderer::EffectQuad q{};
        float centre[3];
        float a[3];
        float b[3];
        for (int i = 0; i < 3; ++i) {
            centre[i] = cam[i] + pos[i] * k;
            a[i] = (c * right[i] + 1.0625f * s * down[i]) * h;
            b[i] = (-s * right[i] + 1.0625f * c * down[i]) * h;
        }
        for (int i = 0; i < 3; ++i) {
            q.corner[0][i] = centre[i] + a[i];
            q.corner[1][i] = centre[i] + b[i];
            q.corner[2][i] = centre[i] - b[i];
            q.corner[3][i] = centre[i] - a[i];
        }
        for (std::uint32_t& col : q.rgba) {
            col = rgba;
        }
        const float st[4][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
        std::memcpy(q.st, st, sizeof(st));
        (void)tex0;
        q.tex0 = 1ull << 34;  // TCC, MODULATE; the texture is the sky's own (kSkyTextureKey)
        q.tbp = kSkyTextureKey + r[2];
        q.cbp = 0;
        q.clamp = 5;
        q.tex1 = 0xFF9000000120ull;
        q.alpha = r[3];
        quads.push_back(q);
    }
    if (!quads.empty()) {
        std::stable_sort(
            quads.begin(),
            quads.end(),
            [](const renderer::EffectQuad& a, const renderer::EffectQuad& b) {
                if (a.tbp != b.tbp) {
                    return a.tbp < b.tbp;
                }
                return a.alpha < b.alpha;
            }
        );
        g->effects.insert(g->effects.begin(), quads.begin(), quads.end());
    }
    g->sky_sprites.clear();
}

void particle(
    std::span<const std::uint8_t> ram,
    const std::uint8_t* record,
    std::uint32_t pixels,
    std::uint32_t clut,
    int log2_side
) {
    (void)ram;
    State::Particle p;
    std::memcpy(p.record.data(), record, p.record.size());
    p.pixels = pixels & 0x01FFFFFF;
    p.clut = clut & 0x01FFFFFF;
    p.log2_side = log2_side;
    g->particles.push_back(p);
}

namespace {

// A particle texture's pool key: its 32 x 32 PSMT8 pixels and its 16 x 16 CSM1 palette (both in game
// memory) decoded to RGBA once, in the chip's alpha units (0x80 = opaque), and placed at a key of its
// own, kept while the renderer lives. Earlier the pixels and palette went through the pool as
// uploads at keys handed out per frame in pool order: a key named one texture this frame and
// another the next, and the conversions the pool kept mixed them up (the nanotech crates' orbs as
// white squares). 0 when the addresses are not in memory.
// Refactor note: effect textures without a source in game memory (effect_quad) still go through
// the pool at their GS blocks.
std::uint32_t particle_texture(
    std::span<const std::uint8_t> ram, std::uint32_t pixels, std::uint32_t clut
) {
    constexpr std::uint32_t kParticleTextureKey = 0x30000;  // past the sky's (kSkyTextureKey)
    constexpr std::size_t kSide = 32;
    if (!g->renderer || pixels == 0 || clut == 0 || pixels + kSide * kSide > ram.size()
        || clut + 1024 > ram.size()) {
        return 0;
    }
    if (g->particle_textures_for
        != g->renderer.get()) {  // a new renderer (a level loaded): a new pool
        g->particle_textures.clear();
        g->particle_textures_for = g->renderer.get();
    }
    const std::span<const std::uint8_t> indices = ram.subspan(pixels, kSide * kSide);
    const std::span<const std::uint8_t> colours = ram.subspan(clut, 1024);
    const std::uint64_t id =
        renderer::hash_bytes(indices.data(), indices.size())
        ^ (renderer::hash_bytes(colours.data(), colours.size()) * 0x9E3779B97F4A7C15ull);
    if (const auto found = g->particle_textures.find(id); found != g->particle_textures.end()) {
        return found->second;
    }
    const renderer::gs::Texa texa{0, false, 0x80};
    const std::vector<std::uint32_t> palette = renderer::clut_csm1(colours, 0x00, 16, 256, texa);
    renderer::Rgba8Image image =
        renderer::convert_t8(indices, kSide, kSide, palette, renderer::AlphaScale::Gs);
    const renderer::TextureHandle h = g->renderer->textures().add(
        std::move(image),
        renderer::AlphaScale::Gs,
        std::format("particle {:#x}/{:#x}", pixels, clut)
    );
    const std::uint32_t key =
        kParticleTextureKey + static_cast<std::uint32_t>(g->particle_textures.size());
    g->renderer->textures().place(key, h);
    g->particle_textures.emplace(id, key);
    return key;
}

// The update a particle record's type byte picks (GameProfile::particle_updates): its address and
// name, "?" for a game without the table.
std::string particle_update(std::span<const std::uint8_t> ram, unsigned type) {
    const gaddr table = g->profile->particle_updates();
    if (table == 0) {
        return "?";
    }
    const gaddr at = (OPENRAC_DATA(table) + 4u * type) & 0x01FFFFFFu;
    if (at + 4 > ram.size()) {
        return "?";
    }
    std::uint32_t fn;
    std::memcpy(&fn, ram.data() + at, 4);
    const char* name = openrac_guest_function_name(fn);
    return std::format("{:#x} {}", fn, name != nullptr ? name : "(no C)");
}

// A particle record's depth along the view, as particle_quads culls and fades it by: z from the
// camera to its position (`centre`; a ribbon's, kind 3, the middle of its two ends), and z and the
// record's near and far in 1/4096 units. Outside near to far it is not drawn.
struct ParticleDepth {
    std::array<float, 3> centre;
    float z;
    int z12, near12, far12;

    bool beyond() const { return z12 <= near12 || z12 >= far12; }
};

ParticleDepth particle_depth(const std::uint8_t* r, const host::GameState& state) {
    float p1[3], p2[3];
    std::memcpy(p1, r + 0x10, sizeof(p1));
    std::memcpy(p2, r + 0x20, sizeof(p2));
    std::array<float, 3> centre = {p1[0], p1[1], p1[2]};
    if ((r[1] & 3) == 3) {
        for (int i = 0; i < 3; ++i) {
            centre[i] = (p1[i] + p2[i]) * 0.5f;
        }
    }
    const std::array<float, 3>& cam = state.camera_position;
    const std::array<float, 3>& fwd = state.forward;
    const float z = (centre[0] - cam[0]) * fwd[0] + (centre[1] - cam[1]) * fwd[1]
                    + (centre[2] - cam[2]) * fwd[2];
    return {
        centre,
        z,
        static_cast<int>(z * 4096.0f),
        (r[9] & 0xF) << 10,
        std::min((r[9] >> 4) << 17, 0x1F4000)
    };
}

// Developer tools, for a diagnostics recording: the frame's particles by type, each type with the
// update its record's type byte picks (GameProfile::particle_updates) and how many of them are not
// drawn, and the records within 6 units of Ratchet, whole, in hex.
void note_particles(std::span<const std::uint8_t> ram, const host::GameState& state) {
    const host::LiveMoby* hero = nullptr;
    for (const host::LiveMoby& m : state.mobys) {
        if (m.class_id == 0) {
            hero = &m;
            break;
        }
    }
    const auto callback = [&](unsigned type) {
        return particle_update(ram, type);
    };
    constexpr float kNear = 6.0f;
    std::array<int, 256> counts{};
    // Per type, those particle_quads leaves out: lines (kind 2, not drawn yet), those without a
    // texture, and those outside the record's near and far depth.
    std::array<int, 256> lines{}, untextured{}, beyond{};
    std::string near;
    int listed = 0;
    for (const State::Particle& p : g->particles) {
        const std::uint8_t* r = p.record.data();
        ++counts[r[0]];
        if ((r[1] & 3) == 2) {
            ++lines[r[0]];
        } else if (p.pixels == 0 || p.clut == 0) {
            ++untextured[r[0]];
        } else if (particle_depth(r, state).beyond()) {
            ++beyond[r[0]];
        }
        if (hero == nullptr || listed >= 40) {
            continue;
        }
        float pos[3];
        std::memcpy(pos, r + 0x10, sizeof(pos));
        const float dx = pos[0] - hero->matrix[12], dy = pos[1] - hero->matrix[13],
                    dz = pos[2] - hero->matrix[14];
        if (!(dx * dx + dy * dy + dz * dz < kNear * kNear)) {
            continue;
        }
        std::string hex;
        for (std::size_t i = 0; i < p.record.size(); ++i) {
            hex += std::format("{:02x}", r[i]);
        }
        near += std::format(
            "{}{{\"type\":{},\"pixels\":\"{:x}\",\"clut\":\"{:x}\",\"record\":\"{}\"}}",
            listed ? "," : "",
            r[0],
            p.pixels,
            p.clut,
            hex
        );
        ++listed;
    }
    std::string out = "{\"types\":[";
    bool first = true;
    for (unsigned t = 0; t < counts.size(); ++t) {
        if (counts[t] == 0) {
            continue;
        }
        out += std::format(
            "{}{{\"type\":{},\"count\":{},\"lines\":{},\"untextured\":{},\"beyond\":{},"
            "\"update\":\"{}\"}}",
            first ? "" : ",",
            t,
            counts[t],
            lines[t],
            untextured[t],
            beyond[t],
            callback(t)
        );
        first = false;
    }
    out += "],\"near\":[" + near + "]}";
    effects_dump::particles(std::move(out));
}

}  // namespace

// The frame's particles as effect quads, as the game's VU1 sprite program draws them (ReRAC's
// particle notes, docs/plan/particles.md section 7; ISC License, Copyright (c) 2026 ReRAC
// contributors): culled and faded by depth (the record's near and far, at most 500 units), back to
// front; a sprite faces the camera, its half-diagonal size / 210000 units (y scaled by 1.0625 as the
// game does on screen), turned by its rotation byte; a flat quad lies in the world's XY plane; a
// ribbon joins its two ends across the view. Lines are not drawn yet.
void particle_quads(std::span<const std::uint8_t> ram, const host::GameState& state) {
    const auto f32 = [](const std::uint8_t* b) {
        float v;
        std::memcpy(&v, b, 4);
        return v;
    };
    const auto u32 = [](const std::uint8_t* b) {
        std::uint32_t v;
        std::memcpy(&v, b, 4);
        return v;
    };
    const std::array<float, 3> cam = state.camera_position;
    const std::array<float, 3> right{-state.left[0], -state.left[1], -state.left[2]};
    const std::array<float, 3> down{-state.up[0], -state.up[1], -state.up[2]};

    struct Item {
        float depth;
        renderer::EffectQuad quad;
    };

    std::vector<Item> items;
    // RAM cannot change during extraction. Resolve each visible address pair once,
    // retaining the content cache in particle_texture across passes/animation.
    game::ParticleTextureCache texture_cache;
    static const bool& texture_reference = stats::probe(
        "OPENRAC_VK_PARTICLE_TEXTURE_REUSE_OFF",
        "Reference: hash the texture of every visible particle"
    );
    const bool reuse_textures = gl::is_vulkan() && !texture_reference;
    std::uint64_t texture_resolves = 0;
    const auto resolve_texture = [&](std::uint32_t pixels, std::uint32_t palette) {
        ++texture_resolves;
        return particle_texture(ram, pixels, palette);
    };
    if (effects_dump::armed()) {
        note_particles(ram, state);
    }
    const bool covering = draw_coverage::armed();
    // particle_update's, by type, for OPENRAC_DRAW_COVERAGE.
    std::vector<std::string> updates(covering ? 256 : 0);
    const auto cover = [&](const std::uint8_t* r, std::string_view reason) {
        if (covering) {
            if (updates[r[0]].empty()) {
                updates[r[0]] = particle_update(ram, r[0]);
            }
            draw_coverage::particle(r[0], reason, updates[r[0]]);
        }
    };
    for (const State::Particle& p : g->particles) {
        const std::uint8_t* r = p.record.data();
        const int kind = r[1] & 3;
        if (kind == 2 || p.pixels == 0 || p.clut == 0) {
            cover(r, kind == 2 ? "lines" : "untextured");
            continue;
        }
        std::array<float, 3> p1{f32(r + 0x10), f32(r + 0x14), f32(r + 0x18)};
        std::array<float, 3> p2{f32(r + 0x20), f32(r + 0x24), f32(r + 0x28)};
        const ParticleDepth depth = particle_depth(r, state);
        if (depth.beyond()) {
            cover(r, "beyond");
            continue;
        }
        cover(r, "");
        const float z = depth.z;
        const int fade =
            std::min({(depth.far12 - depth.z12) >> 4, depth.z12 - depth.near12, 0x1000});
        const auto faded = [fade](std::uint32_t c) {
            return (c & 0x00FFFFFFu) | (((c >> 24) * static_cast<std::uint32_t>(fade) >> 12) << 24);
        };
        renderer::EffectQuad q{};
        const std::uint32_t c1 = faded(u32(r + 4));
        for (std::uint32_t& c : q.rgba) {
            c = c1;
        }
        const float st[4][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
        std::memcpy(q.st, st, sizeof(st));
        const float theta = static_cast<float>(r[8]) * 6.2831853f / 256.0f;
        const float c = std::cos(theta);
        const float s = std::sin(theta);
        if (kind == 0) {
            const float h = f32(r + 0xC) / 210000.0f * (z / (z + 0.5f));
            float a[3];
            float b[3];
            for (int i = 0; i < 3; ++i) {
                a[i] = (c * right[i] + 1.0625f * s * down[i]) * h;
                b[i] = (-s * right[i] + 1.0625f * c * down[i]) * h;
            }
            for (int i = 0; i < 3; ++i) {
                q.corner[0][i] = p1[i] + a[i];
                q.corner[1][i] = p1[i] + b[i];
                q.corner[2][i] = p1[i] - b[i];
                q.corner[3][i] = p1[i] - a[i];
            }
        } else if (kind == 1) {
            const float h = f32(r + 0xC) / 420000.0f;
            const float d[4][2] = {{c, s}, {-s, c}, {s, -c}, {-c, -s}};
            for (int k = 0; k < 4; ++k) {
                q.corner[k][0] = p1[0] + d[k][0] * h;
                q.corner[k][1] = p1[1] + d[k][1] * h;
                q.corner[k][2] = p1[2];
            }
        } else {
            const float w1 = f32(r + 0x1C);
            const float k2 = f32(r + 0x2C);
            const float e[3] = {p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]};
            const float m[3] =
                {depth.centre[0] - cam[0], depth.centre[1] - cam[1], depth.centre[2] - cam[2]};
            float n[3] =
                {e[1] * m[2] - e[2] * m[1], e[2] * m[0] - e[0] * m[2], e[0] * m[1] - e[1] * m[0]};
            const float len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            if (len <= 0.0f) {
                continue;
            }
            for (float& v : n) {
                v /= len;
            }
            for (int i = 0; i < 3; ++i) {
                q.corner[0][i] = p1[i] + n[i] * w1;
                q.corner[1][i] = p1[i] - n[i] * w1;
                q.corner[2][i] = p2[i] + n[i] * w1 * k2;
                q.corner[3][i] = p2[i] - n[i] * w1 * k2;
            }
            const std::uint32_t c2 = faded(u32(r + 0xC));
            q.rgba[2] = c2;
            q.rgba[3] = c2;
        }
        // Its texture: 32 x 32 PSMT8 with a 256-colour CSM1 palette, decoded here and kept in the pool
        // at a key of its own (particle_texture).
        q.tbp = reuse_textures ? texture_cache.resolve(p.pixels, p.clut, resolve_texture)
                               : resolve_texture(p.pixels, p.clut);
        q.cbp = 0;
        if (q.tbp == 0) {
            continue;
        }
        q.tex0 = 1ull << 34;  // TCC, MODULATE; the texture is the one placed at q.tbp
        q.clamp = 5;
        q.tex1 = 0xFF9000000120ull;
        q.alpha = r[3];
        items.push_back({z, q});
    }
    stats::gauge("particle texture resolves", double(texture_resolves));
    stats::gauge("particle texture reuse", reuse_textures ? 1 : 0);
    std::stable_sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        return a.depth > b.depth;
    });
    for (const Item& it : items) {
        g->effects.push_back(it.quad);
    }
    g->particles.clear();
    g->sky_sprites.clear();
}

void moby_colour(std::uint32_t object, unsigned colour, int level) {
    if (!g) {
        return;
    }
    g->scripted_moby_colours.set(level, object, colour);
}

int ui_quad(const openrac_game_ui_rect& quad) {
    // The 2D layer's quads are drawn natively (on the way off the GS packets); OPENRAC_UI_REPLAY
    // replays the game's packets instead, for comparing.
    static const bool native = !env::flag(env::Var::UiReplay);
    if (!g || !native || g->ui_quads.size() >= 0xFFFFFFu) {  // the marker's index is 24 bits
        return -1;
    }
    renderer::UiQuad q;
    for (int i = 0; i < 4; ++i) {
        q.x[i] = quad.x[i];
        q.y[i] = quad.y[i];
        q.u[i] = quad.u[i];
        q.v[i] = quad.v[i];
    }
    q.z = quad.z;
    q.rgba = quad.rgba;
    q.tex0 = quad.tex0;
    q.flags = quad.flags;
    g->ui_quads.push_back(q);
    return static_cast<int>((g->ui_quads.size() - 1) | (g->ui_generation & 0x3F) << 24);
}

}  // namespace openrac::frontend

// Called on the game thread: disable legacy displacement only with a working replacement.
extern "C" int openrac_game_enhanced_water(void) {
    using namespace openrac;
    return c_boundary::call("openrac_game_enhanced_water", 0, [] {
        return int(
            frontend::g && frontend::g->game.starts_with("rac1-") && frontend::g->loaded == 1
            && frontend::enhanced_graphics_available() && settings::get().enhanced_graphics
            && enhanced::water_available()
        );
    });
}
