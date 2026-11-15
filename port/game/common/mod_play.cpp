// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' play calls (mod_play.h, interface 1.8).

#include "mod_play.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <mutex>
#include <string>
#include <vector>

#include "common/log.h"
#include "mod_touch.h"
#include "mods.h"

// The game's calls into the mods' world (game_host.h), set while mods are loaded.
int (*openrac_game_moby_update)(std::int32_t slot, int phase) = nullptr;
int (*openrac_game_metal_detect)(std::int32_t slot, std::int32_t key) = nullptr;
int (*openrac_game_moby_hit)(
    std::int32_t victim,
    std::int32_t attacker,
    std::uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
) = nullptr;

namespace openrac::mods {

static_assert(OPENRAC_PLAY_NONE == OPENRAC_GAME_PHASE_NONE);
static_assert(OPENRAC_PLAY_FRONT_END == OPENRAC_GAME_PHASE_FRONT_END);
static_assert(OPENRAC_PLAY_LOADING == OPENRAC_GAME_PHASE_LOADING);
static_assert(OPENRAC_PLAY_PLAYING == OPENRAC_GAME_PHASE_PLAYING);
static_assert(OPENRAC_PLAY_SCENE == OPENRAC_GAME_PHASE_SCENE);
static_assert(OPENRAC_PLAY_MENU == OPENRAC_GAME_PHASE_MENU);
static_assert(OPENRAC_SHARED_GOLD_WEAPONS == OPENRAC_GAME_SHARED_GOLD_WEAPONS);
static_assert(OPENRAC_SHARED_SKILL_POINTS == OPENRAC_GAME_SHARED_SKILL_POINTS);
static_assert(OPENRAC_SHARED_LEVEL_FLAGS == OPENRAC_GAME_SHARED_LEVEL_FLAGS);
static_assert(OPENRAC_SHARED_SPECIAL_ITEMS == OPENRAC_GAME_SHARED_SPECIAL_ITEMS);
// A pose is the game's after its size: copied as one block.
static_assert(offsetof(openrac_pose, level) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_pose) == sizeof(std::uint32_t) + sizeof(openrac_game_pose));
// So are the held pieces and a shot.
static_assert(OPENRAC_HELD_MAX == OPENRAC_GAME_HELD_MAX);
static_assert(sizeof(openrac_held_piece) == sizeof(openrac_game_held_piece));
static_assert(offsetof(openrac_held, count) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_held) == sizeof(std::uint32_t) + sizeof(openrac_game_held));
static_assert(offsetof(openrac_shot, item) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_shot) == sizeof(std::uint32_t) + sizeof(openrac_game_shot));
static_assert(offsetof(openrac_shot2, item) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_shot2) == sizeof(std::uint32_t) + sizeof(openrac_game_shot2));
static_assert(OPENRAC_SHOT_START == OPENRAC_GAME_SHOT_START);
static_assert(OPENRAC_SHOT_MOVE == OPENRAC_GAME_SHOT_MOVE);
static_assert(OPENRAC_SHOT_STOP == OPENRAC_GAME_SHOT_STOP);
static_assert(OPENRAC_SHOT_BURST == OPENRAC_GAME_SHOT_BURST);
// So is a moby (interface 1.9).
static_assert(offsetof(openrac_moby, slot) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_moby) == sizeof(std::uint32_t) + sizeof(openrac_game_moby_state));
static_assert(offsetof(openrac_moby_run, runs) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_moby_run) == sizeof(std::uint32_t) + sizeof(openrac_game_moby_run));
static_assert(offsetof(openrac_moby_deletion, by) == sizeof(std::uint32_t));
static_assert(
    sizeof(openrac_moby_deletion) == sizeof(std::uint32_t) + sizeof(openrac_game_moby_deletion)
);

// The collision triangles are the game's records as they are (interface 1.12).
static_assert(sizeof(openrac_collision_triangle) == 14 * sizeof(float));

namespace {

// A save file is read whole: none of a game's is larger.
constexpr std::uintmax_t kSaveMax = 1u << 20;
// The most sounds the mods load together (OPENRAC_SOUNDS_MAX each).
constexpr std::size_t kSoundsMax = 4 * OPENRAC_SOUNDS_MAX;

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

// The world (interface 1.9): the mobys the mods drive, by slot (whose, in which level load), the
// gate and the hit watcher (one mod's each), and the slot whose update the gate's call before it
// is about (-1: none, where a stand-in is refused).
constexpr std::size_t kSlots = 4096;

struct Drive {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
};

struct Gate {
    const openrac_mod* mod = nullptr;
    openrac_moby_update_fn fn = nullptr;
    void* user = nullptr;
};

struct HitWatch {
    const openrac_mod* mod = nullptr;
    openrac_hit_fn fn = nullptr;
    void* user = nullptr;
};

std::vector<Drive> g_drives(kSlots);
Gate g_gate;

// The mod told of the metal detector's digs.
struct MetalWatch {
    openrac_mod* mod = nullptr;
    openrac_metal_detect_fn fn = nullptr;
    void* user = nullptr;
};

MetalWatch g_metal;
HitWatch g_hits;
int g_before_slot = -1;

// The mod that holds the card filter, what it asked, and the card folder its writes go to.
const openrac_mod* g_card_mod = nullptr;
int g_card_mode = OPENRAC_CARD_PASS;
std::string g_card_folder;
// A mod's own card folder's name at most, and the note beside a save (card_note_write).
constexpr std::size_t kCardFolderMost = 16;
constexpr std::size_t kCardNoteMost = 1024;
const openrac_mod* g_loading_mod = nullptr;

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

// The card folder a mod's folder name means: the game's own for "", else the game's own with the
// name after it ("coop": BESCES-50916RATCHET-coop). Empty for a name that is not one word, so a mod
// reaches nothing outside the card.
std::string card_folder(const char* name) {
    if (g_play == nullptr || g_play->save_folder == nullptr || name == nullptr) {
        return {};
    }
    const std::string word = name;
    if (word.size() > kCardFolderMost
        || !std::all_of(word.begin(), word.end(), [](unsigned char c) {
               return std::isalnum(c) != 0 || c == '_';
           })) {
        return {};
    }
    return word.empty() ? std::string(g_play->save_folder) : g_play->save_folder + ("-" + word);
}

// The path of save `slot` in the card folder `folder`; empty past the last slot.
std::filesystem::path save_path(const std::string& folder, int slot) {
    if (g_play == nullptr || g_play->save_file == nullptr || folder.empty() || slot < 0
        || slot >= g_play->save_slots) {
        return {};
    }
    char name[64];
    std::snprintf(name, sizeof name, g_play->save_file, slot);
    return g_cards / folder / name;
}

// The bytes of the file at `path`; empty for none.
std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
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

// The bytes of save `slot` of the card folder `folder`; empty for none there. A write stopped
// between its two renames (lib/mc.c) leaves the save before it beside it.
std::vector<std::uint8_t> read_save(const std::string& folder, int slot) {
    const std::filesystem::path path = save_path(folder, slot);
    if (path.empty()) {
        return {};
    }
    std::vector<std::uint8_t> bytes = read_file(path);
    if (bytes.empty() && folder != g_play->save_folder) {
        std::filesystem::path previous = path;
        previous += ".bak";
        bytes = read_file(previous);
    }
    return bytes;
}

std::vector<std::uint8_t> read_save(int slot) {
    return g_play != nullptr && g_play->save_folder != nullptr
               ? read_save(g_play->save_folder, slot)
               : std::vector<std::uint8_t>{};
}

// The note beside a save in a mod's folder (card_note_write).
std::filesystem::path note_path(const std::string& folder, int slot) {
    std::filesystem::path path = save_path(folder, slot);
    if (!path.empty()) {
        path += ".party";
    }
    return path;
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

// A level's phases, where a travel may leave it from (1.9: its scenes and menus too, as the pause
// menu's Quit Game leaves from the menu: a party's member goes with its host from anywhere).
bool in_a_level(int phase) {
    return phase == OPENRAC_PLAY_PLAYING || phase == OPENRAC_PLAY_SCENE
           || phase == OPENRAC_PLAY_MENU;
}

int api_play_travel(openrac_mod* mod, int level) {
    std::int32_t now = -1;
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_request.kind != Request::Kind::kNone) {
        return OPENRAC_HOOK_TAKEN;
    }
    if (!in_a_level(phase_now(&now)) || level < -1 || level >= g_play->levels) {
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

int api_save_slot_current(void) {
    return g_play != nullptr && g_play->save_slot_current != nullptr ? g_play->save_slot_current()
                                                                     : -1;
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

// Whether a mod drives the moby in `slot` in this level load (whose: `mod` when given).
const openrac_mod* driver(int slot) {
    if (g_play == nullptr || slot < 0 || static_cast<std::size_t>(slot) >= kSlots) {
        return nullptr;
    }
    const Drive& d = g_drives[static_cast<std::size_t>(slot)];
    return d.mod != nullptr && d.loads == g_play->loads() ? d.mod : nullptr;
}

openrac_game_pose pose_of(const openrac_moby& m) {
    openrac_game_pose p{};
    std::copy(m.position, m.position + 3, p.position);
    std::copy(m.rotation, m.rotation + 3, p.rotation);
    p.scale = m.scale;
    p.seq = m.seq;
    p.frame = m.frame;
    p.next_frame = m.next_frame;
    p.anim_time = m.anim_time;
    p.anim_speed = m.anim_speed;
    p.item = -1;
    return p;
}

int api_moby_list(openrac_moby* out, int max) {
    if (g_play == nullptr || g_play->mobys == nullptr || max < 0) {
        return 0;
    }
    std::vector<openrac_game_moby_state> states(static_cast<std::size_t>(max));
    const int n = g_play->mobys(states.data(), max);
    for (int i = 0; i < std::min(n, max) && out != nullptr; ++i) {
        out[i].size = sizeof(openrac_moby);
        std::memcpy(
            &out[i].slot, &states[static_cast<std::size_t>(i)], sizeof(openrac_game_moby_state)
        );
        if (driver(out[i].slot) != nullptr) {
            out[i].flags |= OPENRAC_MOBY_DRIVEN;
        }
    }
    return n;
}

int api_moby_slot(int key) {
    return g_play != nullptr && g_play->moby_slot != nullptr ? g_play->moby_slot(key) : -1;
}

gaddr moby_address(int slot) {
    return g_play != nullptr && g_play->moby_at != nullptr ? g_play->moby_at(slot) : 0;
}

int api_moby_drive(openrac_mod* mod, int slot, int on) {
    if (mod == nullptr || moby_address(slot) == 0 || static_cast<std::size_t>(slot) >= kSlots) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const openrac_mod* now = driver(slot);
    if (now != nullptr && now != mod) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_drives[static_cast<std::size_t>(slot)] = on != 0 ? Drive{mod, g_play->loads()} : Drive{};
    return OPENRAC_HOOK_OK;
}

int api_moby_pose(openrac_mod* mod, int slot, const openrac_moby* pose) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0 || pose == nullptr || pose->size < sizeof(openrac_moby)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const openrac_game_pose p = pose_of(*pose);
    g_play->moby_pose(m, &p);
    return OPENRAC_HOOK_OK;
}

int api_moby_kill(openrac_mod* mod, int slot) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->moby_kill(m);
    if (static_cast<std::size_t>(slot) < kSlots) {
        g_drives[static_cast<std::size_t>(slot)] = Drive{};
    }
    return OPENRAC_HOOK_OK;
}

int api_moby_spawn(openrac_mod* mod, int oclass, const openrac_moby* pose) {
    if (mod == nullptr || g_play == nullptr || g_play->moby_spawn == nullptr || pose == nullptr
        || pose->size < sizeof(openrac_moby)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int slot = g_play->moby_spawn(oclass);
    if (slot < 0 || static_cast<std::size_t>(slot) >= kSlots) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_drives[static_cast<std::size_t>(slot)] = Drive{mod, g_play->loads()};
    api_moby_pose(mod, slot, pose);
    return slot;
}

int api_figure_slot(openrac_mod* mod, int id) {
    if (g_play == nullptr || g_play->hero_slot == nullptr) {
        return -1;
    }
    if (id < 0) {
        return g_play->hero_slot();
    }
    const Figure* f = own_figure(mod, id);
    return f != nullptr ? g_play->slot_of(f->moby) : -1;
}

int api_on_moby_update(openrac_mod* mod, openrac_moby_update_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_gate.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_gate = {mod, fn, user};
    return OPENRAC_HOOK_OK;
}

int api_hero_stand_in(openrac_mod* mod, const float position[3], int figure) {
    if (mod == nullptr || mod != g_gate.mod || g_before_slot < 0 || position == nullptr
        || g_play == nullptr || g_play->stand_in == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const Figure* f = figure >= 0 ? own_figure(mod, figure) : nullptr;
    g_play->stand_in(position, f != nullptr ? f->moby : 0);
    return OPENRAC_HOOK_OK;
}

int api_on_moby_hit(openrac_mod* mod, openrac_hit_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_hits.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_hits = {mod, fn, user};
    return OPENRAC_HOOK_OK;
}

int api_moby_hit(openrac_mod* mod, const openrac_hit* hit) {
    const gaddr victim = hit != nullptr ? moby_address(hit->victim) : 0;
    if (mod == nullptr || victim == 0 || hit->size < sizeof(openrac_hit)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->post_hit(
        victim, moby_address(hit->attacker), hit->mask, hit->damage, hit->position, hit->velocity
    );
    return OPENRAC_HOOK_OK;
}

int api_play_eyes(openrac_mod* mod, const float* xyz, int count) {
    if (mod == nullptr || g_play == nullptr || g_play->eyes == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->eyes(xyz, xyz != nullptr ? std::clamp(count, 0, OPENRAC_EYES_MAX) : 0);
    return OPENRAC_HOOK_OK;
}

int api_play_overlay_pause(openrac_mod* mod, int on) {
    if (mod == nullptr || g_play == nullptr || g_play->keep_running == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->keep_running(on != 0 ? 1 : 0);
    return OPENRAC_HOOK_OK;
}

int api_moby_slot_of(openrac_gaddr moby) {
    return g_play != nullptr && g_play->slot_of != nullptr
                   && moby_address(g_play->slot_of(moby)) == moby
               ? g_play->slot_of(moby)
               : -1;
}

// A skip a mod asked for: the ticks it is pressed for still (play_skipping), and what it was asked
// in (the phase, the level's scene: -1 none), so it never presses into the next scene or movie.
constexpr int kSkipTicks = 40;
int g_skip_ticks = 0;
int g_skip_phase = OPENRAC_PLAY_NONE;
std::int32_t g_skip_scene = -1;

std::int32_t scene_now() {
    std::int32_t played = 0;
    return g_play != nullptr && g_play->scene_playing != nullptr ? g_play->scene_playing(&played)
                                                                 : -1;
}

int api_play_skip(openrac_mod* mod) {
    if (mod == nullptr || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::int32_t level = -1;
    g_skip_ticks = kSkipTicks;
    g_skip_phase = phase_now(&level);
    g_skip_scene = scene_now();
    return OPENRAC_HOOK_OK;
}

int api_bolts(int add) {
    return g_play != nullptr && g_play->bolts != nullptr ? g_play->bolts(add) : 0;
}

int api_hero_held(openrac_held* out) {
    if (out == nullptr || out->size < sizeof(openrac_held) || g_play == nullptr
        || g_play->hero_held == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_held held{};
    if (g_play->hero_held(&held) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(&out->count, &held, sizeof held);
    return OPENRAC_HOOK_OK;
}

int api_figure_held(openrac_mod* mod, int id, const openrac_held* held) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || held == nullptr || held->size < sizeof(openrac_held)
        || g_play->figure_held == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_held h{};
    std::memcpy(&h, &held->count, sizeof h);
    h.count = std::clamp<std::int32_t>(h.count, 0, OPENRAC_GAME_HELD_MAX);
    g_play->figure_held(f->moby, &h);
    return OPENRAC_HOOK_OK;
}

int api_hero_shots(openrac_shot* out, int max) {
    if (out == nullptr || max <= 0 || g_play == nullptr || g_play->hero_shots == nullptr) {
        return 0;
    }
    std::array<openrac_game_shot, OPENRAC_SHOTS_MAX> shots{};
    const int n = g_play->hero_shots(shots.data(), std::min(max, OPENRAC_SHOTS_MAX));
    for (int i = 0; i < n; ++i) {
        out[i].size = sizeof(openrac_shot);
        std::memcpy(&out[i].item, &shots[static_cast<std::size_t>(i)], sizeof(openrac_game_shot));
    }
    return n;
}

int api_figure_shot(openrac_mod* mod, int id, const openrac_shot* shot) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || shot == nullptr || shot->size < sizeof(openrac_shot)
        || g_play->figure_shot == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_shot s{};
    std::memcpy(&s, &shot->item, sizeof s);
    return g_play->figure_shot(f->moby, &s) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// One line over the game's picture, the last mod's to set it: set on the game's thread, drawn on
// the window's.
std::mutex g_notice_lock;
std::string g_notice;

int api_play_notice(openrac_mod* mod, const char* text) {
    if (mod == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_notice_lock);
    g_notice = text != nullptr ? text : "";
    return OPENRAC_HOOK_OK;
}

// The level's scenes: the one playing, and one to start as the level's own trigger does (the game
// starts it once the player is in control; -1 drops a request still waiting).
int api_scene_playing(int* frame) {
    std::int32_t played = 0;
    const std::int32_t scene =
        g_play != nullptr && g_play->scene_playing != nullptr ? g_play->scene_playing(&played) : -1;
    if (frame != nullptr) {
        *frame = scene >= 0 ? played : 0;
    }
    return scene >= 0 ? scene : -1;
}

int api_play_scene(openrac_mod* mod, int scene) {
    if (mod == nullptr || g_play == nullptr || g_play->scene_start == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->scene_start(scene < 0 ? -1 : scene) != 0 ? OPENRAC_HOOK_OK
                                                            : OPENRAC_HOOK_UNKNOWN;
}

int api_moby_hide(openrac_mod* mod, int slot, int hidden) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0 || g_play->moby_hide == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->moby_hide(m, hidden != 0 ? 1 : 0);
    return OPENRAC_HOOK_OK;
}

int api_moby_deletion(int slot, openrac_moby_deletion* out) {
    if (out == nullptr || out->size < sizeof(openrac_moby_deletion) || g_play == nullptr
        || g_play->moby_deletion == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_moby_deletion d{};
    if (g_play->moby_deletion(slot, &d) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->size = sizeof(openrac_moby_deletion);
    std::memcpy(&out->by, &d, sizeof d);
    return OPENRAC_HOOK_OK;
}

int api_moby_run(int slot, openrac_moby_run* out) {
    if (out == nullptr || out->size < sizeof(openrac_moby_run) || g_play == nullptr
        || g_play->moby_run == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_moby_run r{};
    if (g_play->moby_run(slot, &r) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->size = sizeof(openrac_moby_run);
    std::memcpy(&out->runs, &r, sizeof r);
    return OPENRAC_HOOK_OK;
}

// ---- Interface 1.14: the calls a shared game needs. Each answers "unknown" in a game that has not
// got it (play_scene_from always, for now), so a mod tests for it before using it. ----

int api_card_filter(openrac_mod* mod, int mode, const char* folder) {
    if (mod == nullptr || g_play == nullptr || g_play->card_session == nullptr
        || g_play->card_filter == nullptr || (g_card_mod != nullptr && g_card_mod != mod)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (mode == OPENRAC_CARD_PASS) {
        // Only once no write of the party's can reach the player's saves: the mod asks again.
        if (g_card_mode != OPENRAC_CARD_PASS && g_play->card_session(0) == 0) {
            return OPENRAC_HOOK_TAKEN;
        }
        g_play->card_filter(OPENRAC_CARD_PASS, "");
        g_card_mod = nullptr;
        g_card_mode = OPENRAC_CARD_PASS;
        g_card_folder.clear();
        return OPENRAC_HOOK_OK;
    }
    const std::string named = mode == OPENRAC_CARD_REDIRECT ? card_folder(folder) : std::string();
    if ((mode == OPENRAC_CARD_REDIRECT && (named.empty() || named == g_play->save_folder))
        || g_play->card_filter(mode, named.c_str()) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_card_mod = mod;
    g_card_mode = mode;
    g_card_folder = named;
    g_play->card_session(1);
    log::info("mods: {} keeps the game's saves off the player's card", mod_id(mod));
    return OPENRAC_HOOK_OK;
}

int api_card_slot_hint(openrac_mod* mod, int slot) {
    if (g_play == nullptr || mod != g_card_mod || slot < -1 || slot >= g_play->save_slots) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->card_slot(slot);
    return OPENRAC_HOOK_OK;
}

int api_coop_save_write(openrac_mod* mod, int slot, const void* bytes, std::uint32_t size) {
    // Bytes from anywhere, another player's game among them: only what the game would read.
    if (g_play == nullptr || g_play->save_check == nullptr || mod != g_card_mod
        || g_card_mode != OPENRAC_CARD_REDIRECT || slot < 0 || slot >= g_play->save_slots
        || bytes == nullptr || size == 0 || size > kSaveMax
        || g_play->save_check(static_cast<const std::uint8_t*>(bytes), size) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->card_write(slot, static_cast<const std::uint8_t*>(bytes), size) != 0
               ? OPENRAC_HOOK_OK
               : OPENRAC_HOOK_UNKNOWN;
}

int api_save_slot2(const char* folder, int slot, openrac_save_info* out) {
    if (g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (slot < 0 || slot >= g_play->save_slots) {
        return -1;
    }
    if (out == nullptr || out->size < sizeof(openrac_save_info)) {
        return 0;
    }
    const std::vector<std::uint8_t> bytes = read_save(card_folder(folder), slot);
    return save_info(bytes.data(), bytes.size(), out) ? 1 : 0;
}

std::size_t api_save_read2(const char* folder, int slot, void* out, std::size_t size) {
    const std::vector<std::uint8_t> bytes = read_save(card_folder(folder), slot);
    if (bytes.empty() || !save_info(bytes.data(), bytes.size(), nullptr)) {
        return 0;
    }
    if (out != nullptr && size >= bytes.size()) {
        std::memcpy(out, bytes.data(), bytes.size());
    }
    return bytes.size();
}

std::uint32_t api_card_saves() {
    return g_play != nullptr && g_play->card_saves != nullptr ? g_play->card_saves() : 0;
}

int api_card_note_write(openrac_mod* mod, int slot, const char* text) {
    const std::filesystem::path path = mod != nullptr && mod == g_card_mod
                                           ? note_path(g_card_folder, slot)
                                           : std::filesystem::path{};
    if (path.empty() || text == nullptr || std::strlen(text) >= kCardNoteMost) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // A note half written would lose a party its slot: a whole one or the one before.
    std::error_code e;
    std::filesystem::create_directories(path.parent_path(), e);
    std::filesystem::path temp = path;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!(out << text)) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    std::filesystem::rename(temp, path, e);
    return e ? OPENRAC_HOOK_UNKNOWN : OPENRAC_HOOK_OK;
}

int api_card_note_read(const char* folder, int slot, char* out, std::uint32_t size) {
    const std::string named = card_folder(folder);
    if (out == nullptr || size == 0 || named.empty() || named == g_play->save_folder) {
        return 0;
    }
    // (card_folder is empty without a game, so g_play is there.)
    std::ifstream in(note_path(named, slot), std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.empty() || text.size() >= size) {
        return 0;
    }
    std::memcpy(out, text.c_str(), text.size() + 1);
    return static_cast<int>(text.size());
}

int api_shared_read(int tag, void* out, std::uint32_t size) {
    if (g_play == nullptr || g_play->shared_read == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return out != nullptr ? g_play->shared_read(tag, out, size) : -1;
}

int api_shared_give(int tag, const void* data, std::uint32_t size) {
    if (g_play == nullptr || g_play->shared_give == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return data != nullptr && g_play->shared_give(tag, data, size) != 0 ? OPENRAC_HOOK_OK
                                                                        : OPENRAC_HOOK_UNKNOWN;
}

int api_mission_state_read(int level, std::uint8_t* out) {
    if (g_play == nullptr || g_play->mission_state_read == nullptr || out == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return g_play->mission_state_read(level, out) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_mission_state_write(int level, const std::uint8_t* in) {
    if (g_play == nullptr || g_play->mission_state_write == nullptr || in == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return g_play->mission_state_write(level, in) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_mission_state_clear(int level, const std::uint8_t* flags) {
    if (g_play == nullptr || g_play->mission_state_clear == nullptr || flags == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return g_play->mission_state_clear(level, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_place(openrac_mod* mod, const float position[3], float yaw, std::uint32_t flags) {
    if (mod == nullptr || position == nullptr || g_play == nullptr || g_play->hero_place == nullptr
        || !std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2])
        || !std::isfinite(yaw)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_place(position, yaw, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_grab(openrac_mod* mod, int animation, std::uint32_t flags) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_grab == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_grab(animation, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_release(openrac_mod* mod) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_let_go == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_let_go() != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_hold_at(openrac_mod* mod, const float position[3], float yaw) {
    if (mod == nullptr || position == nullptr || g_play == nullptr
        || g_play->hero_hold_at == nullptr || !std::isfinite(position[0])
        || !std::isfinite(position[1]) || !std::isfinite(position[2]) || !std::isfinite(yaw)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_hold_at(position, yaw) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_stand_in_as(openrac_mod* mod, const openrac_stand_in* as, int figure) {
    if (mod == nullptr || mod != g_gate.mod || g_before_slot < 0 || as == nullptr
        || g_play == nullptr || g_play->stand_in_as == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const Figure* f = figure >= 0 ? own_figure(mod, figure) : nullptr;
    g_play->stand_in_as(as->position, f != nullptr ? f->moby : 0, as->state, as->health, as->flags);
    return OPENRAC_HOOK_OK;
}

int api_hero_stand_in_read(openrac_mod* mod, openrac_stand_in* out) {
    if (mod == nullptr || mod != g_gate.mod || out == nullptr || g_play == nullptr
        || g_play->stand_in_read == nullptr
        || g_play->stand_in_read(out->position, &out->yaw, &out->state, &out->health, &out->flags)
               == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

int api_hero_revive(openrac_mod* mod, const float position[3], std::uint32_t flags) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_revive == nullptr
        || (position != nullptr
            && (!std::isfinite(position[0]) || !std::isfinite(position[1])
                || !std::isfinite(position[2])))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_revive(position, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_shots2(openrac_shot2* out, int max) {
    if (out == nullptr || max <= 0 || g_play == nullptr || g_play->hero_shots2 == nullptr) {
        return 0;
    }
    std::array<openrac_game_shot2, OPENRAC_SHOTS_MAX> shots{};
    const int n = g_play->hero_shots2(shots.data(), std::min(max, OPENRAC_SHOTS_MAX));
    for (int i = 0; i < n; ++i) {
        out[i].size = sizeof(openrac_shot2);
        std::memcpy(&out[i].item, &shots[static_cast<std::size_t>(i)], sizeof(openrac_game_shot2));
    }
    return n;
}

int api_figure_shot2(openrac_mod* mod, int id, const openrac_shot2* shot) {
    Figure* f = own_figure(mod, id);
    if (f == nullptr || shot == nullptr || shot->size < sizeof(openrac_shot2)
        || g_play->figure_shot2 == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_shot2 s{};
    std::memcpy(&s, &shot->item, sizeof s);
    return g_play->figure_shot2(f->moby, &s) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_moby_list2(openrac_moby2* out, int max) {
    if (g_play == nullptr || g_play->mobys == nullptr || max < 0) {
        return 0;
    }
    std::vector<openrac_game_moby_state> states(static_cast<std::size_t>(max));
    const int n = g_play->mobys(states.data(), max);
    for (int i = 0; i < std::min(n, max) && out != nullptr; ++i) {
        const openrac_game_moby_state& s = states[static_cast<std::size_t>(i)];
        openrac_moby2& m = out[i];
        m.size = sizeof(openrac_moby2);
        m.slot = s.slot;
        m.key = s.key;
        m.oclass = s.oclass;
        m.flags = s.flags | (driver(s.slot) != nullptr ? OPENRAC_MOBY_DRIVEN : 0u);
        m.state = s.state;
        m.mission = g_play->moby_mission != nullptr ? g_play->moby_mission(s.slot) : -1;
        std::copy(std::begin(s.position), std::end(s.position), m.position);
        std::copy(std::begin(s.rotation), std::end(s.rotation), m.rotation);
        m.scale = s.scale;
        m.seq = s.seq;
        m.frame = s.frame;
        m.next_frame = s.next_frame;
        m.anim_time = s.anim_time;
        m.anim_speed = s.anim_speed;
    }
    return n;
}

// Who hears of the game's camera scripts (on_camera_script), and the one showing one here.
struct CameraScriptWatch {
    const openrac_mod* mod = nullptr;
    openrac_camera_script_fn fn = nullptr;
    void* user = nullptr;
};

CameraScriptWatch g_camera_scripts;
const openrac_mod* g_camera_shower = nullptr;

bool has_camera_scripts() {
    return g_play != nullptr && g_play->camera_script_next != nullptr
           && g_play->camera_script_view != nullptr && g_play->camera_script_play != nullptr
           && g_play->camera_script_show != nullptr;
}

int api_on_camera_script(openrac_mod* mod, openrac_camera_script_fn fn, void* user) {
    if (!has_camera_scripts()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_camera_scripts.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_camera_scripts = {mod, fn, user};
    return OPENRAC_HOOK_OK;
}

int api_camera_script_play(openrac_mod* mod, int id) {
    if (mod == nullptr || !has_camera_scripts() || id < OPENRAC_CAMERA_SCRIPT_CUT
        || (g_camera_shower != nullptr && g_camera_shower != mod)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_play->camera_script_play(id) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_camera_shower = id >= 0 ? mod : nullptr;
    return OPENRAC_HOOK_OK;
}

int api_camera_script_view(float eye[3], float rotation[3], float where[3], int* stand_in) {
    if (eye == nullptr || rotation == nullptr || where == nullptr || !has_camera_scripts()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::int32_t standing = 0;
    if (g_play->camera_script_view(eye, rotation, where, &standing) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (stand_in != nullptr) {
        *stand_in = standing;
    }
    return OPENRAC_HOOK_OK;
}

int api_camera_script_show(openrac_mod* mod, const float eye[3], const float rotation[3]) {
    if (mod == nullptr || mod != g_camera_shower || eye == nullptr || rotation == nullptr
        || !has_camera_scripts() || !std::isfinite(eye[0] + eye[1] + eye[2])
        || !std::isfinite(rotation[0] + rotation[1] + rotation[2])) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->camera_script_show(eye, rotation) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_play_scene_from(openrac_mod*, int, int) {
    return OPENRAC_HOOK_UNKNOWN;
}

int api_play_live_picture(openrac_mod* mod, int on) {
    if (mod == nullptr || g_play == nullptr || g_play->live_picture == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->live_picture(on != 0 ? 1 : 0);
    return OPENRAC_HOOK_OK;
}

int game_metal_detect(std::int32_t slot, std::int32_t key) {
    return g_metal.fn != nullptr ? g_metal.fn(g_metal.user, slot, key) : 1;
}

int api_on_metal_detect(openrac_mod* mod, openrac_metal_detect_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_metal.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_metal = {mod, fn, user};
    openrac_game_metal_detect = game_metal_detect;
    return OPENRAC_HOOK_OK;
}

int api_camera_position(float out[3]) {
    if (out == nullptr || g_play == nullptr || g_play->camera_position == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->camera_position(out) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_level_line(const float from[3], const float to[3], int* blocked) {
    if (from == nullptr || to == nullptr || blocked == nullptr || g_play == nullptr
        || g_play->level_line == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->level_line(from, to, blocked) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// The game's calls (game_host.h): the gate around an update, and a hit posted.
int game_moby_update(std::int32_t slot, int phase) {
    // A driven moby's update does not run, and the gate is not asked.
    if (driver(slot) != nullptr) {
        return phase == OPENRAC_MOBY_BEFORE ? 1 : 0;
    }
    if (g_gate.fn == nullptr) {
        return 0;
    }
    if (phase != OPENRAC_MOBY_BEFORE) {
        g_gate.fn(g_gate.user, slot, phase);
        return 0;
    }
    g_before_slot = slot;
    const int skip = g_gate.fn(g_gate.user, slot, phase);
    g_before_slot = -1;
    return skip != 0 ? 1 : 0;
}

int game_moby_hit(
    std::int32_t victim,
    std::int32_t attacker,
    std::uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
) {
    if (g_hits.fn == nullptr) {
        return 0;
    }
    openrac_hit hit{};
    hit.size = sizeof hit;
    hit.victim = victim;
    hit.attacker = attacker;
    hit.mask = mask;
    hit.damage = damage;
    std::copy(position, position + 3, hit.position);
    std::copy(velocity, velocity + 3, hit.velocity);
    return g_hits.fn(g_hits.user, &hit) != 0 ? 1 : 0;
}

int api_world_family(char* out, std::uint32_t size) {
    if (g_play == nullptr || g_play->world_family == nullptr || out == nullptr || size == 0) {
        return 0;
    }

    const std::size_t length = std::strlen(g_play->world_family);

    if (length >= size) {
        return 0;
    }

    std::memcpy(out, g_play->world_family, length + 1);
    return static_cast<int>(length);
}

int api_sim_digest(std::int32_t level, std::uint32_t* out) {
    if (g_play == nullptr || g_play->sim_digest == nullptr || out == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    *out = g_play->sim_digest(level);
    return *out != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// ---- Interface 1.12: the hero moved by a mod, the collision triangles, props and sounds ----

// A prop a mod made: whose, its mesh, the level load it was made in, its matrix once posed.
struct Prop {
    const openrac_mod* mod = nullptr;
    std::filesystem::path mesh;
    std::uint32_t loads = 0;
    bool posed = false;
    bool hidden = false;
    std::array<float, 16> matrix{};
    std::array<float, 4> tint{1.0f, 1.0f, 1.0f, 1.0f};
    int blend = 0;
};

// A sound a mod loaded: whose, and the host's id of it.
struct Sound {
    const openrac_mod* mod = nullptr;
    int host = -1;
};

// The props, the meshes asked for (and their count's generation), behind a lock: the front end
// reads them on its own thread.
std::mutex g_props_lock;
std::array<Prop, OPENRAC_PROPS_MAX> g_props;
std::vector<std::filesystem::path> g_prop_meshes;
unsigned g_prop_generation = 0;
std::vector<Sound> g_sounds;
std::vector<std::pair<const openrac_mod*, std::uint32_t>> g_voices;
int (*g_sound_load)(const std::filesystem::path&) = nullptr;
std::uint32_t (*g_sound_play)(int, float, float, bool) = nullptr;
int (*g_sound_set)(std::uint32_t, float, float) = nullptr;
void (*g_sound_stop)(std::uint32_t) = nullptr;
void (*g_joint_offsets)(std::uint32_t, const openrac_joint_offset*, int) = nullptr;

// The hero's joint offsets each mod set (hero_set_joints), in the order the mods first set theirs:
// all of them go to the hero together, each mod's composed after the ones before it, so two mods
// that turn his joints do not undo each other.
struct JointSet {
    const openrac_mod* mod = nullptr;
    std::vector<openrac_joint_offset> joints;
};

std::vector<JointSet> g_joint_sets;

// Hands every mod's joint offsets to the hero at `moby` (none: his own animation alone).
void apply_joint_sets(std::uint32_t moby) {
    std::vector<openrac_joint_offset> all;
    for (const JointSet& s : g_joint_sets) {
        all.insert(all.end(), s.joints.begin(), s.joints.end());
    }
    g_joint_offsets(moby, all.empty() ? nullptr : all.data(), static_cast<int>(all.size()));
}

// The mod holding the game's camera (camera_set), and the heat haze patches each mod drew, of a
// level load, behind a lock: the front end reads them on its own thread.
const openrac_mod* g_camera_mod = nullptr;
std::uint32_t g_camera_loads = 0;

struct Haze {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    std::vector<openrac_haze> patches;
};

std::mutex g_haze_lock;
std::vector<Haze> g_haze;

// The lines of text each mod shows in the game's HUD (hud_text_set), of a level load: set and read
// on the game's thread, behind a lock as the haze is.
struct HudTexts {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    std::vector<openrac_hud_text> lines;
};

std::mutex g_hud_lock;
std::vector<HudTexts> g_hud;

std::uint32_t loads_now() {
    return g_play != nullptr ? g_play->loads() : 0;
}

int api_hero_set_pose(openrac_mod* mod, const openrac_pose* pose) {
    if (mod == nullptr || pose == nullptr || pose->size < sizeof(openrac_pose) || g_play == nullptr
        || g_play->hero_set_pose == nullptr || !holds_hook(mod, g_play->hero_update_hook)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose p{};
    std::memcpy(&p, &pose->level, sizeof p);
    return g_play->hero_set_pose(&p) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// Any mod may turn the hero's joints (ports before took them only from the holder of his update
// hook): its offsets replace its own last ones and are composed with the other mods' (g_joint_sets).
int api_hero_set_joints(openrac_mod* mod, const openrac_joint_offset* joints, int count) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_moby == nullptr
        || g_joint_offsets == nullptr || count < 0 || count > OPENRAC_JOINT_OFFSETS_MAX
        || (count > 0 && joints == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::uint32_t moby = g_play->hero_moby();
    if (moby == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    auto it = std::find_if(g_joint_sets.begin(), g_joint_sets.end(), [&](const JointSet& s) {
        return s.mod == mod;
    });
    if (count == 0) {
        if (it != g_joint_sets.end()) {
            g_joint_sets.erase(it);
        }
    } else {
        if (it == g_joint_sets.end()) {
            it = g_joint_sets.insert(g_joint_sets.end(), JointSet{mod, {}});
        }
        it->joints.assign(joints, joints + count);
    }
    std::size_t total = 0;
    for (const JointSet& s : g_joint_sets) {
        total += s.joints.size();
    }
    if (total > OPENRAC_JOINT_OFFSETS_MAX) {
        // More than the hero takes in all: this mod's last call is undone.
        std::erase_if(g_joint_sets, [&](const JointSet& s) { return s.mod == mod; });
        apply_joint_sets(moby);
        return OPENRAC_HOOK_UNKNOWN;
    }
    apply_joint_sets(moby);
    return OPENRAC_HOOK_OK;
}

int api_collision_triangles(
    const float lo[3], const float hi[3], openrac_collision_triangle* out, int max
) {
    if (g_play == nullptr || g_play->collision_triangles == nullptr || lo == nullptr
        || hi == nullptr) {
        return 0;
    }
    return g_play->collision_triangles(lo, hi, out, out != nullptr ? std::max(max, 0) : 0);
}

// The mod's prop `id` of this level load, null for one that is not; one gone with its level is let
// go. Under g_props_lock.
Prop* own_prop(const openrac_mod* mod, int id) {
    if (mod == nullptr || id < 0 || id >= static_cast<int>(g_props.size())) {
        return nullptr;
    }
    Prop& p = g_props[static_cast<std::size_t>(id)];
    if (p.mod != mod) {
        return nullptr;
    }
    if (p.loads != loads_now()) {
        p = Prop{};
        return nullptr;
    }
    return &p;
}

int api_prop_create(openrac_mod* mod, const char* mesh_path) {
    std::int32_t level = -1;
    if (mod == nullptr || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::filesystem::path mesh = mod_file(mod, mesh_path);
    if (mesh.empty() || mesh.extension() != ".glb") {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const std::uint32_t loads = loads_now();
    for (Prop& p : g_props) {
        if (p.mod != nullptr && p.loads != loads) {
            p = Prop{};
        }
    }
    for (std::size_t i = 0; i < g_props.size(); ++i) {
        if (g_props[i].mod != nullptr) {
            continue;
        }
        g_props[i] = Prop{mod, mesh, loads};
        // A mesh no level has read yet: the scenery is read again with it.
        if (std::find(g_prop_meshes.begin(), g_prop_meshes.end(), mesh) == g_prop_meshes.end()) {
            g_prop_meshes.push_back(mesh);
            ++g_prop_generation;
        }
        return static_cast<int>(i);
    }
    return OPENRAC_HOOK_UNKNOWN;
}

int api_prop_pose(openrac_mod* mod, int id, const openrac_prop_pose* pose) {
    const auto finite = [](const float* v, int n) {
        return std::all_of(v, v + n, [](float f) { return std::isfinite(f); });
    };
    if (pose == nullptr || pose->size < sizeof(openrac_prop_pose)
        || !(
            (pose->flags & OPENRAC_PROP_MATRIX) != 0
                ? finite(pose->matrix, 16)
                : finite(pose->position, 3) && finite(pose->rotation, 4) && finite(&pose->scale, 1)
        )
        || ((pose->flags & OPENRAC_PROP_TINT) != 0 && !finite(pose->tint, 4))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if ((pose->flags & OPENRAC_PROP_MATRIX) != 0) {
        std::copy(pose->matrix, pose->matrix + 16, p->matrix.begin());
    } else {
        // The rotation's matrix (a unit quaternion), each axis scaled, then the place.
        const float x = pose->rotation[0], y = pose->rotation[1], z = pose->rotation[2],
                    w = pose->rotation[3], s = pose->scale;
        const float m[9] = {
            1 - 2 * (y * y + z * z),
            2 * (x * y + z * w),
            2 * (x * z - y * w),
            2 * (x * y - z * w),
            1 - 2 * (x * x + z * z),
            2 * (y * z + x * w),
            2 * (x * z + y * w),
            2 * (y * z - x * w),
            1 - 2 * (x * x + y * y),
        };
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                p->matrix[static_cast<std::size_t>(4 * c + r)] = m[3 * c + r] * s;
            }
            p->matrix[static_cast<std::size_t>(4 * c + 3)] = 0.0f;
        }
        p->matrix[12] = pose->position[0];
        p->matrix[13] = pose->position[1];
        p->matrix[14] = pose->position[2];
        p->matrix[15] = 1.0f;
    }
    if ((pose->flags & OPENRAC_PROP_TINT) != 0) {
        std::copy(pose->tint, pose->tint + 4, p->tint.begin());
        p->blend = pose->blend <= OPENRAC_PROP_ADD ? static_cast<int>(pose->blend) : 0;
    } else {
        p->tint = {1.0f, 1.0f, 1.0f, 1.0f};
        p->blend = 0;
    }
    p->posed = true;
    return OPENRAC_HOOK_OK;
}

int api_prop_hide(openrac_mod* mod, int id, int hidden) {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    p->hidden = hidden != 0;
    return OPENRAC_HOOK_OK;
}

int api_prop_remove(openrac_mod* mod, int id) {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    *p = Prop{};
    return OPENRAC_HOOK_OK;
}

int api_camera_view(openrac_camera* out) {
    if (out == nullptr || out->size < sizeof(openrac_camera) || g_play == nullptr
        || g_play->camera_view == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    float eye[3], rows[9];
    if (g_play->camera_view(eye, rows) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::copy(eye, eye + 3, out->eye);
    std::copy(rows, rows + 3, out->forward);
    std::copy(rows + 3, rows + 6, out->left);
    std::copy(rows + 6, rows + 9, out->up);
    return OPENRAC_HOOK_OK;
}

// Whether `v` is a unit vector (to a few parts in a thousand).
bool unit(const float v[3]) {
    const float n = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    return std::isfinite(n) && std::fabs(n - 1.0f) < 0.01f;
}

int api_camera_set(openrac_mod* mod, const openrac_camera* camera) {
    if (mod == nullptr || g_play == nullptr || g_play->camera_set == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (g_camera_mod != nullptr && g_camera_loads != loads_now()) {
        g_camera_mod = nullptr;  // its level went, and the camera with it
    }
    if (g_camera_mod != nullptr && g_camera_mod != mod) {
        return OPENRAC_HOOK_TAKEN;
    }
    if (camera == nullptr) {
        if (g_camera_mod == mod) {
            g_play->camera_set(nullptr, nullptr);
            g_camera_mod = nullptr;
        }
        return OPENRAC_HOOK_OK;
    }
    const auto square = [](const float a[3], const float b[3]) {
        return std::fabs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) < 0.01f;
    };
    if (camera->size < sizeof(openrac_camera) || !unit(camera->forward) || !unit(camera->left)
        || !unit(camera->up) || !square(camera->forward, camera->left)
        || !square(camera->forward, camera->up) || !square(camera->left, camera->up)
        || !std::all_of(camera->eye, camera->eye + 3, [](float v) { return std::isfinite(v); })) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    float rows[9];
    std::copy(camera->forward, camera->forward + 3, rows);
    std::copy(camera->left, camera->left + 3, rows + 3);
    std::copy(camera->up, camera->up + 3, rows + 6);
    if (g_play->camera_set(camera->eye, rows) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_camera_mod = mod;
    g_camera_loads = loads_now();
    return OPENRAC_HOOK_OK;
}

int api_haze_set(openrac_mod* mod, const openrac_haze* patches, int count) {
    std::int32_t level = -1;
    if (mod == nullptr || count < 0 || (count > 0 && patches == nullptr)
        || (count > 0 && !in_level(phase_now(&level)))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_haze_lock);
    const std::uint32_t loads = loads_now();
    std::size_t others = 0;
    for (const Haze& h : g_haze) {
        if (h.mod != mod && h.loads == loads) {
            others += h.patches.size();
        }
    }
    if (others + static_cast<std::size_t>(count) > OPENRAC_HAZE_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int i = 0; i < count; ++i) {
        const openrac_haze& p = patches[i];
        if (!std::all_of(p.position, p.position + 3, [](float v) { return std::isfinite(v); })
            || !std::isfinite(p.radius) || !std::isfinite(p.strength) || p.radius < 0.0f
            || p.strength < 0.0f) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    std::erase_if(g_haze, [&](const Haze& h) { return h.mod == mod || h.loads != loads; });
    if (count > 0) {
        g_haze.push_back({mod, loads, std::vector<openrac_haze>(patches, patches + count)});
    }
    return OPENRAC_HOOK_OK;
}

int api_hud_text_set(openrac_mod* mod, const openrac_hud_text* lines, int count) {
    std::int32_t level = -1;
    if (mod == nullptr || count < 0 || (count > 0 && lines == nullptr)
        || (count > 0 && !in_level(phase_now(&level)))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int i = 0; i < count; ++i) {
        const openrac_hud_text& l = lines[i];
        const bool ended =
            std::find(std::begin(l.text), std::end(l.text), '\0') != std::end(l.text);
        const auto within = [](float v, float lo, float hi) {
            return std::isfinite(v) && v >= lo && v <= hi;
        };
        if (!ended || !within(l.x, 0.0f, 1.0f) || !within(l.y, 0.0f, 1.0f)
            || !within(l.scale, 0.25f, 4.0f)) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    const std::lock_guard<std::mutex> lock(g_hud_lock);
    const std::uint32_t loads = loads_now();
    std::size_t others = 0;
    for (const HudTexts& h : g_hud) {
        if (h.mod != mod && h.loads == loads) {
            others += h.lines.size();
        }
    }
    if (others + static_cast<std::size_t>(count) > OPENRAC_HUD_TEXTS_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::erase_if(g_hud, [&](const HudTexts& h) { return h.mod == mod || h.loads != loads; });
    if (count > 0) {
        g_hud.push_back({mod, loads, std::vector<openrac_hud_text>(lines, lines + count)});
    }
    return OPENRAC_HOOK_OK;
}

// The hero's looping sounds each mod started (hero_sound_play), stopped with the mod.
struct HeroVoice {
    openrac_mod* mod;
    std::uint32_t loads;
    int voice;
};

std::vector<HeroVoice> g_hero_voices;

int api_hero_sound_play(openrac_mod* mod, int sound, int loop) {
    std::int32_t level = -1;
    if (mod == nullptr || g_play == nullptr || g_play->hero_sound_play == nullptr || sound < 0
        || !in_level(phase_now(&level))) {
        return -1;
    }
    const int voice = g_play->hero_sound_play(sound, loop != 0 ? 1 : 0);
    const std::uint32_t loads = loads_now();
    std::erase_if(g_hero_voices, [&](const HeroVoice& v) {
        return v.loads != loads || v.voice == voice;
    });
    if (voice >= 0 && loop != 0) {
        g_hero_voices.push_back({mod, loads, voice});
    }
    return voice;
}

// The looping hero sounds of one mod (all: null) stopped, those of this level load still playing.
void stop_hero_voices(const openrac_mod* mod) {
    for (const HeroVoice& v : g_hero_voices) {
        if ((mod == nullptr || v.mod == mod) && v.loads == loads_now() && g_play != nullptr
            && g_play->hero_sound_stop != nullptr) {
            g_play->hero_sound_stop(v.voice);
        }
    }
    std::erase_if(g_hero_voices, [&](const HeroVoice& v) {
        return mod == nullptr || v.mod == mod;
    });
}

void api_hero_sound_stop(openrac_mod* mod, int voice) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_sound_stop == nullptr || voice < 0) {
        return;
    }
    std::erase_if(g_hero_voices, [&](const HeroVoice& v) { return v.voice == voice; });
    g_play->hero_sound_stop(voice);
}

int api_hero_hurt(openrac_mod* mod, int damage) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_hurt == nullptr) {
        return -1;
    }
    return g_play->hero_hurt(damage);
}

int api_touch_define(openrac_mod* mod, const char* json) {
    std::string error;
    if (mod == nullptr || json == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (!touch::define(mod_id(mod), json, error)) {
        log::warn("mods: {}", error);
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

int api_touch_icon(openrac_mod* mod, const char* name, const char* svg) {
    std::string error;
    if (mod == nullptr || name == nullptr || svg == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (!touch::icon(mod_id(mod), name, svg, error)) {
        log::warn("mods: {}", error);
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

void api_touch_push(openrac_mod* mod, const char* context) {
    if (mod != nullptr) {
        touch::push(mod_id(mod), context != nullptr ? context : "");
    }
}

void api_touch_hint(openrac_mod* mod, const char* hint, int on) {
    if (mod != nullptr && hint != nullptr) {
        touch::hint(mod_id(mod), hint, on != 0);
    }
}

int api_sound_load(openrac_mod* mod, const char* path) {
    if (mod == nullptr || g_sound_load == nullptr || g_sounds.size() >= kSoundsMax) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const auto own = std::count_if(g_sounds.begin(), g_sounds.end(), [&](const Sound& s) {
        return s.mod == mod;
    });
    const std::filesystem::path file = mod_file(mod, path);
    const std::filesystem::path ext = file.extension();
    if (own >= OPENRAC_SOUNDS_MAX || file.empty() || (ext != ".wav" && ext != ".ogg")) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int host = g_sound_load(file);
    if (host < 0) {
        log::warn("mods: {} could not read the sound {}", mod_id(mod), path);
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_sounds.push_back({mod, host});
    return static_cast<int>(g_sounds.size() - 1);
}

// Voices that ended (the host's sound_set answers no more) are forgotten as new ones are asked for.
void forget_ended_voices() {
    std::erase_if(g_voices, [](const auto& v) {
        return g_sound_set == nullptr || g_sound_set(v.second, -1.0f, -1.0f) == 0;
    });
}

std::uint32_t api_sound_play(openrac_mod* mod, int sound, float volume, float pitch, int loop) {
    if (mod == nullptr || g_sound_play == nullptr || sound < 0
        || sound >= static_cast<int>(g_sounds.size())
        || g_sounds[static_cast<std::size_t>(sound)].mod != mod || !std::isfinite(volume)
        || !std::isfinite(pitch)) {
        return 0;
    }
    forget_ended_voices();
    if (g_voices.size() >= OPENRAC_SOUND_VOICES_MAX) {
        return 0;
    }
    const std::uint32_t voice = g_sound_play(
        g_sounds[static_cast<std::size_t>(sound)].host,
        std::max(volume, 0.0f),
        std::max(pitch, 0.0f),
        loop != 0
    );
    if (voice != 0) {
        g_voices.emplace_back(mod, voice);
    }
    return voice;
}

// Whether `voice` is one the mod played.
bool own_voice(const openrac_mod* mod, std::uint32_t voice) {
    return mod != nullptr && voice != 0
           && std::find(g_voices.begin(), g_voices.end(), std::make_pair(mod, voice))
                  != g_voices.end();
}

int api_sound_set(openrac_mod* mod, std::uint32_t voice, float volume, float pitch) {
    if (g_sound_set == nullptr || !own_voice(mod, voice) || !std::isfinite(volume)
        || !std::isfinite(pitch)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_sound_set(voice, std::max(volume, 0.0f), std::max(pitch, 0.0f)) != 0
               ? OPENRAC_HOOK_OK
               : OPENRAC_HOOK_UNKNOWN;
}

void api_sound_stop(openrac_mod* mod, std::uint32_t voice) {
    if (g_sound_stop != nullptr && own_voice(mod, voice)) {
        g_sound_stop(voice);
        std::erase(g_voices, std::make_pair(static_cast<const openrac_mod*>(mod), voice));
    }
}
}  // namespace

void fill_play_api(
    openrac_api& api, const openrac_game_play* play, const std::filesystem::path& cards
) {
    g_play = play;
    g_cards = cards;
    api.play_status = api_play_status;
    api.world_family = api_world_family;
    api.sim_digest = api_sim_digest;
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
    api.save_slot_current = api_save_slot_current;
    api.moby_list = api_moby_list;
    api.moby_slot = api_moby_slot;
    api.moby_drive = api_moby_drive;
    api.moby_pose = api_moby_pose;
    api.moby_kill = api_moby_kill;
    api.moby_spawn = api_moby_spawn;
    api.figure_slot = api_figure_slot;
    api.on_moby_update = api_on_moby_update;
    api.hero_stand_in = api_hero_stand_in;
    api.on_moby_hit = api_on_moby_hit;
    api.moby_hit = api_moby_hit;
    api.play_eyes = api_play_eyes;
    api.bolts = api_bolts;
    api.play_overlay_pause = api_play_overlay_pause;
    api.moby_slot_of = api_moby_slot_of;
    api.play_skip = api_play_skip;
    api.hero_held = api_hero_held;
    api.figure_held = api_figure_held;
    api.hero_shots = api_hero_shots;
    api.figure_shot = api_figure_shot;
    api.play_notice = api_play_notice;
    api.scene_playing = api_scene_playing;
    api.play_scene = api_play_scene;
    api.moby_hide = api_moby_hide;
    api.moby_deletion = api_moby_deletion;
    api.camera_position = api_camera_position;
    api.moby_run = api_moby_run;
    api.card_filter = api_card_filter;
    api.card_slot_hint = api_card_slot_hint;
    api.coop_save_write = api_coop_save_write;
    api.save_slot2 = api_save_slot2;
    api.save_read2 = api_save_read2;
    api.card_saves = api_card_saves;
    api.card_note_write = api_card_note_write;
    api.card_note_read = api_card_note_read;
    api.shared_read = api_shared_read;
    api.shared_give = api_shared_give;
    api.mission_state_read = api_mission_state_read;
    api.mission_state_write = api_mission_state_write;
    api.mission_state_clear = api_mission_state_clear;
    api.hero_place = api_hero_place;
    api.hero_grab = api_hero_grab;
    api.hero_release = api_hero_release;
    api.hero_revive = api_hero_revive;
    api.hero_shots2 = api_hero_shots2;
    api.figure_shot2 = api_figure_shot2;
    api.moby_list2 = api_moby_list2;
    api.on_camera_script = api_on_camera_script;
    api.camera_script_play = api_camera_script_play;
    api.camera_script_view = api_camera_script_view;
    api.camera_script_show = api_camera_script_show;
    api.level_line = api_level_line;
    api.play_scene_from = api_play_scene_from;
    api.play_live_picture = api_play_live_picture;
    api.on_metal_detect = api_on_metal_detect;
    api.hero_hold_at = api_hero_hold_at;
    api.hero_stand_in_as = api_hero_stand_in_as;
    api.hero_stand_in_read = api_hero_stand_in_read;
}

void fill_motion_api(openrac_api& api, const Host& host) {
    g_sound_load = host.sound_load;
    g_sound_play = host.sound_play;
    g_sound_set = host.sound_set;
    g_sound_stop = host.sound_stop;
    g_joint_offsets = host.joint_offsets;
    api.hero_set_pose = api_hero_set_pose;
    api.hero_set_joints = api_hero_set_joints;
    api.collision_triangles = api_collision_triangles;
    api.prop_create = api_prop_create;
    api.prop_pose = api_prop_pose;
    api.prop_hide = api_prop_hide;
    api.prop_remove = api_prop_remove;
    api.sound_load = api_sound_load;
    api.sound_play = api_sound_play;
    api.sound_set = api_sound_set;
    api.sound_stop = api_sound_stop;
    api.camera_view = api_camera_view;
    api.camera_set = api_camera_set;
    api.haze_set = api_haze_set;
    api.hud_text_set = api_hud_text_set;
    api.hero_sound_play = api_hero_sound_play;
    api.hero_sound_stop = api_hero_sound_stop;
    api.hero_hurt = api_hero_hurt;
    api.touch_define = api_touch_define;
    api.touch_icon = api_touch_icon;
    api.touch_push = api_touch_push;
    api.touch_hint = api_touch_hint;
}

std::vector<HazeDraw> haze_drawn() {
    std::vector<HazeDraw> out;
    const std::lock_guard<std::mutex> lock(g_haze_lock);
    const std::uint32_t loads = loads_now();
    for (const Haze& h : g_haze) {
        if (h.loads != loads) {
            continue;
        }
        for (const openrac_haze& p : h.patches) {
            out.push_back({{p.position[0], p.position[1], p.position[2]}, p.radius, p.strength});
        }
    }
    return out;
}

std::vector<openrac_hud_text> hud_texts() {
    std::vector<openrac_hud_text> out;
    const std::lock_guard<std::mutex> lock(g_hud_lock);
    const std::uint32_t loads = loads_now();
    for (const HudTexts& h : g_hud) {
        if (h.loads == loads) {
            out.insert(out.end(), h.lines.begin(), h.lines.end());
        }
    }
    return out;
}

std::vector<PropDraw> props_drawn() {
    std::vector<PropDraw> out;
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const std::uint32_t loads = loads_now();
    for (std::size_t i = 0; i < g_props.size(); ++i) {
        const Prop& p = g_props[i];
        if (p.mod != nullptr && p.posed && !p.hidden && p.loads == loads) {
            out.push_back({static_cast<int>(i), p.mesh, p.matrix, p.tint, p.blend});
        }
    }
    return out;
}

std::vector<std::filesystem::path> prop_meshes() {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    return g_prop_meshes;
}

unsigned prop_meshes_generation() {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    return g_prop_generation;
}

void play_loading(const openrac_mod* mod) {
    g_loading_mod = mod;
    // The game asks only once a mod has a gate or a watcher; drives are checked in the gate.
    openrac_game_moby_update = g_play != nullptr ? game_moby_update : nullptr;
    openrac_game_moby_hit = g_play != nullptr ? game_moby_hit : nullptr;
}

void play_update() {
    // The game's camera scripts, as they went.
    if (has_camera_scripts()) {
        std::int32_t id = 0;
        std::int32_t started = 0;
        while (g_play->camera_script_next(&id, &started) != 0) {
            if (g_camera_scripts.fn != nullptr) {
                g_camera_scripts.fn(g_camera_scripts.user, id, started);
            }
        }
    }
    // The game's card words, kept as a party's game needs them whatever its menus did.
    if (g_card_mode != OPENRAC_CARD_PASS && g_play != nullptr) {
        g_play->card_session(1);
    }
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
    } else if (r.kind == Request::Kind::kTravel && in_a_level(phase)) {
        const bool going = g_play->travel(r.level) != 0;
        log::info(
            "mods: {} {} {}",
            mod_id(r.mod),
            going ? "travels to" : "could not travel to",
            r.level >= 0 ? "level " + std::to_string(r.level) : std::string("the title")
        );
    }
}

bool play_skipping() {
    if (g_skip_ticks <= 0) {
        return false;
    }
    // The scene it was asked in is over (out of it, or another in its place): nothing more pressed.
    std::int32_t level = -1;
    if (phase_now(&level) != g_skip_phase || scene_now() != g_skip_scene) {
        g_skip_ticks = 0;
        return false;
    }
    --g_skip_ticks;
    return true;
}

std::string play_notice() {
    const std::lock_guard<std::mutex> lock(g_notice_lock);
    return g_notice;
}

bool overlay_shows_level() {
    std::int32_t level = -1;
    return phase_now(&level) == OPENRAC_PLAY_PLAYING;
}

void reset_play() {
    {
        const std::lock_guard<std::mutex> lock(g_notice_lock);
        g_notice.clear();
    }
    g_skip_ticks = 0;
    // A scene a mod asked for and the game has not started yet.
    if (g_play != nullptr && g_play->scene_start != nullptr) {
        g_play->scene_start(-1);
    }
    g_figures.fill(Figure{});
    {
        const std::lock_guard<std::mutex> lock(g_props_lock);
        g_props.fill(Prop{});
    }
    {
        const std::lock_guard<std::mutex> lock(g_haze_lock);
        g_haze.clear();
    }
    {
        const std::lock_guard<std::mutex> lock(g_hud_lock);
        g_hud.clear();
    }
    if (!g_joint_sets.empty()) {
        g_joint_sets.clear();
        if (g_joint_offsets != nullptr) {
            g_joint_offsets(0, nullptr, 0);
        }
    }
    touch::clear();
    stop_hero_voices(nullptr);
    if (g_camera_mod != nullptr && g_play != nullptr && g_play->camera_set != nullptr) {
        g_play->camera_set(nullptr, nullptr);
    }
    g_camera_mod = nullptr;
    for (const auto& v : g_voices) {
        if (g_sound_stop != nullptr) {
            g_sound_stop(v.second);
        }
    }
    g_voices.clear();
    g_sounds.clear();
    g_request = Request{};
    std::fill(g_drives.begin(), g_drives.end(), Drive{});
    g_gate = Gate{};
    g_hits = HitWatch{};
    g_metal = MetalWatch{};
    g_camera_scripts = {};
    g_camera_shower = nullptr;
    g_before_slot = -1;
    g_loading_mod = nullptr;
    if (g_play != nullptr && g_play->keep_running != nullptr) {
        g_play->keep_running(0);
    }
    openrac_game_moby_update = nullptr;
    openrac_game_moby_hit = nullptr;
    openrac_game_metal_detect = nullptr;
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
    {
        const std::lock_guard<std::mutex> lock(g_props_lock);
        for (Prop& p : g_props) {
            if (p.mod == mod) {
                p = Prop{};
            }
        }
    }
    {
        const std::lock_guard<std::mutex> lock(g_haze_lock);
        std::erase_if(g_haze, [&](const Haze& h) { return h.mod == mod; });
    }
    {
        const std::lock_guard<std::mutex> lock(g_hud_lock);
        std::erase_if(g_hud, [&](const HudTexts& h) { return h.mod == mod; });
    }
    if (std::erase_if(g_joint_sets, [&](const JointSet& s) { return s.mod == mod; }) != 0
        && g_joint_offsets != nullptr) {
        const std::uint32_t moby =
            g_play != nullptr && g_play->hero_moby != nullptr ? g_play->hero_moby() : 0;
        if (g_joint_sets.empty() || moby == 0) {
            g_joint_offsets(0, nullptr, 0);
        } else {
            apply_joint_sets(moby);
        }
    }
    stop_hero_voices(mod);
    if (g_camera_mod == mod) {
        if (g_play != nullptr && g_play->camera_set != nullptr) {
            g_play->camera_set(nullptr, nullptr);
        }
        g_camera_mod = nullptr;
    }
    std::erase_if(g_voices, [&](const auto& v) {
        if (v.first == mod && g_sound_stop != nullptr) {
            g_sound_stop(v.second);
        }
        return v.first == mod;
    });
    for (Drive& d : g_drives) {
        if (d.mod == mod) {
            d = Drive{};
        }
    }
    if (g_gate.mod == mod) {
        g_gate = Gate{};
    }
    if (g_hits.mod == mod) {
        g_hits = HitWatch{};
    }
    // A refused mod's library is closed immediately after this cleanup. None of
    // its observers may survive until a later camera event or metal-detector dig.
    if (g_metal.mod == mod) {
        g_metal = MetalWatch{};
        openrac_game_metal_detect = nullptr;
    }

    if (g_camera_scripts.mod == mod) {
        g_camera_scripts = {};
    }

    if (g_camera_shower == mod) {
        g_camera_shower = nullptr;
    }
}

}  // namespace openrac::mods
