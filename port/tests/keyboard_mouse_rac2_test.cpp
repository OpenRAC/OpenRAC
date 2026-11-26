// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's built-in keyboard and mouse (game/rac2/keyboard_mouse) loaded as the port loads
// it, against a host table of the test's own: where the game is (the on-screen pad's context)
// decides what the keys press; WASD is the left stick, the mouse the right stick in play only, the
// ship swaps fire and the wrench, the title's confirm presses Start too, and the button icons show
// the keys only while the keyboard and mouse are in use.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "game/common/frontend/mouse_turn.h"
#include "game/common/key_prompts.h"
#include "game/common/keyboard_mouse/core.hpp"
#include "game/common/keyboard_mouse/kbm.hpp"
#include "game/rac2/host/follow_mouse.h"
#include "game/rac2/host/gui_texts.h"
#include "game/rac2/host/ui_quads.h"
#include "game/rac2/keyboard_mouse/keyboard_mouse.h"
#include "game/rac2/keyboard_mouse/places.hpp"
#include "key_prompts_fonts.h"
#include "openrac/game_host.h"
#include "openrac/openrac_mod.h"
#include "platform/gyro.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c);                    \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

bool g_keyboard_mouse_in_use = false;

namespace openrac::frontend {
bool keyboard_mouse_in_use() {
    return g_keyboard_mouse_in_use;
}

// The keys the mod hands on for the mods' button labels (interface 1.31), cell by cell.
std::array<std::string, 10> g_prompt_keys;

void set_prompt_keys(const std::string* names, int count) {
    g_prompt_keys = {};
    for (int i = 0; names != nullptr && i < count && i < 10; ++i) {
        g_prompt_keys[static_cast<std::size_t>(i)] = names[i];
    }
}

// The rest of the frontend's side (frontend/active_input.h): the gyro and the device options, kept
// here for the checks.
float g_gyro_yaw = 0.0f;
float g_gyro_pitch = 0.0f;
bool g_gyro_available = false;
openrac::platform::GyroOptions g_gyro_options;

void set_prompt_device(int) {}

void set_mouse_with_gamepad(bool) {}

void set_gyro_options(const openrac::platform::GyroOptions& options) {
    g_gyro_options = options;
}

bool gyro_turn(float& yaw, float& pitch) {
    yaw = g_gyro_available ? g_gyro_yaw : 0.0f;
    pitch = g_gyro_available ? g_gyro_pitch : 0.0f;
    return g_gyro_available;
}

void pending_gyro(float& yaw, float& pitch) {
    yaw = pitch = 0.0f;
}
}  // namespace openrac::frontend

// What the game drew (rac2/host/font.c, ui2d.c, map_markers.c): nothing in these checks.
extern "C" {
// The texts the next take returns (a menu's list when set).
int g_texts = 0;

int openrac_rac2_take_gui_texts(openrac_rac2_gui_text* out) {
    for (int i = 0; i < g_texts; ++i) {
        // The first is the selected one: a colour of its own.
        const std::uint32_t colour = i == 0 ? 0x80F0F0F0u : 0x80808080u;
        out[i] = openrac_rac2_gui_text{
            1u,
            colour,
            200.0f,
            100.0f + 20.0f * static_cast<float>(i),
            300.0f,
            112.0f + 20.0f * static_cast<float>(i),
            -1
        };
    }
    return g_texts;
}

int openrac_rac2_take_prompt_spots(openrac_rac2_prompt_spot*) {
    return 0;
}

int openrac_rac2_take_ui_quads(openrac_rac2_ui_quad*) {
    return 0;
}

void openrac_rac2_ui_quads_tick(void) {}

int openrac_rac2_take_map(float*) {
    return 0;
}

void openrac_rac2_map_move(float, float, float) {}

void openrac_rac2_screen_size(float*, float*) {}

// The follow camera's hook (follow_mouse.h), as the host keeps it.
openrac_rac2_follow_mouse_fn g_follow_mouse = nullptr;

void openrac_rac2_set_follow_mouse(openrac_rac2_follow_mouse_fn fn) {
    g_follow_mouse = fn;
}

openrac_rac2_aligned_mouse_fn g_aligned_mouse = nullptr;

void openrac_rac2_set_aligned_mouse(openrac_rac2_aligned_mouse_fn fn) {
    g_aligned_mouse = fn;
}

int openrac_rac2_follow_axis(float up[3]) {
    up[0] = 0.0f;
    up[1] = 0.0f;
    up[2] = 1.0f;
    return 1;
}

float openrac_rac2_follow_tilt_range(void) {
    return 0.69813168f;
}
}

namespace openrac::frontend {
MouseTurnFn g_mouse_turn = nullptr;

void set_mouse_turn(MouseTurnFn fn) {
    g_mouse_turn = fn;
}
}  // namespace openrac::frontend

// Where the game is, as touch.c would say.
const char* g_context = "foot";

extern "C" int (*openrac_game_touch)(const char**, const char* [OPENRAC_TOUCH_HINTS_MAX]) =
    [](const char** context, const char* [OPENRAC_TOUCH_HINTS_MAX]) {
        *context = g_context;
        return g_context != nullptr ? 0 : -1;
    };

namespace {

constexpr std::uint32_t kMemorySize = 0x00200000u;
std::vector<std::uint8_t> g_memory(kMemorySize);
std::string g_version = "ntsc";
openrac_input g_input{};
openrac_pad_fn g_pad = nullptr;
void* g_pad_user = nullptr;
int g_capture = -1;
std::uint32_t g_taken = 0;
bool g_prompts = true;
const char* g_gyro = "off";            // the "gyro" setting
const char* g_prompt_device = "auto";  // the "prompt_device" setting

void host_log(openrac_mod*, int level, const char* text) {
    std::printf("[%d] keyboard-mouse: %s\n", level, text);
}

void host_fail(openrac_mod*, const char* reason) {
    std::printf("failed: %s\n", reason);
}

bool g_low_latency = true;  // the "mouse_low_latency" setting

// Settings a check sets, as their JSON-less text: the mod reads them as set.
std::map<std::string, std::string> g_written;

const std::string* written(const char* key) {
    const auto it = g_written.find(key);
    return it != g_written.end() ? &it->second : nullptr;
}

int setting_bool(openrac_mod*, const char* key) {
    if (const std::string* w = written(key)) {
        return *w == "true" ? 1 : 0;
    }

    if (std::strcmp(key, "key_prompts") == 0) {
        return g_prompts ? 1 : 0;
    }
    if (std::strcmp(key, "mouse_low_latency") == 0) {
        return g_low_latency ? 1 : 0;
    }

    if (std::strcmp(key, "aim_vignette") == 0) {
        return 1;
    }
    return std::strcmp(key, "capture_cursor") == 0 || std::strcmp(key, "menu_mouse") == 0 ? 1 : 0;
}

std::int64_t setting_int(openrac_mod*, const char*) {
    return 0;
}

double setting_float(openrac_mod*, const char* key) {
    if (const std::string* w = written(key)) {
        return std::stod(*w);
    }

    if (std::strcmp(key, "mouse_sensitivity") == 0
        || std::strcmp(key, "mouse_sensitivity_y") == 0) {
        return 1.0;
    }
    if (std::strcmp(key, "walk_speed") == 0) {
        return 0.5;
    }
    if (std::strcmp(key, "quick_select_dead_zone") == 0) {
        return 30.0;
    }
    return 0.0;  // stick_ramp_ms: at once
}

// The Aim bind (none by default; the Modern preset's right click is the Quick Select's here).
const char* g_aim_bind = "";

const char* setting_string(openrac_mod*, const char* key) {
    if (const std::string* w = written(key)) {
        return w->c_str();
    }

    if (std::strcmp(key, "aim") == 0) {
        return g_aim_bind;
    }

    for (const kbm::ActionInfo& a : kbm::kActions) {
        if (std::strcmp(a.key, key) == 0) {
            return a.fallback;
        }
    }
    if (std::strcmp(key, "gyro") == 0) {
        return g_gyro;
    }
    if (std::strcmp(key, "prompt_device") == 0) {
        return g_prompt_device;
    }

    if (std::strcmp(key, "strafe_fire") == 0) {
        return "off";
    }

    return std::strcmp(key, "camera_tilt") == 0 ? "hold" : "";
}

int host_on_pad(openrac_mod*, openrac_pad_fn fn, void* user) {
    g_pad = fn;
    g_pad_user = user;
    return 0;
}

int host_on_settings(openrac_mod*, openrac_frame_fn, void*) {
    return OPENRAC_HOOK_OK;
}

openrac_gaddr host_relocate(openrac_gaddr a) {
    return a;
}

const openrac_input* host_input() {
    return &g_input;
}

int host_take_input(openrac_mod*, std::uint32_t devices) {
    g_taken |= devices;
    return 0;
}

void host_capture_mouse(openrac_mod*, int capture) {
    g_capture = capture;
}

// Item_Equip as the disc's build would give it (rac2.item.equip): the items it was asked for, and
// the name the mod looked it up by.
std::vector<std::int32_t> g_equipped;
std::string g_equip_name;

std::int32_t fake_equip(std::int32_t item) {
    g_equipped.push_back(item);
    return 1;
}

void* host_function(const char* name) {
    g_equip_name = name;
    return reinterpret_cast<void*>(&fake_equip);
}

// The mod's overlay (openrac_api.on_overlay), drawn by the checks on a picture of their own: how
// many rectangles it drew, and how far the nearest one's middle is from a point the check names.
openrac_overlay_fn g_overlay = nullptr;
void* g_overlay_user = nullptr;
int g_rects = 0;
float g_nearest = 1e9f;
float g_near_x = 0.0f;
float g_near_y = 0.0f;

int host_on_overlay(openrac_mod*, openrac_overlay_fn fn, void* user) {
    g_overlay = fn;
    g_overlay_user = user;
    return OPENRAC_HOOK_OK;
}

int host_play_status(openrac_play_status*) {
    return OPENRAC_HOOK_UNKNOWN;
}

void draw_overlay(float near_x, float near_y) {
    g_rects = 0;
    g_nearest = 1e9f;
    g_near_x = near_x;
    g_near_y = near_y;
    openrac_overlay o{};
    o.size = sizeof o;
    o.rect = [](void*, float x, float y, float w, float h, std::uint32_t) {
        ++g_rects;
        g_nearest =
            std::min(g_nearest, std::hypot(x + w * 0.5f - g_near_x, y + h * 0.5f - g_near_y));
    };
    g_overlay(g_overlay_user, &o);
}

openrac_api make_api() {
    openrac_api api{};
    api.size = sizeof api;
    api.api_major = OPENRAC_MOD_API_MAJOR;
    api.api_minor = OPENRAC_MOD_API_MINOR;
    api.game = "rac2";
    api.version = g_version.c_str();
    api.openrac_version = "0.0.0";
    api.platform = "test";
    api.memory = g_memory.data();
    api.log = host_log;
    api.fail = host_fail;
    api.setting_bool = setting_bool;
    api.setting_int = setting_int;
    api.setting_float = setting_float;
    api.setting_string = setting_string;
    api.on_pad = host_on_pad;
    api.relocate_data = host_relocate;
    api.input = host_input;
    api.take_input = host_take_input;
    api.capture_mouse = host_capture_mouse;
    api.on_settings = host_on_settings;
    api.function = host_function;
    api.on_overlay = host_on_overlay;
    api.play_status = host_play_status;
    return api;
}

void tick() {
    const std::uint32_t t = g_input.tick + 1;
    g_input = openrac_input{};
    g_input.size = sizeof g_input;
    g_input.tick = t;
    g_input.focused = 1;
    g_input.captured = 1;
}

void hold(int key) {
    g_input.keys[key] = OPENRAC_KEY_HELD;
}

struct Report {
    std::uint8_t bytes[18];

    std::uint16_t buttons() const { return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8)); }

    bool held(std::uint16_t button) const { return (buttons() & button) == 0; }
};

Report read_pad() {
    Report r{};
    r.bytes[0] = r.bytes[1] = 0xFF;
    r.bytes[2] = r.bytes[3] = r.bytes[4] = r.bytes[5] = 0x80;
    g_pad(g_pad_user, 0, r.bytes, 18);
    return r;
}

constexpr int kKeyW = 26, kKeySpace = 44, kKeyEnter = 40, kKeyF = 9, kShiftLeft = 225;

// The table lives on: the mod keeps it.
openrac_api g_api{};

void load_ok() {
    g_api = make_api();
    CHECK(openrac::keyboard_mouse::load(&g_api, reinterpret_cast<openrac_mod*>(1)) == 0);
    CHECK(g_taken == (OPENRAC_INPUT_KEYBOARD | OPENRAC_INPUT_MOUSE));
}

void contexts() {
    CHECK(kbm2::context_of("foot") == kbm::Context::Play);
    CHECK(kbm2::context_of("ship") == kbm::Context::Play);
    // The free camera closes only with Select or R3: the map key must press Select there.
    CHECK(kbm2::context_of("look_around") == kbm::Context::Play);
    CHECK(
        kbm::buttons_for(kbm2::context_of("look_around"), kbm::bit(kbm::Action::Map), -1)
        == kbm::kSelect
    );
    CHECK(kbm2::context_of("menu") == kbm::Context::Menu);
    CHECK(kbm2::context_of("front_end") == kbm::Context::Menu);
    CHECK(kbm2::context_of("vendor") == kbm::Context::Vendor);
    CHECK(kbm2::context_of("dialog") == kbm::Context::Dialog);
    CHECK(kbm2::context_of("cutscene") == kbm::Context::Cutscene);
    CHECK(kbm2::context_of(nullptr) == kbm::Context::Cutscene);
    CHECK(kbm2::ship_swap(kbm::kCircle) == kbm::kSquare);
    CHECK(kbm2::ship_swap(kbm::kSquare | kbm::kCross) == (kbm::kCircle | kbm::kCross));
    const auto ship = kbm2::prompt_actions("ship");
    CHECK(ship[1] == kbm::Action::Fire);    // Square
    CHECK(ship[3] == kbm::Action::Wrench);  // Circle
}

void play() {
    g_context = "foot";
    load_ok();
    // W: the left stick forward; Space: Cross; Shift: L2, Going Commando's own strafe.
    tick();
    hold(kKeyW);
    hold(kKeySpace);
    hold(kShiftLeft);
    Report r = read_pad();
    CHECK(r.bytes[5] < 0x40);
    CHECK(r.held(kbm::kCross));
    CHECK(r.held(kbm::kL2));
    CHECK(g_capture == 1);

    // The mouse turns the right stick in play.
    tick();
    g_input.mouse_dx = 20.0f;
    r = read_pad();
    CHECK(r.bytes[2] > 0x80);

    // The left click fires: Circle on foot, Square in the ship.
    tick();
    g_input.mouse_held = OPENRAC_MOUSE_LEFT;
    CHECK(read_pad().held(kbm::kCircle));
    g_context = "ship";
    tick();
    g_input.mouse_held = OPENRAC_MOUSE_LEFT;
    r = read_pad();
    CHECK(r.held(kbm::kSquare) && !r.held(kbm::kCircle));
    tick();
    hold(kKeyF);
    CHECK(read_pad().held(kbm::kCircle));
}

void menus() {
    // In a menu the cursor is free, the mouse moves no stick, Enter is Cross.
    g_context = "menu";
    tick();
    g_input.mouse_dx = 20.0f;
    hold(kKeyEnter);
    Report r = read_pad();
    CHECK(g_capture == 0);
    CHECK(r.bytes[2] == 0x80);
    CHECK(r.held(kbm::kCross) && !r.held(kbm::kStart));

    // The title: confirming presses Start as well.
    g_context = "front_end";
    tick();
    tick();
    hold(kKeyEnter);
    r = read_pad();
    CHECK(r.held(kbm::kCross) && r.held(kbm::kStart));

    // The title's menus (a list shown): Start would go back, so confirming is Cross only.
    g_texts = 3;
    tick();
    tick();
    hold(kKeyEnter);
    r = read_pad();
    CHECK(r.held(kbm::kCross) && !r.held(kbm::kStart));

    // A click beside the list's items confirms nothing; one on the selected item confirms it.
    const auto click = [](float x, float y) {
        const auto point = [&] {
            tick();
            g_input.pointer_x = x / 512.0f;
            g_input.pointer_y = y / 416.0f;
            g_input.pointer_inside = 1;
        };
        point();
        g_input.mouse_held = g_input.mouse_pressed = OPENRAC_MOUSE_LEFT;
        bool cross = read_pad().held(kbm::kCross);
        for (int i = 0; i < 3; ++i) {
            point();
            cross = read_pad().held(kbm::kCross) || cross;
        }
        return cross;
    };
    g_context = "menu";
    tick();
    tick();
    CHECK(!click(450.0f, 380.0f));
    CHECK(click(250.0f, 106.0f));
    g_texts = 0;
    CHECK(
        kbm2::confirm_presses_start("front_end", 2) && !kbm2::confirm_presses_start("front_end", 3)
    );
    CHECK(!kbm2::confirm_presses_start("menu", 0) && !kbm2::confirm_presses_start(nullptr, 0));

    // A dialog: Escape goes back (Triangle), never Cross.
    g_context = "dialog";
    tick();
    tick();
    hold(41);  // Escape
    r = read_pad();
    CHECK(r.held(kbm::kTriangle) && !r.held(kbm::kCross));

    // No context (a load): only skips.
    g_context = nullptr;
    tick();
    tick();
    hold(kKeyW);
    r = read_pad();
    CHECK(!r.held(kbm::kCross));
}

void prompts() {
    namespace kp = openrac::key_prompts;
    g_context = "foot";
    constexpr std::uint32_t kBank = 0x00100000u, kBankWord = 0x001B1604u, kTable = 0x001B9740u;
    std::memcpy(g_memory.data() + kBankWord, &kBank, 4);
    for (std::uint32_t f = 0; f < 3; ++f) {
        const auto texels = static_cast<std::uint16_t>(f * kp::kFontBytes / 16);
        const auto clut = static_cast<std::uint16_t>(3 * kp::kFontBytes / 16);
        std::memcpy(g_memory.data() + kTable + 16 * (f + 1) + 8, &texels, 2);
        std::memcpy(g_memory.data() + kTable + 16 * (f + 1) + 0xA, &clut, 2);
    }
    for (std::uint32_t i = 0; i < 3 * kp::kFontBytes; ++i) {
        g_memory[kBank + i] = static_cast<std::uint8_t>(i % 32);
    }
    for (std::uint32_t f = 0; f < 3; ++f) {
        draw_tabs(g_memory.data() + kBank + f * openrac::key_prompts::kFontBytes);
    }
    for (std::uint32_t i = 1; i < 256; ++i) {
        std::uint8_t* e = g_memory.data() + kBank + 3 * kp::kFontBytes + 4 * i;
        e[0] = e[1] = e[2] = static_cast<std::uint8_t>(i);
        e[3] = 0x80;
    }
    const std::vector<std::uint8_t> game(
        g_memory.begin() + kBank, g_memory.begin() + kBank + 3 * kp::kFontBytes
    );
    const auto unchanged = [&] {
        return std::equal(game.begin(), game.end(), g_memory.begin() + kBank);
    };
    load_ok();
    g_keyboard_mouse_in_use = false;
    tick();
    read_pad();
    CHECK(unchanged());
    g_keyboard_mouse_in_use = true;
    tick();
    read_pad();
    CHECK(!unchanged());
    CHECK(!openrac::frontend::g_prompt_keys[0].empty());  // handed on for the mods' labels
    g_keyboard_mouse_in_use = false;
    tick();
    read_pad();
    CHECK(unchanged());
    CHECK(openrac::frontend::g_prompt_keys[0].empty());

    // Button icons fixed to the gamepad: never the keys.
    g_prompt_device = "gamepad";
    load_ok();
    g_keyboard_mouse_in_use = true;
    tick();
    read_pad();
    CHECK(unchanged());
    // Fixed to the keys: they stay on the gamepad too (a mouse or gyro beside it).
    g_prompt_device = "keyboard";
    load_ok();
    g_keyboard_mouse_in_use = false;
    tick();
    read_pad();
    CHECK(!unchanged());
    g_keyboard_mouse_in_use = false;
    g_prompt_device = "auto";
    load_ok();
}

void other_games() {
    // Not Going Commando, or a disc it has no addresses for: it does not load.
    g_version = "jp";
    g_api = make_api();
    CHECK(openrac::keyboard_mouse::load(&g_api, reinterpret_cast<openrac_mod*>(1)) != 0);
    g_version = "ntsc";
}

}  // namespace

float mem_float(std::uint32_t a) {
    float f = 0.0f;
    std::memcpy(&f, g_memory.data() + a, 4);
    return f;
}

void set_float(std::uint32_t a, float f) {
    std::memcpy(g_memory.data() + a, &f, 4);
}

// The mouse turns the follow camera itself once the camera has been seen: no right stick, the
// eye's offsets turned at once, and the tilt held through the game's target.
void follow_camera() {
    constexpr std::uint32_t kCamera = 0x00150000u, kData = 0x00160000u, kTilt = 0x00170000u;
    constexpr std::uint32_t kTurn = 0x00170010u;
    g_context = "foot";
    load_ok();
    CHECK(g_follow_mouse != nullptr);
    std::memcpy(g_memory.data() + kCamera + 0x70u, &kData, 4);
    for (std::uint32_t off : {0x160u, 0x170u}) {
        set_float(kData + off, -5.0f);
        set_float(kData + off + 4u, 0.0f);
        set_float(kData + off + 8u, 0.0f);
    }
    set_float(kTilt, 0.0f);

    // The first tick the camera is not known yet: the mouse is the right stick.
    tick();
    g_input.mouse_dx = 20.0f;
    CHECK(read_pad().bytes[2] != 0x80);
    g_follow_mouse(kCamera, kTurn, kTilt);

    // Once seen, sideways motion turns both offsets about up, the stick stays at rest.
    tick();
    g_input.mouse_dx = 100.0f;
    CHECK(read_pad().bytes[2] == 0x80);
    g_follow_mouse(kCamera, kTurn, kTilt);
    const float yaw = -100.0f * kbm::kRadiansPerPixel;
    CHECK(std::abs(mem_float(kData + 0x160u) - (-5.0f * std::cos(yaw))) < 1e-4f);
    CHECK(std::abs(mem_float(kData + 0x164u) - (-5.0f * std::sin(yaw))) < 1e-4f);
    CHECK(std::abs(mem_float(kData + 0x174u) - mem_float(kData + 0x164u)) < 1e-6f);
    CHECK(mem_float(kData + 0x168u) == 0.0f);

    // Mouse down raises the eye, and the tilt is held as the game's target.
    tick();
    g_input.mouse_dy = 100.0f;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x168u) > 0.5f && mem_float(kData + 0x178u) > 0.5f);
    const float w = mem_float(kData + 0x1C8u);
    CHECK(w > 0.0f && mem_float(kTilt) > 0.3f);
    tick();
    read_pad();
    set_float(kTilt, 0.0f);
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x1C8u) == w && mem_float(kTilt) > 0.3f);

    // The game locks the turn: the mouse leaves it be.
    std::int32_t lock = 1;
    std::memcpy(g_memory.data() + kData + 0x1D0u, &lock, 4);
    const float x = mem_float(kData + 0x160u);
    tick();
    g_input.mouse_dx = 100.0f;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x160u) == x);
    lock = 0;
    std::memcpy(g_memory.data() + kData + 0x1D0u, &lock, 4);

    // A menu: the camera is not the mouse's.
    g_context = "menu";
    tick();
    g_input.mouse_dx = 100.0f;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x160u) == x);
}

// The mouse turning the first-person view (L1) writes its rows as the game builds them: the side
// up x view, which the hero's aim copies and places his gun by, the up view x side.
void first_person_rows() {
    constexpr std::uint32_t kView = 0x00158000u, kViewState = 0x00159000u;
    const float world_up[3] = {0.0f, 0.0f, 1.0f};
    g_context = "foot";
    load_ok();
    CHECK(g_aligned_mouse != nullptr);
    set_float(kView, 1.0f);
    set_float(kView + 4u, 0.0f);
    set_float(kView + 8u, 0.0f);
    set_float(kViewState + 0x10u, 0.0f);
    tick();
    read_pad();
    g_aligned_mouse(kView, kViewState, world_up, 0, 0);
    tick();
    g_input.mouse_dx = 100.0f;
    read_pad();
    g_aligned_mouse(kView, kViewState, world_up, 0, 0);
    const kbm::Vec3 view{mem_float(kView), mem_float(kView + 4u), mem_float(kView + 8u)};
    const kbm::Vec3 side{
        mem_float(kView + 0x10u), mem_float(kView + 0x14u), mem_float(kView + 0x18u)
    };
    const kbm::Vec3 game_side = kbm::cross(kbm::kWorldUp, view);
    CHECK(std::abs(view.y) > 0.01f);
    CHECK(std::abs(side.x - game_side.x) < 1e-5f && std::abs(side.y - game_side.y) < 1e-5f);
    CHECK(std::abs(mem_float(kView + 0x28u) - 1.0f) < 1e-5f);
}

// The gyro turns the follow camera as the mouse does, with the cursor free (a gamepad player), in
// play only, and for "aiming" only while strafing.
void gyro_follow() {
    constexpr std::uint32_t kCamera = 0x00150000u, kData = 0x00160000u, kTilt = 0x00170000u;
    constexpr std::uint32_t kTurn = 0x00170010u;
    using openrac::frontend::g_gyro_available;
    using openrac::frontend::g_gyro_yaw;
    g_context = "foot";
    g_gyro = "always";
    load_ok();
    CHECK(openrac::frontend::g_gyro_options.enabled);
    std::memcpy(g_memory.data() + kCamera + 0x70u, &kData, 4);
    const auto reset_offsets = [&] {
        for (std::uint32_t off : {0x160u, 0x170u}) {
            set_float(kData + off, -5.0f);
            set_float(kData + off + 4u, 0.0f);
            set_float(kData + off + 8u, 0.0f);
        }
    };
    reset_offsets();
    g_gyro_available = true;
    g_gyro_yaw = 0.0f;
    tick();
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);  // the camera is seen

    // The controller turned 0.1 rad left: the camera too, with the cursor free.
    tick();
    g_input.captured = 0;
    g_gyro_yaw = 0.1f;
    CHECK(read_pad().bytes[2] == 0x80);
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(std::abs(mem_float(kData + 0x164u) - (-5.0f * std::sin(0.1f))) < 1e-4f);

    // In a menu it does nothing.
    reset_offsets();
    g_context = "menu";
    tick();
    g_input.captured = 0;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x164u) == 0.0f);

    // Only while aiming: not while walking, yes while L2 strafes.
    g_context = "foot";
    g_gyro = "aiming";
    load_ok();
    tick();
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    tick();
    g_input.captured = 0;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x164u) == 0.0f);
    tick();
    g_input.captured = 0;
    hold(kShiftLeft);  // the strafe key: L2
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(std::abs(mem_float(kData + 0x164u) - (-5.0f * std::sin(0.1f))) < 1e-4f);

    // Edges, gyro always on: none of these turn the camera.
    g_gyro = "always";
    load_ok();
    tick();
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    const auto no_turn = [&](auto&& setup) {
        reset_offsets();
        tick();
        g_input.captured = 0;
        setup();
        read_pad();
        g_follow_mouse(kCamera, kTurn, kTilt);
        return mem_float(kData + 0x164u) == 0.0f;
    };
    // The Quick Select held open: the gyro does not move the ring or the camera.
    CHECK(no_turn([] {
        g_input.captured = 1;
        g_input.mouse_held = OPENRAC_MOUSE_RIGHT;
    }));
    // The port's overlay open, or the window not focused.
    CHECK(no_turn([] { g_input.overlay = 1; }));
    CHECK(no_turn([] { g_input.focused = 0; }));
    // No gyro read (no controller with one).
    CHECK(no_turn([] { g_gyro_available = false; }));
    g_gyro_available = true;
    // The cursor free: the mouse's motion is not the camera's, the gyro's still is.
    g_gyro_yaw = 0.0f;
    CHECK(no_turn([] { g_input.mouse_dx = 100.0f; }));
    // The pad's right stick out: it wins, as over the mouse.
    reset_offsets();
    tick();
    g_input.captured = 0;
    g_gyro_yaw = 0.1f;
    {
        Report r{};
        r.bytes[0] = r.bytes[1] = 0xFF;
        r.bytes[2] = 0xFF;  // right stick fully right
        r.bytes[3] = r.bytes[4] = r.bytes[5] = 0x80;
        g_pad(g_pad_user, 0, r.bytes, 18);
    }
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(mem_float(kData + 0x164u) == 0.0f);

    // The frames between ticks: with the cursor free they leave the mouse's motion out.
    tick();
    g_input.captured = 0;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    openrac::frontend::MouseTurn turn;
    CHECK(openrac::frontend::g_mouse_turn != nullptr);
    CHECK(openrac::frontend::g_mouse_turn(200.0f, 0.0f, &turn));
    CHECK(turn.yaw == 0.0f);
    // Captured, they show it.
    tick();
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    CHECK(openrac::frontend::g_mouse_turn(200.0f, 0.0f, &turn) && turn.yaw != 0.0f);

    // The gyro button's names: each choice of the manifest is a button, "none" none.
    CHECK(kbm::gyro_button_of("none") == -1 && kbm::gyro_button_of("") == -1);
    for (const char* name :
         {"touchpad",
          "rightstick",
          "leftstick",
          "rightshoulder",
          "leftshoulder",
          "paddle1",
          "paddle2",
          "paddle3",
          "paddle4",
          "misc1"}) {
        CHECK(kbm::gyro_button_of(name) >= 0);
    }

    g_gyro_available = false;
    g_gyro_yaw = 0.0f;
    g_gyro = "off";
}

// Q swings the follow camera round behind Ratchet over its ticks, and the mouse turning sideways
// stops the swing. Aim held on foot presses L1 (the first-person view) and darkens the picture's
// edges, the movement keys leaving the stick alone; let go, the overlay draws nothing.
void camera_reset_and_aim() {
    constexpr std::uint32_t kCamera = 0x00150000u, kData = 0x00160000u, kTilt = 0x00170000u;
    constexpr std::uint32_t kTurn = 0x00170010u;
    constexpr std::uint32_t kHeroYaw = 0x00189E20u + 0x98u;
    constexpr int kKeyQ = 20, kKeyG = 10;
    g_version = "ntsc";
    g_context = "foot";
    g_aim_bind = "KeyG";
    std::fill(g_memory.begin(), g_memory.end(), 0);
    load_ok();
    std::memcpy(g_memory.data() + kCamera + 0x70u, &kData, 4);

    for (std::uint32_t off : {0x160u, 0x170u}) {
        set_float(kData + off, -5.0f);
        set_float(kData + off + 4u, 0.0f);
        set_float(kData + off + 8u, 1.0f);
    }

    // Ratchet faces +y: behind him is -y.
    set_float(kHeroYaw, 1.5707964f);
    const auto run = [&](int ticks, auto&& input) {
        for (int i = 0; i < ticks; ++i) {
            tick();
            input();
            read_pad();
            g_follow_mouse(kCamera, kTurn, kTilt);
        }
    };
    const auto none = [] {
    };
    run(1, none);

    // Q: after the swing the eye is behind him, at the same distance and height.
    run(1, [] { g_input.keys[kKeyQ] = OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED; });
    run(40, none);
    CHECK(std::abs(mem_float(kData + 0x160u)) < 1e-3f);
    CHECK(std::abs(mem_float(kData + 0x164u) - (-5.0f)) < 1e-3f);
    CHECK(std::abs(mem_float(kData + 0x168u) - 1.0f) < 1e-4f);
    CHECK(std::abs(mem_float(kData + 0x174u) - (-5.0f)) < 1e-3f);

    // Ratchet turns round (faces -y): Q again, and the mouse moving sideways stops the swing short.
    set_float(kHeroYaw, -1.5707964f);
    run(1, [] { g_input.keys[kKeyQ] = OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED; });
    run(3, none);
    run(1, [] { g_input.mouse_dx = 1.0f; });
    const float stopped = mem_float(kData + 0x164u);
    run(40, none);
    CHECK(stopped < 0.0f && mem_float(kData + 0x164u) == stopped);

    // Aim held: L1 (the first-person view), the follow camera left as it was, the picture's edges
    // darkened (the crosshair is the game's own).
    const auto aim = [] {
        g_input.keys[kKeyG] = OPENRAC_KEY_HELD;
    };
    const float before = mem_float(kData + 0x164u);
    tick();
    aim();
    Report r = read_pad();
    CHECK(r.held(kbm::kL1) && !r.held(kbm::kL2));
    run(10, aim);
    CHECK(mem_float(kData + 0x164u) == before);
    CHECK(g_overlay != nullptr);
    draw_overlay(256.0f, 224.0f);
    CHECK(g_rects == 4 * 12 && g_nearest > 100.0f);

    // The movement keys leave the stick alone while aiming: the first-person view turns with it.
    tick();
    aim();
    g_input.keys[kKeyW] = OPENRAC_KEY_HELD;
    r = read_pad();
    CHECK(r.bytes[4] == 0x80 && r.bytes[5] == 0x80);

    // Let go: the look fades, then the overlay draws nothing; L1 up.
    run(10, none);
    draw_overlay(256.0f, 224.0f);
    CHECK(g_rects == 0);
    tick();
    CHECK(!read_pad().held(kbm::kL1));

    // Not on foot (swimming): Aim is L1 only, no look on the picture.
    g_context = "swim";
    run(10, aim);
    draw_overlay(256.0f, 224.0f);
    CHECK(g_rects == 0);
    g_context = "foot";
    g_aim_bind = "";
}

// The number keys and the wheel take a Quick Select slot's item through Item_Equip, on each disc at
// its own addresses: on foot only, not with the ring open, and never the item already in hand. Last
// weapon takes the item held before the one in hand.
void weapons() {
    struct Where {
        const char* version;
        std::uint32_t hero;
        std::uint32_t slots;
        const char* equip;
    };

    constexpr std::uint32_t kHeld = 0x1248u;
    constexpr int kDigit1 = 30, kDigit2 = 31, kDigit3 = 32, kKeyX = 27;

    for (const Where& w :
         {Where{"ntsc", 0x00189E20u, 0x001A73B8u, "func_002AD860"},
          Where{"pal", 0x00189EA0u, 0x001A7438u, "func_002AE3C0"},
          Where{"ntsc2", 0x00189E20u, 0x001A73B8u, "func_002AE6C8"}}) {
        g_version = w.version;
        g_context = "foot";
        std::fill(g_memory.begin(), g_memory.end(), 0);
        // Slots 1, 2 and 4 hold items (30, 42, 12); 3 and the rest are empty. 30 is in hand.
        const std::int32_t slots[8] = {30, 42, 0, 12, 0, 0, 0, 0};
        std::memcpy(&g_memory[w.slots], slots, sizeof slots);
        const auto set_held = [&](std::int32_t item) {
            std::memcpy(&g_memory[w.hero + kHeld], &item, sizeof item);
        };
        set_held(30);
        g_written = {{"weapon_last", "KeyX"}};
        load_ok();
        CHECK(g_equip_name == w.equip);
        g_equipped.clear();

        const auto press = [](int key) {
            tick();
            g_input.keys[key] = OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED;
            read_pad();
        };
        const auto wheel = [](float notches) {
            tick();
            g_input.wheel_y = notches;
            read_pad();
        };

        // 2: slot 2's item. 3: an empty slot, nothing. 1: the item in hand, nothing.
        press(kDigit2);
        CHECK(g_equipped == std::vector<std::int32_t>{42});
        press(kDigit3);
        CHECK(g_equipped.size() == 1);
        // The hero dropped the request (he was still switching): 2 again asks again.
        press(kDigit2);
        CHECK(g_equipped == (std::vector<std::int32_t>{42, 42}));
        set_held(42);
        press(kDigit2);
        CHECK(g_equipped.size() == 2);

        // The wheel down: the next slot that holds an item (slot 4's 12); a second notch before he
        // has it out goes on from it, round to slot 1's 30. Up: back the other way.
        set_held(42);
        wheel(-1.0f);
        CHECK(g_equipped.back() == 12);
        wheel(-1.0f);
        CHECK(g_equipped.back() == 30);
        set_held(30);
        wheel(1.0f);
        CHECK(g_equipped.back() == 12);
        const std::size_t asked = g_equipped.size();

        // Not with the Quick Select held open, nor in the ship, swimming or in a menu.
        set_held(12);
        tick();
        g_input.mouse_held = OPENRAC_MOUSE_RIGHT;
        g_input.keys[kDigit1] = OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED;
        read_pad();
        CHECK(g_equipped.size() == asked);

        for (const char* c : {"ship", "swim", "menu"}) {
            g_context = c;
            press(kDigit1);
            wheel(-1.0f);
            CHECK(g_equipped.size() == asked);
        }

        g_context = "foot";
        press(kDigit1);
        CHECK(g_equipped.size() == asked + 1 && g_equipped.back() == 30);

        // 30 out now, 12 before it: X takes 12 back.
        set_held(30);
        tick();
        read_pad();
        press(kKeyX);
        CHECK(g_equipped.size() == asked + 2 && g_equipped.back() == 12);
    }

    g_written.clear();
    g_version = "ntsc";
}

// Strafe while firing: L2 held with Fire on foot and for half a second after (30 ticks at 60 a
// second), or always; not when off, nor while aiming (L1) or swimming.
void strafe_while_firing() {
    constexpr int kKeyG = 10;
    g_context = "foot";
    g_aim_bind = "KeyG";
    const auto with = [](const char* mode) {
        g_written.clear();
        g_written["strafe_fire"] = mode;
        load_ok();
    };
    const auto l2 = [](bool fire, bool aim = false) {
        tick();

        if (fire) {
            g_input.mouse_held = OPENRAC_MOUSE_LEFT;
        }

        if (aim) {
            g_input.keys[kKeyG] = OPENRAC_KEY_HELD;
        }

        return read_pad().held(kbm::kL2);
    };

    with("firing");
    CHECK(!l2(false));
    CHECK(l2(true));
    bool held_after = true;

    for (int i = 0; i < 29; ++i) {
        held_after = held_after && l2(false);
    }

    CHECK(held_after && !l2(false));
    // Aiming (first person) or swimming: no strafe of ours.
    CHECK(!l2(true, true));
    g_context = "swim";
    CHECK(!l2(true));
    g_context = "foot";

    with("always");
    CHECK(l2(false));
    with("off");
    CHECK(!l2(true));

    g_written.clear();
    g_aim_bind = "";
}

// The comforts of a modern shooter: Aim toggles with a press; the mouse turns by aim_sensitivity's
// share while aiming; Dodge holds the strafe and the way the keys go (back when none) and presses
// jump a tick in.
void modern_comforts() {
    constexpr std::uint32_t kCamera = 0x00150000u, kData = 0x00160000u, kTilt = 0x00170000u;
    constexpr std::uint32_t kTurn = 0x00170010u;
    constexpr int kKeyG = 10, kKeyH = 11, kKeyD = 7;
    g_version = "ntsc";
    g_context = "foot";
    g_aim_bind = "KeyG";
    const auto fresh = [&](std::map<std::string, std::string> settings) {
        g_written = std::move(settings);
        std::fill(g_memory.begin(), g_memory.end(), 0);
        load_ok();
    };
    const auto press = [](int key) {
        g_input.keys[key] = OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED;
    };

    // Aim toggles: a press aims (L1) and it stays without the key; another press stops.
    fresh({{"aim_toggle", "true"}});
    tick();
    press(kKeyG);
    CHECK(read_pad().held(kbm::kL1));
    tick();
    CHECK(read_pad().held(kbm::kL1));
    tick();
    press(kKeyG);
    read_pad();
    tick();
    CHECK(!read_pad().held(kbm::kL1));

    // Dodge: right held, the strafe and right a tick, then jump, the flip over in eight ticks; with
    // no key held it goes back.
    fresh({{"dodge", "KeyH"}});
    tick();
    press(kKeyH);
    g_input.keys[kKeyD] = OPENRAC_KEY_HELD;
    Report r = read_pad();
    CHECK(r.held(kbm::kL2) && !r.held(kbm::kCross) && r.bytes[4] > 0xC0);
    tick();
    r = read_pad();
    CHECK(r.held(kbm::kL2) && r.held(kbm::kCross) && r.bytes[4] > 0xC0);

    for (int i = 0; i < 6; ++i) {
        tick();
        r = read_pad();
    }

    CHECK(r.held(kbm::kL2) && !r.held(kbm::kCross));
    tick();
    CHECK(!read_pad().held(kbm::kL2));
    tick();
    press(kKeyH);
    r = read_pad();
    CHECK(r.held(kbm::kL2) && r.bytes[5] > 0xC0);

    // Aiming at half the sensitivity: the camera the mouse turns (here the follow camera) turns by
    // half the mouse's turn, and the vignette is drawn.
    fresh({{"aim_sensitivity", "0.5"}});
    std::memcpy(g_memory.data() + kCamera + 0x70u, &kData, 4);

    for (std::uint32_t off : {0x160u, 0x170u}) {
        set_float(kData + off, -5.0f);
    }

    tick();
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    tick();
    g_input.keys[kKeyG] = OPENRAC_KEY_HELD;
    g_input.mouse_dx = 100.0f;
    read_pad();
    g_follow_mouse(kCamera, kTurn, kTilt);
    const float yaw = -50.0f * kbm::kRadiansPerPixel;
    CHECK(std::abs(mem_float(kData + 0x164u) - (-5.0f * std::sin(yaw))) < 1e-4f);

    for (int i = 0; i < 8; ++i) {
        tick();
        g_input.keys[kKeyG] = OPENRAC_KEY_HELD;
        read_pad();
    }

    draw_overlay(256.0f, 224.0f);
    CHECK(g_rects == 4 * 12);

    g_written.clear();
    g_aim_bind = "";
}

int main() {
    contexts();
    weapons();

    for (const char* v : {"ntsc", "pal"}) {
        g_version = v;
        play();
        menus();
    }
    g_version = "ntsc";
    prompts();
    follow_camera();
    first_person_rows();
    gyro_follow();
    other_games();
    camera_reset_and_aim();
    strafe_while_firing();
    modern_comforts();
    std::puts("keyboard-mouse rac2: all passed");
    return 0;
}
