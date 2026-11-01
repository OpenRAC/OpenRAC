// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mod loader (game/common/mods.h) with real mods: mod_loader_test_mod.c built twice, as
// test-a and test-b, installed into a plugins folder of the test's own with hook points of its own
// (one, Test_Add, written as hostgen's hooks.c writes them). Load, hook chaining in load order,
// settings, frame and pad callbacks, a mod's own refusal, an unknown hook point and a wrong
// signature, status.json, and the slots left null when nothing loads.

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "check.h"
#include "common/json.h"
#include "mod_manifest.h"
#include "mod_play.h"
#include "mods.h"
#include "openrac/guest.h"
#include "openrac/script_binding.h"

namespace fs = std::filesystem;
using namespace openrac;

namespace {

// The hook point, as hooks.c writes one.
void* g_slot_add = nullptr;
int add_game(int x) {
    return x + 1;
}
int add(int x) {
    if (g_slot_add != nullptr) {
        return reinterpret_cast<int (*)(int)>(g_slot_add)(x);
    }
    return add_game(x);
}
const mods::HookPoint kHooks[] = {
    {"Test_Add", "func_00100000", "i32(i32)", &g_slot_add, reinterpret_cast<void*>(add_game)},
};

// The function the mods find by name (api->function): what they report.
std::vector<int> g_records;
extern "C" void test_record(int v) {
    g_records.push_back(v);
}

void (*g_pad_hook)(int, unsigned char*, int) = nullptr;

void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream s;
    s << in.rdbuf();
    return s.str();
}

const char* library_name() {
#if defined(_WIN32)
    return "test.dll";
#elif defined(__APPLE__)
    return "libtest.dylib";
#else
    return "libtest.so";
#endif
}

// Installs test-<which> into `plugins`: its library copied in, its manifest.
void install(
    const fs::path& plugins,
    const std::string& which,
    const char* built,
    const std::string& api = "1.12",
    const std::string& extra = ""
) {
    const fs::path dir = plugins / ("test-" + which);
    fs::create_directories(dir / "lib");
    fs::copy_file(built, dir / "lib" / library_name(), fs::copy_options::overwrite_existing);
    write(
        dir / "openrac-mod.json",
        R"({"format": 2, "id": "test-)" + which + R"(", "name": "Test", "version": "1.0.0",
        "api": ")"
            + api + R"(", "games": {"test": ["one"]}, "hooks": ["Test_Add"],)" + extra + R"(
        "plugin": {")"
            + std::string(mods::platform_key()) + R"(": "lib/)" + library_name() + R"("},
        "settings": [{"key": "tag", "name": "Tag", "type": "int", "default": 0},
                     {"key": "behaviour", "name": "Behaviour", "type": "choice", "default": "ok",
                      "choices": [{"value": "ok", "name": "OK"}, {"value": "fail", "name": "Fail"},
                                  {"value": "unknown_hook", "name": "Unknown"},
                                  {"value": "bad_signature", "name": "Bad"},
                                  {"value": "input", "name": "Input"},
                                  {"value": "views", "name": "Views"},
                                  {"value": "menu", "name": "Menu"},
                                  {"value": "savestate", "name": "Savestate"},
                                  {"value": "developer_menu", "name": "Developer menu"},
                                  {"value": "rules", "name": "Rules"},
                                  {"value": "twelve", "name": "Twelve"},
                                  {"value": "game_ui", "name": "Game UI"},
                                  {"value": "play", "name": "Play"},
                                  {"value": "world", "name": "World"}]},
                     {"key": "jump", "name": "Jump", "type": "keybind", "default": "Space"}]})"
    );
}

mods::Host host(const fs::path& plugins) {
    mods::Host h;
    h.game = "test";
    h.version = "one";
    h.openrac_version = "0.0.0";
    h.hooks = kHooks;
    h.hook_count = 1;
    h.list = plugins / "plugins.json";
    h.pad_hook = &g_pad_hook;
    h.now = "2026-10-21T00:00:00+00:00";
    return h;
}

// The test's game play (interface 1.9, game_host.h): the phase and the level loads the test sets,
// five levels, a hero in state 7, figures alive until the next load (the loader checks), saves in
// <cards>/SAVES whose first byte is 'S' and second the level, and what the game was asked to do.
int g_phase = OPENRAC_GAME_PHASE_FRONT_END;
std::uint32_t g_loads = 1;
std::vector<std::string> g_played;
const char* const kLevelNames[] = {"Veldin", "Novalis", "Aridia", "Kerwan", "Eudora"};

// The fake level of the world calls: slots 0 to 2 live (keys 0, 1 and none; the first two placed),
// a moby at 0x3000 + 0x100 * slot, a spawn making slot 3, the hero's moby in slot 9, and the bolts.
bool g_live[8] = {true, true, true, false, false, false, false, false};
int g_bolts = 100;
// The level's scene playing (-1 none): a scene asked for plays at once, at frame 42.
std::int32_t g_scene = -1;
constexpr std::int32_t kKeys[] = {0, 1, -1};

void reset_world() {
    std::fill(std::begin(g_live), std::end(g_live), false);
    std::fill(g_live, g_live + 3, true);
    g_bolts = 100;
    g_scene = -1;
}

std::int32_t slot_at(gaddr moby) {
    return static_cast<std::int32_t>((moby - 0x3000) / 0x100);
}

const openrac_game_play kPlay = {
    5,
    kLevelNames,
    [](std::int32_t* level) {
        *level = g_phase == OPENRAC_GAME_PHASE_PLAYING ? 2 : -1;
        return g_phase;
    },
    [] { return g_loads; },
    [](openrac_game_pose* out) {
        out->state = 7;
        return g_phase == OPENRAC_GAME_PHASE_PLAYING ? 1 : 0;
    },
    [] { return gaddr{0x2000}; },
    [](gaddr moby) { return moby == 0x2000 ? 1 : 0; },
    [](gaddr, const openrac_game_pose* pose) {
        g_played.push_back("pose " + std::to_string(pose->state));
    },
    [](gaddr) { g_played.push_back("remove"); },
    [](int level, const std::uint8_t*, std::uint32_t size) {
        g_played.push_back("start " + std::to_string(level) + " " + std::to_string(size));
        return 1;
    },
    [](int level) {
        g_played.push_back("travel " + std::to_string(level));
        return 1;
    },
    [](std::uint8_t*, std::uint32_t) { return std::uint32_t{0}; },
    "SAVES",
    "save%d.bin",
    2,
    [](const std::uint8_t* save,
       std::uint32_t size,
       std::int32_t* level,
       std::int32_t* bolts,
       std::int32_t* gold,
       std::int32_t* seconds) {
        if (size < 2 || save[0] != 'S') {
            return 0;
        }
        *level = save[1];
        *bolts = *gold = *seconds = 0;
        return 1;
    },
    [](openrac_game_moby_state* out, int max) {
        int n = 0;
        for (int slot = 0; slot < 8; ++slot) {
            if (!g_live[slot]) {
                continue;
            }
            if (n < max) {
                out[n] = {};
                out[n].slot = slot;
                out[n].key = slot < 3 ? kKeys[slot] : -1;
                out[n].flags = slot < 2 ? 0x1u | 0x4u : 0x1u;
            }
            ++n;
        }
        return n;
    },
    [](std::int32_t key) {
        for (int slot = 0; slot < 3; ++slot) {
            if (g_live[slot] && kKeys[slot] == key) {
                return slot;
            }
        }
        return -1;
    },
    [](std::int32_t slot) {
        return slot >= 0 && slot < 8 && g_live[slot] ? gaddr(0x3000 + 0x100 * slot) : gaddr{0};
    },
    [](gaddr moby) { return moby >= 0x3000 ? slot_at(moby) : -1; },
    [](gaddr moby, const openrac_game_pose* pose) {
        g_played.push_back(
            "moby_pose " + std::to_string(slot_at(moby)) + " "
            + std::to_string(static_cast<int>(pose->position[0])) + " "
            + std::to_string(static_cast<int>(pose->scale))
        );
    },
    [](gaddr moby) {
        g_played.push_back("moby_kill " + std::to_string(slot_at(moby)));
        g_live[slot_at(moby)] = false;
    },
    [](std::int32_t oclass) {
        g_played.push_back("moby_spawn " + std::to_string(oclass));
        g_live[3] = true;
        return 3;
    },
    [] { return 9; },
    [](const float position[3], gaddr moby) {
        g_played.push_back(
            "stand_in " + std::to_string(static_cast<int>(position[0])) + " "
            + std::to_string(static_cast<int>(position[2])) + " " + std::to_string(moby)
        );
    },
    [](gaddr victim, gaddr attacker, std::uint32_t mask, float damage, const float*, const float*) {
        g_played.push_back(
            "post_hit " + std::to_string(slot_at(victim)) + " " + std::to_string(slot_at(attacker))
            + " " + std::to_string(mask) + " " + std::to_string(static_cast<int>(damage))
        );
    },
    [](const float*, int count) { g_played.push_back("eyes " + std::to_string(count)); },
    [](std::int32_t add) { return g_bolts += add; },
    [](int on) { g_played.push_back("keep " + std::to_string(on)); },
    // What the hero holds (two pieces) and his shots (one of item 15), the figure's.
    [](openrac_game_held* out) {
        *out = {};
        out->count = 2;
        return g_phase == OPENRAC_GAME_PHASE_PLAYING ? 1 : 0;
    },
    [](gaddr, const openrac_game_held* held) {
        g_played.push_back("held " + std::to_string(held->count));
    },
    [](openrac_game_shot* out, int max) {
        if (max < 1) {
            return 0;
        }
        out[0] = {};
        out[0].item = 15;
        return 1;
    },
    [](gaddr, const openrac_game_shot* shot) {
        g_played.push_back("shot " + std::to_string(shot->item));
        return shot->item == 15 ? 1 : 0;
    },
    [](std::int32_t* frame) {
        *frame = 42;
        return g_scene;
    },
    [](std::int32_t scene) {
        g_played.push_back("scene_start " + std::to_string(scene));
        if (scene >= 0) {
            g_scene = scene;
        }
        return 1;
    },
    // A moby hidden or shown; a deletion seen of slot 2 only (a mod's, in slot 4's update); the
    // camera at x 7.
    [](gaddr moby, int hidden) {
        g_played.push_back(
            "moby_hide " + std::to_string(slot_at(moby)) + " " + std::to_string(hidden)
        );
    },
    [](std::int32_t slot, openrac_game_moby_deletion* out) {
        if (slot != 2) {
            return 0;
        }
        *out = {4, 120, 1, 300, 1};
        return 1;
    },
    [](float out[3]) {
        out[0] = 7.0f;
        out[1] = out[2] = 0.0f;
        return 1;
    },
};

// The state status.json gives a mod, and its reason.
std::pair<std::string, std::string> status_of(const fs::path& plugins, const std::string& id) {
    json::Value v;
    std::string error;
    if (!json::parse(read(plugins / "status.json"), v, error)) {
        return {"unreadable: " + error, ""};
    }
    for (const auto& m : v["mods"].items()) {
        if (m["id"].string() == id) {
            return {m["state"].string(), m["reason"].is_string() ? m["reason"].string() : ""};
        }
    }
    return {"absent", ""};
}

void two_mods_chain_in_load_order(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"tag": 1}},
        {"id": "test-b", "enabled": true, "settings": {"tag": 2}}]})");
    g_records.clear();
    const std::string line = mods::load(host(plugins));
    CHECK(line == "test-a 1.0.0, test-b 1.0.0");
    CHECK(mods::loaded_count() == 2);
    // b loaded last, so it runs first: b(a(game)) = ((0 + 1) * 10 + 1) * 10 + 2.
    CHECK(add(0) == 112);
    CHECK(status_of(plugins, "test-a").first == "loaded");
    CHECK(status_of(plugins, "test-b").first == "loaded");
    CHECK(read(plugins / "status.json").find("\"hook_points\": [\"Test_Add\"]") != std::string::npos);
    // Frame callbacks in load order; a hook taken in one (after load) is refused (1000 + 3).
    CHECK(mods::g_frame_callbacks);
    mods::frame();
    CHECK((g_records == std::vector<int>{1, 1003, 2, 1003}));
    // Pad callbacks, in load order, on the report.
    CHECK(g_pad_hook != nullptr);
    unsigned char report[18] = {10};
    g_pad_hook(0, report, 18);
    CHECK(report[0] == 13);
    mods::reset();
    CHECK(g_slot_add == nullptr && add(0) == 1 && g_pad_hook == nullptr && !mods::g_frame_callbacks);
}

void a_mod_that_fails_is_taken_back(const fs::path& plugins) {
    for (const char* behaviour : {"fail", "unknown_hook", "bad_signature"}) {
        write(plugins / "plugins.json", std::string(R"({"format": 1, "mods": [
            {"id": "test-a", "settings": {"tag": 1}},
            {"id": "test-b", "settings": {"tag": 2, "behaviour": ")") + behaviour + R"("}}]})");
        g_records.clear();
        const std::string line = mods::load(host(plugins));
        CHECK(line == "test-a 1.0.0 (1 not loaded: plugins/status.json)");
        CHECK(add(0) == 11);  // a's only: (0 + 1) * 10 + 1
        const auto [state, reason] = status_of(plugins, "test-b");
        CHECK(state == "failed");
        if (std::string(behaviour) == "fail") {
            CHECK(reason == "this mod refuses on purpose");
        } else if (std::string(behaviour) == "unknown_hook") {
            CHECK(reason.find("no hook point Test_Nothing") != std::string::npos);
        } else {
            CHECK(reason.find("is i32(i32) here, not void(i32)") != std::string::npos);
        }
        mods::frame();
        CHECK((g_records == std::vector<int>{1, 1003}));  // a's callback only
        mods::reset();
        CHECK(g_slot_add == nullptr);
    }
}

void refused_before_opening(const fs::path& plugins) {
    install(plugins, "b", OPENRAC_TEST_MOD_B, "2.0");
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": false}, {"id": "test-b"}, {"id": "test-c"}]})");
    CHECK(mods::load(host(plugins)) == "none (2 not loaded: plugins/status.json)");
    CHECK(g_slot_add == nullptr);
    CHECK(status_of(plugins, "test-a").first == "absent");  // off: not written
    CHECK(status_of(plugins, "test-b").second.find("needs interface 2.0") != std::string::npos);
    CHECK(status_of(plugins, "test-c").second.find("not installed") != std::string::npos);
    mods::reset();
    install(plugins, "b", OPENRAC_TEST_MOD_B);
}

void purist_and_no_list(const fs::path& plugins) {
    mods::Host h = host(plugins);
    h.purist = true;
    CHECK(mods::load(h) == "none (purist mode)");
    CHECK(g_slot_add == nullptr);
    mods::reset();
    h = host(plugins / "nowhere");
    CHECK(mods::load(h) == "none");
    mods::reset();
}

// The library's id must be its manifest's.
void a_library_of_another_id(const fs::path& plugins) {
    fs::copy_file(OPENRAC_TEST_MOD_A, plugins / "test-b" / "lib" / library_name(), fs::copy_options::overwrite_existing);
    write(plugins / "plugins.json", R"({"format": 1, "mods": [{"id": "test-b"}]})");
    CHECK(mods::load(host(plugins)) == "none (1 not loaded: plugins/status.json)");
    CHECK(status_of(plugins, "test-b").second.find("its library is mod \"test-a\"") != std::string::npos);
    mods::reset();
    install(plugins, "b", OPENRAC_TEST_MOD_B);
}

// A player's install, with no checkout anywhere: the launcher's per-user install folder
// (~/OpenRAC by default), the game's set-up folder in it, a mod as the launcher installs one from its
// .zip (the SDK's layout, lib/<platform>/<file>), the list named on the command line as the launcher
// names it, and the program started in an unrelated folder. Nothing is looked for beside the program
// or in a source tree: an installed launcher's own folder (an AppImage, an .app) is read-only.
void an_installed_launchers_layout(const fs::path& root) {
    const fs::path data = root / "home" / "OpenRAC" / "active" / "test" / "data";
    const fs::path plugins = data / "plugins";
    const fs::path mod = plugins / "test-a";
    const fs::path lib = fs::path("lib") / mods::platform_key() / library_name();
    fs::create_directories(mod / lib.parent_path());
    fs::copy_file(OPENRAC_TEST_MOD_A, mod / lib, fs::copy_options::overwrite_existing);
    write(
        mod / "openrac-mod.json",
        R"({"format": 2, "id": "test-a", "name": "Test", "version": "1.0.0",
        "api": "1.12", "games": {"test": ["one"]}, "hooks": ["Test_Add"],
        "plugin": {")"
            + std::string(mods::platform_key()) + R"(": ")" + lib.generic_string() + R"("},
        "settings": [{"key": "tag", "name": "Tag", "type": "int", "default": 0}]})"
    );
    write(plugins / "plugins.json", R"({"format": 1, "mods": [{"id": "test-a", "enabled": true, "settings": {"tag": 4}}]})");
    const fs::path elsewhere = root / "somewhere-else";
    fs::create_directories(elsewhere);
    const fs::path before = fs::current_path();
    fs::current_path(elsewhere);
    const std::string line = mods::load(host(plugins));
    fs::current_path(before);
    CHECK(line == "test-a 1.0.0");
    CHECK(add(0) == 14);
    CHECK(status_of(plugins, "test-a").first == "loaded");
    // Only status.json is written, beside the list; nothing in the folder the program ran in.
    CHECK(fs::is_empty(elsewhere));
    mods::reset();
}

// The keyboard and mouse (interface 1.2): the mod takes the keyboard during load, reads the
// state the host gives for the tick, and asks for the cursor (let go while the overlay is open).
openrac_input g_input{};
std::uint32_t g_taken = 0;
int g_captures = 0;
bool g_captured = false;

void input_reaches_the_mod(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "input", "jump": "KeyQ"}}]})");
    mods::Host h = host(plugins);
    g_input = openrac_input{};
    g_input.size = sizeof g_input;
    g_taken = 0;
    g_captures = 0;
    h.input = [] { return static_cast<const openrac_input*>(&g_input); };
    h.take_input = [](std::uint32_t devices) { g_taken |= devices; };
    h.capture_mouse = [](bool capture) {
        ++g_captures;
        g_captured = capture;
    };
    CHECK(mods::load(h) == "test-a 1.0.0");
    CHECK(g_taken == OPENRAC_INPUT_KEYBOARD);
    CHECK(g_captures == 1 && g_captured);
    g_input.keys[26] = OPENRAC_KEY_HELD;
    g_input.mouse_dx = 7.0f;
    g_input.overlay = 1;
    unsigned char report[18] = {0};
    CHECK(g_pad_hook != nullptr);
    g_pad_hook(0, report, 18);
    CHECK(report[1] == 0x57 && report[2] == 7);
    CHECK(g_captures == 2 && !g_captured);  // the overlay is open: it lets the cursor go
    mods::reset();

    // Without a host's input (a test, no window) the state is all released, never null.
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "input"}}]})");
    CHECK(mods::load(host(plugins)) == "test-a 1.0.0");
    unsigned char quiet[18] = {0, 9, 9};
    g_pad_hook(0, quiet, 18);
    CHECK(quiet[1] == 0 && quiet[2] == 0);
    mods::reset();
}

// The views and the Vulkan device (interface 1.4): one mod has them, given to the host once load
// is over, taken back at reset; a second mod asking is refused; the graphics are none without a
// window; and the mods load before the window only for a manifest with "vulkan_device".
const openrac_view_source* g_views = nullptr;
int g_views_given = 0;
const openrac_vulkan_setup* g_vulkan = nullptr;

// A mod's menu entry and page (interface 1.5): its label listed, its page's drawing sent to the
// host's menu_rect and menu_text while its callback runs (and only then), closed on what the mod
// returns; an entry after load is refused; reset takes the entries back.
std::vector<std::string> g_drawn;

void a_menu_page_draws_through_the_host(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "menu"}}]})");
    mods::Host h = host(plugins);
    g_drawn.clear();
    g_records.clear();
    h.menu_rect = [](int x, int y, int width, int height, std::uint32_t rgba) {
        g_drawn.push_back(std::format("rect {} {} {} {} {:08x}", x, y, width, height, rgba));
    };
    h.menu_text = [](int x, int y, int size, std::uint32_t rgba, const char* text) {
        g_drawn.push_back(std::format("text {} {} {} {:08x} {}", x, y, size, rgba, text));
    };
    CHECK(mods::load(h) == "test-a 1.0.0");
    CHECK(mods::menu_entry_count() == 1 && mods::menu_entry_label(0) == "Join the fun");
    CHECK(mods::menu_entry_label(1).empty());
    openrac_menu_frame frame{};
    frame.size = sizeof frame;
    frame.opened = 1;
    CHECK(!mods::run_menu_page(0, frame));
    CHECK(g_drawn.size() == 2);
    CHECK(g_drawn.size() == 2 && g_drawn[0] == "rect 10 20 30 40 ffffffff");
    CHECK(g_drawn.size() == 2 && g_drawn[1] == "text 1 2 3 ff0000ff HELLO");
    frame.opened = 0;
    frame.pressed = OPENRAC_PAD_CROSS;
    CHECK(mods::run_menu_page(0, frame));
    CHECK(g_drawn.size() == 4 && g_drawn[3] == "text 1 2 3 ff0000ff AGAIN");
    // Outside its page a mod draws nothing, and an entry after load is refused.
    mods::frame();
    CHECK(g_drawn.size() == 4);
    CHECK(std::find(g_records.begin(), g_records.end(), 1100 + 3) != g_records.end());
    CHECK(mods::run_menu_page(1, frame));  // no such page: closed
    mods::reset();
    CHECK(mods::menu_entry_count() == 0);
}

// The mods a party has to agree on (interface 1.11): the mods loaded and their settings as read this
// run, the rules a mod publishes and the host's it takes, the replay switch.
void a_party_reads_the_mods(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "rules", "tag": 7}},
        {"id": "test-b", "enabled": true}]})");
    g_records.clear();
    CHECK(mods::load(host(plugins)) == "test-a 1.0.0, test-b 1.0.0");
    const openrac_api& api = mods::api_for_tests();
    CHECK(api.api_minor >= 11 && api.mod_count() == 2);
    openrac_mod_entry entry{};
    entry.size = sizeof entry;
    CHECK(api.mod_entry(0, &entry) == OPENRAC_HOOK_OK);
    CHECK(std::string(entry.id) == "test-a" && std::string(entry.version) == "1.0.0");
    CHECK(entry.party == OPENRAC_PARTY_MOD_GAMEPLAY && entry.settings == 3);
    CHECK(api.mod_entry(2, &entry) == OPENRAC_HOOK_UNKNOWN);
    openrac_mod_setting setting{};
    setting.size = sizeof setting;
    CHECK(api.mod_setting(0, 0, &setting) == OPENRAC_HOOK_OK);
    CHECK(std::string(setting.key) == "tag" && std::string(setting.value) == "7" && !setting.is_default);
    CHECK(api.mod_setting(0, 1, &setting) == OPENRAC_HOOK_OK);
    CHECK(std::string(setting.value) == "rules" && std::string(setting.shown) == "Rules");
    CHECK(api.mod_setting(1, 0, &setting) == OPENRAC_HOOK_OK);
    CHECK(std::string(setting.value) == "0" && setting.is_default);
    CHECK(api.mod_setting(0, 3, &setting) == OPENRAC_HOOK_UNKNOWN);

    // What it published (a blob too big is refused, what was there stays); nothing from the other.
    char blob[8] = {};
    std::uint32_t version = 0;
    CHECK(api.party_rules_read(0, blob, sizeof blob, &version) == 3);
    CHECK(std::string(blob) == "abc" && version == 1);
    CHECK(std::string(api.party_rules_text(0)) == "Bolts x2");
    CHECK(api.party_rules_read(1, blob, sizeof blob, &version) == -1);

    // The host's reach the one that listens, and only it.
    CHECK(api.party_rules_deliver(0, "xy", 2) == OPENRAC_HOOK_OK);
    CHECK(api.party_rules_deliver(0, nullptr, 0) == OPENRAC_HOOK_OK);
    CHECK(api.party_rules_deliver(1, "xy", 2) == OPENRAC_HOOK_UNKNOWN);
    CHECK(api.party_rules_deliver(7, "xy", 2) == OPENRAC_HOOK_UNKNOWN);
    CHECK(std::count(g_records.begin(), g_records.end(), 7000 + 20 + ('x' - 'a')) == 1);
    CHECK(std::count(g_records.begin(), g_records.end(), 7000) == 1);

    CHECK(api.party_replaying() == 0);
    api.party_replay(1);
    CHECK(api.party_replaying() == 1);
    api.party_replay(0);
    mods::reset();
}

// The calls of interface 1.12 are in the table, and each answers "unknown" for now, to a mod that
// loads against the real table (the test mod's "twelve" checks every one).
void the_1_12_slots_answer_unknown(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "twelve"}},
        {"id": "test-b", "enabled": false}]})");
    CHECK(mods::load(host(plugins)) == "test-a 1.0.0");
    const openrac_api& api = mods::api_for_tests();
    CHECK(api.api_minor >= 12 && api.size >= offsetof(openrac_api, identity_key) + sizeof(void*));
    mods::reset();
}

// A mod-provided desktop menu (interface 1.10): copied strings, queued actions handled at the
// owning mod's tick boundary, labels changed there, and cleanup when mods reset.
void a_mod_provides_a_desktop_menu(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "developer_menu"}}]})");
    g_records.clear();
    CHECK(mods::load(host(plugins)) == "test-a 1.0.0");
    const auto& menus = mods::developer_menus();
    CHECK(menus.size() == 1 && menus[0].title == "Savestates");
    CHECK(menus[0].items.size() == 2);
    CHECK(menus[0].items[0].label == "Empty" && menus[0].items[0].group == "Save");
    CHECK(menus[0].items[0].shortcut == "Shift+Num0" && menus[0].items[0].action == 1);
    CHECK(menus[0].items[1].label == "Empty" && menus[0].items[1].group == "Load");
    CHECK(menus[0].items[1].shortcut == "Ctrl+Num0" && menus[0].items[1].action == 2);

    // Action retrieval and label changes are restricted to the tick-boundary callback.
    mods::frame();
    CHECK(std::find(g_records.begin(), g_records.end(), 1601) != g_records.end());
    CHECK(std::find(g_records.begin(), g_records.end(), 1701) != g_records.end());

    mods::queue_developer_menu_action(menus[0].mod, 1);
    mods::queue_developer_menu_action(menus[0].mod, 99);
    mods::tick_boundary(1);
    CHECK(std::find(g_records.begin(), g_records.end(), 9001) != g_records.end());
    CHECK(std::find(g_records.begin(), g_records.end(), 9100) != g_records.end());
    CHECK(menus[0].items[0].label == "Saved: Veldin");
    CHECK(menus[0].items[1].label == "Empty");

    mods::reset();
    CHECK(mods::developer_menus().empty());
}

// Play (interface 1.8): the phase and level, figures only in a level and gone with it, a pose
// reaching the game, what the hero holds and his shots and a figure's (1.9), the saves on the card
// read through the game's save_info, a start and a travel kept until play_update (one at a time),
// and the levels' names.
void play_reaches_the_game(const fs::path& plugins, const fs::path& dir) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "play"}}]})");
    write(dir / "cards" / "SAVES" / "save0.bin", std::string("S\x01xyz"));
    mods::Host h = host(plugins);
    h.play = &kPlay;
    h.cards = dir / "cards";
    g_phase = OPENRAC_GAME_PHASE_FRONT_END;
    g_loads = 1;
    g_played.clear();
    CHECK(mods::load(h) == "test-a 1.0.0");
    g_records.clear();
    // The front end: no figure, save 0 read and started with, one request at a time.
    mods::frame();
    CHECK((g_records == std::vector<int>{7001, 7101, 7211, 7210, 7305, 7400, 7404, 7404}));
    CHECK(g_played.empty());
    mods::play_update();
    CHECK((g_played == std::vector<std::string>{"start -1 5"}));
    mods::play_update();  // nothing pending
    CHECK(g_played.size() == 1);
    // A level: a figure, posed (not another's id), the hero's pose, a travel, the names.
    g_phase = OPENRAC_GAME_PHASE_PLAYING;
    g_records.clear();
    mods::frame();
    CHECK(
        (g_records
         == std::vector<int>{
             7003, 7500, 7601, 7600, 7707, 7722, 7730, 7731, 7771, 7760, 7761, 7800, 7903
         })
    );
    mods::play_update();
    CHECK(
        (g_played
         == std::vector<std::string>{
             "start -1 5", "pose 42", "held 2", "shot 15", "shot 3", "travel 4"
         })
    );
    // The level loaded again: the figure is gone.
    g_loads = 2;
    g_records.clear();
    mods::frame();
    CHECK((g_records == std::vector<int>{7003, 7601, 7731, 7601}));
    CHECK(g_played.size() == 6);
    // In the level's pause menu (1.9): a travel leaves from there too.
    g_phase = OPENRAC_GAME_PHASE_MENU;
    g_records.clear();
    mods::frame();
    CHECK((g_records == std::vector<int>{7005, 7810}));
    mods::play_update();
    CHECK(g_played.back() == "travel 3");
    mods::reset();
}

// The world (interface 1.9) on the test's fake level: the list and its flags, slots by key, drives
// (refused for a dead slot, shown in the list, gone with the level), poses, kills, a spawn, the
// hero's slot, eyes, bolts and a hit posted; the game's calls into the gate (a driven moby's update
// is skipped without asking the mod, the mod's skip honoured, a stand-in accepted only before an
// update) and the hit watcher (its answer decides the take-back); the gate and the watcher only
// during load; and mods::reset clearing the game's pointers.
void the_world_reaches_the_game(const fs::path& plugins, const fs::path& dir) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "world"}}]})");
    mods::Host h = host(plugins);
    h.play = &kPlay;
    h.cards = dir / "cards";
    g_phase = OPENRAC_GAME_PHASE_PLAYING;
    g_loads = 1;
    g_played.clear();
    reset_world();
    // Nothing asks the mods before they load.
    CHECK(openrac_game_moby_update == nullptr && openrac_game_moby_hit == nullptr);
    CHECK(mods::load(h) == "test-a 1.0.0");
    CHECK(openrac_game_moby_update != nullptr && openrac_game_moby_hit != nullptr);
    g_records.clear();
    mods::frame();
    CHECK(
        (g_records
         == std::vector<int>{
             // The list: 3 mobys, keys 1, 2 (0 and 1 +1) and none, flags placed+updates / updates;
             // a list of 2 still counts 3; the slot of key 1; no slot for key 7.
             8000 + 3,
             8011,
             8025,
             8012,
             8025,
             8010,
             8021,
             8033,
             8041,
             8050,
             // Drive slot 0 on and off, slot 1 on; a dead slot refused; slot 1 shown driven.
             8060,
             8060,
             8060,
             8071,
             8082,
             // A pose, a pose of a dead slot; a kill, a kill of a dead slot; a spawn in slot 3
             // (driven: the list ends with it driven, 3 live after the kill).
             8100,
             8111,
             8120,
             8131,
             8143,
             8163,
             8172,
             // The hero's slot; no figure 0; eyes (the count clamped to 8 in the game); bolts +5.
             8189,
             8190,
             8200,
             8210,
             8220,
             8305,
             // The world kept running under the game's menus; a skip asked for; no scene playing
             // (its frame left 0), scene 3 started and playing at frame 42, a request dropped.
             8320,
             8330,
             8340,
             8350,
             8360,
             8373,
             8381,
             8390,
             // A moby hidden, a dead one refused; slot 2's deletion (by slot 4, a mod's), none of
             // slot 1; the camera.
             8600,
             8611,
             8620,
             8634,
             8641,
             8651,
             8660,
             8677,
             // A stand-in outside the gate; a hit posted, a hit to a dead slot; the gate and the
             // watcher after load.
             8401,
             8500,
             8501,
             8903,
             8913,
         })
    );
    CHECK(
        (g_played
         == std::vector<std::string>{
             "moby_pose 1 4 2",
             "moby_kill 2",
             "moby_spawn 77",
             "moby_pose 3 4 2",
             "eyes 2",
             "eyes 8",
             "eyes 0",
             "keep 1",
             "scene_start 3",
             "scene_start -1",
             "moby_hide 1 1",
             "post_hit 1 0 4 3",
         })
    );
    // The game's gate: a driven moby (slot 1, 3) is skipped without asking the mod; others ask it,
    // which has the hero stand in before the update (not after) and skips slot 2.
    g_records.clear();
    g_played.clear();
    CHECK(openrac_game_moby_update(1, OPENRAC_MOBY_BEFORE) == 1);
    CHECK(openrac_game_moby_update(1, OPENRAC_MOBY_AFTER) == 0);
    CHECK(openrac_game_moby_update(3, OPENRAC_MOBY_BEFORE) == 1);
    CHECK(g_records.empty() && g_played.empty());
    CHECK(openrac_game_moby_update(0, OPENRAC_MOBY_BEFORE) == 0);
    CHECK(openrac_game_moby_update(0, OPENRAC_MOBY_AFTER) == 0);
    CHECK(openrac_game_moby_update(2, OPENRAC_MOBY_BEFORE) == 1);
    CHECK((g_records == std::vector<int>{8700, 8710, 8720, 8702, 8710}));
    CHECK((g_played == std::vector<std::string>{"stand_in 1 3 0", "stand_in 1 3 0"}));
    // The game's hit: the watcher is told and decides (more than 5 damage: taken back).
    const float at[3] = {0, 0, 0};
    g_records.clear();
    CHECK(openrac_game_moby_hit(1, -1, 4, 3.0f, at, at) == 0);
    CHECK(openrac_game_moby_hit(1, 0, 4, 9.0f, at, at) == 1);
    CHECK((g_records == std::vector<int>{8810, 8811}));
    // The level loaded again: the drives are gone with it (and the fake level is whole again).
    g_loads = 2;
    reset_world();
    g_records.clear();
    mods::frame();
    CHECK((g_records == std::vector<int>{8603, 8610}));
    CHECK(openrac_game_moby_update(1, OPENRAC_MOBY_BEFORE) == 0);
    // The skip the mod asked for is pressed over the next pad reads, then no more; reset forgets it,
    // and the mod's line over the picture.
    CHECK(mods::play_skipping() && mods::play_skipping());
    // Only while what it was asked in plays: out of a scene, or the next one, is pressed nothing.
    {
        openrac_api api{};
        int owner = 0;
        mods::fill_play_api(api, &kPlay, dir / "cards");
        g_phase = OPENRAC_GAME_PHASE_SCENE;
        g_scene = 3;
        CHECK(api.play_skip(reinterpret_cast<openrac_mod*>(&owner)) == OPENRAC_HOOK_OK);
        CHECK(mods::play_skipping());
        g_phase = OPENRAC_GAME_PHASE_PLAYING;
        g_scene = -1;
        CHECK(!mods::play_skipping());
        g_phase = OPENRAC_GAME_PHASE_SCENE;
        g_scene = 4;
        CHECK(!mods::play_skipping());
        CHECK(api.play_skip(reinterpret_cast<openrac_mod*>(&owner)) == OPENRAC_HOOK_OK);
        g_scene = 5;
        CHECK(!mods::play_skipping());
        g_phase = OPENRAC_GAME_PHASE_PLAYING;
        g_scene = -1;
    }
    CHECK(mods::play_notice() == "Waiting for the other players to arrive");
    g_played.clear();
    mods::reset();
    CHECK(!mods::play_skipping());
    CHECK(mods::play_notice().empty());
    CHECK(openrac_game_moby_update == nullptr && openrac_game_moby_hit == nullptr);
    // Reset lets the world stop under menus again, and drops a scene still to start.
    CHECK((g_played == std::vector<std::string>{"scene_start -1", "keep 0"}));
}

// The pages of the game's own menus (interface 1.7): the pages and the main-menu item listed, a
// page's content kept as the mod set it (a choice without choices a line of text), the cursor kept
// off a line, the events reaching the mod, what it asks for kept for the front end only while one
// of its pages is on the screen, refusals after load; reset takes them back.
void game_ui_pages_reach_the_front_end(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "game_ui"}}]})");
    g_records.clear();
    CHECK(mods::load(host(plugins)) == "test-a 1.0.0");
    CHECK(mods::ui_page_count() == 2 && mods::ui_main_item_count() == 1);
    const mods::UiMainItem* item = mods::ui_main_item(0);
    CHECK(
        item != nullptr && item->after == OPENRAC_UI_LOAD_GAME && item->label == "Multiplayer"
        && item->page == 0
    );
    CHECK(mods::ui_main_item(1) == nullptr && mods::ui_page(2) == nullptr);
    // The pause menu's Options items (interface 1.9), in the order added.
    CHECK(mods::ui_pause_item_count() == 2);
    CHECK(
        mods::ui_pause_item(0) != nullptr && mods::ui_pause_item(0)->label == "Multiplayer"
        && mods::ui_pause_item(0)->page == 0
    );
    CHECK(
        mods::ui_pause_item(1) != nullptr && mods::ui_pause_item(1)->label == "Party"
        && mods::ui_pause_item(1)->page == 1
    );
    CHECK(mods::ui_pause_item(2) == nullptr && mods::ui_pause_item(-1) == nullptr);
    const mods::UiPage* home = mods::ui_page(0);
    CHECK(home != nullptr && home->title == "Multiplayer" && home->items.size() == 4);
    CHECK(
        home->items[2].kind == OPENRAC_UI_CHOICE && home->items[2].choices.size() == 3
        && home->items[2].choice == 2
    );
    CHECK(home->items[3].kind == OPENRAC_UI_TEXT);
    CHECK(home->footer.size() == 1 && home->footer[0] == "Connected");
    CHECK(mods::ui_page(1)->flags == OPENRAC_UI_OWN_BACK);
    // The cursor skips the line of text, down then up.
    mods::ui_set_cursor(0, 1);
    CHECK(home->cursor == 2);
    mods::ui_set_cursor(0, 3);
    CHECK(home->cursor == 2);
    // Nor on a disabled item.
    mods::UiItem disabled = home->items[0];
    CHECK(mods::takes_cursor(disabled));
    disabled.flags |= OPENRAC_UI_DISABLED;
    CHECK(!mods::takes_cursor(disabled));
    mods::ui_set_choice(0, 2, 0);
    CHECK(home->items[2].choice == 0);
    // Cross on Find match: with no page on the screen the mod's ui_open is refused, with its page
    // on the screen it is kept for the front end.
    openrac_ui_event e{};
    e.kind = OPENRAC_UI_CHOOSE;
    e.item = 0;
    mods::ui_send(0, e);
    CHECK(std::find(g_records.begin(), g_records.end(), 5900 + 1) != g_records.end());
    CHECK(!mods::take_ui_requests().open);
    // From the title's main menu (no page of the mod's on the screen), interface 1.9: its main-menu
    // item's page opens, not another of its pages; with the main menu gone, neither.
    mods::ui_set_main_menu(true);
    g_records.clear();
    mods::ui_send(0, e);
    CHECK(std::find(g_records.begin(), g_records.end(), 5900 + 1) != g_records.end());
    CHECK(!mods::take_ui_requests().open);
    openrac_ui_event own = e;
    own.item = 1;
    mods::ui_send(0, own);
    CHECK(std::find(g_records.begin(), g_records.end(), 5800) != g_records.end());
    mods::UiRequests main = mods::take_ui_requests();
    CHECK(main.open && main.page == 0);
    mods::ui_set_main_menu(false);
    g_records.clear();
    mods::ui_send(0, own);
    CHECK(std::find(g_records.begin(), g_records.end(), 5800 + 1) != g_records.end());
    CHECK(!mods::take_ui_requests().open);
    mods::ui_set_shown(0, false);
    mods::ui_send(0, e);
    CHECK(std::find(g_records.begin(), g_records.end(), 5000 + 20 + 1) != g_records.end());
    mods::UiRequests r = mods::take_ui_requests();
    CHECK(r.open && r.page == 1);
    // The second page shows: its question and sound are kept; a second question is refused while
    // one is open.
    mods::ui_set_shown(1, false);
    e.kind = OPENRAC_UI_SHOW;
    e.item = -1;
    mods::ui_send(1, e);
    r = mods::take_ui_requests();
    CHECK(r.confirm && r.confirm_page == 1 && r.question == "Leave the party?" && r.tag == 7);
    CHECK(r.sounds.size() == 1 && r.sounds[0] == OPENRAC_UI_SOUND_SELECT);
    mods::ui_set_shown(1, true);
    mods::ui_send(1, e);
    CHECK(!mods::take_ui_requests().confirm);
    e.kind = OPENRAC_UI_ANSWER;
    e.value = 1;
    e.tag = 7;
    mods::ui_send(1, e);
    CHECK(std::find(g_records.begin(), g_records.end(), 6000 + 10 + 7) != g_records.end());
    // After load: a page and an item refused.
    mods::frame();
    CHECK(std::find(g_records.begin(), g_records.end(), 1400 + 3) != g_records.end());
    CHECK(std::find(g_records.begin(), g_records.end(), 1500 + 3) != g_records.end());
    CHECK(std::find(g_records.begin(), g_records.end(), 1600 + 3) != g_records.end());
    mods::ui_send(7, e);  // no such page: nothing
    mods::reset();
    CHECK(mods::ui_page_count() == 0 && mods::ui_main_item_count() == 0);
    CHECK(mods::ui_pause_item_count() == 0);
}

// The tick boundary and the snapshot calls (interface 1.6): the boundary taken during load only,
// its callback run by mods::tick_boundary with the tick, the snapshot calls refused for a game
// without a snapshot (the test's) and outside the boundary; reset takes the callback back.
void the_tick_boundary_reaches_the_mod(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "savestate"}}]})");
    g_records.clear();
    CHECK(mods::load(host(plugins)) == "test-a 1.0.0");
    mods::tick_boundary(7);
    CHECK((g_records == std::vector<int>{3007, 3100, 3201, 3301}));
    g_records.clear();
    mods::frame();
    CHECK(std::find(g_records.begin(), g_records.end(), 1200 + 3) != g_records.end());
    CHECK(std::find(g_records.begin(), g_records.end(), 1300 + 1) != g_records.end());
    mods::reset();
    g_records.clear();
    mods::tick_boundary(8);
    CHECK(g_records.empty());
}

// A mod built for 1.5, before the snapshot calls (OPENRAC_MOD_TARGET_MINOR 5, its manifest's "api"),
// loads on this port of 1.6 and runs as before.
void a_mod_for_1_5_still_loads(const fs::path& plugins) {
    install(plugins, "old", OPENRAC_TEST_MOD_OLD, "1.5");
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-old", "enabled": true, "settings": {"tag": 3}}]})");
    CHECK(mods::load(host(plugins)) == "test-old 1.0.0");
    CHECK(status_of(plugins, "test-old").first == "loaded");
    CHECK(add(0) == 13);  // (0 + 1) * 10 + 3
    mods::reset();
}

void views_reach_the_host(const fs::path& plugins) {
    write(plugins / "plugins.json", R"({"format": 1, "mods": [
        {"id": "test-a", "enabled": true, "settings": {"behaviour": "views"}},
        {"id": "test-b", "enabled": true, "settings": {"behaviour": "views"}}]})");
    mods::Host h = host(plugins);
    g_views_given = 0;
    g_records.clear();
    h.set_views = [](const openrac_view_source* source) {
        g_views = source;
        ++g_views_given;
    };
    h.set_vulkan_setup = [](const openrac_vulkan_setup* setup) {
        g_vulkan = setup;
    };
    CHECK(mods::load(h) == "test-a 1.0.0 (1 not loaded: plugins/status.json)");
    CHECK(status_of(plugins, "test-b").second == "its load returned 6");  // the views were a's
    CHECK(g_views_given == 1 && g_views != nullptr && g_vulkan != nullptr);
    CHECK(
        g_views->size == sizeof(openrac_view_source)
        && g_vulkan->size == sizeof(openrac_vulkan_setup)
    );
    CHECK(g_vulkan->create_instance != nullptr && g_vulkan->physical_device == nullptr);

    // A frame as the frontend draws it: begin, then end once it is submitted, then the closing.
    openrac_view views[OPENRAC_VIEW_MAX] = {};
    CHECK(g_views->begin(g_views->user, views, OPENRAC_VIEW_MAX) == 1);
    CHECK(views[0].width == 64 && views[0].height == 48 && views[0].kind == OPENRAC_VIEW_FULL);
    g_views->end(g_views->user, views, 1);
    g_views->closing(g_views->user);
    CHECK((g_records == std::vector<int>{2001, 4000}));
    mods::reset();
    CHECK(g_views == nullptr && g_vulkan == nullptr);

    // Without "vulkan_device" the mods load after the window, as always; with it, before.
    CHECK(!mods::makes_vulkan_device(plugins / "plugins.json", false));
    install(plugins, "a", OPENRAC_TEST_MOD_A, "1.12", R"( "vulkan_device": true,)");
    CHECK(mods::makes_vulkan_device(plugins / "plugins.json", false));
    CHECK(!mods::makes_vulkan_device(plugins / "plugins.json", true));  // purist: none
    CHECK(!mods::makes_vulkan_device(plugins / "nowhere.json", false));
    install(plugins, "a", OPENRAC_TEST_MOD_A);
}

}  // namespace

// A hook point renamed after a mod was written (docs/port/MODS.md, section 4): the decompilation now
// calls the function Test_Add_Renamed, and the game's binding keeps Test_Add as an alias of its
// permanent id, test.add. The mod still asks for Test_Add, and binds; status.json names the id.
void an_old_name_still_binds(const fs::path& plugins) {
    static const mods::HookPoint renamed[] = {
        {"Test_Add_Renamed", "func_00100000", "i32(i32)", &g_slot_add, reinterpret_cast<void*>(add_game)},
    };
    static const openrac_script_alias aliases[] = {
        {"Test_Add", "test.add"},
        {"Test_Add_Renamed", "test.add"},
        {"func_00100000", "test.add"},
    };
    static const char* const discs[] = {"one"};
    static const openrac_script_binding binding = {"test", 1, discs, 0, nullptr, 0, nullptr, nullptr, 3, aliases};
    CHECK(std::string(openrac_script_hook_id(&binding, "Test_Add")) == "test.add");
    CHECK(std::string(openrac_script_hook_id(&binding, "func_00100000")) == "test.add");
    CHECK(std::string(openrac_script_hook_id(&binding, "test.add")) == "test.add");
    CHECK(std::string(openrac_script_hook_id(&binding, "Unknown")) == "Unknown");
    CHECK(std::string(openrac_script_hook_id(nullptr, "Test_Add")) == "Test_Add");
    write(plugins / "plugins.json", R"({"format": 1, "mods": [{"id": "test-a", "enabled": true, "settings": {"tag": 5}}]})");
    mods::Host h = host(plugins);
    h.hooks = renamed;
    h.scripts = &binding;
    CHECK(mods::load(h) == "test-a 1.0.0");
    CHECK(add(0) == 15);
    CHECK(status_of(plugins, "test-a").first == "loaded");
    const std::string status = read(plugins / "status.json");
    CHECK(status.find("\"hooks\": [\"test.add\"]") != std::string::npos);
    CHECK(status.find("\"hook_points\": [\"test.add\"]") != std::string::npos);
    mods::reset();
}

int main() {
    const fs::path dir = fs::temp_directory_path() / ("openrac_mod_loader_test_" + std::to_string(std::rand()));
    fs::remove_all(dir);
    const fs::path plugins = dir / "plugins";
    install(plugins, "a", OPENRAC_TEST_MOD_A);
    install(plugins, "b", OPENRAC_TEST_MOD_B);
    static const openrac_fn_entry functions[] = {
        {0x00100100u, reinterpret_cast<openrac_host_fn>(test_record), "Test_Record"},
    };
    openrac_guest_register(OPENRAC_OVERLAY_EXE, functions, 1);
    two_mods_chain_in_load_order(plugins);
    a_mod_that_fails_is_taken_back(plugins);
    refused_before_opening(plugins);
    purist_and_no_list(plugins);
    a_library_of_another_id(plugins);
    an_installed_launchers_layout(dir);
    an_old_name_still_binds(plugins);
    input_reaches_the_mod(plugins);
    views_reach_the_host(plugins);
    a_menu_page_draws_through_the_host(plugins);
    a_mod_provides_a_desktop_menu(plugins);
    a_party_reads_the_mods(plugins);
    the_1_12_slots_answer_unknown(plugins);
    the_tick_boundary_reaches_the_mod(plugins);
    game_ui_pages_reach_the_front_end(plugins);
    play_reaches_the_game(plugins, dir);
    the_world_reaches_the_game(plugins, dir);
    a_mod_for_1_5_still_loads(plugins);
    std::error_code e;
    fs::remove_all(dir, e);
    return test::result();
}
