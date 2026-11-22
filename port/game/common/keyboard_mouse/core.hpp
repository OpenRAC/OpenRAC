// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What every game's built-in keyboard and mouse does the same way, joined to the mod interface:
// the settings, the tick's binds (which count where), the menus' directions, the cursor's capture,
// the pad report each tick, the key prompts and the turn the frames between ticks show. A game's
// keyboard_mouse.cpp adds only what is its own: where the game is, its menus under the mouse and
// where its camera keeps what the mouse turns (docs/port/KEYBOARD_MOUSE.md, "Adding a game").
#ifndef KBM_CORE_HPP
#define KBM_CORE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "game/common/frontend/active_input.h"
#include "game/common/frontend/mouse_turn.h"
#include "game/common/key_labels.h"
#include "game/common/key_prompts.h"
#include "game/common/keyboard_mouse/kbm.hpp"
#include "openrac/keys.h"
#include "openrac/mod.hpp"
#include "platform/gyro.h"

namespace kbm {

// ---- Settings ----

// When the gyro aims (platform/gyro.h): never, always in play, or only while aiming (the game's
// own aim: first person, strafing).
enum class GyroWhen {
    Off,
    Always,
    Aiming
};

// The player's settings (openrac-mod.json, kbm_<key> lines of the version's settings file).
struct Settings {
    Bindings binds;
    MoveTuning move;
    LookTuning look;
    float ring_dead_zone = 30.0f;
    bool capture = true;
    bool menu_mouse = true;
    bool prompts = true;
    int prompt_device = -1;  // the icons: -1 the device in use, 0 always the gamepad's, 1 the keys
    bool mouse_with_gamepad = true;
    GyroWhen gyro_when = GyroWhen::Off;
    openrac::platform::GyroOptions gyro;
};

// A setting's text, "" for one this game's settings do not have (its default is then taken).
inline std::string text_setting(openrac::Mod& mod, const char* key) {
    try {
        return mod.setting<std::string>(key);
    } catch (const openrac::Error&) {
        return "";
    }
}

// A number setting, `fallback` for one this game's settings do not have.
inline float float_setting(openrac::Mod& mod, const char* key, float fallback) {
    try {
        return static_cast<float>(mod.setting<double>(key));
    } catch (const openrac::Error&) {
        return fallback;
    }
}

inline bool bool_setting(openrac::Mod& mod, const char* key, bool fallback) {
    try {
        return mod.setting<bool>(key);
    } catch (const openrac::Error&) {
        return fallback;
    }
}

// The gyro button's setting: SDL's name for a gamepad button ("touchpad", "rightstick",
// "paddle1"...), -1 for none.
inline int gyro_button_of(const std::string& name) {
    // SDL_GamepadButton's numbers (SDL_gamepad.h), which stay the same across SDL 3.
    struct Name {
        const char* name;
        int button;
    };

    static constexpr Name kButtons[] = {
        {"leftstick", 7},
        {"rightstick", 8},
        {"leftshoulder", 9},
        {"rightshoulder", 10},
        {"misc1", 15},
        {"paddle1", 16},
        {"paddle2", 17},
        {"paddle3", 18},
        {"paddle4", 19},
        {"touchpad", 20},
    };
    for (const Name& n : kButtons) {
        if (name == n.name) {
            return n.button;
        }
    }
    return -1;
}

// The gyro's settings (the "Gyro" section), its sensitivities as the camera's turn for the
// controller's.
inline void read_gyro(openrac::Mod& mod, Settings& s) {
    const std::string when = text_setting(mod, "gyro");
    s.gyro_when = when == "always"   ? GyroWhen::Always
                  : when == "aiming" ? GyroWhen::Aiming
                                     : GyroWhen::Off;
    openrac::platform::GyroOptions& g = s.gyro;
    g.enabled = s.gyro_when != GyroWhen::Off;
    g.space = openrac::platform::gyro_space_from(text_setting(mod, "gyro_space").c_str());
    g.sensitivity_x = float_setting(mod, "gyro_sensitivity", 2.0f);
    g.sensitivity_y = float_setting(mod, "gyro_sensitivity_y", 2.0f);
    g.invert_x = bool_setting(mod, "gyro_invert_x", false);
    g.invert_y = bool_setting(mod, "gyro_invert_y", false);
    // Degrees a second in the settings, radians here.
    constexpr float kRadiansPerDegree = 0.017453292f;
    g.dead_zone = float_setting(mod, "gyro_dead_zone", 0.0f) * kRadiansPerDegree;
    g.smoothing = float_setting(mod, "gyro_smoothing", 0.0f) * kRadiansPerDegree;
    g.auto_calibrate = bool_setting(mod, "gyro_calibrate", true);
    g.button = gyro_button_of(text_setting(mod, "gyro_button"));
    g.button_action =
        openrac::platform::gyro_button_action_from(text_setting(mod, "gyro_button_action").c_str());
}

// The settings as they are now, warning of any two actions that share a bind. `ticks_per_second`
// turns the stick's ramp into the game's ticks.
inline Settings read_settings(openrac::Mod& mod, float ticks_per_second) {
    Settings s;
    s.binds = read_bindings([&](const char* key) { return text_setting(mod, key); });

    // Warn of each pair of actions that share a bind.
    for (const Conflict& c : conflicts(s.binds)) {
        mod.warn(
            std::string(openrac_bind_to_name(c.bind)) + " is bound to both "
            + kActions[static_cast<std::size_t>(c.a)].key + " and "
            + kActions[static_cast<std::size_t>(c.b)].key + ": it does both"
        );
    }

    s.move.walk = static_cast<float>(mod.setting<double>("walk_speed"));
    s.move.rise_ticks =
        static_cast<float>(mod.setting<double>("stick_ramp_ms")) * ticks_per_second / 1000.0f;
    s.look.sensitivity = static_cast<float>(mod.setting<double>("mouse_sensitivity"));
    s.look.sensitivity_y = static_cast<float>(mod.setting<double>("mouse_sensitivity_y"));
    s.look.invert_y = mod.setting<bool>("invert_y");
    s.look.pitch_holds = mod.setting<std::string>("camera_tilt") != "return";
    s.ring_dead_zone = static_cast<float>(mod.setting<double>("quick_select_dead_zone"));
    s.capture = mod.setting<bool>("capture_cursor");
    s.menu_mouse = mod.setting<bool>("menu_mouse");
    s.prompts = mod.setting<bool>("key_prompts");
    const std::string icons = text_setting(mod, "prompt_device");
    s.prompt_device = icons == "keyboard" ? 1 : icons == "gamepad" ? 0 : -1;
    s.mouse_with_gamepad = bool_setting(mod, "mouse_with_gamepad", true);
    read_gyro(mod, s);

    // What the port itself does with them: the cursor, the mods' button labels, the gyro.
    openrac::frontend::set_mouse_with_gamepad(s.mouse_with_gamepad);
    openrac::frontend::set_prompt_device(s.prompt_device);
    openrac::frontend::set_gyro_options(s.gyro);
    return s;
}

// ---- The tick's binds ----

// A tick's input and where the game is. A game's own tick may add to it.
struct TickInput {
    const openrac_input* in = nullptr;
    Context where = Context::Play;
    bool play = false;     // in play, the cursor captured: the mouse's play binds count
    bool pointer = false;  // out of play, the cursor over the game: the mouse's menu binds count
};

// Where the mouse counts this tick, from the port's input and where the game is: in play with the
// cursor captured, or out of play with the cursor over the game; neither while the port's overlay
// has the input.
inline void where_mouse_counts(TickInput& t) {
    const openrac_input* in = t.in;
    const bool play = t.where == Context::Play;
    t.play = play && in->captured != 0 && in->overlay == 0;
    t.pointer = !play && in->pointer_inside != 0 && in->focused != 0 && in->overlay == 0
                && in->size >= offsetof(openrac_input, reserved2);
}

// Whether bind `b` of action `a` counts now: a mouse bind only where the mouse does; menu actions
// only out of play; nothing while the port's overlay has the input; a wheel only if `wheel_too`.
inline bool bind_counts(const TickInput& t, Action a, int b, bool wheel_too) {
    const bool menu = kActions[static_cast<std::size_t>(a)].group == Group::Menu;
    if (b == 0 || t.in->overlay != 0 || (menu && t.where == Context::Play)) {
        return false;
    }
    if (!wheel_too && b >= OPENRAC_BIND_WHEEL_UP) {
        return false;
    }
    return !is_mouse(b) || (menu ? t.pointer : t.play);
}

// Whether one of an action's binds is held, or went down since the last tick (a tap shorter than a
// tick still presses for one).
inline bool active(const Bindings& binds, const TickInput& t, Action a) {
    for (int b : binds.of(a)) {
        if (bind_counts(t, a, b, true)
            && (openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b))) {
            return true;
        }
    }
    return false;
}

// Whether one of an action's binds went down this tick (not held from before): the presses that
// act once, such as taking an item or clicking.
inline bool pressed_now(const Bindings& binds, const TickInput& t, Action a) {
    for (int b : binds.of(a)) {
        if (bind_counts(t, a, b, true) && openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }
    return false;
}

// Whether one of an action's binds other than a wheel's is down: a key held makes a menu direction
// repeat, a wheel notch is only one step.
inline bool held_not_wheel(const Bindings& binds, const TickInput& t, Action a) {
    for (int b : binds.of(a)) {
        if (bind_counts(t, a, b, false)
            && (openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b))) {
            return true;
        }
    }
    return false;
}

// Whether one of an action's wheel binds turned this tick (a notch is one press).
inline bool wheel_notch(const Bindings& binds, const TickInput& t, Action a) {
    for (int b : binds.of(a)) {
        if (b >= OPENRAC_BIND_WHEEL_UP && bind_counts(t, a, b, true)
            && openrac_bind_pressed(t.in, b)) {
            return true;
        }
    }
    return false;
}

// Whether one of an action's mouse binds is down (`now`: went down this tick), where the mouse
// counts: a click, not the same action from a key.
inline bool mouse_bind(const Bindings& binds, const TickInput& t, Action a, bool now) {
    for (int b : binds.of(a)) {
        if (b != 0 && is_mouse(b) && bind_counts(t, a, b, false)
            && (now ? openrac_bind_pressed(t.in, b)
                    : openrac_bind_held(t.in, b) || openrac_bind_pressed(t.in, b))) {
            return true;
        }
    }
    return false;
}

// The actions held this tick, before the latch holds back those held since the game changed where
// it is.
inline ActionSet actions_held(const Bindings& binds, const TickInput& t) {
    ActionSet held = 0;
    for (int a = 0; a < kActionCount; ++a) {
        if (active(binds, t, static_cast<Action>(a))) {
            held |= bit(static_cast<Action>(a));
        }
    }
    return held;
}

// Out of play: the menu directions as the d-pad, a key repeating while held, a wheel notch one
// queued press (unless `wheel_steps` is false: the wheel does something else there, such as zoom
// a map).
struct MenuDirections {
    std::array<Repeat, 4> repeat{};

    void reset() { repeat = {}; }

    void step(
        const Bindings& binds,
        const TickInput& t,
        ActionSet held,
        bool wheel_steps,
        Pulser& pulses,
        Virtual& v
    ) {
        constexpr std::array<Action, 4> dirs =
            {Action::MenuUp, Action::MenuDown, Action::MenuLeft, Action::MenuRight};
        constexpr std::array<std::uint16_t, 4> pads = {kUp, kDown, kLeft, kRight};
        for (std::size_t i = 0; i < dirs.size(); ++i) {
            const bool on = (held & bit(dirs[i])) != 0 && held_not_wheel(binds, t, dirs[i]);
            if ((held & bit(dirs[i])) != 0 && !on && wheel_steps
                && wheel_notch(binds, t, dirs[i])) {
                pulses.push(pads[i]);
            } else if (repeat[i].step(on)) {
                v.held = static_cast<std::uint16_t>(v.held | pads[i]);
            }
        }
        v.held = static_cast<std::uint16_t>(v.held | pulses.step());
    }
};

// ---- The left stick ----

// The left stick into v: while the Quick Select is held open (`ring`), Triangle held and the mouse
// pointing the stick at a slot (its motion counts only with the cursor captured in play); else the
// movement keys, everywhere (in a menu the game reads the stick as the d-pad).
inline void left_stick(
    const TickInput& t,
    ActionSet held,
    bool ring,
    const Settings& s,
    RingPointer& pointer,
    KeyStick& keys,
    Virtual& v
) {
    if (ring) {
        v.held = static_cast<std::uint16_t>(v.held | kTriangle);
        if (t.play) {
            pointer.add(t.in->mouse_dx, t.in->mouse_dy, s.ring_dead_zone);
        }
        const auto [x, y] = pointer.stick(s.ring_dead_zone);
        v.left_x = x;
        v.left_y = y;
        keys = KeyStick{};
        return;
    }
    const auto on = [&](Action a) {
        return (held & bit(a)) != 0;
    };
    keys.step(
        on(Action::MoveForward),
        on(Action::MoveBack),
        on(Action::MoveLeft),
        on(Action::MoveRight),
        on(Action::Walk),
        s.move
    );
    v.left_x = keys.x;
    v.left_y = keys.y;
}

// ---- The gyro ----

// The gyro's turn this tick as mouse motion (gyro_pixels): in play only, with the window focused
// and the port's overlay shut, and for Aiming only while `aiming`. `on` says the gyro aims now.
struct GyroLook {
    float dx = 0.0f;
    float dy = 0.0f;
    bool on = false;
};

inline GyroLook gyro_look(const TickInput& t, const Settings& s, bool aiming) {
    GyroLook g;
    if (s.gyro_when == GyroWhen::Off || t.where != Context::Play || t.in->overlay != 0
        || t.in->focused == 0 || (s.gyro_when == GyroWhen::Aiming && !aiming)) {
        return g;
    }
    float yaw = 0.0f;
    float pitch = 0.0f;
    if (!openrac::frontend::gyro_turn(yaw, pitch)) {
        return g;
    }
    g.on = true;
    gyro_pixels(yaw, pitch, s.look, g.dx, g.dy);
    return g;
}

// The frames between ticks: the turn the next tick will make. The mouse's motion only where the
// tick reads it (`mouse_counts`: the cursor captured in play), then the gyro's while it aims.
inline void add_pending_gyro(bool on, bool mouse_counts, const Settings& s, float& dx, float& dy) {
    if (!mouse_counts) {
        dx = dy = 0.0f;
    }
    if (!on) {
        return;
    }
    float yaw = 0.0f;
    float pitch = 0.0f;
    openrac::frontend::pending_gyro(yaw, pitch);
    float gx = 0.0f;
    float gy = 0.0f;
    gyro_pixels(yaw, pitch, s.look, gx, gy);
    dx += gx;
    dy += gy;
}

// ---- The cursor ----

// The cursor captured in play, free everywhere else, asked of the port only when that changes. The
// port frees it too while the window is not focused or its overlay is open.
struct Capture {
    int asked = -1;  // 1 captured, 0 free, -1 before the first ask

    void reset() { asked = -1; }

    void update(openrac::Mod& mod, bool wanted, bool play, const openrac_input* in) {
        const int want = wanted && play && in->focused != 0 && in->overlay == 0 ? 1 : 0;
        if (want != asked) {
            mod.capture_mouse(want != 0);
            asked = want;
        }
    }
};

// ---- The pad report ----

// A report the game reads holds the buttons and both sticks.
inline constexpr int kMinReportSize = 6;
// How far a gamepad's right stick byte may be from its centre before the camera is the pad's: the
// games' dead zone of 0.3 of the stick's travel (seen in game code).
inline constexpr int kPadLookDeadBytes = 38;

// What the gamepad holds in a report, before the keyboard and mouse join it.
struct PadState {
    std::uint16_t buttons = 0;  // Button bits held
    bool look = false;          // its right stick past the dead zone: the camera is the pad's
};

// The first pad's report, before the game reads it: `step(in, pad)` once a tick, when the port has
// read the input anew (however often the game reads the pad), then the tick's pad from the keys
// and the mouse merged on top of the gamepad's. `tick` is the caller's last stepped tick.
template <class Step>
void on_pad(
    openrac::Mod& mod,
    int port,
    std::uint8_t* report,
    int size,
    std::uint32_t& tick,
    const Virtual& pad,
    Step step
) {
    if (port != 0 || size < kMinReportSize) {
        return;
    }
    const openrac_input* in = mod.api().input();
    if (in->tick != tick) {
        tick = in->tick;
        PadState state;
        state.buttons = static_cast<std::uint16_t>(~(report[0] | (report[1] << 8)));
        state.look = std::abs(report[2] - kStickRestByte) > kPadLookDeadBytes
                     || std::abs(report[3] - kStickRestByte) > kPadLookDeadBytes;
        step(in, state);
    }
    merge(pad, report, size);
}

// ---- The button icons ----

// The game's button icons show the keys that press them while the player is on the keyboard and
// mouse (key_prompts.h), and the game's own again on the gamepad; `actions` is the action of each
// icon cell where the game is now. Only the fonts' pictures change, never logic. The game's fonts
// are at `font_bank` (the effect bank's word) and `font_table` (the effect texture table), already
// relocated.
inline void paint_prompts(
    openrac::Mod& mod,
    openrac::key_prompts::Painter& painter,
    std::uint32_t font_bank,
    std::uint32_t font_table,
    const Settings& settings,
    const std::array<Action, 8>& actions
) {
    namespace kp = openrac::key_prompts;
    constexpr std::uint32_t kRamBytes = 32u << 20;
    kp::Fonts fonts;
    if (!kp::find_fonts(mod.memory().base(), kRamBytes, font_bank, font_table, fonts)) {
        painter.forget();
        openrac::frontend::set_prompt_keys(nullptr, 0);
        return;
    }
    kp::Labels labels{};
    // The keys while the player is on the keyboard and mouse, or always or never as the player
    // fixed them ("Button icons": a mouse or gyro beside the pad then does not flip them).
    const bool keys = settings.prompt_device < 0 ? openrac::frontend::keyboard_mouse_in_use()
                                                 : settings.prompt_device == 1;
    if (settings.prompts && keys) {
        for (std::size_t i = 0; i < actions.size(); ++i) {
            const int bind = actions[i] != Action::Count ? settings.binds.of(actions[i])[0] : 0;
            if (bind != 0) {
                labels[i] = openrac_bind_to_name(bind);
            }
        }
        // The sticks: the movement keys and the mouse.
        labels[kp::LeftStick] = move_label(settings.binds);
        labels[kp::RightStick] = "Mouse";
    }
    // The same keys for the mods' button labels (interface 1.31).
    openrac::frontend::set_prompt_keys(labels.data(), static_cast<int>(labels.size()));
    painter.update(fonts, labels);
}

// ---- The frames between ticks ----

// The turn the next tick makes for mouse motion (dx, dy) it has not read yet, while the mouse
// turns a camera itself (frontend/mouse_turn.h): about `up`, the turn and the tilt only where the
// game lets the mouse make them, the view's elevation within `pitch_limit`.
inline void direct_turn(
    float dx,
    float dy,
    const LookTuning& t,
    Vec3 up,
    OrbitLocks locks,
    float pitch_limit,
    bool first_person,
    openrac::frontend::MouseTurn* out
) {
    *out = {};
    out->first_person = first_person;
    if (!locks.turn) {
        out->yaw = DirectLook::yaw(dx, dy, t);
    }
    if (!locks.tilt) {
        out->pitch = DirectLook::pitch(dx, dy, t);
    }
    out->up[0] = up.x;
    out->up[1] = up.y;
    out->up[2] = up.z;
    out->pitch_limit = pitch_limit;
}

}  // namespace kbm

#endif  // KBM_CORE_HPP
