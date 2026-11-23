// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' play state (mod_play.h, interface 1.8): what the play calls in mod_api/<area>.cpp share.

#include "mod_play.h"

#include <algorithm>
#include <array>
#include <atomic>
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
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/log.h"
#include "mod_api/internal.h"
#include "mod_touch.h"
#include "mods.h"
#include "openrac/guest.h"

// The game's calls into the mods' world (game_host.h), set while mods are loaded.
int (*openrac_game_moby_update)(std::int32_t slot, int phase) = nullptr;
int (*openrac_game_metal_detect)(std::int32_t slot, std::int32_t key) = nullptr;
void (*openrac_game_weapon_fired)(const openrac_game_weapon_shot* shot) = nullptr;
void (*openrac_game_shot_ended)(const openrac_game_shot_end* end) = nullptr;
int (*openrac_game_moby_look)(gaddr moby, std::int32_t item) = nullptr;
void (*openrac_game_particle_look)(gaddr record, std::int32_t item) = nullptr;
int (*openrac_game_moby_hit)(
    std::int32_t victim,
    std::int32_t attacker,
    std::uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
) = nullptr;

namespace openrac::mods {

using namespace detail;

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

// So are a weapon's shots (interface 1.29).
static_assert(offsetof(openrac_weapon_shot, item) == sizeof(std::uint32_t));
static_assert(
    sizeof(openrac_weapon_shot) == sizeof(std::uint32_t) + sizeof(openrac_game_weapon_shot)
);
static_assert(offsetof(openrac_shot_end, item) == sizeof(std::uint32_t));
static_assert(sizeof(openrac_shot_end) == sizeof(std::uint32_t) + sizeof(openrac_game_shot_end));
static_assert(offsetof(openrac_spawn_live, oclass) == sizeof(std::uint32_t));
static_assert(
    sizeof(openrac_spawn_live) == sizeof(std::uint32_t) + sizeof(openrac_game_spawn_live)
);

// The collision triangles are the game's records as they are (interface 1.12).
static_assert(sizeof(openrac_collision_triangle) == 14 * sizeof(float));

namespace detail {

const openrac_game_play* g_play = nullptr;
std::filesystem::path g_cards;
std::array<Figure, OPENRAC_FIGURES_MAX> g_figures;
Request g_request;

std::vector<Drive> g_drives(kSlots);
Gate g_gate;

MetalWatch g_metal;
HitWatch g_hits;
int g_before_slot = -1;

int g_card_mode = OPENRAC_CARD_PASS;
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

// A level's phases, where a travel may leave it from (1.9: its scenes and menus too, as the pause
// menu's Quit Game leaves from the menu: a party's member goes with its host from anywhere).
bool in_a_level(int phase) {
    return phase == OPENRAC_PLAY_PLAYING || phase == OPENRAC_PLAY_SCENE
           || phase == OPENRAC_PLAY_MENU;
}

// Whether a mod drives the moby in `slot` in this level load (whose: `mod` when given).
const openrac_mod* driver(int slot) {
    if (g_play == nullptr || slot < 0 || static_cast<std::size_t>(slot) >= kSlots) {
        return nullptr;
    }
    const Drive& d = g_drives[static_cast<std::size_t>(slot)];
    return d.mod != nullptr && d.loads == g_play->loads() ? d.mod : nullptr;
}

gaddr moby_address(int slot) {
    return g_play != nullptr && g_play->moby_at != nullptr ? g_play->moby_at(slot) : 0;
}

int g_skip_ticks = 0;
int g_skip_phase = OPENRAC_PLAY_NONE;
std::int32_t g_skip_scene = -1;

std::int32_t scene_now() {
    std::int32_t played = 0;
    return g_play != nullptr && g_play->scene_playing != nullptr ? g_play->scene_playing(&played)
                                                                 : -1;
}

// One line over the game's picture, the last mod's to set it: set on the game's thread, drawn on
// the window's.
std::mutex g_notice_lock;
std::string g_notice;

// ---- Interface 1.14: the calls a shared game needs. Each answers "unknown" in a game that has not
// got it (play_scene_from always, for now), so a mod tests for it before using it. ----

CameraScriptWatch g_camera_scripts;
const openrac_mod* g_camera_shower = nullptr;

bool has_camera_scripts() {
    return g_play != nullptr && g_play->camera_script_next != nullptr
           && g_play->camera_script_view != nullptr && g_play->camera_script_play != nullptr
           && g_play->camera_script_show != nullptr;
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

// ---- Interface 1.12: the hero moved by a mod, the collision triangles, props and sounds ----

// The props, the meshes asked for (and their count's generation), behind a lock: the front end
// reads them on its own thread.
//
// g_prop_meshes are the meshes the props of one level load were made of (g_prop_meshes_loads), each
// once, in the order first asked for: what the front end has to hold in the scene to draw them. A
// new level load starts the list again (its props are all gone), so a level is not read with the
// meshes of the ones before it. g_prop_generation grows each time the list changes, for the front
// end to look at it again only then.
std::mutex g_props_lock;
std::array<Prop, OPENRAC_PROPS_MAX> g_props;
std::vector<std::filesystem::path> g_prop_meshes;
std::uint32_t g_prop_meshes_loads = 0;
unsigned g_prop_generation = 0;

// Under g_props_lock, which they share with the props: the front end reads them with the props'.
std::array<Model, OPENRAC_MODELS_MAX> g_models;
std::map<int, SlotModel> g_slot_models;
// By class and scope (one bit: OPENRAC_MODEL_WORLD or OPENRAC_MODEL_MENU).
std::map<std::pair<int, std::uint32_t>, ClassModel> g_class_models;
// By address (interface 1.33): the mobys the level's own list may not hold, the game reads their class.
std::map<gaddr, SlotModel> g_address_models;

std::vector<VisualSet> g_visuals;
std::vector<int> g_visual_classes;
unsigned g_visual_generation = 0;
std::uint32_t g_sprite_tint = UINT32_MAX;

// Every mod's materials, in the order set (the last set is drawn), under g_props_lock.
std::vector<MaterialEntry> g_materials;
// The mods' materials on classes (interface 1.34), by class and scope bit, under g_props_lock.
std::map<std::pair<int, std::uint32_t>, ClassMaterial> g_class_materials;

std::vector<Sound> g_sounds;
std::vector<std::pair<const openrac_mod*, std::uint32_t>> g_voices;
int (*g_sound_load)(const std::filesystem::path&) = nullptr;
std::uint32_t (*g_sound_play)(int, float, float, bool) = nullptr;
int (*g_sound_set)(std::uint32_t, float, float) = nullptr;
void (*g_sound_stop)(std::uint32_t) = nullptr;
void (*g_joint_offsets)(std::uint32_t, const openrac_joint_offset*, int) = nullptr;

std::vector<JointSet> g_joint_sets;

// Hands every mod's joint offsets to the moby at `moby` (none: its own animation alone).
void apply_joint_sets(std::uint32_t moby) {
    std::vector<openrac_joint_offset> all;
    for (const JointSet& s : g_joint_sets) {
        if (s.moby == moby) {
            all.insert(all.end(), s.joints.begin(), s.joints.end());
        }
    }
    g_joint_offsets(moby, all.empty() ? nullptr : all.data(), static_cast<int>(all.size()));
}

// Offsets of an earlier level's mobies (not the hero's) are gone with that level: the address
// may hold another moby now. Run each frame (play_update) and before each set.
void drop_stale_joint_sets() {
    if (g_joint_sets.empty() || g_joint_offsets == nullptr) {
        return;
    }
    const std::uint32_t loads = loads_now();
    const std::uint32_t hero =
        g_play != nullptr && g_play->hero_moby != nullptr ? g_play->hero_moby() : 0;
    if (hero == 0) {
        return;  // still loading: the hero's own offsets are kept for the next level
    }
    std::vector<std::uint32_t> stale;
    std::erase_if(g_joint_sets, [&](const JointSet& s) {
        if (s.loads == loads || s.moby == hero) {
            return false;
        }
        if (std::find(stale.begin(), stale.end(), s.moby) == stale.end()) {
            stale.push_back(s.moby);
        }
        return true;
    });
    for (const std::uint32_t old : stale) {
        apply_joint_sets(old);
    }
}

// One mod's offsets on one moby set (count 0: taken off), and all of that moby's handed to it:
// OPENRAC_HOOK_UNKNOWN, with the mod's last ones kept, past OPENRAC_JOINT_OFFSETS_MAX in all.
int set_joint_set(
    const openrac_mod* mod, std::uint32_t moby, const openrac_joint_offset* joints, int count
) {
    drop_stale_joint_sets();
    const std::uint32_t loads = loads_now();
    auto it = std::find_if(g_joint_sets.begin(), g_joint_sets.end(), [&](const JointSet& s) {
        return s.mod == mod && s.moby == moby;
    });
    std::size_t others = 0;
    for (const JointSet& s : g_joint_sets) {
        if (s.moby == moby && s.mod != mod) {
            others += s.joints.size();
        }
    }
    if (others + static_cast<std::size_t>(count) > OPENRAC_JOINT_OFFSETS_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (count == 0) {
        if (it != g_joint_sets.end()) {
            g_joint_sets.erase(it);
        }
    } else {
        if (it == g_joint_sets.end()) {
            it = g_joint_sets.insert(g_joint_sets.end(), JointSet{mod, moby, loads, {}});
        }
        it->loads = loads;
        it->joints.assign(joints, joints + count);
    }
    apply_joint_sets(moby);
    return OPENRAC_HOOK_OK;
}

// A mod's camera's field of view (camera_fov): degrees (0 the game's) and the level load it was
// set in, read by the window's thread.
std::atomic<float> g_camera_fov{0.0f};
std::atomic<std::uint32_t> g_camera_fov_loads{0};

void release_camera_fov() {
    g_camera_fov.store(0.0f, std::memory_order_relaxed);
}

void set_camera_fov(float degrees) {
    g_camera_fov_loads.store(loads_now(), std::memory_order_relaxed);
    g_camera_fov.store(degrees, std::memory_order_relaxed);
}

// The mods that hide the game's HUD (hud_hide), each with its level load, behind g_hud_lock.
std::vector<std::pair<const openrac_mod*, std::uint32_t>> g_hud_hidden;

std::vector<MusicGain> g_music_gains;
void (*g_music_gain)(int, float) = nullptr;
int (*g_joint_frame)(std::uint32_t, int, float*) = nullptr;

void apply_music_gain() {
    if (g_music_gain == nullptr || g_play == nullptr || g_play->music_group == nullptr) {
        return;
    }
    const int group = g_play->music_group();
    if (group < 0) {
        return;
    }
    float gain = 1.0f;
    for (const MusicGain& g : g_music_gains) {
        gain = std::min(gain, g.gain);
    }
    g_music_gain(group, gain);
}

std::mutex g_air_lock;
std::optional<AirSet> g_air;
std::vector<Stretch> g_stretches;
std::vector<TieHidden> g_ties_hidden;

// The air, slow motion and hidden ties of one mod (all: null) taken off.
void drop_air(const openrac_mod* mod) {
    const std::lock_guard<std::mutex> lock(g_air_lock);
    if (g_air && (mod == nullptr || g_air->mod == mod)) {
        g_air.reset();
    }
    std::erase_if(g_ties_hidden, [&](const TieHidden& h) {
        return mod == nullptr || h.mod == mod;
    });
    std::erase_if(g_stretches, [&](const Stretch& s) { return mod == nullptr || s.mod == mod; });
}

// The mod holding the game's camera (camera_set), and the heat haze patches each mod drew, of a
// level load, behind a lock: the front end reads them on its own thread.
const openrac_mod* g_camera_mod = nullptr;

std::mutex g_haze_lock;
std::vector<Haze> g_haze;

std::mutex g_hud_lock;
std::vector<HudTexts> g_hud;

std::uint32_t loads_now() {
    return g_play != nullptr ? g_play->loads() : 0;
}

// Whether `v` is a unit vector (to a few parts in a thousand).
bool unit(const float v[3]) {
    const float n = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    return std::isfinite(n) && std::fabs(n - 1.0f) < 0.01f;
}

std::vector<HeroVoice> g_hero_voices;

// The looping hero sounds of one mod (all: null) stopped, those of this level load still playing.
void stop_hero_voices(const openrac_mod* mod) {
    for (const HeroVoice& v : g_hero_voices) {
        if ((mod == nullptr || v.mod == mod) && v.loads == loads_now() && g_play != nullptr) {
            if (v.moby == 0 && g_play->hero_sound_stop != nullptr) {
                g_play->hero_sound_stop(v.voice);
            } else if (v.moby != 0 && g_play->moby_sound_stop != nullptr) {
                g_play->moby_sound_stop(v.moby, v.voice);
            }
        }
    }
    std::erase_if(g_hero_voices, [&](const HeroVoice& v) {
        return mod == nullptr || v.mod == mod;
    });
}

}  // namespace detail

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
    api.collision_line = api_collision_line;
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
    g_music_gain = host.music_gain;
    g_joint_frame = host.joint_frame;
    api.hero_set_pose = api_hero_set_pose;
    api.hero_set_joints = api_hero_set_joints;
    api.collision_triangles = api_collision_triangles;
    api.prop_create = api_prop_create;
    api.moby_visuals = api_moby_visuals;
    api.guest_text = api_guest_text;
    api.sprite_tint = api_sprite_tint;
    api.pickup_animation = api_pickup_animation;
    api.moby_material = api_moby_material;
    api.moby_material_class = api_moby_material_class;
    api.moby_copy_check = api_moby_copy_check;
    api.moby_copy = api_moby_copy;
    api.moby_pool_raise = api_moby_pool_raise;
    api.moby_pool_status = api_moby_pool_status;
    api.on_weapon_fire = api_on_weapon_fire;
    api.weapon_refire = api_weapon_refire;
    api.on_shot_end = api_on_shot_end;
    api.moby_spawn_live = api_moby_spawn_live;
    api.weapon_look = api_weapon_look;
    api.moby_status = api_moby_status;
    api.weapon_record = api_weapon_record;
    api.prop_pose = api_prop_pose;
    api.prop_attach = api_prop_attach;
    api.prop_frame = api_prop_frame;
    api.prop_hide = api_prop_hide;
    api.prop_remove = api_prop_remove;
    api.prop_set_skin = api_prop_set_skin;
    api.model_create = api_model_create;
    api.moby_model_slot = api_moby_model_slot;
    api.moby_model_class = api_moby_model_class;
    api.particle_spawn_ex = api_particle_spawn_ex;
    api.particle_texture_register = api_particle_texture_register;
    api.moby_model_at = api_moby_model_at;
    api.sound_load = api_sound_load;
    api.sound_play = api_sound_play;
    api.sound_set = api_sound_set;
    api.sound_stop = api_sound_stop;
    api.camera_view = api_camera_view;
    api.camera_set = api_camera_set;
    api.camera_visibility_bypass = api_camera_visibility_bypass;
    api.rac1_ammo_panel_offset = api_rac1_ammo_panel_offset;
    api.haze_set = api_haze_set;
    api.camera_shake = api_camera_shake;
    api.hud_text_set = api_hud_text_set;
    api.hero_sound_play = api_hero_sound_play;
    api.hero_sound_stop = api_hero_sound_stop;
    api.hero_hurt = api_hero_hurt;
    api.touch_define = api_touch_define;
    api.touch_icon = api_touch_icon;
    api.touch_push = api_touch_push;
    api.touch_hint = api_touch_hint;
    api.camera_fov = api_camera_fov;
    api.hud_hide = api_hud_hide;
    api.music_volume = api_music_volume;
    api.moby_set_joints = api_moby_set_joints;
    api.moby_joint_position = api_moby_joint_position;
    api.moby_sound_play = api_moby_sound_play;
    api.moby_sound_stop = api_moby_sound_stop;
    api.scene_air = api_scene_air;
    api.time_stretch = api_time_stretch;
    api.tie_hide = api_tie_hide;
    api.prop_animate = api_prop_animate;
    api.moby_joint_frame = api_moby_joint_frame;
    api.moby_set_joint_poses = api_moby_set_joint_poses;
    api.moby_uid = api_moby_uid;
}

float camera_fov() {
    const float degrees = g_camera_fov.load(std::memory_order_relaxed);
    return degrees > 0.0f && g_camera_fov_loads.load(std::memory_order_relaxed) == loads_now()
               ? degrees
               : 0.0f;
}

std::optional<openrac_air> scene_air() {
    const std::lock_guard<std::mutex> lock(g_air_lock);
    if (!g_air || g_air->loads != loads_now()) {
        return std::nullopt;
    }
    return g_air->air;
}

float time_stretch() {
    const std::lock_guard<std::mutex> lock(g_air_lock);
    const std::uint32_t loads = loads_now();
    float most = 1.0f;
    for (const Stretch& s : g_stretches) {
        if (s.loads == loads) {
            most = std::max(most, s.stretch);
        }
    }
    return most;
}

std::vector<int> hidden_ties() {
    const std::lock_guard<std::mutex> lock(g_air_lock);
    const std::uint32_t loads = loads_now();
    std::vector<int> out;
    for (const TieHidden& h : g_ties_hidden) {
        if (h.loads == loads) {
            out.push_back(h.tie);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool hud_hidden() {
    const std::lock_guard<std::mutex> lock(g_hud_lock);
    const std::uint32_t loads = loads_now();
    return std::any_of(g_hud_hidden.begin(), g_hud_hidden.end(), [&](const auto& h) {
        return h.second == loads;
    });
}

bool ammo_panel_offset(int handle, int& x, int& y) {
    const auto found = g_ammo_panel_offsets.find(handle);
    if (found == g_ammo_panel_offsets.end() || found->second.loads != loads_now()) {
        return false;
    }
    x = found->second.x;
    y = found->second.y;
    return true;
}

bool host_camera_overridden() {
    return g_play != nullptr && g_play->camera_overridden != nullptr
           && g_play->camera_overridden() != 0;
}

bool camera_visibility_bypassed() {
    const std::lock_guard<std::mutex> lock(g_camera_visibility_lock);
    const auto loads = loads_now();
    return std::ranges::any_of(g_camera_visibility, [loads](const auto& request) {
        return request.second == loads;
    });
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
            out.push_back(
                {static_cast<int>(i),
                 p.mesh,
                 p.matrix,
                 p.tint,
                 p.blend,
                 p.attached_moby,
                 p.attached_joint,
                 p.skin,
                 p.animation,
                 p.seconds,
                 p.loop}
            );
        }
    }
    return out;
}

std::vector<MobyModelDraw> moby_models_drawn() {
    std::vector<MobyModelDraw> out;
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const std::uint32_t loads = loads_now();
    for (const auto& [slot, s] : g_slot_models) {
        if (s.mod != nullptr && s.loads == loads) {
            out.push_back({s.address, s.oclass, 0, s.mesh, s.matrix, s.hidden});
        }
    }
    for (const auto& [address, s] : g_address_models) {
        if (s.mod != nullptr && s.loads == loads) {
            out.push_back({address, s.oclass, 0, s.mesh, s.matrix, s.hidden});
        }
    }
    for (const auto& [key, c] : g_class_models) {
        if (c.mod != nullptr && c.loads == loads) {
            out.push_back({0, key.first, key.second, c.mesh, c.matrix, false});
        }
    }
    return out;
}

void prop_placed(int id, const std::array<float, 16>& matrix) {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    if (id < 0 || static_cast<std::size_t>(id) >= g_props.size()) {
        return;
    }
    Prop& p = g_props[static_cast<std::size_t>(id)];
    if (p.mod != nullptr && p.attached_moby != 0) {
        p.placed = matrix;
        p.was_placed = true;
    }
}

std::vector<std::filesystem::path> prop_meshes() {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    // The list of a level load that is over is not the next one's.
    return g_prop_meshes_loads == loads_now() ? g_prop_meshes
                                              : std::vector<std::filesystem::path>{};
}

std::uint32_t sprite_tint() {
    return g_sprite_tint;
}

std::vector<int> visual_classes() {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    return g_visual_classes;
}

unsigned visual_classes_generation() {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    return g_visual_generation;
}

std::vector<openrac_moby_visual> moby_visuals() {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::vector<openrac_moby_visual> result;
    for (const auto& set : g_visuals) {
        if (set.loads == loads_now()) {
            result.insert(result.end(), set.visuals.begin(), set.visuals.end());
        }
    }
    return result;
}

// Returns the materials on the frame's live mobys, and forgets the ones whose moby is gone
// (mods.h). Rebuilds the list only on a frame where some mod has a material set.
std::vector<MobyMaterial> moby_materials(const std::vector<LiveMobyId>& live) {
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::vector<MobyMaterial> result;
    // No mod has set one: nothing to look up, most frames.
    if (g_materials.empty()) {
        return result;
    }
    std::unordered_map<openrac_gaddr, int> classes;
    classes.reserve(live.size());
    // The frame's live mobys by address, each with its class.
    for (const LiveMobyId& m : live) {
        classes.emplace(m.moby, m.oclass);
    }
    std::vector<MaterialEntry> kept;
    // Each one set, in the order set: kept, drawn, or dropped with its moby.
    for (MaterialEntry& e : g_materials) {
        // Another level load's materials are gone with its mobys.
        if (e.loads != loads) {
            continue;
        }
        const auto found = classes.find(e.material.moby);
        // Not listed yet, kept: the renderer's frame may come from before the game made the moby.
        // Listed before and not now: the game removed it.
        if (found == classes.end()) {
            // Never listed: still waiting for the game's first frame with it.
            if (e.oclass < 0) {
                kept.push_back(e);
            }
            continue;
        }
        // The first frame that lists it fixes its class; another class there is another moby.
        if (e.oclass < 0) {
            e.oclass = found->second;
        } else if (e.oclass != found->second) {
            continue;
        }
        kept.push_back(e);
        // The last set wins: one set earlier on this moby, by another mod, gives way.
        std::erase_if(result, [&](const MobyMaterial& r) { return r.moby == e.material.moby; });
        result.push_back(e.material);
    }
    g_materials = std::move(kept);
    return result;
}

std::vector<ClassMaterialDraw> class_materials() {
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::vector<ClassMaterialDraw> out;
    // This level load's, one a class and scope.
    for (const auto& [key, c] : g_class_materials) {
        if (c.mod != nullptr && c.loads == loads) {
            out.push_back({key.first, key.second, {0, c.material, c.rgb}});
        }
    }
    return out;
}

MobyMaterial moby_material_drawn(std::uint32_t moby) {
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    // The last set on it is the one drawn (moby_materials).
    for (auto e = g_materials.rbegin(); e != g_materials.rend(); ++e) {
        // Its own, of this level load.
        if (e->loads == loads && e->material.moby == moby) {
            return e->material;
        }
    }

    return MobyMaterial{moby};
}

std::unordered_map<std::uint32_t, MobyMaterial> energy_materials() {
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::unordered_map<std::uint32_t, MobyMaterial> out;
    // In the order set, so the last set on a moby is the one kept; another material set later on
    // it takes it out again.
    for (const MaterialEntry& e : g_materials) {
        if (e.loads != loads) {
            continue;
        }
        if (OPENRAC_MOBY_MATERIAL_KIND(e.material.material) == OPENRAC_MOBY_MATERIAL_ENERGY) {
            out.insert_or_assign(e.material.moby, e.material);
        } else {
            out.erase(e.material.moby);
        }
    }

    return out;
}

std::unordered_map<std::uint32_t, MobyMaterial> particle_looks() {
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::unordered_map<std::uint32_t, MobyMaterial> out;
    for (const auto& [record, look] : g_particle_looks) {
        if (look.loads == loads) {
            out.emplace(record, look.look);
        }
    }
    return out;
}

bool moby_platinum(std::uint32_t moby) {
    return moby_material_drawn(moby).material == OPENRAC_MOBY_MATERIAL_PLATINUM;
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

// The overrides of mobys the game freed or made again in their slots are no one's: dropped. Once a
// tick, on the game thread (the game's list is read outside the lock).
void drop_gone_slot_models() {
    std::vector<std::pair<int, SlotModel>> held;
    {
        const std::lock_guard<std::mutex> lock(g_props_lock);
        held.assign(g_slot_models.begin(), g_slot_models.end());
    }
    if (held.empty()) {
        return;
    }
    const MobyClasses classes;
    for (const auto& [slot, s] : held) {
        if (s.loads == loads_now() && moby_address(slot) == s.address
            && classes.of(slot) == s.oclass) {
            continue;
        }
        const std::lock_guard<std::mutex> lock(g_props_lock);
        const auto now = g_slot_models.find(slot);
        if (now != g_slot_models.end() && now->second.address == s.address) {
            g_slot_models.erase(now);
        }
    }
}

// The overrides of mobys whose class at their address changed (or whose level went) are no one's:
// dropped. Once a tick, on the game thread (the game's class is read outside the lock).
void drop_gone_address_models() {
    std::vector<std::pair<gaddr, SlotModel>> held;
    {
        const std::lock_guard<std::mutex> lock(g_props_lock);
        held.assign(g_address_models.begin(), g_address_models.end());
    }
    for (const auto& [address, s] : held) {
        const std::int32_t oclass = g_play != nullptr && g_play->moby_class_at != nullptr
                                        ? g_play->moby_class_at(address)
                                        : -1;
        if (s.loads == loads_now() && oclass == s.oclass) {
            continue;
        }
        const std::lock_guard<std::mutex> lock(g_props_lock);
        const auto now = g_address_models.find(address);
        if (now != g_address_models.end() && now->second.loads == s.loads
            && now->second.oclass == s.oclass) {
            g_address_models.erase(now);
        }
    }
}

void play_update() {
    drop_gone_slot_models();
    drop_gone_address_models();
    drop_stale_joint_sets();
    // The music's gains went with their level: back at full.
    const std::uint32_t loads = loads_now();
    if (std::erase_if(g_music_gains, [&](const MusicGain& g) { return g.loads != loads; }) != 0) {
        apply_music_gain();
    }
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
        g_sprite_tint = UINT32_MAX;
        return false;
    }
    --g_skip_ticks;
    return true;
}

std::string play_notice() {
    const std::lock_guard<std::mutex> lock(g_notice_lock);
    return g_notice;
}

std::uint32_t overlay_phase_now() {
    std::int32_t level = -1;
    switch (phase_now(&level)) {
        case OPENRAC_PLAY_PLAYING:
            return OPENRAC_OVERLAY_PLAYING;
        case OPENRAC_PLAY_MENU:
            return OPENRAC_OVERLAY_MENU;
        case OPENRAC_PLAY_SCENE:
            return OPENRAC_OVERLAY_SCENE;
        default:
            return 0;
    }
}

void reset_play() {
    drop_air(nullptr);
    g_ammo_panel_offsets.clear();
    {
        const std::lock_guard<std::mutex> lock(g_camera_visibility_lock);
        g_camera_visibility.clear();
    }
    {
        const std::lock_guard<std::mutex> lock(g_notice_lock);
        g_notice.clear();
    }
    g_skip_ticks = 0;
    g_sprite_tint = UINT32_MAX;
    // A scene a mod asked for and the game has not started yet.
    if (g_play != nullptr && g_play->scene_start != nullptr) {
        g_play->scene_start(-1);
    }
    g_figures.fill(Figure{});
    {
        const std::lock_guard<std::mutex> lock(g_props_lock);
        g_props.fill(Prop{});
        g_visuals.clear();
        g_visual_classes.clear();
        ++g_visual_generation;
        g_materials.clear();
        g_class_materials.clear();
        g_models.fill(Model{});
        g_slot_models.clear();
        g_class_models.clear();

        // No prop is left to need its mesh; the scene lets it go with its level.
        if (!g_prop_meshes.empty()) {
            g_prop_meshes.clear();
            ++g_prop_generation;
        }
    }
    {
        const std::lock_guard<std::mutex> lock(g_haze_lock);
        g_haze.clear();
    }
    {
        const std::lock_guard<std::mutex> lock(g_hud_lock);
        g_hud.clear();
        g_hud_hidden.clear();
    }
    if (!g_music_gains.empty()) {
        g_music_gains.clear();
        apply_music_gain();
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
    release_camera_fov();
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
    drop_weapon_shots(nullptr);
}

void drop_figures(const openrac_mod* mod) {
    drop_weapon_shots(mod);
    drop_air(mod);
    {
        const std::lock_guard<std::mutex> lock(g_props_lock);
        std::erase_if(g_visuals, [&](const auto& set) { return set.mod == mod; });
        std::erase_if(g_materials, [&](const MaterialEntry& e) { return e.mod == mod; });
        std::erase_if(g_class_materials, [&](const auto& c) { return c.second.mod == mod; });
    }
    std::erase_if(g_ammo_panel_offsets, [mod](const auto& entry) {
        return entry.second.mod == mod;
    });
    {
        const std::lock_guard<std::mutex> lock(g_camera_visibility_lock);
        g_camera_visibility.erase(mod);
    }
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
        for (Model& m : g_models) {
            if (m.mod == mod) {
                m = Model{};
            }
        }
        std::erase_if(g_slot_models, [&](const auto& s) { return s.second.mod == mod; });
        std::erase_if(g_class_models, [&](const auto& c) { return c.second.mod == mod; });
    }
    {
        const std::lock_guard<std::mutex> lock(g_haze_lock);
        std::erase_if(g_haze, [&](const Haze& h) { return h.mod == mod; });
    }
    {
        const std::lock_guard<std::mutex> lock(g_hud_lock);
        std::erase_if(g_hud, [&](const HudTexts& h) { return h.mod == mod; });
        std::erase_if(g_hud_hidden, [&](const auto& h) { return h.first == mod; });
    }
    if (std::erase_if(g_music_gains, [&](const MusicGain& g) { return g.mod == mod; }) != 0) {
        apply_music_gain();
    }
    std::vector<std::uint32_t> turned;
    for (const JointSet& s : g_joint_sets) {
        if (s.mod == mod && std::find(turned.begin(), turned.end(), s.moby) == turned.end()) {
            turned.push_back(s.moby);
        }
    }
    std::erase_if(g_joint_sets, [&](const JointSet& s) { return s.mod == mod; });
    if (g_joint_offsets != nullptr) {
        for (const std::uint32_t moby : turned) {
            apply_joint_sets(moby);
        }
    }
    stop_hero_voices(mod);
    if (g_camera_mod == mod) {
        if (g_play != nullptr && g_play->camera_set != nullptr) {
            g_play->camera_set(nullptr, nullptr);
        }
        g_camera_mod = nullptr;
        release_camera_fov();
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

int openrac_moby_platinum(gaddr moby) {
    return openrac::mods::moby_platinum(moby) ? 1 : 0;
}

int openrac_moby_obsidian(gaddr moby, uint32_t* rgb) {
    const openrac::mods::MobyMaterial drawn = openrac::mods::moby_material_drawn(moby);
    // Not obsidian: nothing to say of its colour.
    if (drawn.material != OPENRAC_MOBY_MATERIAL_OBSIDIAN) {
        return 0;
    }
    if (rgb != nullptr) {
        *rgb = drawn.rgb;
    }
    return 1;
}
