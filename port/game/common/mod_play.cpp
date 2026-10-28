// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' play calls (mod_play.h, interface 1.8).

#include "mod_play.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "common/log.h"
#include "mods.h"

namespace openrac::mods {

static_assert(OPENRAC_PLAY_NONE == OPENRAC_GAME_PHASE_NONE);
static_assert(OPENRAC_PLAY_FRONT_END == OPENRAC_GAME_PHASE_FRONT_END);
static_assert(OPENRAC_PLAY_LOADING == OPENRAC_GAME_PHASE_LOADING);
static_assert(OPENRAC_PLAY_PLAYING == OPENRAC_GAME_PHASE_PLAYING);
static_assert(OPENRAC_PLAY_SCENE == OPENRAC_GAME_PHASE_SCENE);
static_assert(OPENRAC_PLAY_MENU == OPENRAC_GAME_PHASE_MENU);
// A pose is the game's after its size: copied as one block.
static_assert(offsetof(openrac_pose, level) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_pose) == sizeof(std::uint32_t) + sizeof(openrac_game_pose));

namespace {

// A save file is read whole: none of a game's is larger.
constexpr std::uintmax_t kSaveMax = 1u << 20;

// A figure a mod made: whose, its moby, the level load it was made in.
struct Figure {
    const openrac_mod* mod = nullptr;
    gaddr moby = 0;
    std::uint32_t loads = 0;
};

// A start or travel a mod asked for, done at the frame's end (play_update).
struct Request {
    enum class Kind {
        kNone,
        kStart,
        kTravel
    };
    Kind kind = Kind::kNone;
    const openrac_mod* mod = nullptr;
    int level = 0;
    std::vector<std::uint8_t> save;
};

const openrac_game_play* g_play = nullptr;
std::filesystem::path g_cards;
std::array<Figure, OPENRAC_FIGURES_MAX> g_figures;
Request g_request;

int phase_now(std::int32_t* level) {
    *level = -1;
    return g_play != nullptr ? g_play->phase(level) : OPENRAC_PLAY_NONE;
}

bool in_level(int phase) {
    return phase == OPENRAC_PLAY_PLAYING || phase == OPENRAC_PLAY_SCENE
           || phase == OPENRAC_PLAY_MENU;
}

// Whether a figure is there still: made, in this level load, its moby not taken back by the game.
bool live(const Figure& f) {
    return f.mod != nullptr && g_play != nullptr && f.loads == g_play->loads()
           && g_play->figure_alive(f.moby) != 0;
}

// The mod's figure `id`, null for one that is not the mod's; one gone with its level is let go.
Figure* own_figure(const openrac_mod* mod, int id) {
    if (mod == nullptr || id < 0 || id >= static_cast<int>(g_figures.size())) {
        return nullptr;
    }
    Figure& f = g_figures[static_cast<std::size_t>(id)];
    if (f.mod != mod) {
        return nullptr;
    }
    if (!live(f)) {
        f = Figure{};
        return nullptr;
    }
    return &f;
}

// The bytes of the memory card's save `slot`; empty for none there (or past the last slot).
std::vector<std::uint8_t> read_save(int slot) {
    if (g_play == nullptr || g_play->save_folder == nullptr || g_play->save_file == nullptr
        || slot < 0 || slot >= g_play->save_slots) {
        return {};
    }
    char name[64];
    std::snprintf(name, sizeof name, g_play->save_file, slot);
    const std::filesystem::path path = g_cards / g_play->save_folder / name;
    std::error_code e;
    const std::uintmax_t size = std::filesystem::file_size(path, e);
    if (e || size == 0 || size > kSaveMax) {
        return {};
    }
    std::ifstream in(path, std::ios::binary);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size))) {
        return {};
    }
    return bytes;
}

// What a save's bytes hold; false for one the game would not load.
bool save_info(const std::uint8_t* save, std::size_t size, openrac_save_info* out) {
    if (g_play == nullptr || save == nullptr || size > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    std::int32_t level = 0, bolts = 0, gold = 0, seconds = 0;
    if (g_play->save_info(save, static_cast<std::uint32_t>(size), &level, &bolts, &gold, &seconds)
        == 0) {
        return false;
    }
    if (out != nullptr) {
        out->level = level;
        out->bolts = bolts;
        out->gold_bolts = gold;
        out->seconds = seconds;
    }
    return true;
}

int api_play_status(openrac_play_status* out) {
    if (out == nullptr || out->size < sizeof(openrac_play_status)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->phase = phase_now(&out->level);
    out->levels = g_play != nullptr ? g_play->levels : 0;
    out->loads = g_play != nullptr ? g_play->loads() : 0;
    return g_play != nullptr ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

const char* api_level_name(int level) {
    if (g_play == nullptr || g_play->level_names == nullptr || level < 0
        || level >= g_play->levels) {
        return nullptr;
    }
    return g_play->level_names[level];
}

int api_hero_pose(openrac_pose* out) {
    if (out == nullptr || out->size < sizeof(openrac_pose) || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose pose{};
    if (g_play->hero_pose(&pose) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(&out->level, &pose, sizeof pose);
    return OPENRAC_HOOK_OK;
}

int api_figure_create(openrac_mod* mod) {
    std::int32_t level = -1;
    if (mod == nullptr || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // Figures gone with an earlier level make room.
    for (Figure& f : g_figures) {
        if (f.mod != nullptr && !live(f)) {
            f = Figure{};
        }
    }
    for (std::size_t i = 0; i < g_figures.size(); ++i) {
        if (g_figures[i].mod != nullptr) {
            continue;
        }
        const gaddr moby = g_play->figure_spawn();
        // The level's pool is full.
        if (moby == 0) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        g_figures[i] = {mod, moby, g_play->loads()};
        return static_cast<int>(i);
    }
    return OPENRAC_HOOK_UNKNOWN;
}

int api_figure_pose(openrac_mod* mod, int id, const openrac_pose* pose) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || pose == nullptr || pose->size < sizeof(openrac_pose)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose p{};
    std::memcpy(&p, &pose->level, sizeof p);
    g_play->figure_pose(f->moby, &p);
    return OPENRAC_HOOK_OK;
}

int api_figure_remove(openrac_mod* mod, int id) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->figure_remove(f->moby);
    *f = Figure{};
    return OPENRAC_HOOK_OK;
}

int api_play_start(openrac_mod* mod, int level, const void* save, size_t size) {
    std::int32_t now = -1;
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_request.kind != Request::Kind::kNone) {
        return OPENRAC_HOOK_TAKEN;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(save);
    // The front end, a level the game has, a save it takes.
    if (phase_now(&now) != OPENRAC_PLAY_FRONT_END || level >= g_play->levels
        || (bytes != nullptr && !save_info(bytes, size, nullptr))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_request = {Request::Kind::kStart, mod, level, {}};
    if (bytes != nullptr) {
        g_request.save.assign(bytes, bytes + size);
    }
    return OPENRAC_HOOK_OK;
}

int api_play_travel(openrac_mod* mod, int level) {
    std::int32_t now = -1;
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_request.kind != Request::Kind::kNone) {
        return OPENRAC_HOOK_TAKEN;
    }
    if (phase_now(&now) != OPENRAC_PLAY_PLAYING || level < 0 || level >= g_play->levels) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_request = {Request::Kind::kTravel, mod, level, {}};
    return OPENRAC_HOOK_OK;
}

size_t api_save_capture(void* out, size_t size) {
    if (g_play == nullptr) {
        return 0;
    }
    const auto room = static_cast<std::uint32_t>(
        std::min<size_t>(size, std::numeric_limits<std::uint32_t>::max())
    );
    return g_play->save_capture(out != nullptr ? static_cast<std::uint8_t*>(out) : nullptr, room);
}

int api_save_slot(int slot, openrac_save_info* out) {
    if (g_play == nullptr || slot < 0 || slot >= g_play->save_slots) {
        return -1;
    }
    if (out == nullptr || out->size < sizeof(openrac_save_info)) {
        return 0;
    }
    const std::vector<std::uint8_t> bytes = read_save(slot);
    return save_info(bytes.data(), bytes.size(), out) ? 1 : 0;
}

size_t api_save_read(int slot, void* out, size_t size) {
    const std::vector<std::uint8_t> bytes = read_save(slot);
    if (bytes.empty() || !save_info(bytes.data(), bytes.size(), nullptr)) {
        return 0;
    }
    if (out != nullptr && size >= bytes.size()) {
        std::memcpy(out, bytes.data(), bytes.size());
    }
    return bytes.size();
}

}  // namespace

void fill_play_api(
    openrac_api& api, const openrac_game_play* play, const std::filesystem::path& cards
) {
    g_play = play;
    g_cards = cards;
    api.play_status = api_play_status;
    api.level_name = api_level_name;
    api.hero_pose = api_hero_pose;
    api.figure_create = api_figure_create;
    api.figure_pose = api_figure_pose;
    api.figure_remove = api_figure_remove;
    api.play_start = api_play_start;
    api.play_travel = api_play_travel;
    api.save_capture = api_save_capture;
    api.save_slot = api_save_slot;
    api.save_read = api_save_read;
}

void play_update() {
    if (g_request.kind == Request::Kind::kNone || g_play == nullptr) {
        return;
    }
    Request r = std::move(g_request);
    g_request = Request{};
    std::int32_t level = -1;
    const int phase = phase_now(&level);
    // Asked in the phase it needs; the game moved on since (a load began): nothing.
    if (r.kind == Request::Kind::kStart && phase == OPENRAC_PLAY_FRONT_END) {
        const bool started = g_play->start(
                                 r.level,
                                 r.save.empty() ? nullptr : r.save.data(),
                                 static_cast<std::uint32_t>(r.save.size())
                             )
                             != 0;
        log::info(
            "mods: {} {} {}",
            mod_id(r.mod),
            started ? "started" : "could not start",
            r.save.empty() ? "a new game in level " + std::to_string(r.level)
                           : "a save" + (r.level >= 0 ? " in level " + std::to_string(r.level) : "")
        );
    } else if (r.kind == Request::Kind::kTravel && phase == OPENRAC_PLAY_PLAYING) {
        const bool going = g_play->travel(r.level) != 0;
        log::info(
            "mods: {} {} level {}",
            mod_id(r.mod),
            going ? "travels to" : "could not travel to",
            r.level
        );
    }
}

void reset_play() {
    g_figures.fill(Figure{});
    g_request = Request{};
}

void drop_figures(const openrac_mod* mod) {
    for (Figure& f : g_figures) {
        if (f.mod == mod) {
            if (live(f)) {
                g_play->figure_remove(f.moby);
            }
            f = Figure{};
        }
    }
    if (g_request.mod == mod) {
        g_request = Request{};
    }
}

}  // namespace openrac::mods
