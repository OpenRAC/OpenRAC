// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The player's input side of the frontend, a slice of frontend/state.h: the on-screen touch pad
// (draw_touch_pad), the cursor the mods capture (apply_mouse_capture, capture_mouse), the keyboard
// and mouse as the mods read them (keyboard_mouse, take_keyboard), and the pad the game reads each
// frame (pad), with the scripted presses of OPENRAC_PRESS and Start's skip of a scene
// (skip_after_mods). The events themselves reach platform::Input in frontend.cpp's frame loop.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>

#include "button_labels.h"
#include "common/env.h"
#include "common/log.h"
#include "debug_menu.h"
#include "debug_overlay.h"
#include "frontend.h"
#include "game/common/frontend/active_input.h"
#include "game/common/frontend/camera_latch.h"
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/input_latency.h"
#include "game/common/frontend/state.h"
#include "game/common/frontend/touch_controls.h"
#include "mod_menu.h"
#include "mod_ui.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "renderer/ui/touch_overlay.h"
#include "settings.h"

namespace openrac::frontend {

// The on-screen pad, kept inside the screen's safe area (a camera's notch at a phone's side).
void draw_touch_pad(int window_width, int window_height) {
    platform::TouchPad& touch = g->input.touch_pad();
    SDL_Rect safe{};
    int w = 0;
    int h = 0;
    if (SDL_GetWindowSafeArea(g->window->sdl(), &safe)
        && SDL_GetWindowSize(g->window->sdl(), &w, &h) && h > 0) {
        const float height = static_cast<float>(h);
        touch.set_safe_area(
            static_cast<float>(safe.x) / height, static_cast<float>(w - safe.x - safe.w) / height
        );
    }
    // Mobile options are in a drawer (debug_overlay.h), opened with the pad's own button.
    touch.set_menu_button(debug_overlay::drawer_available());
    if (touch.take_menu_press()) {
        debug_overlay::toggle_drawer();
    }
    // The player's size, camera speed and layout (the drawer's Touch controls). The layout is the
    // pad's own while it is edited, and written to the settings when that ends.
    settings::Settings& set = settings::get();
    static std::string applied_layout;
    static bool was_editing = false;
    // What the game is doing picks the buttons (the Arc style), unless the player keeps the
    // DualShock's (touch_controls.h).
    touch_controls::update(touch, set.touch_style);
    touch.set_scale(static_cast<float>(set.touch_scale) / 100.0f);
    touch.set_look_speed(static_cast<float>(set.touch_camera) / 100.0f);
    const bool editing = debug_overlay::touch_editing();
    if (debug_overlay::take_touch_reset()) {
        touch.reset_layout();
        applied_layout.clear();
    }
    if (!editing && !was_editing && set.touch_layout != applied_layout) {
        touch.set_layout(set.touch_layout);
        applied_layout = set.touch_layout;
    }
    touch.set_editing(editing);
    debug_overlay::set_touch_guides(touch.guide_x(), touch.guide_y());
    touch.step_selected_size(debug_overlay::take_touch_size_steps());
    debug_overlay::set_touch_selected(
        touch.selected() != platform::TouchGroup::Count ? touch.selected_size() : -1.0f
    );
    if (was_editing && !editing) {
        set.touch_layout = touch.layout();
        applied_layout = set.touch_layout;
        settings::save();
    }
    was_editing = editing;
    if (touch.visible()) {
        run_graphics([&] {
            renderer::ui::touch_overlay::draw(touch, window_width, window_height);
        });
    } else if (window_width > 0 && window_height > 0) {
        touch.set_aspect(static_cast<float>(window_width) / static_cast<float>(window_height));
    }
}

// The player's device for the labels of the pad's buttons (button_labels.h, interface 1.31): the
// keyboard and mouse, or the make of the first controller connected (none: the game's own icons).
namespace {
int g_prompt_device = -1;
bool g_mouse_with_gamepad = true;
}  // namespace

void set_prompt_device(int device) {
    g_prompt_device = device;
}

void set_mouse_with_gamepad(bool on) {
    g_mouse_with_gamepad = on;
}

void set_gyro_options(const platform::GyroOptions& options) {
    if (g) {
        g->input.set_gyro_options(options);
    }
}

bool gyro_turn(float& yaw, float& pitch) {
    yaw = pitch = 0.0f;
    if (!g || !g->input.gyro_available()) {
        return false;
    }
    yaw = g->input.gyro_yaw();
    pitch = g->input.gyro_pitch();
    return true;
}

void pending_gyro(float& yaw, float& pitch) {
    yaw = pitch = 0.0f;
    if (g) {
        g->input.pending_gyro(yaw, pitch);
    }
}

void publish_button_device(bool keyboard_mouse) {
    button_labels::Device device;
    device.keys = g_prompt_device < 0 ? keyboard_mouse : g_prompt_device == 1;
    int count = 0;
    if (SDL_JoystickID* pads = SDL_GetGamepads(&count)) {
        if (count > 0) {
            switch (SDL_GetGamepadTypeForID(pads[0])) {
                case SDL_GAMEPAD_TYPE_PS3:
                case SDL_GAMEPAD_TYPE_PS4:
                case SDL_GAMEPAD_TYPE_PS5:
                    device.pad = button_labels::Pad::PlayStation;
                    break;
                case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
                case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
                case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
                case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
                    device.pad = button_labels::Pad::Nintendo;
                    break;
                default:
                    device.pad = button_labels::Pad::Xbox;
                    break;
            }
        }
        SDL_free(pads);
    }
    button_labels::set_device(device);
}

// The cursor as the mods ask for it, while nothing else has the mouse: captured only with the
// window focused, the overlay's windows, the menu and the debug menu shut, and the player on the
// keyboard and mouse (platform/active_device.h), or on a gamepad too while the mouse plays beside
// it (set_mouse_with_gamepad: a mouse, or a gyro a driver such as Steam Input turns into one).
void apply_mouse_capture() {
    if (!g || !g->window || g->window->sdl() == nullptr) {
        return;
    }
    SDL_Window* w = g->window->sdl();
    const bool focused = (SDL_GetWindowFlags(w) & SDL_WINDOW_INPUT_FOCUS) != 0;
    const bool keyboard_mouse = g->input.active_device() == platform::ActiveDevice::KeyboardMouse;
    publish_button_device(keyboard_mouse);
    // On a gamepad too, where a mouse is connected; never on a phone or tablet, whose touch screen
    // SDL may report as a mouse.
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    const bool mouse_beside_pad = false;
#else
    const bool mouse_beside_pad = g_mouse_with_gamepad && SDL_HasMouse();
#endif
    const bool capture =
        focused && !debug_menu::is_open()
        && (debug_overlay::camera_debug_capturing()
            || (g->capture_wanted && (keyboard_mouse || mouse_beside_pad)
                && !debug_overlay::free_camera_enabled() && !debug_overlay::wants_cursor()));
    if (SDL_GetWindowRelativeMouseMode(w) != capture) {
        // Captured, the window keeps the cursor as well: no click lands on another window.
        SDL_SetWindowMouseGrab(w, capture);
        SDL_SetWindowRelativeMouseMode(w, capture);
        // The motion SDL reports as it warps or puts back the cursor is not the player's.
        g->input.device_switch().settle(SDL_GetTicksNS());
    }
    g->input.set_captured(capture);
    // The pointer where one of the overlay's windows uses it; over the game's own menus only for
    // a mod that points at them with the mouse (the keyboard and mouse), while the player is on
    // the mouse and has moved it in the last two seconds, as Dusklight does. Hidden otherwise. (A
    // captured mouse hides it anyway; let go, SDL shows it as set here.)
    constexpr std::uint64_t kCursorIdleNs = 2'000'000'000;
    const std::uint64_t moved = g->input.device_switch().last_motion();
    const bool pointer = debug_overlay::wants_cursor()
                         || (keyboard_mouse && g->keyboard_taken && !capture && moved != 0
                             && SDL_GetTicksNS() - moved < kCursorIdleNs);
    if (SDL_CursorVisible() != pointer) {
        pointer ? SDL_ShowCursor() : SDL_HideCursor();
    }
}

bool keyboard_mouse_in_use() {
    return g && g->input.active_device() == platform::ActiveDevice::KeyboardMouse;
}

void set_prompt_keys(const std::string* names, int count) {
    button_labels::KeyNames keys;
    for (int i = 0; names != nullptr && i < count && i < static_cast<int>(keys.size()); ++i) {
        keys[static_cast<std::size_t>(i)] = names[i];
    }
    button_labels::set_key_names(keys);
}

KeyboardMouseView keyboard_mouse() {
    if (!g) {
        return {};
    }
    KeyboardMouseView view{
        &g->input.keyboard_mouse(),
        debug_overlay::visible() || debug_menu::is_open() || debug_overlay::free_camera_enabled()
    };
    // The keyboard is player 2's (players.h): the port's own keyboard table presses its pad, and
    // a mod on pad 0 reads nothing held.
    if (g->input.keyboard_player() == 1) {
        static platform::KeyboardMouse none;
        none.tick = g->input.keyboard_mouse().tick;
        view.state = &none;
        return view;
    }
    SDL_Window* w = g->window ? g->window->sdl() : nullptr;
    if (w == nullptr) {
        return view;
    }
    // The cursor in the window's points, then in its pixels, then over the game's 4:3 picture: the
    // picture's box for the aspect setting, and the 4:3 middle of it where it is wider (where the
    // game's 2D screen, its menus, are drawn).
    float mx = 0.0f;
    float my = 0.0f;
    SDL_GetMouseState(&mx, &my);
    int points_w = 0;
    int points_h = 0;
    SDL_GetWindowSize(w, &points_w, &points_h);
    int window_width = 0;
    int window_height = 0;
    g->window->drawable_size(window_width, window_height);
    if (points_w <= 0 || points_h <= 0 || window_width <= 0 || window_height <= 0) {
        return view;
    }
    const float px = mx * static_cast<float>(window_width) / static_cast<float>(points_w);
    const float py = my * static_cast<float>(window_height) / static_cast<float>(points_h);
    const Box box =
        frame_box(window_width, window_height, picture_aspect(window_width, window_height));
    if (box.width <= 0 || box.height <= 0) {
        return view;
    }
    const float tv_width =
        std::min(static_cast<float>(box.width), static_cast<float>(box.height) * kTvAspect);
    const float tv_x =
        static_cast<float>(box.x) + (static_cast<float>(box.width) - tv_width) / 2.0f;
    view.pointer_x = (px - tv_x) / tv_width;
    view.pointer_y = (py - static_cast<float>(box.y)) / static_cast<float>(box.height);
    // The menus follow the cursor only while the player is on the keyboard and mouse: a mouse left
    // over the window does not move them under a gamepad.
    view.pointer_inside = g->input.active_device() == platform::ActiveDevice::KeyboardMouse
                          && SDL_GetMouseFocus() == w && view.pointer_x >= 0.0f
                          && view.pointer_x <= 1.0f && view.pointer_y >= 0.0f
                          && view.pointer_y <= 1.0f;
    return view;
}

void take_keyboard() {
    if (g) {
        g->keyboard_taken = true;
        g->input.set_keyboard_table(false);
    }
}

void open_controls(const std::filesystem::path& dir, const std::string& game) {
    if (!g) {
        return;
    }
    // Mappings for controllers SDL does not know as gamepads (SDL's gamecontrollerdb.txt format):
    // with one, a controller is read by position as any other.
    std::error_code error;
    const std::filesystem::path mappings = dir / "gamecontrollerdb.txt";
    if (std::filesystem::exists(mappings, error)) {
        const int added = SDL_AddGamepadMappingsFromFile(mappings.string().c_str());
        if (added < 0) {
            log::warn("controls: {} cannot be read: {}", mappings.string(), SDL_GetError());
        } else {
            log::info("controls: {} controller mapping(s) from {}", added, mappings.string());
        }
    }
    const auto apply = [](const controls::Store::PlayerBindings& bindings,
                          const platform::Players& players) {
        g->input.set_bindings(bindings);
        g->input.set_players(players);
    };
    g->controls.open(dir, game, apply);
    settings::Settings& set = settings::get();
    if (!set.bindings.empty() && !g->controls.file(controls::Scope::Game).read_only()) {
        const int moved = g->controls.adopt(set.bindings);
        log::info(
            "controls: {} binding(s) of the settings file moved to {}",
            moved,
            g->controls.path(controls::Scope::Game).string()
        );
        set.bindings.clear();
        settings::save();
    }
}

controls::Store* controls_store() {
    return g ? &g->controls : nullptr;
}

platform::Input* input() {
    return g ? &g->input : nullptr;
}

void capture_mouse(bool capture) {
    if (g) {
        g->capture_wanted = capture;
    }
}

void menus_after_mods(int port, std::uint16_t& buttons) {
    if (port == 0) {
        mod_menu::filter_pad(buttons);  // a mod's menu page has the pad while it is open
        mod_ui::filter_pad(buttons);    // and a mod's page of the game's widgets while shown
        debug_menu::filter_pad(buttons, openrac_game_loaded_overlay());
    }
}

void skip_after_mods(int port, std::uint16_t& buttons) {
    // A mod's skip (openrac_api.play_skip: a party's scenes ending together) presses as a map's
    // own skip does.
    // OPENRAC_SKIP_SCENES presses it too.
    static const bool skip_scenes = env::flag(env::Var::SkipScenes);
    const bool asked = port == 0 && (mods::play_skipping() || skip_scenes);
    if (openrac_game_skip != nullptr
        && (asked || (port == 0 && debug_overlay::meat_proxy_review_skips_flight()))) {
        openrac_game_skip();
    }
    if (port == 0 && g
        && (asked || (openrac_game_map != nullptr && openrac_game_map->skip_cutscenes)
            || debug_overlay::meat_proxy_review_skips_landing())
        && openrac_game_skip_buttons != nullptr && (g->index & 1) != 0) {
        const int skip = openrac_game_skip_buttons();
        if (skip != 0) {
            // Use the game's scene teardown. Pulse rather than hold: a scene can ignore presses
            // during its first ticks, then needs a fresh Start (or Cross for a ship scene).
            buttons = static_cast<std::uint16_t>(buttons & ~((skip & 0xFFFF) | 0x8));
        }
    }
    if (port == 0 && settings::get().skip_cutscenes && (buttons & 0x8) == 0) {
        if (openrac_game_skip_buttons != nullptr) {
            buttons = static_cast<std::uint16_t>(buttons & ~(openrac_game_skip_buttons() & 0xFFFF));
        }
        if (openrac_game_skip != nullptr) {
            openrac_game_skip();
        }
    }
}

void set_motors(int port, bool small, std::uint8_t large) {
    if (g) {
        g->input.set_motors(port, {small, large});
    }
}

void refresh_pads() {
    if (g && g->window && settings::get().responsive_controls) {
        g->input.refresh_pads();
        input_latency::sampled(g->input.pad(0).buttons);  // [debug tools] OPENRAC_INPUT_LATENCY
    }
}

bool pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]) {
    if (!g || port < 0 || port >= platform::kPadPorts) {
        return false;
    }
    if (port == 0) {
        debug_overlay::camera_pad_chord();  // before the read: its toggle holds the chord back
        debug_overlay::menu_pad_chord();
    }
    const platform::Pad& p = g->input.pad(port);
    *buttons = p.buttons;
    analog[0] = p.right_x;
    analog[1] = p.right_y;
    analog[2] = p.left_x;
    analog[3] = p.left_y;
    g->input.taps_read(port);
    if (port == 0) {
        input_latency::pad_read(*buttons, g->index);  // [debug tools] OPENRAC_INPUT_LATENCY
    }
    // OPENRAC_PRESS=FRAME:MASK:FRAMES[:LX:LY[:RX:RY]],... holds buttons (mask bits as the pad reports
    // them, 0x4000 cross, 0x8 start) and optionally the left and right sticks (0-255, 128 centred; 0
    // is left or up) for scripted checks. FRAME counts the window's frames (world, 2D and movie frames).
    // cFRAME counts from control instead (Route: the same moment in every version); kPERIOD holds the
    // buttons the first FRAMES of every PERIOD frames while what plays can be skipped and control has
    // not come yet (Start through scenes and the flight), counted on window frames; sPERIOD does the
    // same counted from the frame what plays became skippable, which is the same tick of the scene in
    // every version (k's window frames fall at other ticks of it in PAL and US); SPERIOD as s, before
    // and after control (the scenes and flights a route starts once the player has control).
    static const std::optional<std::string> press = env_string(env::Var::Press);
    if (port == 0 && press) {
        for (const char* at = press->c_str(); at && *at;) {
            unsigned long frame = 0, mask = 0, length = 5;
            unsigned int lx = 128, ly = 128, rx = 128, ry = 128;
            const char kind = *at == 'c' || *at == 'k' || *at == 's' || *at == 'S' ? *at : 0;
            const int read = std::sscanf(
                at + (kind != 0 ? 1 : 0),
                "%lu:%lx:%lu:%u:%u:%u:%u",
                &frame,
                &mask,
                &length,
                &lx,
                &ly,
                &rx,
                &ry
            );
            bool active = false;
            if (kind == 'c') {
                const std::int64_t since = static_cast<std::int64_t>(g->index) - g_route.control;
                active = g_route.control >= 0 && since >= static_cast<std::int64_t>(frame)
                         && since < static_cast<std::int64_t>(frame + length);
            } else if (kind == 'k') {
                active =
                    g_route.control < 0 && frame > 0 && skippable() && g->index % frame < length;
            } else if (kind == 's' || kind == 'S') {
                active = (kind == 'S' || g_route.control < 0) && frame > 0 && skippable()
                         && g_route.skippable_from >= 0
                         && (static_cast<std::int64_t>(g->index) - g_route.skippable_from)
                                    % static_cast<std::int64_t>(frame)
                                < static_cast<std::int64_t>(length);
            } else {
                active = g->index >= frame && g->index < frame + length;
            }
            if (read >= 2 && active) {
                *buttons = static_cast<std::uint16_t>(*buttons & ~mask);
                if (read >= 5) {
                    analog[2] = static_cast<std::uint8_t>(lx);
                    analog[3] = static_cast<std::uint8_t>(ly);
                }
                if (read >= 7) {
                    analog[0] = static_cast<std::uint8_t>(rx);
                    analog[1] = static_cast<std::uint8_t>(ry);
                }
            }
            at = std::strchr(at, ',');
            at = at ? at + 1 : nullptr;
        }
        // The late camera latch turns a scripted run's frames by the scripted stick, as the
        // player's by the pad's.
        latch_scripted_stick(analog[0]);
    }
    // OPENRAC_PRESS2=FRAME:MASK:FRAMES,...: port 1's buttons by script (a second local player),
    // counted on the window's frames as OPENRAC_PRESS's plain windows are.
    static const std::optional<std::string> press2 = env_string(env::Var::Press2);
    if (port == 1 && press2) {
        for (const char* at = press2->c_str(); at && *at;) {
            unsigned long frame = 0, mask = 0, length = 5;
            if (std::sscanf(at, "%lu:%lx:%lu", &frame, &mask, &length) >= 2 && g->index >= frame
                && g->index < frame + length) {
                *buttons = static_cast<std::uint16_t>(*buttons & ~mask);
            }
            at = std::strchr(at, ',');
            at = at ? at + 1 : nullptr;
        }
    }
    if (port == 0) {
        scripted_pad(*buttons, analog);  // [debug tools] OPENRAC_RECORD, _REPLAY and _WALK
    }
    if (port == 0) {
        // Scripted and replayed sticks turn at normal speed.
        openrac_game_camera_speed = analog[0] == p.right_x ? p.camera_speed : 1.0f;
    }
    // Start skips an in-game scene the way the game itself skips it: a pressed Start also presses
    // what the game's own skip of the scene playing reads (rac1: in a cutscene the four shoulders,
    // since the game skips one on Start with L1, L2, R1 and R2 held, Start alone only once it is
    // beaten, ReRAC cutscenes_transitions.md, "skip"; in the ship's landing and take-off Cross).
    // Bits are active low: 0x8 Start, 0x4000 Cross, 0x0F00 L2 R2 L1 R1.
    // With a mod on the pad's reports, after the mods (skip_after_mods), so a Start a mod presses
    // (a key bound to it) skips as the pad's does.
    // The menus over the game take the pad the same way, after the mods: a key a mod presses (the
    // keyboard and mouse, a controller a mod reads itself) moves their cursor as the pad's does.
    if (openrac_game_pad_report_hook == nullptr) {
        skip_after_mods(port, *buttons);
        menus_after_mods(port, *buttons);
    }
    // Keep the game at its own camera/player pose while the developer flies independently.
    if (debug_overlay::free_camera_enabled()) {
        *buttons = 0xffff;
        std::fill_n(analog, 4, std::uint8_t{128});
    }
    if (port == 0) {
        debug_overlay::pad(*buttons, analog);  // [debug tools]
    }
    if (env::flag(env::Var::TraceInput)) {
        static std::uint64_t last_frame[platform::kPadPorts]{};
        if (g->index >= last_frame[port] + 25) {
            last_frame[port] = g->index;
            log::info(
                "input frame {} port {}: buttons {:#x}, right {} {}, left {} {}",
                g->index,
                port,
                *buttons,
                analog[0],
                analog[1],
                analog[2],
                analog[3]
            );
        }
    }
    return true;
}

}  // namespace openrac::frontend
