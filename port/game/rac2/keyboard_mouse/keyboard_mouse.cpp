// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Keyboard and mouse for Going Commando, built into the port (keyboard_mouse.h). The same actions,
// binds and settings as Ratchet & Clank's (game/rac1/keyboard_mouse, its manifest), on the pad the
// game reads: the keys press its buttons, WASD is the left stick, the mouse is the right stick in
// play (the camera turns as a stick turns it), holding the Quick Select's bind opens it and the
// mouse picks its slot. Where the game is comes from the on-screen pad's context (touch.c). It
// writes only the pad report and the fonts' button icons (key_prompts.h), never game state.

#include "keyboard_mouse.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "game/common/frontend/active_input.h"
#include "game/common/key_labels.h"
#include "game/common/key_prompts.h"
#include "openrac/game_host.h"
#include "openrac/mod.hpp"
#include "places.hpp"

namespace {

using kbm::Action;
using kbm::Context;

// The game's fonts on each disc (key_prompts.h): the effect bank's word and the effect texture
// table.
struct Disc {
    const char* version;
    std::uint32_t font_bank;
    std::uint32_t font_table;
};

constexpr std::array<Disc, 3> kDiscs = {{
    {"ntsc", 0x001B1604u, 0x001B9740u},
    {"pal", 0x001B1604u, 0x001B9740u},
    {"ntsc2", 0x001B1584u, 0x001B96C0u},
}};

constexpr std::uint32_t kRamBytes = 32u << 20;
constexpr std::uint32_t kNoTick = 0xFFFFFFFFu;
// A report the game reads holds the buttons and both sticks.
constexpr int kMinReportSize = 6;
// The pad's right stick past the game's dead zone (0.3 of its travel): the camera is the pad's.
constexpr int kPadLookDeadBytes = 38;

openrac::Mod* s_mod = nullptr;
const Disc* s_disc = nullptr;

// The settings, read at load and again when they change.
kbm::Bindings s_binds;
kbm::MoveTuning s_move;
kbm::LookTuning s_look_tuning;
float s_ring_dead_zone = 30.0f;
bool s_capture = true;
bool s_prompts = true;

// What the keys and the mouse hold now.
kbm::Virtual s_pad;
kbm::KeyStick s_keys;
kbm::MouseLook s_look;
kbm::RingPointer s_ring;
kbm::Latch s_latch;
kbm::Pulser s_pulses;
std::array<kbm::Repeat, 4> s_repeat;
bool s_ring_open = false;
std::uint32_t s_tick = kNoTick;
int s_asked_capture = -1;
openrac::key_prompts::Painter s_painter;

// A tick's input and where the game is.
struct Tick {
    const openrac_input* in = nullptr;
    const char* touch = nullptr;
    Context where = Context::Play;
    bool play = false;     // in play, the cursor captured: the mouse's play binds count
    bool pointer = false;  // out of play, the cursor over the game: the mouse's menu binds count
};

// Whether a bind of `a` counts now: a mouse bind only where the mouse does; menu actions only out
// of play; nothing while the port's overlay has the input.
bool bind_counts(const Tick& t, Action a, int b, bool wheel_too) {
    const bool menu = kbm::kActions[static_cast<std::size_t>(a)].group == kbm::Group::Menu;
    if (b == 0 || t.in->overlay != 0 || (menu && t.where == Context::Play)) {
        return false;
    }
    if (!wheel_too && b >= OPENRAC_BIND_WHEEL_UP) {
        return false;
    }
    return !kbm::is_mouse(b) || (menu ? t.pointer : t.play);
}

bool active(const Tick& t, Action a) {
    for (int b : s_binds.of(a)) {
        if (bind_counts(t, a, b, true)
            && (openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b))) {
            return true;
        }
    }
    return false;
}

bool held_not_wheel(const Tick& t, Action a) {
    for (int b : s_binds.of(a)) {
        if (bind_counts(t, a, b, false)
            && (openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b))) {
            return true;
        }
    }
    return false;
}

bool wheel_notch(const Tick& t, Action a) {
    for (int b : s_binds.of(a)) {
        if (b >= OPENRAC_BIND_WHEEL_UP && bind_counts(t, a, b, true)
            && openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }
    return false;
}

kbm::ActionSet actions_held(const Tick& t) {
    kbm::ActionSet held = 0;
    for (int a = 0; a < kbm::kActionCount; ++a) {
        if (active(t, static_cast<Action>(a))) {
            held |= kbm::bit(static_cast<Action>(a));
        }
    }
    return held;
}

// Out of play: the menu directions as the d-pad, a key repeating while held, a wheel notch one
// press.
void menu_directions(const Tick& t, kbm::ActionSet held, kbm::Virtual& v) {
    constexpr std::array<Action, 4> dirs =
        {Action::MenuUp, Action::MenuDown, Action::MenuLeft, Action::MenuRight};
    constexpr std::array<std::uint16_t, 4> pads = {kbm::kUp, kbm::kDown, kbm::kLeft, kbm::kRight};
    for (std::size_t i = 0; i < dirs.size(); ++i) {
        const bool on = (held & kbm::bit(dirs[i])) != 0 && held_not_wheel(t, dirs[i]);
        if (!on && wheel_notch(t, dirs[i])) {
            s_pulses.push(pads[i]);
        } else if (s_repeat[i].step(on)) {
            v.held = static_cast<std::uint16_t>(v.held | pads[i]);
        }
    }
    v.held = static_cast<std::uint16_t>(v.held | s_pulses.step());
}

// The cursor captured in play, free everywhere else, asked of the port when that changes.
void update_capture(bool play, const openrac_input* in) {
    const int want = s_capture && play && in->focused != 0 && in->overlay == 0 ? 1 : 0;
    if (want != s_asked_capture) {
        s_mod->capture_mouse(want != 0);
        s_asked_capture = want;
    }
}

// The button icons show the keys while the keyboard and mouse are in use (key_prompts.h).
void update_prompts(const Tick& t) {
    namespace kp = openrac::key_prompts;
    kp::Fonts fonts;
    if (!kp::find_fonts(
            s_mod->memory().base(),
            kRamBytes,
            s_mod->relocate(s_disc->font_bank),
            s_mod->relocate(s_disc->font_table),
            fonts
        )) {
        s_painter.forget();
        return;
    }
    kp::Labels labels{};
    if (s_prompts && openrac::frontend::keyboard_mouse_in_use()) {
        const auto actions = kbm2::prompt_actions(t.touch);
        for (std::size_t i = 0; i < actions.size(); ++i) {
            const int bind = actions[i] != Action::Count ? s_binds.of(actions[i])[0] : 0;
            if (bind != 0) {
                labels[i] = openrac::keys::icon_label(openrac_bind_to_name(bind));
            }
        }
    }
    s_painter.update(fonts, labels);
}

// The tick's pad from the keyboard and mouse.
void step(const openrac_input* in, bool pad_look) {
    Tick t;
    t.in = in;
    const char* hints[OPENRAC_TOUCH_HINTS_MAX] = {};
    if (openrac_game_touch == nullptr || openrac_game_touch(&t.touch, hints) < 0) {
        t.touch = nullptr;
    }
    t.where = kbm2::context_of(t.touch);
    const bool play = t.where == Context::Play;
    t.play = play && in->captured != 0 && in->overlay == 0;
    t.pointer = !play && in->pointer_inside != 0 && in->focused != 0 && in->overlay == 0
                && in->size >= offsetof(openrac_input, reserved2);
    update_capture(play, in);

    const Context before = s_latch.last;
    const kbm::ActionSet held = s_latch.filter(t.where, actions_held(t));
    if (before != t.where) {
        s_pulses.clear();
    }

    kbm::Virtual v;
    // A dialog: Escape goes back (Triangle), never Cross, which could answer yes.
    const bool dialog_escape = t.where == Context::Dialog && (held & kbm::bit(Action::Pause)) != 0;
    v.held = kbm::buttons_for(t.where, dialog_escape ? held & ~kbm::bit(Action::Pause) : held, -1);
    if (dialog_escape) {
        v.held = static_cast<std::uint16_t>(v.held | kbm::kTriangle);
    }
    if (kbm2::is_ship(t.touch)) {
        v.held = kbm2::ship_swap(v.held);
    }
    // The title's "Press Start": confirming presses Start as well.
    if (t.touch != nullptr && std::string_view(t.touch) == "front_end"
        && (held & (kbm::bit(Action::MenuConfirm) | kbm::bit(Action::Jump))) != 0) {
        v.held = static_cast<std::uint16_t>(v.held | kbm::kStart);
    }
    if (!play) {
        menu_directions(t, held, v);
    } else {
        s_pulses.clear();
    }

    // The Quick Select: Triangle held while its bind is, the mouse pointing the left stick.
    const bool ring = play && (held & kbm::bit(Action::QuickSelect)) != 0;
    if (ring && !s_ring_open) {
        s_ring.reset();
    }
    s_ring_open = ring;
    if (ring) {
        v.held = static_cast<std::uint16_t>(v.held | kbm::kTriangle);
        if (t.play) {
            s_ring.add(in->mouse_dx, in->mouse_dy, s_ring_dead_zone);
        }
        const auto [x, y] = s_ring.stick(s_ring_dead_zone);
        v.left_x = x;
        v.left_y = y;
        s_keys = kbm::KeyStick{};
    } else {
        const auto on = [&](Action a) {
            return (held & kbm::bit(a)) != 0;
        };
        s_keys.step(
            on(Action::MoveForward),
            on(Action::MoveBack),
            on(Action::MoveLeft),
            on(Action::MoveRight),
            on(Action::Walk),
            s_move
        );
        v.left_x = s_keys.x;
        v.left_y = s_keys.y;
    }

    // In play with the cursor captured, the mouse is the right stick; the pad's own stick wins.
    if (t.play && !ring && !pad_look) {
        s_look.step(in->mouse_dx, in->mouse_dy, false, s_look_tuning);
        v.right_x = s_look.x;
        v.right_y = s_look.y;
    } else if (!play) {
        s_look.reset();
    }

    s_pad = v;
    update_prompts(t);
}

void on_pad(int port, std::uint8_t* report, int size) {
    if (port != 0 || size < kMinReportSize) {
        return;
    }
    const openrac_input* in = s_mod->api().input();
    if (in->tick != s_tick) {
        s_tick = in->tick;
        const bool pad_look = std::abs(report[2] - kbm::kStickRestByte) > kPadLookDeadBytes
                              || std::abs(report[3] - kbm::kStickRestByte) > kPadLookDeadBytes;
        step(in, pad_look);
    }
    kbm::merge(s_pad, report, size);
}

// A setting's text, "" for one this game's settings do not have (its default is then taken).
std::string text_setting(openrac::Mod& mod, const char* key) {
    try {
        return mod.setting<std::string>(key);
    } catch (const openrac::Error&) {
        return "";
    }
}

// The game's ticks a second: the PAL disc's 50, the others' 60.
float ticks_per_second(const openrac::Mod& mod) {
    return mod.version() == "pal" ? 50.0f : 60.0f;
}

void read_settings(openrac::Mod& mod) {
    s_binds = kbm::read_bindings([&](const char* key) { return text_setting(mod, key); });
    s_move.walk = static_cast<float>(mod.setting<double>("walk_speed"));
    s_move.rise_ticks =
        static_cast<float>(mod.setting<double>("stick_ramp_ms")) * ticks_per_second(mod) / 1000.0f;
    s_look_tuning.sensitivity = static_cast<float>(mod.setting<double>("mouse_sensitivity"));
    s_look_tuning.y_ratio = static_cast<float>(mod.setting<double>("mouse_y_ratio"));
    s_look_tuning.acceleration = static_cast<float>(mod.setting<double>("mouse_acceleration"));
    s_look_tuning.invert_y = mod.setting<bool>("invert_y");
    s_look_tuning.pitch_holds = mod.setting<std::string>("camera_tilt") != "return";
    s_ring_dead_zone = static_cast<float>(mod.setting<double>("quick_select_dead_zone"));
    s_capture = mod.setting<bool>("capture_cursor");
    s_prompts = mod.setting<bool>("key_prompts");
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") {
        throw openrac::Error("this is Going Commando's keyboard and mouse");
    }
    if (!mod.has_input()) {
        throw openrac::Error("needs OpenRAC with mod interface 1.3 (the keyboard and mouse)");
    }
    s_mod = &mod;
    s_disc = nullptr;
    for (const Disc& d : kDiscs) {
        if (mod.version() == d.version) {
            s_disc = &d;
        }
    }
    if (s_disc == nullptr) {
        throw openrac::Error("no addresses for the " + std::string(mod.version()) + " disc");
    }
    read_settings(mod);
    s_pad = kbm::Virtual{};
    s_keys = kbm::KeyStick{};
    s_look.reset();
    s_ring.reset();
    s_latch = kbm::Latch{};
    s_pulses.clear();
    s_repeat = {};
    s_ring_open = false;
    s_tick = kNoTick;
    s_asked_capture = -1;
    s_painter.forget();

    mod.take_input(OPENRAC_INPUT_KEYBOARD | OPENRAC_INPUT_MOUSE);
    mod.on_settings([] { read_settings(*s_mod); });
    mod.on_pad([](int port, std::uint8_t* report, int size) { on_pad(port, report, size); });
    mod.info("keyboard and mouse on the " + std::string(mod.version()) + " disc");
}

}  // namespace

namespace openrac::keyboard_mouse {

void query(openrac_mod_info* info) {
    info->api_major = OPENRAC_MOD_API_MAJOR;
    info->api_minor = OPENRAC_MOD_API_MINOR;
    info->id = "keyboard-mouse";
    info->version = "2.1.0";
}

int load(const openrac_api* api, openrac_mod* mod) {
    return openrac::detail::load(api, mod, ::load);
}

}  // namespace openrac::keyboard_mouse
