// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The window of a native game: once a frame, what the game holds in its memory is drawn by the
// port's renderer, and the keyboard and controllers are read for the game's pad library.
//
// Only built where the window libraries are (port/cmake/Window.cmake); the program runs without a
// window otherwise. What a game's memory means (where its camera and objects are) is the game's,
// read through its profile (game/common/profile.h): so far Ratchet & Clank's, with each level's
// geometry from the extractor's port export (editor/port.py).

#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

#include "openrac/game_host.h"
#include "openrac/openrac_mod.h"

namespace openrac::controls {
class Store;
}

namespace openrac::platform {
class Input;
}

namespace openrac::frontend {

// Opens the window. `levels` holds the extracted levels (level_00, level_01, ...).
bool open(const std::string& game_id, const std::filesystem::path& levels, std::string& error);

// Draws a frame from the game's main memory and reads the input; false once the window was closed.
// The game's world renderers are drawing now: the camera they draw with is kept for this frame's
// world (a menu may set its own camera afterwards to draw its frame objects).
void world_drawn(std::span<const std::uint8_t> ram);
// The world drawn next this frame lists its own mobys (a level's under a menu's moby context): its
// mobys are the frame's, and the menu's picture of the frame is drawn from it; the menu's 2D and
// its own mobys go over it (frontend/menu_layers.h, openrac_game_world_under_menu).
void world_under_menu();
void moby_colour(std::uint32_t object, unsigned colour, int level);

// The game drew `count` mobys from `first` with the camera it holds now (the page menu's frame
// objects, drawn with the menu's camera): they are drawn as that camera saw them.
void mobys_drawn(std::span<const std::uint8_t> ram, std::uint32_t first, int count);

// A world effect quad the game drew this frame (openrac_game_effect_quad).
void effect_quad(std::span<const std::uint8_t> ram, const openrac_game_quad& quad);
// A sky shell the game drew this frame, with its matrix (openrac_game_sky_shell).
void sky_shell(int shell, const float* matrix);

// A quad of the 2D layer drawn natively (openrac_game_ui_quad): its index for the marker, or -1
// when the 2D layer is replayed from the game's packets.
int ui_quad(const openrac_game_ui_rect& quad);

// How far the HUD's side-anchored elements go out past the game's picture, in its 512ths of that
// picture's width (openrac_game_hud_side): the player's "HUD: Edges" in a wider picture, else 0.
int hud_side();

// How many times wider than 4:3 the picture is, at least 1 (openrac_game_picture_wider): what the
// frame's horizontal tangent is the game's times (render's projection, picture_scale).
float picture_wider();

// A sky sprite the game drew this frame (openrac_game_sky_sprite).
void sky_sprite(const std::uint8_t* record, std::uint64_t tex0);

// A live particle the game drew this frame (openrac_game_particle).
void particle(std::span<const std::uint8_t> ram, const std::uint8_t* record, std::uint32_t pixels,
              std::uint32_t clut, int log2_side);

// `draws`: the layers the game's renderers drew this frame (bit n = renderer::Bucket n; all bits
// when the game does not report them).
bool frame(std::span<const std::uint8_t> ram, std::uint32_t chain, std::uint32_t draws);

// Captures the next tick's owned CPU scene while an earlier Vulkan snapshot renders.
// The game thread calls this with stable RAM before finish_render(); false on capture failure.
[[nodiscard]] bool prepare_frame(std::span<const std::uint8_t> ram);

// Completes a pending render and presents it on the window thread. False on render failure.
[[nodiscard]] bool finish_render();
// True while a submitted snapshot has not yet been presented; queried only by the game thread.
bool render_pending();

// Drops host-derived frame, level and GPU state after guest memory is restored. The next game frame
// rebuilds the level renderers and replays the game's drawing commands.
void savestate_restored(std::uint64_t tick);

// Frames between the game's ticks: the window draws at the display's rate, the camera and mobys
// moved from the last tick's to the latest's by when each frame is shown (OPENRAC_UNCAPPED or
// OPENRAC_NO_BLEND: one frame a tick). set_tick: when the tick about to be drawn was due, and the
// tick's length. interpolating: it is on; wants_between: the latest tick has a world to move;
// between: draws one if a refresh before `next_tick` (plus the lag) still shows this tick, and
// reads the window's events: 1 drawn or queued, 0 none due, -1 the window was closed or rendering failed.
// An owned Vulkan job may remain in flight when the next game tick starts.
// `late`: how long after its time the tick came (a machine that draws slower than the game ticks):
// frame() leaves the frames of ticks that come more than a tick late undrawn. `work`: how long
// the game's own work for it took, from the end of the wait before it.
void set_tick(
    std::chrono::steady_clock::time_point tick,
    std::chrono::duration<double> period,
    std::chrono::duration<double> late = std::chrono::duration<double>(0.0),
    std::chrono::duration<double> work = std::chrono::duration<double>(0.0)
);
bool interpolating();

// The game's text lookup, asked for an id of the port's own (the rows it adds to the game's
// options, options_menu.h): the string in game memory, or 0.
std::uint32_t text(int id);
bool wants_between();
int between(std::chrono::steady_clock::time_point next_tick);

// An image the game sent to the GS outside the display list (its library's image transfer): kept
// for every renderer the window makes, as the chip keeps it in its memory. `base` in 256-byte
// blocks, `width_units` in 64 pixels, `pixels` in raster order in `psm`.
void upload_image(
    std::uint32_t base,
    std::uint32_t width_units,
    std::uint8_t psm,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint8_t> pixels
);

// The game reading a PSMCT32 rectangle of its draw buffer back (sceGsExecStoreImage): answered
// with the last frame drawn, scaled to the chip's pixels. False if the rectangle is not in the
// draw buffer (nothing written).
bool store_image(std::uint32_t base, std::uint32_t width_units, std::uint8_t psm, int x, int y,
                 int width, int height, std::span<std::uint8_t> out);

// Shows one full-screen picture the game puts straight into its display buffer (a movie's frame,
// a boot still), RGBA rows top to bottom, with `black` (0 to 1) of black over it (the game's
// FadeToBlack), and reads the input. Counts as a frame (shots, OPENRAC_PRESS). False once the window
// was closed.
bool show_picture(const std::uint8_t* rgba, int width, int height, float black);

// Window frames since the player got control in a scripted run (OPENRAC_PRESS's and OPENRAC_WRITE's
// c entries), -1 before.
std::int64_t since_control();

// The pad of a port as the game's pad library reads it: buttons active low, then the right and
// left sticks (x, y each).
bool pad(int port, std::uint16_t* buttons, std::uint8_t analog[4]);

// With the player's responsive_controls setting, reads the pads again just before the game's next
// tick runs (the end of program::vsync, after its wait): the tick reads the sticks and buttons as
// they are when it starts, not as they were when the last tick ended. Nothing else of the input
// changes (the window's events stay queued for the next frame), and the tick still reads one state
// of the pad. Off (the default), the tick reads the pad as the last tick's end left it.
void refresh_pads();

// Start skipping what plays (the skip_cutscenes setting): what Start also presses for the game's
// own skip, and the game's skip hook. pad() does it itself without mods on the pad's reports; with
// some, the pad library calls it on the report the mods leave (openrac_game_pad_after_mods).
void skip_after_mods(int port, std::uint16_t& buttons);

// The menus over the game (a mod's menu page, a mod's page of the game's widgets, the debug menu)
// take pad 0 while they are open, leaving the game none of it. Called where skip_after_mods is.
void menus_after_mods(int port, std::uint16_t& buttons);

// The keyboard and mouse for the mods (mod interface 1.2, platform/input.h's KeyboardMouse): as
// the last frame read them; null without a window. `overlay` is set while the port's own overlay
// or menu has the input.
struct KeyboardMouseView {
    const void* state = nullptr;  // const platform::KeyboardMouse*
    bool overlay = false;
    // The cursor over the game's 4:3 picture (the middle of a wider one), 0..1 each way; inside
    // while it is over that picture in the focused window.
    float pointer_x = 0.0f;
    float pointer_y = 0.0f;
    bool pointer_inside = false;
};
KeyboardMouseView keyboard_mouse();

// A mod took the keyboard: the port's own keyboard table stops pressing pad 0's buttons.
void take_keyboard();

// Reads the player's controls of `game` ("rac1") from `dir` (controls_store.h) and presses the pads
// from them, from the next read on; the controller mappings in `dir`'s gamecontrollerdb.txt, if
// there is one, are added to SDL's. Bindings the version's settings file held (bind_* keys, before
// the controls had files of their own) move into the game's controls file once.
void open_controls(const std::filesystem::path& dir, const std::string& game);

// The player's controls and the input they press, for the controls window
// (debug/controls_window.cpp); null while no window is open.
controls::Store* controls_store();
platform::Input* input();

// A mod asks for the cursor captured (relative mouse mode) or let go. Applied each frame, and let
// go anyway while the window is not focused or the overlay or the debug menu is open.
void capture_mouse(bool capture);

// What the port draws with, as a mod reads it (interface 1.4, openrac_mod.h's openrac_graphics):
// the backend none while no window is open.
const openrac_graphics* mod_graphics();

// The views a mod draws besides the window's picture (interface 1.4, openrac_view_source): drawn
// in each frame from now on; null for none. While a mod has views, Vulkan draws on the window's
// thread (no render worker). Without a mod's views nothing of them runs.
void set_view_source(const openrac_view_source* source);

// While the debug overlay holds the game (its pause): draws the last tick again and reads the
// window's events until the overlay lets a tick through. False once the window was closed.
bool hold();

// A tick on which the game sent nothing to draw (program/vsync.cpp): the last tick drawn again
// and the window's events read. False once the window was closed.
bool keep_picture();

// Current level has validated, uploaded probes and a supported renderer.
bool enhanced_graphics_available();
std::string enhanced_graphics_error();

void close();

}  // namespace openrac::frontend
