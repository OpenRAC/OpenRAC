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
#include <string>
#include <vector>

#include "game/common/frontend/mouse_turn.h"
#include "game/common/key_prompts.h"
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

int setting_bool(openrac_mod*, const char* key) {
    if (std::strcmp(key, "key_prompts") == 0) {
        return g_prompts ? 1 : 0;
    }
    return std::strcmp(key, "capture_cursor") == 0 || std::strcmp(key, "menu_mouse") == 0 ? 1 : 0;
}

std::int64_t setting_int(openrac_mod*, const char*) {
    return 0;
}

double setting_float(openrac_mod*, const char* key) {
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

const char* setting_string(openrac_mod*, const char* key) {
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

    g_gyro_available = false;
    g_gyro_yaw = 0.0f;
    g_gyro = "off";
}

int main() {
    contexts();
    for (const char* v : {"ntsc", "pal"}) {
        g_version = v;
        play();
        menus();
    }
    g_version = "ntsc";
    prompts();
    follow_camera();
    gyro_follow();
    other_games();
    std::puts("keyboard-mouse rac2: all passed");
    return 0;
}
