// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native mod loader (docs/port/MODS.md): at start, the launcher's list of mods
// (<data>/plugins/plugins.json) checked (mod_manifest.h), each library that passes opened and
// loaded through the interface of sdk/include/openrac/openrac_mod.h, its hooks bound to the game's
// hook points (hostgen's hooks.c), and what was done written to status.json. Then, each tick and
// each pad report, the callbacks the mods asked for. With no mod loaded nothing here runs: the hook
// points' slots stay null and the per-tick call is one test of a flag.

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mod_manifest.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/openrac_mod.h"
#include "openrac/script_binding.h"

namespace openrac::mods {

int rac1_weapon_hud_icon(int native_icon);

// One hook point as hostgen's hooks.c lists it (openrac_game_hook_points): the same layout.
struct HookPoint {
    const char* name;
    const char* symbol;
    const char* signature;
    void** slot;
    void* original;
};

// The game's tick and display-list state needed by a snapshot. The executable supplies these
// callbacks through Host so the mod loader stays usable without linking the game program (tests,
// manifest tools). `overlay` is the game's host-side mirror of the guest level.
struct SavestateRuntime {
    std::uint64_t frame = 0;
    gaddr display_chain = 0;
    unsigned draws = 0;
    bool reports_draws = false;
    bool vsync_since_kick = false;
    int overlay = -1;
};

// What a developer menu item is: a press (1.10), or since 1.19 a switch or a choice the menu turns
// over or moves itself (developer_menu_switch, developer_menu_choice).
enum class MenuItemKind {
    Press,
    Switch,
    Choice
};

struct DeveloperMenuItem {
    std::string label;
    std::string shortcut;
    std::string group;
    std::uint32_t action = 0;
    MenuItemKind kind = MenuItemKind::Press;
    int value = 0;                    // a switch's 0 or 1, a choice's index
    std::vector<std::string> values;  // a choice's values
    // Its look (1.28, developer_menu_look): a row to read, a locked one, text at its right, a
    // progress bar (negative: none), an icon of the mods' (touch_icon).
    bool info = false;
    bool locked = false;
    std::string look_value;
    float progress = -1.0f;
    std::string icon;
};

struct DeveloperMenu {
    openrac_mod* mod = nullptr;
    std::string title;
    std::vector<DeveloperMenuItem> items;
    std::deque<std::uint32_t> actions;
};

// A mod built into the port (docs/port/MODS.md, "Built-in mods"): its manifest and its two entries,
// in the program itself. It loads first, with no folder and no library, for the games and discs
// its manifest names, even in purist mode: to the player it is part of the port, not a mod. Its
// settings come from the program (Host::built_in_settings). A mod of the launcher's list with the
// same id is not loaded.
struct BuiltIn {
    std::string_view manifest;  // its openrac-mod.json
    openrac_mod_query_fn query = nullptr;
    openrac_mod_load_fn load = nullptr;
};

// What the loader needs of the program.
// The flag on a joint offset's number that sets the joint's pose (openrac_api.moby_set_joint_poses)
// rather than composing with it.
constexpr std::uint32_t kJointPoseSet = 0x80000000u;

struct Host {
    std::string game;             // "rac1"
    std::string version;          // "pal"
    std::string openrac_version;  // build_info::version()
    const HookPoint* hooks = nullptr;
    int hook_count = 0;
    std::filesystem::path list;   // plugins.json; the mods' folders are beside it
    std::filesystem::path stack;  // the stacked data packs' stack.json; empty: none in this run
    bool purist = false;          // --no-mods: nothing is read or loaded
    int (*loaded_level)(void) = nullptr;
    // Where the pad library looks for a callback on each report (game_host.h,
    // openrac_game_pad_report_hook): set when a mod asks for one.
    void (**pad_hook)(int port, unsigned char* report, int size) = nullptr;
    // The time status.json says it was written ("2026-10-21T14:02:11+02:00").
    std::string now;
    // The game's binding for script mods (openrac/script_binding.h): null where it has none, and
    // then a script mod is refused.
    const openrac_script_binding* scripts = nullptr;
    // The keyboard and mouse (interface 1.2): the state for the tick about to run (null: a test's,
    // or no window, and then nothing is held), the port's keyboard table given up to a mod, and the
    // cursor captured or let go as a mod asks.
    const openrac_input* (*input)(void) = nullptr;
    void (*take_input)(std::uint32_t devices) = nullptr;
    void (*capture_mouse)(bool capture) = nullptr;
    // The graphics context (interface 1.4): what the port draws with (null: a test's, or no
    // window, and then the backend is none); the views a mod asked for, given once load is over
    // (null: none, as at reset); the Vulkan device setup a mod asked for, likewise.
    const openrac_graphics* (*graphics)(void) = nullptr;
    void (*set_views)(const openrac_view_source* source) = nullptr;
    void (*set_vulkan_setup)(const openrac_vulkan_setup* setup) = nullptr;
    // A mod's menu page (interface 1.5): where its rectangles and text go, while its callback runs
    // (null: a test's, or no window, and then nothing is drawn).
    void (*menu_rect)(int x, int y, int width, int height, std::uint32_t rgba) = nullptr;
    void (*menu_text)(int x, int y, int size, std::uint32_t rgba, const char* text) = nullptr;
    // A link handed to the system (interface 1.14; null: a test's, or no window). True when it
    // went.
    bool (*open_url)(const char* url) = nullptr;
    // Puts text on the system clipboard (interface 1.14; null: a test's, or no window).
    bool (*set_clipboard)(const char* text) = nullptr;
    // The running game's essential host-side tick state and post-restore audio/renderer rebuild.
    bool (*capture_savestate_runtime)(SavestateRuntime* state) = nullptr;
    void (*restore_savestate_runtime)(const SavestateRuntime* state) = nullptr;
    std::size_t (*savestate_extra_size)(void) = nullptr;
    int (*savestate_extra_save)(void* out, std::size_t size) = nullptr;
    int (*savestate_extra_load)(const void* in, std::size_t size) = nullptr;
    void (*prepare_cold_savestate)(int overlay) = nullptr;
    // The game's play (interface 1.8, game_host.h; null: none, and the play calls say so) and the
    // memory card's folder its saves are in (openrac_game_card_dir).
    const openrac_game_play* play = nullptr;
    std::filesystem::path cards;
    // Where each mod's own folder goes (interface 1.11, openrac_api.data_dir): <this>/<mod id>.
    // Empty: none, and data_dir gives null.
    std::filesystem::path mod_data;
    // The installed games' data folders (interface 1.33, openrac_api.game_data_dir): the running game's
    // own data folder (`data`), and the folder the installed games sit in (games_root: <games_root>/<game>/data
    // each, the launcher's <install>/active). Empty games_root: the port cannot say for another game.
    std::filesystem::path data;
    std::filesystem::path games_root;
    // The mods' sounds (interface 1.12, openrac_api.sound_*): a file decoded and kept (its id, -1
    // when it cannot be read), a voice of it played (0 for none), changed (0: it has ended; a
    // volume below 0 changes nothing, only asks whether it still plays) and stopped. Null: no
    // sound output (a test's, or no window), and the sound calls say so.
    int (*sound_load)(const std::filesystem::path& file) = nullptr;
    std::uint32_t (*sound_play)(int sound, float volume, float pitch, bool loop) = nullptr;
    int (*sound_set)(std::uint32_t voice, float volume, float pitch) = nullptr;
    void (*sound_stop)(std::uint32_t voice) = nullptr;
    // The hero's joints turned by a mod (interface 1.12, openrac_api.hero_set_joints): the offsets
    // put on moby `moby`'s pose wherever it is evaluated (count 0: taken off). A joint number
    // with kJointPoseSet (moby_set_joint_poses) sets that joint's turn and place instead of
    // composing with them. Null: no drawing (a test's), and the call says so.
    void (*joint_offsets)(std::uint32_t moby, const openrac_joint_offset* joints, int count) =
        nullptr;
    // The mods built into this program, and the values of their settings by mod id, as text
    // ("1", "0.5", "KeyW,ArrowUp"): what a list entry's would say. A value missing or not of the
    // setting's type is its default.
    std::vector<BuiltIn> built_ins;
    std::map<std::string, std::map<std::string, std::string>> built_in_settings;
    // A built-in's setting changed in the game (its own setting_set_* or the menu): keep `text`
    // for key `key` of mod `id`. Null: kept for this run only.
    void (*built_in_write)(const std::string& id, const std::string& key, const std::string& text) =
        nullptr;
    // About once a second: the built-ins' settings again when the launcher changed them while the
    // game runs; false when nothing changed. Null: never.
    bool (*built_in_reread)(std::map<std::string, std::map<std::string, std::string>>& out) =
        nullptr;
    // The edited levels' pack the game reads now (interface 1.14, openrac_api.pack_folder; it
    // changes with the destination played), and the game's language (options_menu::language: 0
    // English, 1 French, 2 German, 3 Spanish, 4 Italian). Null: none (a test's), and the calls say
    // so: no pack, English.
    const char* (*pack_folder)(void) = nullptr;
    int (*language)(void) = nullptr;
    // What a cutscene needs: the game's streams of volume group `group` at `gain` (0
    // to 1, eased; openrac_api.music_volume), and joint `joint` of moby `moby` in the level, as
    // drawn: its x, y, z axes and its place, 12 floats (1, or 0 for none:
    // openrac_api.moby_joint_frame). Null: none (a test's), and the calls say so.
    void (*music_gain)(int group, float gain) = nullptr;
    int (*joint_frame)(std::uint32_t moby, int joint, float* out) = nullptr;
    // A mod's motor values for pad `port` (interface 1.37, openrac_api.set_motors), played with
    // the game's own (openrac_game_set_motors) and held as long; `mod` tells the mods apart (none
    // played before the window opens). drop_motors forgets a mod's (null: every mod's). Null: no
    // pads (a test's, or a build without a window), and set_motors says so.
    void (*motors)(const void* mod, int port, bool small_motor, std::uint8_t large_motor) = nullptr;
    void (*drop_motors)(const void* mod) = nullptr;
};

// A built-in mod's setting (BuiltIn), for the port's own menu: its manifest entry and value, or
// null when no built-in of that id is loaded or it has no such setting for this game.
struct BuiltInSetting {
    const Setting* spec = nullptr;
    SettingValue value;
};

BuiltInSetting built_in_setting(std::string_view id, std::string_view key);
// The settings a loaded built-in declares for this game, in its manifest's order (valid while the
// program runs); empty when none is loaded.
std::vector<const Setting*> built_in_settings(std::string_view id);
// Its presets (the manifest's "presets"), each with only the values of settings for this game.
std::vector<Preset> built_in_presets(std::string_view id);
// Sets a built-in's setting from the port's menu, `literal` its JSON ("true", "1.5", "\"KeyW\""),
// checked as setting_set_* checks it. The mod is told (on_settings) and the value kept. False when
// refused.
bool set_built_in_setting(std::string_view id, std::string_view key, const std::string& literal);

// Loads the mods the list has on. Returns the session header's line: "lock-strafe 1.0.0", "none",
// "none (purist mode)". Called once, after the game's functions are registered, before it runs:
// after the window opens, or before it when a mod creates the Vulkan device.
std::string load(const Host& host);

struct VulkanStartup {
    bool creates_device = false;
    bool uses_queue = false;
};

// Reads enabled manifests before opening the window. Device setup needs early mod loading;
// queue access needs game-thread device creation.
VulkanStartup vulkan_startup(const std::filesystem::path& list, bool purist);

// True while a loaded mod has declared native Vulkan access or registered legacy views.
bool vulkan_needs_game_thread();

// True when the registered view source can supply native Vulkan images.
bool view_source_can_use_vulkan();

// Each loaded mod's openrac_mod_unload, last first (at exit).
void unload();

// The mods' frame callbacks, after each game tick.
extern bool g_frame_callbacks;
void run_frame_callbacks();

inline void frame() {
    if (g_frame_callbacks) {
        run_frame_callbacks();
    }
}

// Called after the game's vertical-blank handlers. Snapshot APIs are valid only while these
// callbacks run.
void tick_boundary(std::uint32_t tick);

// The mods' tabs in OpenRAC's menu (developer_menu).
const std::vector<DeveloperMenu>& developer_menus();
void queue_developer_menu_action(openrac_mod* mod, std::uint32_t action);
// The menu turned a switch over or moved a choice (1.19): its new state, and its action queued as
// for a press. A value the item cannot have is ignored.
void set_developer_menu_value(openrac_mod* mod, std::uint32_t action, int value);

// How many mods are loaded.
int loaded_count();
// Whether the mod `id` is loaded (on, and its load went through).
bool loaded(std::string_view id);
// The table the mods are given, for the tests' direct calls.
const openrac_api& api_for_tests();

// The loaded mods' assets folders (MODS.md, section 3), the last loaded first: a file in one takes
// the place of the same path in those after it and in the extracted levels.
const std::vector<std::filesystem::path>& asset_roots();

// A loaded mod's id ("multiplayer"), for the log; "a mod" for none.
const char* mod_id(const openrac_mod* mod);

// Whether `mod` has taken the hook point `name` (its id or any alias), for the calls only the
// holder of a hook point may make (interface 1.12, hero_set_pose).
bool holds_hook(const openrac_mod* mod, const char* name);

// The installed games (interface 1.33). The data folder of `game` (the running game's own, else
// games_root/<game>/data); empty when the port has none for it.
std::filesystem::path game_data_folder(const Host& host, std::string_view game);
// Whether a data folder is an extracted game's: it holds iso_data or port.
bool holds_game_data(const std::filesystem::path& data);

// A file inside `mod`'s folder by its path there (relative, with /, no "." or ".." part), when it
// is a file; empty otherwise (interface 1.12: a prop's mesh, a sound).
std::filesystem::path mod_file(const openrac_mod* mod, const char* path);

// ---- Props (interface 1.12, mod_play.cpp) ----

// A prop to draw this frame: its mesh (a file of a mod's) and the matrix that places it,
// column-major, in the game's world.
struct PropDraw {
    int id = 0;  // its id (openrac_api.prop_create's)
    std::filesystem::path mesh;
    std::array<float, 16> matrix{};
    std::array<float, 4> tint{1.0f, 1.0f, 1.0f, 1.0f};
    int blend = 0;  // OPENRAC_PROP_OPAQUE, _ALPHA or _ADD
    // Fixed to a moby's joint (interface 1.18, prop_attach): the moby (a game address, 0 for
    // none), its joint, and `matrix` is then the offset in the joint's frame.
    std::uint32_t attached_moby = 0;
    int attached_joint = -1;
    // Its joint matrices (interface 1.33, openrac_api.prop_set_skin): 16 floats each, column-major,
    // the mesh's bind-pose vertex to where the joint puts it; null for a prop in its bind pose.
    std::shared_ptr<const std::vector<std::array<float, 16>>> skin;
    // Its .glb's animation to draw it in (-1: the mesh as stored), the time into it, looped
    // (openrac_api.prop_animate).
    int animation = -1;
    float seconds = 0.0f;
    bool loop = false;
};

// The props shown in the level now (posed, not hidden, of this level load). From any thread.
std::vector<PropDraw> props_drawn();

// The renderer drew prop `id`, fixed to a joint, with `matrix` this frame (interface 1.18,
// prop_frame tells the mod). From any thread.
void prop_placed(int id, const std::array<float, 16>& matrix);

// Render-only appearances, kept separately from game memory and scoped to one level load.
std::vector<int> visual_classes();
unsigned visual_classes_generation();
std::uint32_t sprite_tint();
std::vector<openrac_moby_visual> moby_visuals();

// ---- Moby materials (interface 1.23, mod_play.cpp) ----

// A live moby as the renderer lists it this frame: its address and class.
struct LiveMobyId {
    openrac_gaddr moby = 0;
    int oclass = 0;
};

// A mod's material on one live moby (openrac_api.moby_material), as the renderer draws it.
struct MobyMaterial {
    openrac_gaddr moby = 0;
    std::uint32_t material = OPENRAC_MOBY_MATERIAL_NONE;  // OPENRAC_MOBY_MATERIAL_*.
    std::uint32_t rgb = 0xFFFFFF;                         // 0xRRGGBB, 0xFF a channel unchanged.
};

// The mods' materials on the frame's live mobys `live` (the level's whole list, not only the ones
// drawn): one per moby, the last set where two mods set one. A material whose moby is missing from
// `live` after it was seen there, or holds another class than when first seen, is the game's removal
// of that moby, and is dropped. From the render thread.
std::vector<MobyMaterial> moby_materials(const std::vector<LiveMobyId>& live);

// Whether the material drawn on `moby` this level load is OPENRAC_MOBY_MATERIAL_PLATINUM (the last
// one set on it), for the game's own effects of that moby (openrac_moby_platinum). From the game's
// thread.
bool moby_platinum(std::uint32_t moby);

// The material drawn on `moby` this level load (the last one set on it), its material
// OPENRAC_MOBY_MATERIAL_NONE when there is none, for the game's own effects of that moby
// (openrac_moby_obsidian). From the game's thread.
MobyMaterial moby_material_drawn(std::uint32_t moby);

// A mod's material on every moby of class `oclass` in `scope` (one bit: OPENRAC_MODEL_WORLD or
// OPENRAC_MODEL_MENU; openrac_api.moby_material_class, interface 1.34); `material.moby` is 0.
struct ClassMaterialDraw {
    int oclass = -1;
    std::uint32_t scope = 0;
    MobyMaterial material;
};

// The mods' materials on classes this level load, one for each class and scope (the last set).
// From any thread.
std::vector<ClassMaterialDraw> class_materials();

// The energy materials (OPENRAC_MOBY_MATERIAL_ENERGY, interface 1.25) drawn this level load, by
// moby: the last material set on each, where it is energy or paint with its effects bit
// (OPENRAC_MOBY_MATERIAL_PAINT_EFFECTS, 1.39). For the effects those mobys make
// (energy_effects.h), read once a tick. From the game's thread.
std::unordered_map<std::uint32_t, MobyMaterial> energy_materials();
// The meshes the props of the level load now running were made of, each once, in the order first
// asked for (empty for a level load none was made in), and a count that grows whenever the list
// changes. The front end loads the meshes it does not hold into its scene when the count has
// grown (no new read of the level); a level it reads reads the list's meshes with it. From any
// thread.
std::vector<std::filesystem::path> prop_meshes();
unsigned prop_meshes_generation();

// ---- Moby models (interface 1.33, mod_play.cpp) ----

// A moby a mod draws with a mesh of its own (openrac_api.moby_model_slot): the moby by its address
// and class (an override is for the moby that was in the slot when it was set), the mesh (a model of
// the level's scene by its path, as a prop's is), the matrix from the mesh's coordinates to the
// coordinates of the moby's own model, column-major, and whether the game's joint matrices pose it.
//
// A moby override by class (interface 1.33, openrac_api.moby_model_class) is the same with `address`
// 0 and `scope` the one scope (OPENRAC_MODEL_WORLD or _MENU) it is for; a slot's, and an address's
// (interface 1.33, moby_model_at: a moby the level's list may not hold), has `scope` 0.
struct MobyModelDraw {
    std::uint32_t address = 0;
    int oclass = -1;
    std::uint32_t scope = 0;
    std::filesystem::path mesh;
    std::array<float, 16> matrix{};
    // Suppress this moby, including its native model, for this frame.
    bool hidden = false;
};

// The mobys the mods draw with a mesh of their own in the level now (this level load's). From any
// thread.
std::vector<MobyModelDraw> moby_models_drawn();

// The looks mods gave weapons' shots (openrac_api.weapon_look, interface 1.29) on the particle
// records the game made for them (their mobys, blasts and guns), by record address (each look's
// moby is the record's address), read once a tick. From the game's thread.
std::unordered_map<std::uint32_t, MobyMaterial> particle_looks();

// ---- Heat haze (interface 1.12, mod_play.cpp) ----

// A patch of heat haze to draw this frame (openrac_haze), in the game's world.
struct HazeDraw {
    std::array<float, 3> position{};
    float radius = 0.0f;
    float strength = 0.0f;
};

// The heat haze patches of this level load, every mod's. From any thread.
std::vector<HazeDraw> haze_drawn();

// The lines of text the mods show in the game's HUD this level load (openrac_hud_text), every
// mod's, in the order they set them. From any thread.
std::vector<openrac_hud_text> hud_texts();

// The vertical field of view a mod's camera asks for this level load, in degrees (openrac_api
// camera_fov), 0 for the game's own. Read by the window's thread.
float camera_fov();

// Whether a mod hides the game's own HUD this level load (openrac_api.hud_hide).
bool hud_hidden();

// Whether a loaded mod drives the pads' motors: it has made a set_motors call the port took (0, 0
// counts, so a mod can say so at load), until its unload or the mods' reset. The menu shows the
// vibration settings then, in a game without rumble of its own too. From any thread.
bool any_mod_motors();

// What the mods set the menu's option `name` to (openrac_api.menu_option_set): an
// OPENRAC_MENU_OPTION_* state, DEFAULT when none did. From any thread.
int menu_option(std::string_view name);

// The same, with the name of the mod that set it (empty for DEFAULT).
struct MenuOptionState {
    int state = OPENRAC_MENU_OPTION_DEFAULT;
    std::string by;
};

MenuOptionState menu_option_state(std::string_view name);

// The air a mod draws this level load with (openrac_api.scene_air), none for the game's own. From
// any thread.
std::optional<openrac_air> scene_air();

// How many times as long a game tick lasts for the mods' slow motion this level load
// (openrac_api.time_stretch), 1 for the game's speed. From any thread.
float time_stretch();

// The level's ties the mods hide from the picture this level load (openrac_api.tie_hide), sorted.
// From any thread.
std::vector<int> hidden_ties();

// Play (interface 1.8, mod_play.h): the start or travel a mod asked for, done now, on the game's
// thread after the frame's menus (frontend/frame.cpp, beside the developer menu's start); nothing
// when none is pending.
void play_update();

// Whether a mod asked for the scene playing to be skipped (openrac_api.play_skip), for this pad
// read: the skip is pressed for a while (frontend/input.cpp, skip_after_mods).
bool play_skipping();

// The line a mod shows over the game's picture (openrac_api.play_notice), empty for none; the
// debug overlay draws it. From any thread.
std::string play_notice();

// The front-end menu entries the mods added (interface 1.5), in load order: how many, and an
// entry's label ("" past the list).
int menu_entry_count();
std::string menu_entry_label(int index);

// Whether `url` is a mod code's link, the only link a mod may have opened: openrac://modset/ and a
// code, in the characters a code has.
bool is_mod_code_link(std::string_view url);

// Drawing over the game (interface 1.14, and 1.31's phases): the phase a frame is drawn over (an
// OPENRAC_OVERLAY_* bit, 0 for none or no mod drawing), read on the game's thread when the tick is
// kept; whether a mod draws over a frame of that phase; and each such mod's callback with
// `overlay` (its `phase` the frame's). On the window thread, with the frame finished.
std::uint32_t overlay_phase();
bool overlay_wanted(std::uint32_t phase);
void run_overlays(const openrac_overlay& overlay);

// The OPENRAC_OVERLAY_* bit of a frame of the game's phase `phase` (OPENRAC_PLAY_*); 0 for a phase
// nothing is drawn over. openrac_overlay.phase holds this bit, not the game's phase.
std::uint32_t overlay_bit(int phase);
// The mod whose overlay callback is running (null outside one), so that what it draws is its own
// (its blend is its own). On the window thread.
const openrac_mod* overlay_mod();

// Runs entry `index`'s page for a frame: its callback with `frame`, its drawing sent to the host's
// menu_rect and menu_text. True when the page closes (the mod said so, or there is no such entry).
bool run_menu_page(int index, const openrac_menu_frame& frame);

// ---- Pages of the game's own menus (interface 1.7) ----
//
// What the mods put on their pages and what they ask of the screen, kept here; the front end
// (mod_ui.h) shows them with the game's widgets, tells which page is on the screen and where its
// cursor is, and sends the pages their events.

// One item of a page, as its mod last set it (openrac_ui_item), its texts cut to
// OPENRAC_UI_TEXT_MAX.
struct UiItem {
    std::uint32_t kind = OPENRAC_UI_BUTTON;
    std::uint32_t flags = 0;
    std::string label;
    std::vector<std::string> choices;  // OPENRAC_UI_CHOICE's values
    std::uint32_t choice = 0;          // the one shown
};

// Whether the cursor stops on an item: any but a line of text or a disabled one.
bool takes_cursor(const UiItem& item);

// A page (openrac_api.ui_page) and what is on it.
struct UiPage {
    std::uint32_t flags = 0;  // OPENRAC_UI_OWN_BACK
    std::string title;
    std::vector<UiItem> items;
    std::vector<std::string> footer;  // none: the game's hints
    std::uint64_t revision = 0;       // grows with each ui_set
    int cursor = 0;                   // the item under the cursor
};

// A main-menu item (openrac_api.ui_main_item): after which of the game's, its label, its page.
struct UiMainItem {
    std::string after;
    std::string label;
    int page = 0;
};

// An item of the pause menu's Options page (openrac_api.ui_pause_item, since 1.9): its label,
// its page.
struct UiPauseItem {
    std::string label;
    int page = 0;
};

// What the mods asked of the screen during their events since the last take, for the front end.
struct UiRequests {
    bool open = false;  // ui_open: `page`, or OPENRAC_UI_GO_BACK
    int page = 0;
    bool confirm = false;  // ui_confirm: a question on `confirm_page`
    int confirm_page = 0;
    std::string question;
    std::uint32_t tag = 0;
    std::vector<int> sounds;  // ui_sound, in order
};

// The pages the mods made, by id (null past them), and the main-menu items, in order.
int ui_page_count();
const UiPage* ui_page(int page);
int ui_main_item_count();
const UiMainItem* ui_main_item(int index);
// The pause menu's Options items, in order.
int ui_pause_item_count();
const UiPauseItem* ui_pause_item(int index);

// The front end's side: the page on the screen (-1 none; whose mod may then open, ask and play),
// whether a question is open on it, where a page's cursor is now.
void ui_set_shown(int page, bool question_open);
// Whether the title's main menu is on the screen, settled (a mod may open its main-menu item's
// page from it, since 1.9).
void ui_set_main_menu(bool up);
void ui_set_cursor(int page, int item);
// A choice the player stepped: shown so until the mod sets the page again.
void ui_set_choice(int page, int item, std::uint32_t choice);

// Sends `event` to `page`'s callback (its cursor filled in). Nothing for a page there is not.
void ui_send(int page, openrac_ui_event event);

// What the mods asked for since the last call, emptied.
UiRequests take_ui_requests();

// For tests: unloads every mod, puts each hook point's slot back to null and forgets the
// callbacks, so that load() can run again.
void reset();

}  // namespace openrac::mods
