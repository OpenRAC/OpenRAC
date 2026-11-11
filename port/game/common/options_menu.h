// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's settings (settings.h) in the game's own options: the rows of the front end's Video
// options page, drawn and navigated by the game's widgets in the game's font, in place of the game's
// own rows (PAL's "Video mode", 50 or 60 Hz, which means nothing to a window). For a game whose
// program describes its front end (game_host.h, openrac_front_end): Ratchet & Clank, every disc.
#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace openrac::frontend::options_menu {

// The rows: one per setting, whichever menu shows them (the game's options pages here, the
// overlay's menus and drawer, debug_overlay.cpp).
enum Row {
    kDisplayMode,
    kMonitor,
    kAspect,
    kHud,
    kRenderScale,
    kMsaa,
    kAnisotropy,
    kDrawDistance,
    kVSync,
    kHdr,
    kFrameRate,
    kFrameCap,
    kLowLatency,
    kShowFps,
    kMuteUnfocused,
    kSkipCutscenes,
    kResponsiveControls,
    kRows
};

// After the rows above, when the program's disc has game fixes (fixes.h): a "Game fixes" row that
// heads them (nothing to change), then a row for each, On (the port's fix) or Original (the
// console's behaviour). Every page that lists the rows scrolls through all of them.
int row_count();
int fix_of_row(int row);  // the fix's index (fixes.h), or -1 for any other row

// A row's name, and its setting's value now, as the player reads them.
const char* label(int row);
std::string value(int row);

// A row moved to its next value (by = 1) or the one before (by = -1), within what the graphics
// card offers. Not saved (settings::save) and not applied: the window applies what changed.
void step(int row, int by);

// Once a frame: while the boot program runs (the title and its menus), the Video options page
// shows the port's rows, four at a time, moving with the cursor. Cross or Right moves the row under
// the cursor to its next value, Left to the one before. True when a setting changed (it is saved
// already; the window applies it at once: none needs the game started again).
bool update(int loaded_overlay);

// The pause menu's pages of the mods' items (openrac_api.ui_pause_item), made with the Options
// page's items in each level program, as the Video page: item `index`'s page, its list widget, its
// kPauseRowsShown rows (0x18 bytes each: the label's text id, the address of an int the widget keeps at
// 1 and steps on Cross, three value text ids; a 0 label ends them), their ints, its title box (+0x34
// the title's text id). All 0 before the Options page was seen. mod_ui.cpp fills them.
struct PauseRows {
    std::uint32_t page = 0, list = 0, rows = 0, values = 0, title = 0;
};

constexpr int kPauseRowsShown = 5;
PauseRows pause_rows(int index);

// What a row of the options pages is pressed with this tick: Cross, Left or Right, or nothing.
// The game's pad reading makes the left stick a direction as soon as it leans a third of the way,
// sideways or not (games/rac1 src/game/system/pad.c), so the stick pushed down onto a row a little
// off straight is Left or Right as well: a direction counts from the d-pad, and from the stick only
// while it leans more sideways than up or down.
enum class RowPress { None, Cross, Left, Right };

// The pad's words and stick as the game keeps them (PadState): `raw` the buttons newly down
// without the stick's directions (+0x1B4), `all` with them (+0x1C4), `x` and `y` the left stick
// (+0x108, +0x10C).
inline RowPress row_press(std::uint32_t raw, std::uint32_t all, float x, float y) {
    constexpr std::uint32_t kCross = 0x0040, kRight = 0x2000, kLeft = 0x8000;
    if (((raw | all) & kCross) != 0) {
        return RowPress::Cross;
    }
    if ((raw & (kLeft | kRight)) != 0) {
        return (raw & kRight) != 0 ? RowPress::Right : RowPress::Left;
    }
    if ((all & (kLeft | kRight)) != 0 && std::fabs(x) > std::fabs(y)) {
        return (all & kRight) != 0 ? RowPress::Right : RowPress::Left;
    }
    return RowPress::None;
}

// The same for the first pad, its PadState at `pad` (openrac_front_end.pad).
RowPress row_press(std::uint32_t pad);
// Drops guest-memory addresses cached by the port's Video pages. A restored snapshot can replace
// the guest allocations and level program those addresses referred to.
void savestate_restored();

// The items the Options list is given in place of the game's own (with the Video page's added), or 0
// while it has the game's.
std::uint32_t options_items();

// The game's language as an index into the port's texts: 0 English, 1 French, 2 German, 3 Spanish,
// 4 Italian (the PAL disc's five); English for a game without one.
int language();

// UTF-8 as the game's text: ASCII as it is, the accented letters the game's font has at its own
// codes, any other letter left out.
std::string game_text(const std::string& utf8);

// An English text of the port's (a row's name or value, a page's name) as the game shows it: in
// its language (language() above), as the game's text (game_text above).
std::string shown(const std::string& english);

// The game's text lookup, asked for an id of the port's own: the string in game memory, or 0 for
// an id that is the game's.
std::uint32_t text(int id);

}  // namespace openrac::frontend::options_menu
