// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests openrac/mod.hpp, the C++ layer of the mod interface, against a host table of the test's
// own: the entry points, settings, hooks as lambdas chained to the original, exceptions caught at
// the boundary, callbacks, the stack frame, binds and the input state, and a load that throws.
//
// The test is a mod: it exports the entry points through OPENRAC_MOD_CPP and calls them as the port
// would, with `make_api()`'s table standing in for the port. The expected values come from the
// interface's comments (sdk/include/openrac/openrac_mod.h) and the fake game's own functions: the
// one that doubles, the one that records the angles it is given. Ordering: the state and the fake
// host first, the mod's load next, then one function per behaviour, and `main` last runs them.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "openrac/games/rac1.hpp"
#include "openrac/mod.hpp"

namespace {

// Counts a failed CHECK and goes on, so one run lists every failure; main's exit code is the count.
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c);                    \
            ++g_failures;                                                                          \
        }                                                                                          \
    } while (0)

// ----------------------------------------------------------------------------------------------
// State of the fake host
// ----------------------------------------------------------------------------------------------

// The number of CHECKs that failed. The test is single-threaded.
int g_failures = 0;

// A hook point that returns something, for the original's result.
struct Test_Twice {
    static constexpr const char* name = "Test_Twice";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// The lines the mod logged, each "<level> <text>".
std::vector<std::string> g_log;

// The reason the mod gave for refusing to load; empty when it did not.
std::string g_failed;

// Hook point (by id or name) -> what the port calls: the game's function, then the replacement
// that a take puts in its place.
std::map<std::string, void*> g_slots;

// The angles the fake game's Hero_SetPlanarVelocity was called with.
std::vector<float> g_game_angles;

// The callbacks and their user pointers, as the mod gave them to on_frame and on_pad.
openrac_frame_fn g_frame_fn = nullptr;
void* g_frame_user = nullptr;
openrac_pad_fn g_pad_fn = nullptr;
void* g_pad_user = nullptr;

// The fake game memory (openrac_api.memory) and its stack pointer, a game address.
uint8_t g_memory[256];
openrac_gaddr g_sp = 0x100;

// The input state the port shows, the devices a mod took, and the last capture asked for (-1 for
// none yet).
openrac_input g_input{};
uint32_t g_taken = 0;
int g_capture = -1;

// How many times the mod's frame callback ran, and whether its hook is to throw.
int g_frames = 0;
bool g_throw_in_hook = false;

// ----------------------------------------------------------------------------------------------
// The fake host: the functions of openrac_api
// ----------------------------------------------------------------------------------------------

// The fake game's Hero_SetPlanarVelocity: records the angle it is called with.
void game_set_planar_velocity(float angle) {
    g_game_angles.push_back(angle);
}

// The fake game's Test_Twice: doubles its argument.
int32_t game_twice(int32_t x) {
    return 2 * x;
}

// The members of openrac_api below each do the least the interface says: they log, record or
// return a fixed value, so a check can tell what the layer asked of the port.

// Records the line with its level, as "<level> <text>".
void host_log(openrac_mod*, int level, const char* text) {
    g_log.push_back(std::to_string(level) + " " + text);
}

void host_fail(openrac_mod*, const char* reason) {
    g_failed = reason;
}

int host_setting_bool(openrac_mod*, const char* key) {
    return std::strcmp(key, "trace") == 0;
}

int64_t host_setting_int(openrac_mod*, const char* key) {
    return std::strcmp(key, "count") == 0 ? 42 : 0;
}

double host_setting_float(openrac_mod*, const char* key) {
    return std::strcmp(key, "scale") == 0 ? 1.5 : 0.0;
}

const char* host_setting_string(openrac_mod*, const char* key) {
    return std::strcmp(key, "mode") == 0 ? "lock" : "";
}

// Takes a hook point as the port does: the slot's function becomes the original, the replacement
// takes the slot.
int host_hook(
    openrac_mod*, const char* name, const char* signature, void* replacement, void** original
) {
    auto known = g_slots.find(name);

    // The port has no hook point of that name on this disc.
    if (known == g_slots.end()) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    const std::string want = std::strcmp(name, "Test_Twice") == 0 ? "i32(i32)" : "void(f32)";

    // The signature asked for is another than the hook point's.
    if (want != signature) {
        return OPENRAC_HOOK_SIGNATURE;
    }

    *original = known->second;
    known->second = replacement;
    return OPENRAC_HOOK_OK;
}

int host_on_frame(openrac_mod*, openrac_frame_fn fn, void* user) {
    g_frame_fn = fn;
    g_frame_user = user;
    return 0;
}

int host_on_pad(openrac_mod*, openrac_pad_fn fn, void* user) {
    g_pad_fn = fn;
    g_pad_user = user;
    return 0;
}

void* host_function(const char* name) {
    return std::strcmp(name, "func_twice") == 0 ? reinterpret_cast<void*>(&game_twice) : nullptr;
}

void* host_function_at(openrac_gaddr) {
    return nullptr;
}

openrac_gaddr host_relocate(openrac_gaddr a) {
    return a + 0x10;
}

openrac_gaddr host_relocate_level(int level, openrac_gaddr a) {
    return a + static_cast<openrac_gaddr>(level);
}

int host_loaded_level() {
    return 3;
}

openrac_gaddr host_stack_push(uint32_t size, openrac_gaddr* saved) {
    *saved = g_sp;
    g_sp -= (size + 15u) & ~15u;
    return g_sp;
}

void host_stack_pop(openrac_gaddr saved) {
    g_sp = saved;
}

const openrac_input* host_input() {
    return &g_input;
}

int host_take_input(openrac_mod*, uint32_t devices) {
    g_taken |= devices;
    return 0;
}

void host_capture_mouse(openrac_mod*, int capture) {
    g_capture = capture;
}

// The graphics, views and Vulkan device (1.4): what the fake port was given,
// and whether it has views already (a second asking is taken).
openrac_graphics g_graphics = [] {
  openrac_graphics graphics{};
  graphics.size = sizeof graphics;
  graphics.backend = OPENRAC_GRAPHICS_OPENGL;
  return graphics;
}();
openrac_view_source g_views{};
openrac_vulkan_setup g_vulkan{};

const openrac_graphics *host_graphics() { return &g_graphics; }

int host_on_views(openrac_mod *, const openrac_view_source *source) {
  if (g_views.begin != nullptr) {
    return OPENRAC_HOOK_TAKEN;
  }
  g_views = *source;
  return OPENRAC_HOOK_OK;
}

int host_setup_vulkan(openrac_mod *, const openrac_vulkan_setup *setup) {
  g_vulkan = *setup;
  return OPENRAC_HOOK_OK;
}

// The menu entry (1.5): what the fake port was given, and what the page drew.
std::string g_menu_label;
openrac_menu_fn g_menu_fn = nullptr;
void* g_menu_user = nullptr;
std::vector<std::string> g_menu_drawn;

int host_menu_entry(openrac_mod*, const char* label, openrac_menu_fn fn, void* user) {
    // The fake port has room for one entry.
    if (g_menu_fn != nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_menu_label = label;
    g_menu_fn = fn;
    g_menu_user = user;
    return OPENRAC_HOOK_OK;
}

void host_menu_rect(openrac_mod*, int x, int y, int width, int height, uint32_t rgba) {
    g_menu_drawn.push_back(
        "rect " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(width) + " "
        + std::to_string(height) + " " + std::to_string(rgba)
    );
}

void host_menu_text(openrac_mod*, int x, int y, int size, uint32_t, const char* text) {
    g_menu_drawn.push_back(
        "text " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(size) + " " + text
    );
}

// The fake port's tick boundary (1.6): the callback it was given, and a snapshot of 64 bytes that
// it takes only at that size.
openrac_tick_boundary_fn g_tick_fn = nullptr;
void* g_tick_user = nullptr;
std::size_t g_saved = 0;

int host_on_tick_boundary(openrac_mod*, openrac_tick_boundary_fn fn, void* user) {
    g_tick_fn = fn;
    g_tick_user = user;
    return OPENRAC_HOOK_OK;
}

std::size_t host_savestate_size() {
    return 64;
}

int host_savestate_save(void* out, std::size_t size) {
    if (out == nullptr || size != 64) {
        return -1;
    }
    g_saved = size;
    return 0;
}

int host_savestate_load(const void* data, std::size_t size) {
    return data != nullptr && size == 64 ? 0 : -1;
}

// The pages of the game's own menus (1.7): the fake port keeps one page's content as text, the
// pages' callbacks, the main item, and what the pages asked for.
struct FakePage {
    openrac_ui_fn fn = nullptr;
    void* user = nullptr;
    uint32_t flags = 0;
    std::string shown;  // "title|kind:flags:label[=choice/count]...|footer..."
};
std::vector<FakePage> g_ui_pages;
std::string g_ui_main;
std::vector<std::string> g_ui_asked;

int host_ui_page(openrac_mod*, uint32_t flags, openrac_ui_fn fn, void* user) {
    // The fake port has room for two pages.
    if (fn == nullptr || g_ui_pages.size() >= 2) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_pages.push_back({fn, user, flags, {}});
    return static_cast<int>(g_ui_pages.size()) - 1;
}

int host_ui_main_item(openrac_mod*, const char* after, const char* label, int page) {
    if (std::string(after) != OPENRAC_UI_LOAD_GAME || page < 0 || page >= static_cast<int>(g_ui_pages.size())) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_main = std::string(after) + ":" + label + ":" + std::to_string(page);
    return OPENRAC_HOOK_OK;
}

int host_ui_set(openrac_mod*, int page, const openrac_ui_content* content) {
    if (page < 0 || page >= static_cast<int>(g_ui_pages.size()) || content->size != sizeof *content) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::string text = content->title;
    for (uint32_t i = 0; i < content->item_count; ++i) {
        const openrac_ui_item& item = content->items[i];
        text += "|" + std::to_string(item.kind) + ":" + std::to_string(item.flags) + ":" + item.label;
        if (item.kind == OPENRAC_UI_CHOICE) {
            text += std::string("=") + item.choices[item.choice] + "/" + std::to_string(item.choice_count);
        }
    }
    for (uint32_t i = 0; i < content->footer_count; ++i) {
        text += std::string("|") + content->footer[i];
    }
    g_ui_pages[static_cast<std::size_t>(page)].shown = text;
    return OPENRAC_HOOK_OK;
}

int host_ui_open(openrac_mod*, int page) {
    g_ui_asked.push_back("open " + std::to_string(page));
    return OPENRAC_HOOK_OK;
}

int host_ui_confirm(openrac_mod*, int page, const char* question, uint32_t tag) {
    g_ui_asked.push_back("confirm " + std::to_string(page) + " " + question + " " + std::to_string(tag));
    return OPENRAC_HOOK_OK;
}

int host_ui_cursor(openrac_mod*, int, int item) {
    return item >= 0 ? item : 2;
}

int host_ui_sound(openrac_mod*, int sound) {
    g_ui_asked.push_back("sound " + std::to_string(sound));
    return sound >= 0 && sound <= OPENRAC_UI_SOUND_PAGE ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// Play (interface 1.8): a level of its own, a hero in state 7, one figure, a save of 4 bytes in
// slot 0 (level 3) and as the progress, and what was started or travelled to.
std::vector<std::string> g_played;

int host_play_status(openrac_play_status* out) {
    out->phase = OPENRAC_PLAY_PLAYING;
    out->level = 3;
    out->levels = 19;
    out->loads = 2;
    return OPENRAC_HOOK_OK;
}

const char* host_level_name(int level) {
    return level == 3 ? "Kerwan" : nullptr;
}

int host_hero_pose(openrac_pose* out) {
    out->state = 7;
    out->position[0] = 1.5f;
    return out->size == sizeof *out ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_figure_create(openrac_mod*) {
    return 0;
}

int host_figure_pose(openrac_mod*, int id, const openrac_pose* pose) {
    g_played.push_back("pose " + std::to_string(id) + " " + std::to_string(pose->seq));
    return id == 0 && pose->size == sizeof *pose ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_figure_remove(openrac_mod*, int id) {
    return id == 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_play_start(openrac_mod*, int level, const void* save, size_t size) {
    g_played.push_back("start " + std::to_string(level) + " " + (save != nullptr ? std::to_string(size) : "new"));
    return OPENRAC_HOOK_OK;
}

int host_play_travel(openrac_mod*, int level) {
    g_played.push_back("travel " + std::to_string(level));
    return OPENRAC_HOOK_TAKEN;
}

const std::uint8_t kSave[4] = {'S', 3, 0, 0};

size_t host_save_capture(void* out, size_t size) {
    if (out != nullptr && size >= sizeof kSave) {
        std::memcpy(out, kSave, sizeof kSave);
    }
    return sizeof kSave;
}

int host_save_slot(int slot, openrac_save_info* out) {
    if (slot > 1) {
        return -1;
    }
    out->level = slot == 0 ? 3 : 0;
    return slot == 0 ? 1 : 0;
}

size_t host_save_read(int slot, void* out, size_t size) {
    return slot == 0 ? host_save_capture(out, size) : 0;
}

// The world (interface 1.9): a level of g_world_count live mobys (slot i has key i, its class 100 +
// i, its flags 0x5), slot 1 dead of a kill, what was asked of the game, and the bolts.
int g_world_count = 3;
int g_bolts = 100;
std::vector<std::string> g_world;

int host_moby_list(openrac_moby* out, int max) {
    g_world.push_back("list " + std::to_string(max));
    for (int i = 0; i < std::min(g_world_count, max); ++i) {
        out[i] = {};
        out[i].size = sizeof out[i];
        out[i].slot = i;
        out[i].key = i;
        out[i].oclass = 100 + i;
        out[i].flags = OPENRAC_MOBY_UPDATES | OPENRAC_MOBY_PLACED;
    }
    return g_world_count;
}

int host_moby_slot(int key) {
    return key == 7 ? 7 : -1;
}

int host_moby_drive(openrac_mod*, int slot, int on) {
    g_world.push_back("drive " + std::to_string(slot) + " " + std::to_string(on));
    return slot < g_world_count ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_moby_pose(openrac_mod*, int slot, const openrac_moby* pose) {
    g_world.push_back("pose " + std::to_string(slot) + " " + std::to_string(pose->seq));
    return slot < g_world_count && pose->size == sizeof *pose ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_moby_kill(openrac_mod*, int slot) {
    g_world.push_back("kill " + std::to_string(slot));
    return slot < g_world_count ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_moby_spawn(openrac_mod*, int oclass, const openrac_moby* pose) {
    g_world.push_back("spawn " + std::to_string(oclass) + " " + std::to_string(pose->size == sizeof *pose));
    return oclass == 100 ? 12 : OPENRAC_HOOK_UNKNOWN;
}

int host_figure_slot(openrac_mod*, int id) {
    return id < 0 ? 9 : (id == 0 ? 10 : -1);
}

int host_on_moby_update(openrac_mod*, openrac_moby_update_fn fn, void* user) {
    g_world.push_back("gate " + std::to_string(fn != nullptr && user != nullptr));
    return fn != nullptr ? OPENRAC_HOOK_OK : OPENRAC_HOOK_TOO_LATE;
}

int host_hero_stand_in(openrac_mod*, const float position[3], int figure) {
    g_world.push_back("stand_in " + std::to_string(static_cast<int>(position[1])) + " " + std::to_string(figure));
    return figure >= 0 && figure != 0 ? OPENRAC_HOOK_UNKNOWN : OPENRAC_HOOK_OK;
}

int host_on_moby_hit(openrac_mod*, openrac_hit_fn fn, void*) {
    g_world.push_back("watch");
    return fn != nullptr ? OPENRAC_HOOK_OK : OPENRAC_HOOK_TOO_LATE;
}

int host_moby_hit(openrac_mod*, const openrac_hit* hit) {
    g_world.push_back("hit " + std::to_string(hit->victim) + " " + std::to_string(hit->size == sizeof *hit));
    return hit->victim < g_world_count ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_play_eyes(openrac_mod*, const float* xyz, int count) {
    g_world.push_back("eyes " + std::to_string(count) + " " + std::to_string(static_cast<int>(xyz[3])));
    return OPENRAC_HOOK_OK;
}

int host_bolts(int add) {
    return g_bolts += add;
}

// What the hero holds (one piece, the wrench's class) and his shots (two, items 15 and 10); the
// figure calls recorded in g_world.
int host_hero_held(openrac_held* out) {
    if (out->size < sizeof *out) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->count = 1;
    out->pieces[0].oclass = 0x47;
    return OPENRAC_HOOK_OK;
}

int host_figure_held(openrac_mod*, int id, const openrac_held* held) {
    g_world.push_back("held " + std::to_string(id) + " " + std::to_string(held->count) + " "
                      + std::to_string(held->size == sizeof *held));
    return id == 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_hero_shots(openrac_shot* out, int max) {
    const int items[2] = {15, 10};
    int n = 0;
    for (; n < 2 && n < max; ++n) {
        out[n].size = sizeof out[n];
        out[n].item = items[n];
    }
    return n;
}

int host_figure_shot(openrac_mod*, int id, const openrac_shot* shot) {
    g_world.push_back("shot " + std::to_string(id) + " " + std::to_string(shot->item));
    return id == 0 && shot->size == sizeof *shot ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// The level's scene (1.9): -1 none; a scene asked for plays at once, at frame 30.
int g_scene = -1;

int host_scene_playing(int* frame) {
    *frame = g_scene >= 0 ? 30 : 0;
    return g_scene;
}

int host_play_scene(openrac_mod*, int scene) {
    g_world.push_back("scene " + std::to_string(scene));
    g_scene = scene;
    return OPENRAC_HOOK_OK;
}

// A moby hidden or shown, and a deletion seen of slot 7 only (by slot 3's update, class 120).
int host_moby_hide(openrac_mod*, int slot, int hidden) {
    g_world.push_back("hide " + std::to_string(slot) + " " + std::to_string(hidden));
    return slot >= 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int host_moby_deletion(int slot, openrac_moby_deletion* out) {
    if (slot != 7 || out->size < sizeof *out) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->by = 3;
    out->by_class = 120;
    out->by_mod = 0;
    out->oclass = 500;
    out->loads = 2;
    return OPENRAC_HOOK_OK;
}

// The last run: slot 4 left out by distance, 30 from the camera; the pause menu's level camera.
int host_moby_run(int slot, openrac_moby_run* out) {
    if (out->size < sizeof *out) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->runs = 9;
    out->menu = OPENRAC_RUN_MENU_LEVEL_CAMERA;
    out->ran = slot == 4 ? OPENRAC_MOBY_RUN_FAR : OPENRAC_MOBY_RUN_NONE;
    out->update_dist = slot == 4 ? 20 : 0;
    out->distance = slot == 4 ? 30.0f : 0.0f;
    return OPENRAC_HOOK_OK;
}

int host_camera_position(float out[3]) {
    out[0] = 1.0f;
    out[1] = 2.0f;
    out[2] = 3.0f;
    return OPENRAC_HOOK_OK;
}

// The published 1.11 calls keep their place before anything of 1.12.
static_assert(offsetof(openrac_api, save_slot_current) < offsetof(openrac_api, mod_count));

// A fake host's mods (interface 1.12): one mod with one setting, rules kept and handed back.
int host_mod_count() {
    return 1;
}

int host_mod_entry(int index, openrac_mod_entry* out) {
    if (index != 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    out->id = "difficulty";
    out->name = "Difficulty";
    out->version = "1.2.3";
    out->party = OPENRAC_PARTY_MOD_GAMEPLAY;
    out->settings = 1;
    return OPENRAC_HOOK_OK;
}

int host_mod_setting(int index, int setting, openrac_mod_setting* out) {
    if (index != 0 || setting != 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    out->key = "seed";
    out->name = "Seed";
    out->value = "";
    out->shown = "";
    out->party = OPENRAC_PARTY_SETTING_HOST;
    out->is_default = 1;
    return OPENRAC_HOOK_OK;
}

std::vector<std::uint8_t> g_rules;
std::string g_rules_text;
int g_replay = 0;
openrac_party_rules_fn g_rules_fn = nullptr;

int host_rules_publish(openrac_mod*, const void* data, uint32_t size, const char* text) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    g_rules.assign(bytes, bytes + size);
    g_rules_text = text;
    return OPENRAC_HOOK_OK;
}

int host_rules_read(int index, void* out, uint32_t size, uint32_t* version) {
    if (index != 0 || g_rules.size() > size) {
        return -1;
    }

    std::memcpy(out, g_rules.data(), g_rules.size());
    *version = 4;
    return static_cast<int>(g_rules.size());
}

const char* host_rules_text(int) {
    return g_rules_text.c_str();
}

int host_on_rules(openrac_mod*, openrac_party_rules_fn fn, void*) {
    g_rules_fn = fn;
    return 0;
}

int host_rules_deliver(int, const void* data, uint32_t size) {
    g_rules_fn(nullptr, data, size);
    return OPENRAC_HOOK_OK;
}

void host_replay(int on) {
    g_replay = on;
}

int host_replaying() {
    return g_replay;
}

// A fake host's interface 1.12 calls: each says what it was asked in a record the test reads.
std::string g_12;

void fill_12(openrac_api& api) {
    api.card_filter = [](openrac_mod*, int mode, const char* folder) {
        g_12 = "filter " + std::to_string(mode) + " " + folder;
        return OPENRAC_HOOK_OK;
    };
    api.card_slot_hint = [](openrac_mod*, int slot) {
        g_12 = "hint " + std::to_string(slot);
        return OPENRAC_HOOK_OK;
    };
    api.coop_save_write = [](openrac_mod*, int slot, const void*, uint32_t size) {
        g_12 = "write " + std::to_string(slot) + " " + std::to_string(size);
        return OPENRAC_HOOK_OK;
    };
    api.save_slot2 = [](const char*, int slot, openrac_save_info* out) {
        out->level = 3;
        return slot == 0 ? 1 : 0;
    };
    api.save_read2 = [](const char*, int, void* out, size_t size) -> size_t {
        if (out != nullptr) {
            std::memset(out, 7, size);
        }
        return 5;
    };
    api.card_saves = []() -> uint32_t { return 4; };
    api.card_note_write = [](openrac_mod*, int slot, const char* text) {
        g_12 = "note " + std::to_string(slot) + " " + text;
        return OPENRAC_HOOK_OK;
    };
    api.card_note_read = [](const char*, int slot, char* out, uint32_t size) {
        return slot == 0 ? std::snprintf(out, size, "key") : 0;
    };
    api.shared_read = [](int tag, void* out, uint32_t size) {
        if (tag != 1) {
            return -1;
        }
        std::memset(out, 9, std::min<uint32_t>(size, 2));
        return 2;
    };
    api.shared_give = [](int tag, const void*, uint32_t size) {
        g_12 = "give " + std::to_string(tag) + " " + std::to_string(size);
        return OPENRAC_HOOK_OK;
    };
    api.mission_state_read = [](int level, uint8_t* out) {
        out[0] = static_cast<uint8_t>(level);
        return OPENRAC_HOOK_OK;
    };
    api.mission_state_write = [](int level, const uint8_t* in) {
        g_12 = "mission " + std::to_string(level) + " " + std::to_string(in[1]);
        return OPENRAC_HOOK_OK;
    };
    api.hero_place = [](openrac_mod*, const float p[3], float yaw, uint32_t flags) {
        g_12 = "place " + std::to_string(static_cast<int>(p[0])) + " " + std::to_string(static_cast<int>(yaw))
               + " " + std::to_string(flags);
        return OPENRAC_HOOK_OK;
    };
    api.hero_grab = [](openrac_mod*, int animation, uint32_t) {
        g_12 = "grab " + std::to_string(animation);
        return OPENRAC_HOOK_OK;
    };
    api.hero_release = [](openrac_mod*) {
        g_12 = "release";
        return OPENRAC_HOOK_OK;
    };
    api.hero_hold_at = [](openrac_mod*, const float p[3], float yaw) {
        g_12 = "hold " + std::to_string(static_cast<int>(p[0])) + " " + std::to_string(static_cast<int>(yaw));
        return OPENRAC_HOOK_OK;
    };
    api.hero_stand_in_as = [](openrac_mod*, const openrac_stand_in* as, int figure) {
        g_12 = "as " + std::to_string(as->state) + " " + std::to_string(figure);
        return OPENRAC_HOOK_OK;
    };
    api.hero_stand_in_read = [](openrac_mod*, openrac_stand_in* out) {
        out->state = 120;
        return OPENRAC_HOOK_OK;
    };
    api.hero_revive = [](openrac_mod*, const float p[3], uint32_t) {
        g_12 = p != nullptr ? "revive here" : "revive";
        return OPENRAC_HOOK_OK;
    };
    api.on_overlay = [](openrac_mod*, openrac_overlay_fn fn, void*) {
        g_12 = fn != nullptr ? "overlay" : "no overlay";
        return OPENRAC_HOOK_OK;
    };
    api.hero_shots2 = [](openrac_shot2* out, int max) {
        out[0].item = 11;
        out[0].target_key = 42;
        return max > 0 ? 1 : 0;
    };
    api.figure_shot2 = [](openrac_mod*, int id, const openrac_shot2* shot) {
        g_12 = "shot " + std::to_string(id) + " " + std::to_string(shot->target_key);
        return OPENRAC_HOOK_OK;
    };
    api.moby_list2 = [](openrac_moby2* out, int max) {
        out[0].slot = 4;
        out[0].mission = 7;
        return max > 0 ? 1 : 0;
    };
    api.on_camera_script = [](openrac_mod*, openrac_camera_script_fn, void*) { return OPENRAC_HOOK_OK; };
    api.camera_script_play = [](openrac_mod*, int id) {
        g_12 = "camera " + std::to_string(id);
        return OPENRAC_HOOK_OK;
    };
    api.camera_script_view = [](float eye[3], float rotation[3], float where[3], int* stand_in) {
        eye[0] = 1.0f;
        rotation[2] = 2.0f;
        where[1] = 3.0f;
        *stand_in = 1;
        return OPENRAC_HOOK_OK;
    };
    api.level_line = [](const float from[3], const float to[3], int* blocked) {
        *blocked = from[0] < to[0];
        return from[1] != 0.0f ? OPENRAC_HOOK_UNKNOWN : OPENRAC_HOOK_OK;
    };
    api.camera_script_show = [](openrac_mod*, const float eye[3], const float rotation[3]) {
        g_12 = "show " + std::to_string(static_cast<int>(eye[0] + rotation[2]));
        return OPENRAC_HOOK_OK;
    };
    api.play_scene_from = [](openrac_mod*, int scene, int frame) {
        g_12 = "scene " + std::to_string(scene) + " " + std::to_string(frame);
        return OPENRAC_HOOK_OK;
    };
    api.play_live_picture = [](openrac_mod*, int on) {
        g_12 = "picture " + std::to_string(on);
        return OPENRAC_HOOK_OK;
    };
    api.on_metal_detect = [](openrac_mod*, openrac_metal_detect_fn, void*) { return OPENRAC_HOOK_OK; };
    api.packs_info = [](openrac_packs_info* out) {
        out->count = 1;
        out->digest = 77;
        return OPENRAC_HOOK_OK;
    };
    api.packs_entry = [](int index, openrac_pack_entry* out) {
        if (index != 0) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        out->id = "maps";
        out->version = "2.0.0";
        out->digest = 5;
        out->name = "Maps";
        return OPENRAC_HOOK_OK;
    };
    api.clipboard_set = [](const char* text) {
        g_12 = std::string("clip ") + text;
        return OPENRAC_HOOK_OK;
    };
    api.open_link = [](const char* url) {
        g_12 = std::string("link ") + url;
        return OPENRAC_HOOK_OK;
    };
    api.sim_digest = [](int32_t level, uint32_t* out) {
        if (level != 3) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        *out = 0x1234u;
        return OPENRAC_HOOK_OK;
    };
    api.world_family = [](char* out, uint32_t size) {
        std::snprintf(out, size, "rac1-60-1");
        return 9;
    };
    api.identity_key = [](char* out, uint32_t size) {
        std::snprintf(out, size, "key-1");
        return 5;
    };
}

// Returns a table of the fake host's functions, as a port of the newest interface would fill it.
openrac_api make_api() {
    openrac_api api{};
    api.size = sizeof api;
    api.api_major = OPENRAC_MOD_API_MAJOR;
    api.api_minor = OPENRAC_MOD_API_MINOR;
    api.game = "rac1";
    api.version = "pal";
    api.openrac_version = "0.0.0";
    api.platform = "test";
    api.memory = g_memory;
    api.log = host_log;
    api.fail = host_fail;
    api.setting_bool = host_setting_bool;
    api.setting_int = host_setting_int;
    api.setting_float = host_setting_float;
    api.setting_string = host_setting_string;
    api.hook = host_hook;
    fill_12(api);
    api.mod_count = host_mod_count;
    api.mod_entry = host_mod_entry;
    api.mod_setting = host_mod_setting;
    api.party_rules_publish = host_rules_publish;
    api.party_rules_read = host_rules_read;
    api.party_rules_text = host_rules_text;
    api.on_party_rules = host_on_rules;
    api.party_rules_deliver = host_rules_deliver;
    api.party_replay = host_replay;
    api.party_replaying = host_replaying;
    api.on_frame = host_on_frame;
    api.on_pad = host_on_pad;
    api.function = host_function;
    api.function_at = host_function_at;
    api.relocate_data = host_relocate;
    api.relocate_level_data = host_relocate_level;
    api.loaded_level = host_loaded_level;
    api.stack_push = host_stack_push;
    api.stack_pop = host_stack_pop;
    api.input = host_input;
    api.take_input = host_take_input;
    api.capture_mouse = host_capture_mouse;
    api.graphics = host_graphics;
    api.on_views = host_on_views;
    api.setup_vulkan = host_setup_vulkan;
    api.menu_entry = host_menu_entry;
    api.menu_rect = host_menu_rect;
    api.menu_text = host_menu_text;
    api.on_tick_boundary = host_on_tick_boundary;
    api.savestate_size = host_savestate_size;
    api.savestate_save = host_savestate_save;
    api.savestate_load = host_savestate_load;
    api.ui_page = host_ui_page;
    api.ui_main_item = host_ui_main_item;
    api.ui_set = host_ui_set;
    api.ui_open = host_ui_open;
    api.ui_confirm = host_ui_confirm;
    api.ui_cursor = host_ui_cursor;
    api.ui_sound = host_ui_sound;
    api.play_status = host_play_status;
    api.level_name = host_level_name;
    api.hero_pose = host_hero_pose;
    api.figure_create = host_figure_create;
    api.figure_pose = host_figure_pose;
    api.figure_remove = host_figure_remove;
    api.play_start = host_play_start;
    api.play_travel = host_play_travel;
    api.save_capture = host_save_capture;
    api.save_slot = host_save_slot;
    api.save_read = host_save_read;
    api.moby_list = host_moby_list;
    api.moby_slot = host_moby_slot;
    api.moby_drive = host_moby_drive;
    api.moby_pose = host_moby_pose;
    api.moby_kill = host_moby_kill;
    api.moby_spawn = host_moby_spawn;
    api.figure_slot = host_figure_slot;
    api.on_moby_update = host_on_moby_update;
    api.hero_stand_in = host_hero_stand_in;
    api.on_moby_hit = host_on_moby_hit;
    api.moby_hit = host_moby_hit;
    api.play_eyes = host_play_eyes;
    api.bolts = host_bolts;
    api.hero_held = host_hero_held;
    api.figure_held = host_figure_held;
    api.hero_shots = host_hero_shots;
    api.figure_shot = host_figure_shot;
    api.scene_playing = host_scene_playing;
    api.play_scene = host_play_scene;
    api.moby_hide = host_moby_hide;
    api.moby_deletion = host_moby_deletion;
    api.camera_position = host_camera_position;
    api.moby_run = host_moby_run;
    return api;
}

// ----------------------------------------------------------------------------------------------
// The mod under test
// ----------------------------------------------------------------------------------------------

// The mod's load function: reads each kind of setting, takes two hooks, tries two hook takes that
// must fail, registers both callbacks and calls the helpers of the layer. Throws nothing.
void load(openrac::Mod& mod) {
    CHECK(mod.game() == "rac1");
    CHECK(mod.version() == "pal");
    CHECK(mod.setting<bool>("trace"));
    CHECK(mod.setting<int>("count") == 42);
    CHECK(mod.setting<int64_t>("count") == 42);
    CHECK(mod.setting<double>("scale") == 1.5);
    CHECK(mod.setting<float>("scale") == 1.5f);
    CHECK(mod.setting<std::string>("mode") == "lock");

    mod.hook<openrac::rac1::Hero_SetPlanarVelocity>([](auto original, float angle) {
        // The test turns this on to see an exception stop at the boundary.
        if (g_throw_in_hook) {
            throw std::runtime_error("on purpose");
        }

        original(angle + 1.0f);
    });

    const int32_t offset = 5;

    mod.hook<Test_Twice>([offset](auto original, int32_t x) { return original(x) + offset; });

    // A hook point the port lacks, or one taken twice: an Error, and nothing hooked.
    bool threw = false;

    // The fake port has no slot for this point, so the take must be refused with the port's words.
    try {
        mod.hook<openrac::rac1::Hero_TurnToHeading>(
            [](auto original, int32_t m, float a, float d, float l) { original(m, a, d, l); }
        );
    } catch (const openrac::Error& e) {
        threw = std::string(e.what()).find("no such hook point") != std::string::npos;
    }

    CHECK(threw);
    threw = false;

    // Test_Twice was taken above, so a second take must be refused by the layer itself.
    try {
        mod.hook<Test_Twice>([](auto original, int32_t x) { return original(x); });
    } catch (const openrac::Error&) {
        threw = true;
    }

    CHECK(threw);

    mod.on_frame([] {
        // The second tick throws, to see the callback's exception logged and swallowed.
        if (++g_frames == 2) {
            throw std::runtime_error("frame two");
        }
    });
    mod.on_pad([](int port, uint8_t* report, int size) {
        CHECK(port == 0 && size == 4);
        report[0] ^= 0xFF;
    });

    auto twice = mod.required_function<int32_t (*)(int32_t)>("func_twice");

    CHECK(twice(4) == 8);
    CHECK(mod.function<void (*)()>("nothing") == nullptr);
    CHECK(mod.relocate(0x100) == 0x110);
    CHECK(mod.relocate_level(2, 0x100) == 0x102);
    CHECK(mod.loaded_level() == 3);
    CHECK(mod.has_input());

    mod.take_input(OPENRAC_INPUT_KEYBOARD);
    mod.info("loaded");
}

}  // namespace

OPENRAC_MOD_CPP("test-cpp", "1.2.3", load)

// A hook point's type by its permanent id is the same type (games/rac1.hpp, hooks::), and asks for
// the id.
static_assert(std::is_same_v<
              openrac::rac1::hooks::player::set_planar_velocity,
              openrac::rac1::Hero_SetPlanarVelocity>);
static_assert(
    std::string_view(openrac::rac1::hooks::moby::bolt::update::id) == "rac1.moby.bolt.update"
);
static_assert(std::string_view(openrac::detail::hook_id<Test_Twice>()) == "Test_Twice");

namespace {

// ----------------------------------------------------------------------------------------------
// The checks, one per behaviour
// ----------------------------------------------------------------------------------------------

// Checks the play calls (1.8): the sizes set by the layer, the status, names, poses, figures, the
// saves read whole, a start with and without a save; and none of it on a 1.7 port.
void check_play(openrac::Mod& mod, openrac_api& api) {
    CHECK(mod.has_play());
    const openrac_play_status status = mod.play_status();
    CHECK(status.phase == OPENRAC_PLAY_PLAYING && status.level == 3 && status.loads == 2);
    CHECK(mod.level_name(3) == "Kerwan" && mod.level_name(4).empty());
    openrac_pose pose{};
    CHECK(mod.hero_pose(pose) && pose.state == 7 && pose.position[0] == 1.5f);
    const int figure = mod.figure_create();
    pose.seq = 12;
    CHECK(figure == 0 && mod.figure_pose(figure, pose) && !mod.figure_pose(3, pose));
    CHECK(mod.figure_remove(0) && !mod.figure_remove(1));
    openrac_save_info info{};
    CHECK(mod.save_slot(0, info) == 1 && info.level == 3 && info.size == sizeof info);
    CHECK(mod.save_slot(1, info) == 0 && mod.save_slot(2, info) == -1);
    const std::vector<std::uint8_t> save = mod.save_read(0);
    CHECK(save.size() == 4 && save[0] == 'S' && mod.save_read(1).empty());
    CHECK(mod.save_capture() == save);
    CHECK(mod.play_start(-1, save) == OPENRAC_HOOK_OK && mod.play_start(0) == OPENRAC_HOOK_OK);
    CHECK(mod.play_travel(5) == OPENRAC_HOOK_TAKEN);
    CHECK((g_played == std::vector<std::string>{"pose 0 12", "pose 3 12", "start -1 4", "start 0 new",
                                                 "travel 5"}));
    // A 1.7 port: no play, and the calls answer nothing.
    api.api_minor = 7;
    CHECK(!mod.has_play() && mod.play_status().phase == OPENRAC_PLAY_NONE && mod.figure_create() == -1);
    CHECK(mod.save_capture().empty() && mod.play_start(0) == OPENRAC_HOOK_UNKNOWN);
    api.api_minor = OPENRAC_MOD_API_MINOR;
}

// Checks the world calls (1.9): the list (and again with room when the port has more than 256),
// slots, drives, poses, kills, spawns, figure slots, the gate and watcher (taken during load, a
// refusal thrown), the stand-in, hits, eyes and bolts; and none of it on a 1.8 port.
int gate_fn(void*, int, int) {
    return 0;
}

int hit_fn(void*, const openrac_hit*) {
    return 0;
}

void check_world(openrac::Mod& mod, openrac_api& api) {
    CHECK(mod.has_world());
    g_world.clear();
    const std::vector<openrac_moby> mobys = mod.moby_list();
    CHECK(mobys.size() == 3 && mobys[2].slot == 2 && mobys[2].key == 2 && mobys[2].oclass == 102);
    CHECK((mobys[0].flags & OPENRAC_MOBY_PLACED) != 0 && (mobys[0].flags & OPENRAC_MOBY_DRIVEN) == 0);
    CHECK((g_world == std::vector<std::string>{"list 256"}));
    // More than 256: asked again with room for all of them.
    g_world.clear();
    g_world_count = 300;
    const std::vector<openrac_moby> many = mod.moby_list();
    CHECK(many.size() == 300 && many[299].slot == 299 && many[299].size == sizeof(openrac_moby));
    CHECK((g_world == std::vector<std::string>{"list 256", "list 300"}));
    g_world_count = 3;
    CHECK(mod.moby_slot(7) == 7 && mod.moby_slot(1) == -1);
    g_world.clear();
    CHECK(mod.moby_drive(1) && mod.moby_drive(1, false) && !mod.moby_drive(5));
    openrac_moby pose{};
    pose.seq = 4;
    CHECK(mod.moby_pose(2, pose) && !mod.moby_pose(5, pose));
    CHECK(mod.moby_kill(0) && !mod.moby_kill(8));
    CHECK(mod.moby_spawn(100, pose) == 12 && mod.moby_spawn(101, pose) == -1);
    CHECK(mod.figure_slot(-1) == 9 && mod.figure_slot(0) == 10 && mod.figure_slot(3) == -1);
    const float where[3] = {1.0f, 2.0f, 3.0f};
    CHECK(mod.hero_stand_in(where, -1) && !mod.hero_stand_in(where, 2));
    openrac_hit hit{};
    hit.victim = 1;
    CHECK(mod.moby_hit(hit));
    hit.victim = 6;
    CHECK(!mod.moby_hit(hit));
    mod.play_eyes({0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f});
    CHECK(mod.bolts() == 100 && mod.bolts(5) == 105);
    mod.on_moby_update(gate_fn, &mod);
    mod.on_moby_hit(hit_fn);
    bool threw = false;
    try {
        mod.on_moby_update(nullptr);
    } catch (const openrac::Error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK((g_world == std::vector<std::string>{
                          "drive 1 1", "drive 1 0", "drive 5 1", "pose 2 4", "pose 5 4", "kill 0",
                          "kill 8", "spawn 100 1", "spawn 101 1", "stand_in 2 -1", "stand_in 2 2",
                          "hit 1 1", "hit 6 1", "eyes 2 5", "gate 1", "watch", "gate 0"}));
    // A 1.8 port: no world, and the calls answer nothing.
    api.api_minor = 8;
    g_world.clear();
    CHECK(!mod.has_world() && mod.moby_list().empty() && mod.moby_slot(7) == -1);
    CHECK(!mod.moby_drive(1) && !mod.moby_pose(2, pose) && !mod.moby_kill(0));
    CHECK(mod.moby_spawn(100, pose) == -1 && mod.figure_slot(-1) == -1 && !mod.hero_stand_in(where, -1));
    CHECK(!mod.moby_hit(hit) && mod.bolts(5) == 0);
    mod.play_eyes({0.0f, 0.0f, 0.0f});
    bool refused = false;
    try {
        mod.on_moby_hit(hit_fn);
    } catch (const openrac::Error&) {
        refused = true;
    }
    CHECK(refused && g_world.empty() && g_bolts == 105);
    api.api_minor = OPENRAC_MOD_API_MINOR;
}

// Checks the held pieces and the shots (1.9): what the hero holds, a figure made to hold it (its
// size set), the hero's shots, a figure firing one; none of it on a port whose table ends before
// them.
void check_held(openrac::Mod& mod, openrac_api& api) {
    CHECK(mod.has_held());
    g_world.clear();
    openrac_held held{};
    CHECK(mod.hero_held(held) && held.count == 1 && held.pieces[0].oclass == 0x47);
    openrac_held unsized{};
    CHECK(mod.figure_held(0, unsized) && !mod.figure_held(2, held));
    const std::vector<openrac_shot> shots = mod.hero_shots();
    CHECK(shots.size() == 2 && shots[0].item == 15 && shots[1].item == 10);
    openrac_shot shot{};
    shot.item = 15;
    CHECK(mod.figure_shot(0, shot) && !mod.figure_shot(1, shot));
    CHECK((g_world == std::vector<std::string>{"held 0 0 1", "held 2 1 1", "shot 0 15", "shot 1 15"}));
    // A port whose table ends at play_skip: no pieces, no shots.
    const std::uint32_t size = api.size;
    api.size = static_cast<std::uint32_t>(offsetof(openrac_api, play_skip) + sizeof(void*));
    g_world.clear();
    CHECK(!mod.has_held() && !mod.hero_held(held) && held.count == 0);
    CHECK(!mod.figure_held(0, held) && mod.hero_shots().empty() && !mod.figure_shot(0, shot));
    CHECK(g_world.empty());
    api.size = size;
}

// Checks the level's scenes (1.9): none playing, one started and then playing with its frames, a
// request dropped; nothing on a port whose table ends before them.
void check_scenes(openrac::Mod& mod, openrac_api& api) {
    CHECK(mod.has_scenes());
    g_world.clear();
    int frame = -1;
    CHECK(mod.scene_playing(&frame) == -1 && frame == 0 && mod.scene_playing() == -1);
    CHECK(mod.play_scene(4) && mod.scene_playing(&frame) == 4 && frame == 30);
    CHECK(mod.play_scene(-1) && mod.scene_playing() == -1);
    CHECK((g_world == std::vector<std::string>{"scene 4", "scene -1"}));
    // A port whose table ends at ui_pause_item: no scenes.
    const std::uint32_t size = api.size;
    api.size = static_cast<std::uint32_t>(offsetof(openrac_api, ui_pause_item) + sizeof(void*));
    g_world.clear();
    frame = -1;
    CHECK(!mod.has_scenes() && mod.scene_playing(&frame) == -1 && frame == 0 && !mod.play_scene(4));
    CHECK(g_world.empty());
    api.size = size;
}

// Checks a moby hidden and shown, its deletion and the camera read (1.9); nothing on a port whose table ends
// at play_scene.
void check_moby_detail(openrac::Mod& mod, openrac_api& api) {
    CHECK(mod.has_moby_detail());
    g_world.clear();
    CHECK(mod.moby_hide(5, true) && mod.moby_hide(5, false) && !mod.moby_hide(-1, true));
    CHECK((g_world == std::vector<std::string>{"hide 5 1", "hide 5 0", "hide -1 1"}));
    openrac_moby_deletion d{};
    CHECK(mod.moby_deletion(7, d) && d.by == 3 && d.by_class == 120 && d.oclass == 500 && d.loads == 2);
    CHECK(!mod.moby_deletion(8, d) && d.by == 0);
    float camera[3] = {0, 0, 0};
    CHECK(mod.camera_position(camera) && camera[0] == 1.0f && camera[2] == 3.0f);
    const std::uint32_t size = api.size;
    api.size = static_cast<std::uint32_t>(offsetof(openrac_api, play_scene) + sizeof(void*));
    g_world.clear();
    CHECK(!mod.has_moby_detail() && !mod.moby_hide(5, true) && !mod.moby_deletion(7, d));
    CHECK(!mod.camera_position(camera));
    CHECK(g_world.empty());
    api.size = size;
}

// Checks that openrac_mod_query reports the interface the mod was built for and its id and version.
void check_query() {
    openrac_mod_info info{};
    info.size = sizeof info;

    openrac_mod_query(&info);

    CHECK(info.api_major == OPENRAC_MOD_API_MAJOR && info.api_minor == OPENRAC_MOD_API_MINOR);
    CHECK(std::string(info.id) == "test-cpp" && std::string(info.version) == "1.2.3");
}

// Checks the hooks as the port calls them: through the slot, to the game's function, and past an
// exception in the replacement.
void check_hooks() {
    // The port calls what the slot holds: the lambda, which goes on to the game's function.
    auto velocity =
        reinterpret_cast<void (*)(float)>(g_slots[openrac::rac1::Hero_SetPlanarVelocity::id]);

    velocity(1.0f);

    CHECK(g_game_angles.size() == 1 && g_game_angles[0] == 2.0f);

    auto twice = reinterpret_cast<int32_t (*)(int32_t)>(g_slots["Test_Twice"]);

    CHECK(twice(10) == 25);
    CHECK(openrac::detail::instance()->original<Test_Twice>() == &game_twice);

    // An exception in the hook stops at the boundary: logged with where, and the original runs.
    g_throw_in_hook = true;
    velocity(7.0f);

    CHECK(g_game_angles.size() == 2 && g_game_angles[1] == 7.0f);
    CHECK(
        !g_log.empty()
        && g_log.back().find("Hero_SetPlanarVelocity: on purpose") != std::string::npos
    );
    CHECK(g_log.back().rfind("3 ", 0) == 0);  // OPENRAC_LOG_ERROR
}

// Checks the callbacks: an exception in one is logged and returns, and the pad callback changes
// the report in place.
void check_callbacks() {
    g_frame_fn(g_frame_user);
    g_frame_fn(g_frame_user);

    CHECK(g_frames == 2);
    CHECK(g_log.back().find("on_frame: frame two") != std::string::npos);

    uint8_t report[4] = {0x0F, 0, 0, 0};

    g_pad_fn(g_pad_user, 0, report, 4);

    CHECK(report[0] == 0xF0);
}

// Checks game memory (read, write, ref) and that a stack frame pushes and pops.
void check_memory_and_stack(openrac::Mod& mod) {
    mod.memory().write<float>(0x21, 3.5f);  // unaligned on purpose

    CHECK(mod.memory().read<float>(0x21) == 3.5f);

    mod.memory().ref<uint32_t>(0x40) = 0xDEADBEEFu;

    CHECK(mod.memory().read<uint32_t>(0x40) == 0xDEADBEEFu);

    {
        openrac::StackFrame frame(mod, 20);

        CHECK(frame.address() == 0x100 - 32);
        CHECK(g_sp == 0x100 - 32);
    }

    CHECK(g_sp == 0x100);
}

// Checks the binds by name, and each name's way there and back.
void check_binds() {
    CHECK(g_taken == OPENRAC_INPUT_KEYBOARD);
    CHECK(openrac::bind_from_name("KeyW") == 26 && openrac::bind_name(26) == "KeyW");
    CHECK(openrac::bind_from_name("MouseRight") == OPENRAC_BIND_MOUSE_RIGHT);
    CHECK(openrac::bind_from_name("WheelDown") == OPENRAC_BIND_WHEEL_DOWN);
    CHECK(openrac::bind_from_name("NoSuchKey") == OPENRAC_BIND_NONE);
    CHECK(openrac::bind_from_name(std::string_view("Space,KeyW").substr(0, 5)) == 44);

    // Every name of the table must map back to its own bind.
    for (int i = 0; i < OPENRAC_BIND_NAME_COUNT; ++i) {
        CHECK(openrac::bind_from_name(openrac_bind_names[i].name) == openrac_bind_names[i].bind);
    }
}

// Checks the input state (1.2) and the pointer (1.3): held and pressed keys, buttons and the wheel,
// the pointer on a port that has it and one that does not, and the capture.
void check_input(openrac::Mod& mod) {
    g_input.keys[26] = OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED;
    g_input.keys[44] = OPENRAC_KEY_PRESSED;  // tapped between two ticks
    g_input.mouse_held = OPENRAC_MOUSE_LEFT;
    g_input.mouse_pressed = OPENRAC_MOUSE_RIGHT;
    g_input.wheel_y = -1.0f;
    g_input.mouse_dx = 3.0f;
    g_input.size = sizeof g_input;
    g_input.pointer_x = 0.25f;
    g_input.pointer_y = 0.5f;
    g_input.pointer_inside = 1;

    const openrac::Input in = mod.input();

    CHECK(
        in.has_pointer() && in.pointer_x() == 0.25f && in.pointer_y() == 0.5f && in.pointer_inside()
    );

    {
        openrac_input old = g_input;  // a port of 1.2: the struct ends before the pointer

        old.size = static_cast<uint32_t>(offsetof(openrac_input, pointer_x));

        const openrac::Input before(&old);

        CHECK(!before.has_pointer() && before.pointer_x() == 0.0f && !before.pointer_inside());
    }

    CHECK(in.held(26) && in.pressed(26) && !in.held(44) && in.pressed(44) && !in.held(4));
    CHECK(in.held(OPENRAC_BIND_MOUSE_LEFT) && !in.pressed(OPENRAC_BIND_MOUSE_LEFT));
    CHECK(!in.held(OPENRAC_BIND_MOUSE_RIGHT) && in.pressed(OPENRAC_BIND_MOUSE_RIGHT));
    CHECK(in.pressed(OPENRAC_BIND_WHEEL_DOWN) && !in.pressed(OPENRAC_BIND_WHEEL_UP));
    CHECK(in.mouse_dx() == 3.0f && !in.held(OPENRAC_BIND_NONE));

    mod.capture_mouse(true);

    CHECK(g_capture == 1);

    mod.capture_mouse(false);

    CHECK(g_capture == 0);
}

// Checks the graphics, the views and the Vulkan device (1.4): what the port
// reports, the source and setup given with their size, a second asking refused
// with the port's words, and a 1.3 port.
void check_views(openrac::Mod &mod, const openrac_api &api) {
  CHECK(mod.has_views());
  CHECK(mod.graphics().backend == OPENRAC_GRAPHICS_OPENGL);

  openrac_view_source source{};
  source.begin = [](void *, openrac_view *, int) { return 0; };
  source.end = [](void *, const openrac_view *, int) {};
  mod.on_views(source);

  CHECK(g_views.size == sizeof(openrac_view_source) &&
        g_views.begin != nullptr);

  bool refused = false;

  // The fake port has views already: the second asking is taken.
  try {
    mod.on_views(source);
  } catch (const openrac::Error &e) {
    refused = std::string(e.what()) == "on_views: another mod has it already";
  }

  CHECK(refused);

  mod.setup_vulkan(openrac_vulkan_setup{});

  CHECK(g_vulkan.size == sizeof(openrac_vulkan_setup));

  openrac_api old = api; // a port of 1.3: the table ends before the graphics
  old.api_minor = 3;
  old.size = static_cast<uint32_t>(offsetof(openrac_api, graphics));

  const openrac::Mod before(&old, nullptr);

  CHECK(!before.has_views());
}

// Checks the menu entry (1.5): the label and page given to the port, the page's drawing through
// MenuPage (centred text at its width), its return closing the page, an exception closing it too,
// a refusal thrown, and a 1.4 port.
void check_menu(openrac::Mod& mod, const openrac_api& api) {
    CHECK(mod.has_menu());
    int throw_at = -1;
    int calls = 0;
    mod.menu_entry("Multiplayer", [&](const openrac::MenuPage& page, const openrac::MenuFrame& frame) {
        ++calls;
        if (calls == throw_at) {
            throw std::runtime_error("page broke");
        }
        page.rect(1, 2, 3, 4, OPENRAC_RGBA(0, 0, 0, 255));
        page.text_centred(10, 2, OPENRAC_RGBA(255, 255, 255, 255), "AB");
        return frame.pressed(OPENRAC_PAD_TRIANGLE);
    });
    CHECK(g_menu_label == "Multiplayer" && g_menu_fn != nullptr);
    openrac_menu_frame frame{};
    frame.size = sizeof frame;
    CHECK(g_menu_fn(g_menu_user, &frame) == OPENRAC_MENU_STAY);
    // "AB" at size 2 is (2 * 6 - 1) * 2 = 22 pixels wide: centred on 512, it starts at 245.
    CHECK(g_menu_drawn.size() == 2 && g_menu_drawn[1] == "text 245 10 2 AB");
    CHECK(openrac::MenuPage::text_width("AB\nABC", 1) == 17);
    frame.pressed = OPENRAC_PAD_TRIANGLE;
    CHECK(g_menu_fn(g_menu_user, &frame) == OPENRAC_MENU_CLOSE);
    throw_at = calls + 1;
    frame.pressed = 0;
    CHECK(g_menu_fn(g_menu_user, &frame) == OPENRAC_MENU_CLOSE);
    CHECK(!g_log.empty() && g_log.back().find("page broke") != std::string::npos);

    bool refused = false;

    // The fake port has room for one: the second is refused.
    try {
        mod.menu_entry("Again", [](const openrac::MenuPage&, const openrac::MenuFrame&) { return true; });
    } catch (const openrac::Error& e) {
        refused = std::string(e.what()) == "menu_entry: refused (no label, or too many)";
    }

    CHECK(refused);

    openrac_api old = api;  // a port of 1.4: the table ends before the menu
    old.api_minor = 4;
    old.size = static_cast<uint32_t>(offsetof(openrac_api, menu_entry));

    const openrac::Mod before(&old, nullptr);

    CHECK(!before.has_menu());
}

// Checks the pages of the game's own menus (1.7): a page made and put in the main menu, its
// content given to the port as C (a button, a disabled one, a line, a choice, a footer), its
// events reaching the callable with a UiPage that opens, asks and plays, an exception logged, the
// fake port's third page refused, and a 1.6 port.
void check_ui(openrac::Mod& mod, const openrac_api& api) {
    CHECK(mod.has_ui());
    std::vector<std::string> seen;
    bool throw_next = false;
    const openrac::UiPage home = mod.ui_page(
        [&](const openrac::UiPage& page, const openrac::UiEvent& event) {
            if (throw_next) {
                throw std::runtime_error("page broke");
            }
            seen.push_back(std::to_string(page.id()) + ":" + std::to_string(event.kind()) + ":"
                           + std::to_string(event.item()) + ":" + std::to_string(event.value()));
            // Cross on the first item opens the other page; a show asks; an answer plays.
            if (event.is(OPENRAC_UI_CHOOSE) && event.item() == 0) {
                CHECK(page.open());
            } else if (event.is(OPENRAC_UI_SHOW)) {
                CHECK(page.confirm("Sure?", 9));
            } else if (event.is(OPENRAC_UI_ANSWER)) {
                CHECK(page.sound(OPENRAC_UI_SOUND_SELECT));
                CHECK(page.back());
            }
        },
        OPENRAC_UI_OWN_BACK
    );
    CHECK(home.id() == 0 && g_ui_pages.size() == 1 && g_ui_pages[0].flags == OPENRAC_UI_OWN_BACK);
    mod.ui_main_item(OPENRAC_UI_LOAD_GAME, "Multiplayer", home);
    CHECK(g_ui_main == "load_game:Multiplayer:0");

    openrac::UiContent content;
    content.title = "Home";
    content.items = {
        openrac::UiItem::button("Find"),
        openrac::UiItem::button("Later", false),
        openrac::UiItem::text("Name"),
        openrac::UiItem::choice_of("Code", {"A", "B", "C"}, 1),
    };
    content.footer = {"Connected"};
    home.set(content);
    CHECK(g_ui_pages[0].shown == "Home|0:0:Find|0:1:Later|2:0:Name|1:0:Code=B/3|Connected");
    CHECK(home.cursor() == 2 && home.set_cursor(3) == 3);

    openrac_ui_event e{};
    e.size = sizeof e;
    e.kind = OPENRAC_UI_CHOOSE;
    e.item = 0;
    g_ui_pages[0].fn(g_ui_pages[0].user, 0, &e);
    e.kind = OPENRAC_UI_SHOW;
    e.item = -1;
    g_ui_pages[0].fn(g_ui_pages[0].user, 0, &e);
    e.kind = OPENRAC_UI_ANSWER;
    e.value = 1;
    e.tag = 9;
    g_ui_pages[0].fn(g_ui_pages[0].user, 0, &e);
    CHECK(seen.size() == 3 && seen[0] == "0:2:0:0" && seen[2] == "0:5:-1:1");
    CHECK(g_ui_asked.size() == 4 && g_ui_asked[0] == "open 0" && g_ui_asked[1] == "confirm 0 Sure? 9"
          && g_ui_asked[2] == "sound 0" && g_ui_asked[3] == "open -1");
    throw_next = true;
    g_ui_pages[0].fn(g_ui_pages[0].user, 0, &e);
    CHECK(!g_log.empty() && g_log.back().find("page broke") != std::string::npos);

    // A second page fits, a third does not: refused, with the layer's words.
    mod.ui_page([](const openrac::UiPage&, const openrac::UiEvent&) {});
    bool refused = false;
    try {
        mod.ui_page([](const openrac::UiPage&, const openrac::UiEvent&) {});
    } catch (const openrac::Error& error) {
        refused = std::string(error.what()) == "ui_page: refused (too many pages)";
    }
    CHECK(refused);
    refused = false;
    try {
        mod.ui_main_item(OPENRAC_UI_OPTIONS, "Elsewhere", home);
    } catch (const openrac::Error&) {
        refused = true;
    }
    CHECK(refused);

    openrac_api old = api;  // a port of 1.6: the table ends before the pages
    old.api_minor = 6;
    old.size = static_cast<uint32_t>(offsetof(openrac_api, ui_page));
    openrac::Mod before(&old, nullptr);
    CHECK(!before.has_ui());
    refused = false;
    try {
        before.ui_page([](const openrac::UiPage&, const openrac::UiEvent&) {});
    } catch (const openrac::Error& error) {
        refused = std::string(error.what()) == "ui_page: requires interface 1.7";
    }
    CHECK(refused);
}

// Checks the tick boundary and the snapshot calls (1.6): the callback given to the port and run with
// the tick, a save inside it, a refused load thrown, an exception logged, and a 1.5 port.
void check_savestates(openrac::Mod& mod, const openrac_api& api) {
    CHECK(mod.has_savestates());
    CHECK(mod.savestate_size() == 64);
    std::vector<unsigned char> bytes(mod.savestate_size());
    std::uint32_t seen = 0;
    mod.on_tick_boundary([&](std::uint32_t tick) {
        seen = tick;
        if (tick == 0) {
            throw std::runtime_error("boundary broke");
        }
        mod.savestate_save(bytes.data(), bytes.size());
    });
    CHECK(g_tick_fn != nullptr);
    g_tick_fn(g_tick_user, 9);
    CHECK(seen == 9 && g_saved == 64);
    g_tick_fn(g_tick_user, 0);
    CHECK(!g_log.empty() && g_log.back().find("boundary broke") != std::string::npos);
    mod.savestate_load(bytes.data(), bytes.size());

    bool refused = false;

    try {
        mod.savestate_load(bytes.data(), 1);
    } catch (const openrac::Error& e) {
        refused = std::string(e.what()) == "savestate_load: refused";
    }

    CHECK(refused);

    openrac_api old = api;  // a port of 1.5: the table ends before the tick boundary
    old.api_minor = 5;
    old.size = static_cast<uint32_t>(offsetof(openrac_api, on_tick_boundary));

    const openrac::Mod before(&old, nullptr);

    CHECK(!before.has_savestates());

    bool needs = false;

    try {
        (void)before.savestate_size();
    } catch (const openrac::Error& e) {
        needs = std::string(e.what()) == "savestate_size: requires interface 1.6";
    }

    CHECK(needs);
}

// Checks that a load that throws refuses the mod with the message, whatever the exception is.
void check_refused_loads(const openrac_api& api, openrac_mod* handle) {
    g_failed.clear();

    CHECK(
        openrac::detail::load(
            &api, handle, [](openrac::Mod&) { throw openrac::Error("not for this disc"); }
        )
        == 1
    );
    CHECK(g_failed == "not for this disc");

    g_failed.clear();

    CHECK(openrac::detail::load(&api, handle, [](openrac::Mod&) { throw 7; }) == 1);
    CHECK(!g_failed.empty());
}

// Checks that a later load of the same library starts afresh: the hook point can be taken again.
void check_reload(const openrac_api& api, openrac_mod* handle) {
    g_slots["Test_Twice"] = reinterpret_cast<void*>(&game_twice);
    g_failed.clear();

    CHECK(
        openrac::detail::load(
            &api,
            handle,
            [](openrac::Mod& m) {
                m.hook<Test_Twice>([](auto original, int32_t x) { return original(x) + 100; });
            }
        )
        == 0
    );
    CHECK(g_failed.empty());

    auto twice = reinterpret_cast<int32_t (*)(int32_t)>(g_slots["Test_Twice"]);

    CHECK(twice(1) == 102);
}

}  // namespace

// Runs every check against the fake host. Returns 0 when all passed, 1 otherwise.
int main() {
    g_slots[openrac::rac1::Hero_SetPlanarVelocity::id] =
        reinterpret_cast<void*>(&game_set_planar_velocity);
    g_slots["Test_Twice"] = reinterpret_cast<void*>(&game_twice);

    openrac_api api = make_api();
    auto* handle = reinterpret_cast<openrac_mod*>(&api);

    check_query();

    CHECK(openrac_mod_load(&api, handle) == 0);
    CHECK(g_failed.empty());

    check_hooks();
    check_callbacks();

    openrac::Mod& mod = *openrac::detail::instance();

    check_memory_and_stack(mod);
    check_binds();
    check_input(mod);
    check_views(mod, api);
    check_menu(mod, api);
    check_savestates(mod, api);
    check_ui(mod, api);
    check_play(mod, api);
    check_world(mod, api);
    check_held(mod, api);
    check_scenes(mod, api);
    check_moby_detail(mod, api);
    {
        // The last run read (1.9); nothing on a port whose table ends at camera_position.
        openrac_moby_run run{};
        CHECK(mod.has_moby_run() && mod.moby_run(4, run) && run.runs == 9 && run.ran == 0);
        CHECK(run.update_dist == 20 && run.distance == 30.0f && run.menu == OPENRAC_RUN_MENU_LEVEL_CAMERA);
        const std::uint32_t size = api.size;
        api.size = static_cast<std::uint32_t>(offsetof(openrac_api, camera_position) + sizeof(void*));
        CHECK(!mod.has_moby_run() && !mod.moby_run(4, run) && run.runs == 0);
        api.size = size;
    }
    {
        // The mods a party agrees on (interface 1.12), and a port without them.
        openrac::Mod mod(&api, handle);
        CHECK(mod.has_party_rules());
        const std::vector<openrac::ModEntry> mods = mod.mods();
        CHECK(mods.size() == 1 && mods[0].id == "difficulty" && mods[0].version == "1.2.3");
        const std::vector<openrac::ModSetting> settings = mod.mod_settings(0);
        CHECK(settings.size() == 1 && settings[0].key == "seed" && settings[0].is_default);
        CHECK(settings[0].party == OPENRAC_PARTY_SETTING_HOST);
        CHECK(mod.party_rules_publish({1, 2, 3}, "Bolts x2"));
        std::vector<std::uint8_t> blob;
        std::uint32_t version = 0;
        CHECK(mod.party_rules_read(0, blob, version) && blob.size() == 3 && version == 4);
        CHECK(mod.party_rules_text(0) == "Bolts x2");
        static std::uint32_t heard = 0;
        CHECK(mod.on_party_rules([](void*, const void*, uint32_t size) { heard = size; }));
        CHECK(mod.party_rules_deliver(0, {9, 9}) && heard == 2);
        CHECK(mod.party_rules_deliver(0, {}) && heard == 0);
        mod.party_replay(true);
        CHECK(mod.party_replaying());
        mod.party_replay(false);
        api.api_minor = 11;
        CHECK(!mod.has_party_rules() && mod.mods().empty() && !mod.party_rules_publish({1}, ""));
        api.api_minor = OPENRAC_MOD_API_MINOR;
    }
    {
        // What a shared game needs (interface 1.12), and a port without it.
        openrac::Mod mod(&api, handle);
        CHECK(mod.has_save_filter() && mod.has_shared_progress() && mod.has_hero_place());
        CHECK(mod.has_overlay() && mod.has_shots2() && mod.has_moby_list2());
        CHECK(mod.has_camera_scripts() && mod.has_metal_detect());
        CHECK(mod.card_filter(OPENRAC_CARD_REDIRECT, "Game-coop") && g_12 == "filter 2 Game-coop");
        CHECK(mod.card_slot_hint(3) && g_12 == "hint 3");
        CHECK(mod.coop_save_write(1, {1, 2, 3}) && g_12 == "write 1 3");
        openrac_save_info info{};
        CHECK(mod.save_slot2("Game-coop", 0, info) == 1 && info.level == 3);
        CHECK(mod.save_read2("Game-coop", 0).size() == 5);
        CHECK(mod.has_card_notes() && mod.card_saves() == 4);
        CHECK(mod.card_note_write(2, "hello") && g_12 == "note 2 hello");
        CHECK(mod.card_note_read("coop", 0) == "key" && mod.card_note_read("coop", 1).empty());
        std::vector<std::uint8_t> section;
        CHECK(mod.shared_read(1, section) && section.size() == 2 && !mod.shared_read(2, section));
        CHECK(mod.shared_give(4, {1, 2, 3}) && g_12 == "give 4 3");
        std::uint8_t missions[OPENRAC_MISSION_STATE_SIZE] = {};
        CHECK(mod.mission_state_read(5, missions) && missions[0] == 5);
        missions[1] = 8;
        CHECK(mod.mission_state_write(2, missions) && g_12 == "mission 2 8");
        const float where[3] = {6, 0, 0};
        CHECK(mod.hero_place(where, 90, OPENRAC_PLACE_INVULNERABLE) && g_12 == "place 6 90 1");
        CHECK(mod.hero_grab(12) && g_12 == "grab 12" && mod.hero_release() && g_12 == "release");
        CHECK(mod.hero_hold_at(where, 2) && g_12 == "hold 6 2");
        openrac_stand_in as{};
        as.state = 7;
        CHECK(mod.hero_stand_in_as(as, 3) && g_12 == "as 7 3");
        CHECK(mod.hero_stand_in_read(as) && as.state == 120);
        CHECK(mod.hero_revive() && g_12 == "revive" && mod.hero_revive(&where) && g_12 == "revive here");
        CHECK(mod.on_overlay([](void*, const openrac_overlay*) {}) && g_12 == "overlay");
        const std::vector<openrac_shot2> shots = mod.hero_shots2();
        CHECK(shots.size() == 1 && shots[0].item == 11 && shots[0].target_key == 42);
        CHECK(mod.figure_shot2(3, shots[0]) && g_12 == "shot 3 42");
        const std::vector<openrac_moby2> mobys = mod.moby_list2();
        CHECK(mobys.size() == 1 && mobys[0].slot == 4 && mobys[0].mission == 7);
        CHECK(mod.camera_script_play(6) && g_12 == "camera 6");
        float eye[3] = {0.0f, 0.0f, 0.0f};
        float rotation[3] = {0.0f, 0.0f, 0.0f};
        float moby[3] = {0.0f, 0.0f, 0.0f};
        bool stand_in = false;
        CHECK(mod.camera_script_view(eye, rotation, moby, &stand_in) && stand_in);
        CHECK(eye[0] == 1.0f && rotation[2] == 2.0f && moby[1] == 3.0f);
        CHECK(mod.camera_script_show(eye, rotation) && g_12 == "show 3");
        const float near_point[3] = {0, 0, 0}, far_point[3] = {1, 0, 0}, outside[3] = {0, 1, 0};
        bool blocked = false;
        CHECK(mod.level_line(near_point, far_point, blocked) && blocked);
        CHECK(mod.level_line(far_point, near_point, blocked) && !blocked);
        CHECK(!mod.level_line(outside, far_point, blocked));
        CHECK(mod.play_scene_from(2, 90) && g_12 == "scene 2 90");
        CHECK(mod.play_live_picture(true) && g_12 == "picture 1");
        openrac_packs_info packs{};
        std::string id, pack_version;
        std::uint32_t digest = 0;
        CHECK(mod.packs_info(packs) && packs.count == 1 && packs.digest == 77);
        CHECK(mod.packs_entry(0, id, pack_version, digest) && id == "maps" && digest == 5);
        CHECK(!mod.packs_entry(1, id, pack_version, digest));
        std::string pack_name;
        CHECK(mod.packs_entry(0, id, pack_name, pack_version, digest) && pack_name == "Maps" && pack_version == "2.0.0");
        CHECK(mod.clipboard_set("K7M2-9QXR") && g_12 == "clip K7M2-9QXR");
        CHECK(mod.open_link("openrac://modset/K7M2-9QXR") && g_12 == "link openrac://modset/K7M2-9QXR");
        CHECK(mod.world_family() == "rac1-60-1" && mod.identity_key() == "key-1");
        CHECK(mod.sim_digest(3) == 0x1234u && mod.sim_digest(4) == 0);

        // A port of 1.11 has none of it: every call answers no and touches nothing.
        g_12.clear();
        api.api_minor = 11;
        CHECK(!mod.has_save_filter() && !mod.has_hero_place() && !mod.has_overlay());
        CHECK(!mod.card_filter(OPENRAC_CARD_DISCARD) && !mod.hero_place(where, 0) && !mod.clipboard_set("x"));
        CHECK(mod.hero_shots2().empty() && mod.moby_list2().empty() && mod.world_family().empty());
        CHECK(mod.sim_digest(3) == 0);
        CHECK(mod.save_slot2("", 0, info) == -1 && mod.save_read2("", 0).empty() && g_12.empty());
        api.api_minor = OPENRAC_MOD_API_MINOR;

        // A table that ends before the 1.12 slots (a port built from older headers) answers no too.
        const std::uint32_t size = api.size;
        api.size = static_cast<std::uint32_t>(offsetof(openrac_api, party_replaying) + sizeof(void*));
        CHECK(!mod.has_shared_progress() && !mod.identity_key().size() && !mod.open_link("openrac://x"));
        api.size = size;
    }
    check_refused_loads(api, handle);
    check_reload(api, handle);

    // The count of failed checks is the whole verdict.
    if (g_failures == 0) {
        std::puts("mod_hpp: all passed");
    }

    return g_failures == 0 ? 0 : 1;
}
