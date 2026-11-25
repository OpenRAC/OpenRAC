// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the files of the mods' interface share (mod_api/<area>.cpp, mods.cpp, mod_play.cpp): the
// loaded mod, the state more than one file reads, and every api_<member> function, which fills
// openrac_api (sdk/api/members.tsv names each member's area, and so its file). Port-internal.

#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/build_info.h"
#include "common/c_boundary.h"
#include "common/crash.h"
#include "common/env.h"
#include "common/json.h"
#include "common/log.h"
#include "mod_library.h"
#include "mod_manifest.h"
#include "mod_play_queries.h"
#include "mod_scripts.h"
#include "mod_touch.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/memory.h"
#include "openrac/openrac_mod.h"

namespace fs = std::filesystem;

// One loaded mod: the handle openrac_mod.h's functions are given.
struct openrac_mod {
    std::string id;
    std::string version;
    std::string file;       // the library's file name (a script mod's: its first script)
    bool script = false;    // a script mod (mod_scripts.h): no library
    bool built_in = false;  // built into the program (BuiltIn): its settings are the program's
    void* library = nullptr;
    const void* base = nullptr;
    openrac::mods::Manifest manifest;
    std::map<std::string, openrac::mods::SettingValue> settings;
    std::string failure;  // fail()'s reason, or a hook point refused
    bool hook_refused = false;
    std::vector<std::pair<void**, void*>> taken;  // each slot it filled, and what was there before
    std::vector<std::string> hooks;               // the hook points it took
    std::string data_dir;                         // data_dir's answer, once made (1.11)
    fs::path dir;                                 // its folder (1.12: props' meshes, sounds)
    openrac_mod_unload_fn unload = nullptr;

    // What a party needs of it (interface 1.14): its settings as text, made when first asked and
    // kept for the life of the mod (the table hands out their strings), the rules it published
    // (none yet: not published), how often, and who takes the host's rules.
    struct PartySetting {
        std::string value, shown;
    };

    std::vector<PartySetting> party_settings;
    bool has_party_rules = false;
    std::vector<std::uint8_t> party_rules;
    std::string party_text;
    std::uint32_t party_version = 0;
    openrac_party_rules_fn party_fn = nullptr;
    void* party_user = nullptr;
};

namespace openrac::mods::detail {

struct FrameCallback {
    openrac_frame_fn fn;
    void* user;
    openrac_mod* mod;
};

struct PadCallback {
    openrac_pad_fn fn;
    void* user;
    openrac_mod* mod;
};

struct TickBoundaryCallback {
    openrac_tick_boundary_fn fn;
    void* user;
    openrac_mod* mod;
};

// A front-end menu entry and the page it opens (interface 1.5).
struct MenuCallback {
    std::string label;
    openrac_menu_fn fn;
    void* user;
    openrac_mod* mod;
};

// A page of the game's own menus (interface 1.7): what is on it, and whose it is.
struct UiPageCallback {
    UiPage page;
    openrac_ui_fn fn;
    void* user;
    openrac_mod* mod;
};

// Who draws over the game (on_overlay, on_overlay_phases), in load order, and over which phases.
struct OverlayCallback {
    openrac_mod* mod;
    openrac_overlay_fn fn;
    void* user;
    std::uint32_t phases;  // OPENRAC_OVERLAY_* bits
};

struct SettingsCallback {
    openrac_frame_fn fn;
    void* user;
    openrac_mod* mod;
};

struct SaveDataCallback {
    openrac_frame_fn fn;
    void* user;
    openrac_mod* mod;
};

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

// The mod told of the metal detector's digs.
struct MetalWatch {
    openrac_mod* mod = nullptr;
    openrac_metal_detect_fn fn = nullptr;
    void* user = nullptr;
};

// Who hears of the game's camera scripts (on_camera_script), and the one showing one here.
struct CameraScriptWatch {
    const openrac_mod* mod = nullptr;
    openrac_camera_script_fn fn = nullptr;
    void* user = nullptr;
};

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
    // Fixed to a moby's joint (1.18): the moby, its joint; `matrix` is the offset then.
    gaddr attached_moby = 0;
    int attached_joint = -1;
    // Where the renderer drew it on that joint last (prop_frame), once it has.
    std::array<float, 16> placed{};
    bool was_placed = false;
    // The joint matrices prop_set_skin gave it (interface 1.33), none for a prop drawn in its bind
    // pose. Never changed once set, only replaced: the front end holds it while it draws.
    std::shared_ptr<const std::vector<std::array<float, 16>>> skin;
    int animation = -1;  // prop_animate's: its .glb's animation, the time into it, looped
    float seconds = 0.0f;
    bool loop = false;
};

// A sound a mod loaded: whose, and the host's id of it.
struct Sound {
    const openrac_mod* mod = nullptr;
    int host = -1;
};

// Ammo panel queue handles are unique within a level load. The game draws and
// mods register offsets on the same thread.
struct AmmoPanelOffset {
    const openrac_mod* mod;
    std::uint32_t loads;
    int x;
    int y;
};

// A model a mod made (model_create): whose, its mesh and the level load it was made in. The mesh is
// one of the level's prop meshes (g_prop_meshes), so the front end holds it in its scene as a
// prop's, and goes with the same level load.
struct Model {
    const openrac_mod* mod = nullptr;
    std::filesystem::path mesh;
    std::uint32_t loads = 0;
};

// What a mod drew a moby with: whose, the moby it was set on (its address and class when it was
// set), the model's mesh as the model was when it was set, and how it is placed and posed.
struct SlotModel {
    const openrac_mod* mod = nullptr;
    gaddr address = 0;
    int oclass = -1;
    std::uint32_t loads = 0;
    std::filesystem::path mesh;
    std::array<float, 16> matrix{};
    bool hidden = false;
};

// What a mod drew the mobys of a class with in one scope (interface 1.33): whose, the mesh, how it is
// placed and posed, and the level load.
struct ClassModel {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    std::filesystem::path mesh;
    std::array<float, 16> matrix{};
};

// A mod's material on every moby of a class in one scope (openrac_api.moby_material_class,
// interface 1.34): the material and rgb moby_material takes.
struct ClassMaterial {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    std::uint32_t material = OPENRAC_MOBY_MATERIAL_NONE;
    std::uint32_t rgb = 0xFFFFFF;
};

struct VisualSet {
    const openrac_mod* mod;
    std::uint32_t loads;
    std::vector<openrac_moby_visual> visuals;
};

// A mod's material on one moby (interface 1.23), for the level load it was set in. `oclass` is the
// moby's class when the renderer first listed it (-1 before that), so that a slot the game frees
// and fills with another class drops it.
struct MaterialEntry {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    MobyMaterial material;
    int oclass = -1;
};

// The joint offsets each mod set on a moby (hero_set_joints for the hero, moby_set_joints for any
// other), in the order the mods first set theirs: all of a moby's go to it together, each mod's
// composed after the ones before it, so two mods that turn its joints do not undo each other.
struct JointSet {
    const openrac_mod* mod = nullptr;
    std::uint32_t moby = 0;
    std::uint32_t loads = 0;  // the level load it was set in
    std::vector<openrac_joint_offset> joints;
};

struct Haze {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    std::vector<openrac_haze> patches;
};

// The lines of text each mod shows in the game's HUD (hud_text_set), of a level load: set and read
// on the game's thread, behind a lock as the haze is.
struct HudTexts {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    std::vector<openrac_hud_text> lines;
};

// The looping sounds each mod started on the hero (hero_sound_play, moby 0) or another moby
// (moby_sound_play), stopped with the mod.
struct HeroVoice {
    openrac_mod* mod;
    std::uint32_t loads;
    int voice;
    gaddr moby = 0;
};

// Each mod's gain on the game's music (music_volume) and the level load it was set in: the
// quietest of this load's applies.
struct MusicGain {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    float gain = 1.0f;
};

// The air a mod draws the picture with (scene_air): the last to set it, of a level load; slow motion
// each mod asks for (time_stretch); the ties each mod hides (tie_hide). Behind g_air_lock: the
// window's thread reads them.
struct AirSet {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    openrac_air air{};
};

struct Stretch {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    float stretch = 1.0f;
};

struct TieHidden {
    const openrac_mod* mod = nullptr;
    std::uint32_t loads = 0;
    int tie = 0;
};

// ---- State kept in mods.cpp and mod_play.cpp ----

extern const Host* g_host;
extern std::vector<OverlayCallback> g_overlays;
extern openrac_mod* g_loading;
extern std::vector<std::unique_ptr<openrac_mod>> g_loaded;
extern std::vector<FrameCallback> g_frames;
extern std::vector<PadCallback> g_pads;
extern PlayQueries g_play_queries;
extern std::vector<MenuCallback> g_menus;
extern std::vector<DeveloperMenu> g_developer_menus;
extern std::vector<UiPageCallback> g_ui_pages;
extern std::vector<UiMainItem> g_ui_main;
extern std::vector<UiPauseItem> g_ui_pause;
extern int g_ui_shown;
extern bool g_ui_question;
extern bool g_ui_main_menu;
extern UiRequests g_ui_requests;
extern openrac_mod* g_drawing;
extern std::vector<TickBoundaryCallback> g_tick_boundaries;
extern openrac_mod* g_tick_mod;
extern bool g_in_tick_boundary;
extern openrac_view_source g_views;
extern openrac_mod* g_views_mod;
extern openrac_vulkan_setup g_vulkan;
extern openrac_mod* g_vulkan_mod;
extern bool g_party_replay;
extern std::vector<SettingsCallback> g_settings_callbacks;
extern std::vector<SaveDataCallback> g_save_data_callbacks;
extern std::map<std::string, std::vector<std::uint8_t>, std::less<>> g_save_data;
extern const openrac_game_play* g_play;

// The class of each moby the game lists, by array slot: -1 for a slot it does not list. Read from
// the game's list, which the mods ask for every tick. Game thread, outside g_props_lock.
struct MobyClasses {
    std::vector<openrac_game_moby_state> states;

    MobyClasses() : states(kSlots) {
        if (g_play != nullptr && g_play->mobys != nullptr) {
            const int n = g_play->mobys(states.data(), static_cast<int>(states.size()));
            states.resize(static_cast<std::size_t>(std::clamp(n, 0, static_cast<int>(kSlots))));
        } else {
            states.clear();
        }
    }

    int of(int slot) const {
        const auto s = std::find_if(states.begin(), states.end(), [&](const auto& m) {
            return m.slot == slot;
        });
        return s != states.end() ? s->oclass : -1;
    }
};

extern std::filesystem::path g_cards;
extern std::array<Figure, OPENRAC_FIGURES_MAX> g_figures;
extern Request g_request;
extern std::vector<Drive> g_drives;
extern Gate g_gate;
extern MetalWatch g_metal;
extern HitWatch g_hits;
extern int g_before_slot;
extern int g_card_mode;
extern const openrac_mod* g_loading_mod;
extern int g_skip_ticks;
extern int g_skip_phase;
extern std::int32_t g_skip_scene;
extern std::mutex g_notice_lock;
extern std::string g_notice;
extern CameraScriptWatch g_camera_scripts;
extern const openrac_mod* g_camera_shower;
extern std::mutex g_props_lock;
extern std::array<Prop, OPENRAC_PROPS_MAX> g_props;
extern std::vector<std::filesystem::path> g_prop_meshes;
extern unsigned g_prop_generation;
extern std::uint32_t g_prop_meshes_loads;
extern std::array<Model, OPENRAC_MODELS_MAX> g_models;
extern std::map<int, SlotModel> g_slot_models;
extern std::map<std::pair<int, std::uint32_t>, ClassModel> g_class_models;
extern std::map<gaddr, SlotModel> g_address_models;
extern std::map<std::pair<int, std::uint32_t>, ClassMaterial> g_class_materials;
extern std::mutex g_camera_visibility_lock;
extern std::map<const openrac_mod*, std::uint32_t> g_camera_visibility;
extern std::map<int, AmmoPanelOffset> g_ammo_panel_offsets;
extern std::vector<VisualSet> g_visuals;
extern std::vector<int> g_visual_classes;
extern unsigned g_visual_generation;
extern std::uint32_t g_sprite_tint;
extern std::vector<MaterialEntry> g_materials;
extern std::vector<Sound> g_sounds;
extern std::vector<std::pair<const openrac_mod*, std::uint32_t>> g_voices;
extern int (*g_sound_load)(const std::filesystem::path&);
extern std::uint32_t (*g_sound_play)(int, float, float, bool);
extern int (*g_sound_set)(std::uint32_t, float, float);
extern void (*g_sound_stop)(std::uint32_t);
extern void (*g_joint_offsets)(std::uint32_t, const openrac_joint_offset*, int);
extern std::vector<JointSet> g_joint_sets;
extern const openrac_mod* g_camera_mod;
extern std::mutex g_haze_lock;
extern std::vector<Haze> g_haze;
extern std::mutex g_hud_lock;
extern std::vector<HudTexts> g_hud;
extern std::vector<HeroVoice> g_hero_voices;
extern std::vector<std::pair<const openrac_mod*, std::uint32_t>> g_hud_hidden;
extern std::vector<MusicGain> g_music_gains;

// The mods that drive the pads' motors (openrac_api.set_motors, mods::any_mod_motors), behind
// g_motor_mods_lock.
extern std::mutex g_motor_mods_lock;
extern std::vector<const openrac_mod*> g_motor_mods;
extern void (*g_music_gain)(int, float);
extern int (*g_joint_frame)(std::uint32_t, int, float*);
extern std::mutex g_air_lock;
extern std::optional<AirSet> g_air;
extern std::vector<Stretch> g_stretches;
extern std::vector<TieHidden> g_ties_hidden;

// ---- Functions more than one file calls, the api_<member> ones among them ----

const char* hook_id(const Host& host, const char* name);
// The hook point a name (its id, name or an alias) names, or null.
const HookPoint* find_hook_point(const char* name);
void api_log(openrac_mod* mod, int level, const char* text);
void api_fail(openrac_mod* mod, const char* reason);
const SettingValue* setting(openrac_mod* mod, const char* key, SettingType a, SettingType b);
int api_setting_bool(openrac_mod* mod, const char* key);
int64_t api_setting_int(openrac_mod* mod, const char* key);
double api_setting_float(openrac_mod* mod, const char* key);
const char* api_setting_string(openrac_mod* mod, const char* key);
int api_hook(
    openrac_mod* mod, const char* name, const char* signature, void* replacement, void** original
);
int api_on_frame(openrac_mod* mod, openrac_frame_fn fn, void* user);
int api_on_tick_boundary(openrac_mod* mod, openrac_tick_boundary_fn fn, void* user);
int api_developer_menu(
    openrac_mod* mod,
    const char* title,
    const openrac_developer_menu_item* items,
    std::uint32_t count
);
std::uint32_t api_take_developer_menu_action(openrac_mod* mod);
void note_prop_mesh(std::uint32_t loads, const std::filesystem::path& mesh);
int api_prop_set_skin(
    openrac_mod* mod, int id, const float* joint_matrices, std::uint32_t joint_count
);
int api_model_create(openrac_mod* mod, const char* mesh_path);
int api_moby_model_slot(openrac_mod* mod, int slot, const openrac_moby_model* m);
int api_moby_model_class(
    openrac_mod* mod, int oclass, std::uint32_t scope, const openrac_moby_model* m
);
int api_moby_model_at(openrac_mod* mod, std::uint32_t address, const openrac_moby_model* m);
int api_particle_spawn_ex(openrac_mod* mod, const openrac_particle_spawn* spawn);
int api_particle_texture_register(openrac_mod* mod, const openrac_particle_texture* texture);
int api_camera_visibility_bypass(openrac_mod* mod, int enabled);
int api_rac1_ammo_panel_offset(openrac_mod* mod, int handle, int x, int y);
int api_camera_shake(openrac_mod* mod, int channel, float amplitude, int frames);
int api_game_data_dir(openrac_mod* mod, const char* game, char* out, std::uint32_t size);
const char* api_data_dir(openrac_mod* mod);
int api_developer_menu_set_label(openrac_mod* mod, std::uint32_t action, const char* label);
DeveloperMenuItem* developer_menu_item(openrac_mod* mod, std::uint32_t action);
int api_developer_menu_switch(openrac_mod* mod, std::uint32_t action, int on);
int api_developer_menu_choice(
    openrac_mod* mod,
    std::uint32_t action,
    const char* const* values,
    std::uint32_t count,
    std::uint32_t index
);
int api_developer_menu_look(
    openrac_mod* mod, std::uint32_t action, const openrac_developer_menu_look* look
);
void* api_hook_entry(const char* name, const char* signature);
int api_developer_menu_value(openrac_mod* mod, std::uint32_t action);
int api_on_pad(openrac_mod* mod, openrac_pad_fn fn, void* user);
void* api_function(const char* name);
void* api_function_at(openrac_gaddr address);
openrac_gaddr api_relocate_data(openrac_gaddr address);
openrac_gaddr api_relocate_level_data(int level, openrac_gaddr address);
int api_loaded_level(void);
openrac_gaddr api_stack_push(uint32_t size, openrac_gaddr* saved);
void api_stack_pop(openrac_gaddr saved);
const openrac_input* api_input(void);
int api_take_input(openrac_mod* mod, uint32_t devices);
void api_capture_mouse(openrac_mod* mod, int capture);
const openrac_graphics* api_graphics(void);
bool native_vulkan_allowed(const mods::Manifest& manifest);
int api_on_views(openrac_mod* mod, const openrac_view_source* source);
int api_setup_vulkan(openrac_mod* mod, const openrac_vulkan_setup* setup);
int api_menu_entry(openrac_mod* mod, const char* label, openrac_menu_fn fn, void* user);
void api_menu_rect(openrac_mod* mod, int x, int y, int width, int height, uint32_t rgba);
void api_menu_text(openrac_mod* mod, int x, int y, int size, uint32_t rgba, const char* text);
UiPageCallback* mod_page(openrac_mod* mod, int page);
int settle_cursor(const UiPage& page, int cursor);
int api_ui_page(openrac_mod* mod, uint32_t flags, openrac_ui_fn fn, void* user);
int api_ui_main_item(openrac_mod* mod, const char* after, const char* label, int page);
int api_ui_pause_item(openrac_mod* mod, const char* label, int page);
int api_ui_set(openrac_mod* mod, int page, const openrac_ui_content* content);
int api_ui_open(openrac_mod* mod, int page);
int api_ui_confirm(openrac_mod* mod, int page, const char* question, uint32_t tag);
int api_ui_cursor(openrac_mod* mod, int page, int item);
int api_ui_sound(openrac_mod* mod, int sound);
int api_mod_count();
int api_mod_entry(int index, openrac_mod_entry* out);
std::string setting_text(const SettingValue& v);
int api_mod_setting(int index, int setting, openrac_mod_setting* out);
int api_party_rules_publish(openrac_mod* mod, const void* data, uint32_t size, const char* text);
int api_party_rules_read(int index, void* out, uint32_t size, uint32_t* version);
const char* api_party_rules_text(int index);
int api_on_party_rules(openrac_mod* mod, openrac_party_rules_fn fn, void* user);
int api_party_rules_deliver(int index, const void* data, uint32_t size);
void api_party_replay(int on);
int api_party_replaying();
int api_on_overlay(openrac_mod* mod, openrac_overlay_fn fn, void* user);
int api_on_overlay_phases(
    openrac_mod* mod, openrac_overlay_fn fn, void* user, std::uint32_t phases
);
int api_button_label(openrac_mod* mod, std::uint32_t button, char* out, std::uint32_t size);
int api_packs_info(openrac_packs_info* out);
int api_packs_entry(int index, openrac_pack_entry* out);
int api_clipboard_set(const char* text);
int api_open_link(const char* url);
int api_plugins_folder(char* out, std::uint32_t size);
// The edited levels' pack the game reads now, or null for none.
const char* pack_folder();
int api_pack_folder(char* out, std::uint32_t size);
int api_game_language(char* out, std::uint32_t size);
int api_identity_key(char* out, std::uint32_t size);
std::string quoted(std::string_view s);
std::string number_text(double d);
int set_setting(
    openrac_mod* mod, const char* key, SettingType a, SettingType b, const std::string& literal
);
int api_setting_set_bool(openrac_mod* mod, const char* key, int value);
int api_setting_set_int(openrac_mod* mod, const char* key, int64_t value);
int api_setting_set_float(openrac_mod* mod, const char* key, double value);
int api_setting_set_string(openrac_mod* mod, const char* key, const char* value);
int api_on_settings(openrac_mod* mod, openrac_frame_fn fn, void* user);
bool same(const SettingValue& x, const SettingValue& y);
int api_save_data_set(openrac_mod* mod, const void* bytes, std::uint32_t size);
std::uint32_t api_save_data_get(openrac_mod* mod, void* out, std::uint32_t size);
int api_on_save_data(openrac_mod* mod, openrac_frame_fn fn, void* user);
int phase_now(std::int32_t* level);
bool in_level(int phase);
bool live(const Figure& f);
Figure* own_figure(const openrac_mod* mod, int id);
bool save_info(const std::uint8_t* save, std::size_t size, openrac_save_info* out);
int api_play_status(openrac_play_status* out);
const char* api_level_name(int level);
int api_hero_pose(openrac_pose* out);
int api_figure_create(openrac_mod* mod);
int api_figure_pose(openrac_mod* mod, int id, const openrac_pose* pose);
int api_figure_remove(openrac_mod* mod, int id);
int api_play_start(openrac_mod* mod, int level, const void* save, size_t size);
bool in_a_level(int phase);
int api_play_travel(openrac_mod* mod, int level);
size_t api_save_capture(void* out, size_t size);
int api_save_slot_current(void);
int api_save_slot(int slot, openrac_save_info* out);
size_t api_save_read(int slot, void* out, size_t size);
const openrac_mod* driver(int slot);
int api_moby_list(openrac_moby* out, int max);
int api_moby_slot(int key);
gaddr moby_address(int slot);
int api_moby_drive(openrac_mod* mod, int slot, int on);
int api_moby_pose(openrac_mod* mod, int slot, const openrac_moby* pose);
int api_moby_kill(openrac_mod* mod, int slot);
int api_moby_spawn(openrac_mod* mod, int oclass, const openrac_moby* pose);
int api_moby_copy_check(openrac_mod* mod, int slot);
int api_moby_copy(openrac_mod* mod, int slot, int index);
int api_moby_pool_raise(openrac_mod* mod, int extra);
int api_moby_pool_status(openrac_mod* mod, openrac_moby_pool* out);
int api_figure_slot(openrac_mod* mod, int id);
int api_on_moby_update(openrac_mod* mod, openrac_moby_update_fn fn, void* user);
int api_hero_stand_in(openrac_mod* mod, const float position[3], int figure);
int api_on_moby_hit(openrac_mod* mod, openrac_hit_fn fn, void* user);
int api_moby_hit(openrac_mod* mod, const openrac_hit* hit);
int api_play_eyes(openrac_mod* mod, const float* xyz, int count);
int api_play_overlay_pause(openrac_mod* mod, int on);
int api_moby_slot_of(openrac_gaddr moby);
int api_collision_line(
    const float from[3],
    const float to[3],
    std::uint32_t flags,
    std::int32_t ignore,
    openrac_line_hit* out
);
std::int32_t scene_now();
int api_play_skip(openrac_mod* mod);
int api_bolts(int add);
int api_hero_held(openrac_held* out);
int api_figure_held(openrac_mod* mod, int id, const openrac_held* held);
int api_hero_shots(openrac_shot* out, int max);
int api_figure_shot(openrac_mod* mod, int id, const openrac_shot* shot);
int api_play_notice(openrac_mod* mod, const char* text);
int api_scene_playing(int* frame);
int api_play_scene(openrac_mod* mod, int scene);
int api_moby_hide(openrac_mod* mod, int slot, int hidden);
int api_moby_deletion(int slot, openrac_moby_deletion* out);
int api_moby_run(int slot, openrac_moby_run* out);
int api_card_filter(openrac_mod* mod, int mode, const char* folder);
int api_card_slot_hint(openrac_mod* mod, int slot);
int api_coop_save_write(openrac_mod* mod, int slot, const void* bytes, std::uint32_t size);
int api_save_slot2(const char* folder, int slot, openrac_save_info* out);
std::size_t api_save_read2(const char* folder, int slot, void* out, std::size_t size);
std::uint32_t api_card_saves();
int api_card_note_write(openrac_mod* mod, int slot, const char* text);
int api_card_note_read(const char* folder, int slot, char* out, std::uint32_t size);
int api_shared_read(int tag, void* out, std::uint32_t size);
int api_shared_give(int tag, const void* data, std::uint32_t size);
int api_mission_state_read(int level, std::uint8_t* out);
int api_mission_state_write(int level, const std::uint8_t* in);
int api_mission_state_clear(int level, const std::uint8_t* flags);
int api_hero_place(openrac_mod* mod, const float position[3], float yaw, std::uint32_t flags);
int api_hero_grab(openrac_mod* mod, int animation, std::uint32_t flags);
int api_hero_release(openrac_mod* mod);
int api_hero_hold_at(openrac_mod* mod, const float position[3], float yaw);
int api_hero_stand_in_as(openrac_mod* mod, const openrac_stand_in* as, int figure);
int api_hero_stand_in_read(openrac_mod* mod, openrac_stand_in* out);
int api_hero_revive(openrac_mod* mod, const float position[3], std::uint32_t flags);
int api_hero_shots2(openrac_shot2* out, int max);
int api_figure_shot2(openrac_mod* mod, int id, const openrac_shot2* shot);
int api_moby_list2(openrac_moby2* out, int max);
bool has_camera_scripts();
int api_on_camera_script(openrac_mod* mod, openrac_camera_script_fn fn, void* user);
int api_camera_script_play(openrac_mod* mod, int id);
int api_camera_script_view(float eye[3], float rotation[3], float where[3], int* stand_in);
int api_camera_script_show(openrac_mod* mod, const float eye[3], const float rotation[3]);
int api_play_scene_from(openrac_mod*, int, int);
int api_play_live_picture(openrac_mod* mod, int on);
int api_on_metal_detect(openrac_mod* mod, openrac_metal_detect_fn fn, void* user);
int api_camera_position(float out[3]);
int api_level_line(const float from[3], const float to[3], int* blocked);
int api_world_family(char* out, std::uint32_t size);
int api_sim_digest(std::int32_t level, std::uint32_t* out);
std::uint32_t api_sprite_tint(openrac_mod* mod, std::uint32_t rgb);
void apply_joint_sets(std::uint32_t moby);
void drop_stale_joint_sets();
int set_joint_set(
    const openrac_mod* mod, std::uint32_t moby, const openrac_joint_offset* joints, int count
);
void apply_music_gain();
void release_camera_fov();
void set_camera_fov(float degrees);
std::uint32_t loads_now();
int api_hero_set_pose(openrac_mod* mod, const openrac_pose* pose);
int api_hero_set_joints(openrac_mod* mod, const openrac_joint_offset* joints, int count);
int api_collision_triangles(
    const float lo[3], const float hi[3], openrac_collision_triangle* out, int max
);
openrac_gaddr api_guest_text(openrac_mod* mod, const char* text);
int api_pickup_animation(openrac_mod* mod, int kind, int item);
int api_moby_visuals(openrac_mod* mod, const openrac_moby_visual* visuals, int count);
int api_moby_material(
    openrac_mod* mod, openrac_gaddr moby, std::uint32_t material, std::uint32_t rgb
);
int api_moby_material_class(
    openrac_mod* mod, int oclass, std::uint32_t scope, std::uint32_t material, std::uint32_t rgb
);
bool moby_material_known(std::uint32_t material, std::uint32_t rgb);
int api_prop_create(openrac_mod* mod, const char* mesh_path);
int api_prop_pose(openrac_mod* mod, int id, const openrac_prop_pose* pose);
int api_prop_attach(
    openrac_mod* mod,
    int id,
    std::int32_t slot,
    std::int32_t joint,
    std::uint32_t flags,
    const float offset[16]
);
int api_prop_frame(openrac_mod* mod, int id, float out[16]);
int api_prop_hide(openrac_mod* mod, int id, int hidden);
int api_prop_remove(openrac_mod* mod, int id);
int api_camera_view(openrac_camera* out);
bool unit(const float v[3]);
int api_camera_set(openrac_mod* mod, const openrac_camera* camera);
int api_haze_set(openrac_mod* mod, const openrac_haze* patches, int count);
int api_hud_text_set(openrac_mod* mod, const openrac_hud_text* lines, int count);
int api_hero_sound_play(openrac_mod* mod, int sound, int loop);
void api_hero_sound_stop(openrac_mod* mod, int voice);
int api_hero_hurt(openrac_mod* mod, int damage);
int api_touch_define(openrac_mod* mod, const char* json);
int api_touch_icon(openrac_mod* mod, const char* name, const char* svg);
void api_touch_push(openrac_mod* mod, const char* context);
void api_touch_hint(openrac_mod* mod, const char* hint, int on);
int api_sound_load(openrac_mod* mod, const char* path);
std::uint32_t api_sound_play(openrac_mod* mod, int sound, float volume, float pitch, int loop);
int api_sound_set(openrac_mod* mod, std::uint32_t voice, float volume, float pitch);
void api_sound_stop(openrac_mod* mod, std::uint32_t voice);

// What OpenRAC Editor's levels need (interface 1.35).
int api_moby_uid(int slot);
int api_moby_set_joints(openrac_mod* mod, int slot, const openrac_joint_offset* joints, int count);
int api_moby_set_joint_poses(
    openrac_mod* mod, int slot, const openrac_joint_offset* joints, int count
);
int api_moby_joint_frame(int slot, int joint, float out[12]);
int api_moby_joint_position(int slot, int joint, float out[3]);
int api_camera_fov(openrac_mod* mod, float degrees);
int api_hud_hide(openrac_mod* mod, int hidden);
int api_music_volume(openrac_mod* mod, float gain);
int api_prop_animate(openrac_mod* mod, int id, int animation, float seconds, int loop);
int api_moby_sound_play(openrac_mod* mod, int slot, int sound, int loop);
void api_moby_sound_stop(openrac_mod* mod, int voice);
int api_scene_air(openrac_mod* mod, const openrac_air* air);
int api_time_stretch(openrac_mod* mod, float stretch);
int api_tie_hide(openrac_mod* mod, int tie, int hidden);
void drop_air(const openrac_mod* mod);

// The hero's weapons and their shots (interface 1.29): mod_api/hero.cpp, mod_api/world.cpp.
// The particle records tied to a weapon's look (render-only), by record address; under
// g_props_lock.
struct ParticleLook {
    std::uint32_t loads = 0;
    MobyMaterial look;
};

extern std::unordered_map<std::uint32_t, ParticleLook> g_particle_looks;
int api_on_weapon_fire(openrac_mod* mod, openrac_weapon_fire_fn fn, void* user);
int api_weapon_refire(openrac_mod* mod, std::uint32_t serial, float yaw, float pitch);
int api_on_shot_end(openrac_mod* mod, openrac_shot_end_fn fn, void* user);
int api_moby_spawn_live(openrac_mod* mod, const openrac_spawn_live* spawn);
int api_weapon_look(openrac_mod* mod, std::int32_t item, std::uint32_t material, std::uint32_t rgb);
int api_moby_status(openrac_mod* mod, std::int32_t slot, std::uint32_t status, std::int32_t ticks);
openrac_gaddr api_weapon_record(openrac_mod* mod, std::int32_t item);
void drop_weapon_shots(const openrac_mod* mod);

int api_weapon_ignore_culling(openrac_mod* mod, std::int32_t item, std::int32_t enabled);

int api_sharp_cells(openrac_mod* mod, const openrac_sharp_page* pages, int count);

int api_set_motors(openrac_mod* mod, int port, int small_motor, int large_motor);

int api_menu_option_set(openrac_mod* mod, const char* option, int state);
// Forgets the menu options `mod` set (nullptr: every mod's).
void drop_menu_options(const openrac_mod* mod);

int api_on_play_query(openrac_mod* mod, std::uint32_t kinds, openrac_play_query_fn fn, void* user);

int api_on_mods_loaded(openrac_mod* mod, openrac_frame_fn fn, void* user);

// What mods offer each other (interface 1.42, core.cpp): forgets the services `mod` offered and its
// on_mods_loaded callbacks (nullptr: every mod's), and calls the callbacks, in load order, once
// every mod has loaded.
void drop_services(const openrac_mod* mod);
void run_mods_loaded();

int api_service_provide(
    openrac_mod* mod, const char* name, std::uint32_t version, const void* table, std::uint32_t size
);

int api_service_find(
    openrac_mod* mod, const char* name, std::uint32_t min_version, openrac_service* out
);

}  // namespace openrac::mods::detail
