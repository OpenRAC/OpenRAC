// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay: a panel in the window's corner, over the game, that says what the port is
// doing: the frame's times, the renderer, memory, the mobys, the game's state and the shader
// programs, with a moby inspector, a memory viewer, pause, single step and slow motion.
// docs/port/DEBUGGING.md describes all of it.
//
// F3 shows and hides it (OPENRAC_OVERLAY or --debug-overlay: shown from the start). Nothing of it
// exists until it is first shown: no ImGui context, no fonts, no measurements beyond the counters
// (common/stats.h). It is drawn with Dear ImGui over the finished frame in the window itself, at
// the window's resolution, so OPENRAC_SHOT's pictures of the game's frame stay without it;
// OPENRAC_OVERLAY_SHOT writes the window with it.
//
// The window (frontend.cpp) calls in at a few marked places and hands over what it already has;
// the overlay reads the game's memory itself and changes none of it.

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "game/common/debug_camera.h"
#include "game/common/profile.h"
#include "renderer/math.h"
#include "renderer/renderer.h"
#include "scene/object_picker.h"

namespace openrac::frontend::debug_overlay {

// Samples window-thread controls into an owned render job; tools remain active while paused.
CameraDebugFrame camera_debug_frame();
// Run requested review travel at the same game-loop boundary as developer and mod travel.
void meat_proxy_review_update();
// The pending review view is waiting for its destination's ship landing.
bool meat_proxy_review_skips_landing();
// The review started a trip and the ship is in its interplanetary flight.
bool meat_proxy_review_skips_flight();
// Observes pointer input after UI routing, including releases and focus loss.
void camera_debug_event(const SDL_Event& event);
// Freecam takes gameplay input but leaves developer shortcuts available.
bool free_camera_enabled();
// A gamepad's L3+R3 toggles freecam, held back from the game's pad (no map on its R3).
void camera_pad_chord();
// Start+Select opens the menu, taken from the game's pad (no pause, no map). Before the game's read.
void menu_pad_chord();
// Alt or right-click requests hidden-cursor freecam look while the window is focused.
bool camera_debug_capturing();

// Shown from the start (--debug-overlay), as OPENRAC_OVERLAY does.
void request();

// Capturing picker geometry is opt-in; pose capture reads this on the render worker.
bool picker_enabled();
// The isolated studio may still need displayed poses after click picking is turned off.
bool picker_snapshot_needed();
void set_picker_enabled(bool enabled);
// Where the click picker writes moby-picker.json: the game's data folder (its logs' folder),
// never the working directory, which can be a source checkout.
void set_picker_report_folder(const std::string& folder);

// Any desktop overlay window holding the mouse, including Enhanced Settings.
bool visible();

// Whether the mouse's pointer is shown: only while one of the overlay's windows or menus that
// a pointer uses is out (the menu, the debug panel, Enhanced Settings, the object viewer, the
// picker, the on-screen buttons' editor). Hidden over the game otherwise.
bool wants_cursor();

// A window event: true if the overlay took it (its keys, or the mouse and keyboard while they are
// over one of its windows), so the game does not see it.
bool event(const SDL_Event& event);

// Once a tick, from frame(): what the game holds now.
struct Tick {
    const char* game = "";
    std::span<const std::uint8_t> ram;
    int level = -1;                              // the level program loaded; -1 the boot program
    const host::GameState* state = nullptr;      // the camera the tick is drawn from, the mobys
    std::uint32_t draws = 0;                     // the layers the game drew (bit n: renderer::Bucket n)
    std::uint64_t tick = 0;                      // the window's frame counter
    std::size_t packet_bytes = 0, ui_quads = 0, effect_quads = 0, strips = 0, mobys_drawn = 0;
};

void tick(const Tick& tick);

// At the start of each picture drawn (draw_kept): opens the frame for the measurements
// (common/stats.h). `first`: the tick's own picture; `held`: drawn again while paused.
void begin(bool first, bool held, float alpha, std::uint64_t tick);

// Before the renderer draws: the passes hidden in the overlay are turned off, wireframe is set.
void before_render(renderer::Renderer* renderer, std::uint64_t frame);

// After the frame was put in the window and before the swap: draws the overlay, if shown.
struct Shown {
    SDL_Window* window = nullptr;
    int window_width = 0, window_height = 0;     // the drawable, in pixels
    int box_x = 0, box_y = 0, box_width = 0, box_height = 0;  // the game's picture in it
    const renderer::Renderer* renderer = nullptr;
    const renderer::Camera* camera = nullptr;    // the picture's view and projection
    int frame_width = 0, frame_height = 0;       // the frame buffer the game is drawn in
    int last_world_width = 0, last_world_height = 0;
    bool vsync = false, interpolating = false;
    double refresh_interval = 0.0;               // seconds; 0 unknown
    double cost = 0.0;                           // seconds: what a frame costs before its swap
    double tick_period = 0.0;                    // seconds
    scene::ObjectPicker* picker = nullptr;
    const scene::LevelData* picker_level = nullptr;
    std::span<const scene::Instance> picker_live;
    std::span<const renderer::Mat4> picker_palette;
    int picker_level_number = -1;
    std::uint64_t picker_tick = 0;
    unsigned picker_layers = 0;
    bool freecam = false;   // The picture's camera came from freecam.
    int camera_level = -1;  // Level that supplied the rendered camera.
    bool enhanced_graphics = false;
};

void draw(const Shown& shown);

// Pad 0 as the game is about to read it (buttons active low, right then left stick).
void pad(std::uint16_t buttons, const std::uint8_t analog[4]);

// ---- Pause, single step, slow motion ----

// True while the game is paused: the window then draws the last tick again instead of letting the
// game run (frontend.cpp, hold). A step asked for lets one tick through.
bool holding();

// How much longer a tick lasts than the game's own (1: full speed; 4: quarter speed).
double tick_stretch();

// ---- The drawer: the window's options on a touch screen ----
//
// On Android/iOS the menus along the window's top (View, Graphics, Sound, Developer), which want a
// mouse and Alt to bring them back, are a drawer instead, as an emulator's: it slides in from the
// left over the paused game, its rows large enough for a thumb and moved through by a gamepad.
// The on-screen pad's options button opens it, as do Back and a gamepad's Guide button; Back,
// Resume, the gamepad's B or a touch beside it closes it.
// OPENRAC_DRAWER=1 has it on other systems too (for looking at it there), in place of the menus.
bool drawer_available();
void toggle_drawer();

// The on-screen pad's layout (platform/touch_pad.h): the drawer's Touch controls, "Move the
// controls", shuts the drawer and leaves the game paused while the player drags the controls
// about, with a bar in the middle of the screen to make them smaller or larger, put them back and
// finish (Done, Back). The window hands it to the pad, and saves the layout when it ends.
bool touch_editing();
// True once after the bar's Reset: the window puts every control at its own place.
bool take_touch_reset();
// The lines a dragged group of the pad has snapped to (TouchPad::guide_x, guide_y), drawn across
// the screen while editing: 0..1 of the width and of the height, below 0 for none.
void set_touch_guides(float x, float y);
// The size of the control the player last took hold of (TouchPad::selected_size), for the bar to
// show and change, below 0 when none is.
void set_touch_selected(float size);
// The steps the bar's - and + asked that control's size to change by since the last call.
int take_touch_size_steps();
// True once after the drawer or the menus changed one of the port's settings (saved already): the
// window applies it (frontend/window.cpp, apply_settings).
bool take_settings_changed();

// ---- Notifications ----

// A card's icon and colour.
enum class Tone : std::uint8_t {
    Info,     // the accent: a hint, a mod's line
    Done,     // green: a file written, a report ready
    Warning,  // amber: something the player can do something about
    Error,    // red: what failed
    Live,     // a red dot that beats: a recording running
};

// A card at the top of the screen (debug/notifications.cpp). Every message the port shows over
// the game is one. Cards stack, newest at the bottom, and wait while the menu is open.
struct Notification {
    std::string id{};     // a later notify with the same id updates this card; empty: never
    std::string title{};  // first line
    std::string text{};   // second line, quieter; may hold several lines
    // Keys shown as caps before the text, "or" between entries; "+" joins keys pressed together.
    std::vector<std::string> keys{};
    std::string folder{};  // a press on the card opens it (or the folder holding this file)
    Tone tone = Tone::Info;
    double seconds = 5.0;  // time on screen (paused while hidden); 0: until dismiss()
};

// Shows `n`, or updates the card with its id.
void notify(Notification n);

// Hides the card with this id, if there is one.
void dismiss(std::string_view id);

// GL objects and the ImGui context, before the window's context goes.
void release();

}  // namespace openrac::frontend::debug_overlay
