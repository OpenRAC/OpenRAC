// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "debug_menu.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#include "common/env.h"
#include "common/log.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "options_menu.h"
#include "renderer/ui/canvas.h"
#include "renderer/ui/picture_blit.h"

namespace openrac::frontend::debug_menu {

namespace {

constexpr int kVisibleLevels = 19;  // rows on screen; the catalog may contain more

// The game's front end as its program describes it (game_host.h, openrac_front_end): its levels
// and how to start one, and its page menu. Null for a game without one.
const openrac_front_end* front_end() {
    const openrac_front_end* fe = openrac_game_front_end;
    return fe != nullptr && fe->levels > 0 && fe->start_level != nullptr ? fe : nullptr;
}

int level_count() {
    const openrac_front_end* fe = front_end();
    return fe != nullptr ? fe->levels : 1;
}

// The launcher's developer mode (OPENRAC_DEVELOPER=1): without it the menus are the game's own.
bool developer() {
    return env::flag(env::Var::Developer);
}

// Open only over the boot program (title and main menu), where a new game can be started, unless
// the game can start a level from inside one.
bool available(int loaded_overlay) {
    return developer() && front_end() != nullptr
           && (loaded_overlay == OPENRAC_OVERLAY_EXE || front_end()->start_level_anywhere != 0);
}

struct State {
    bool open = false;
    int cursor = 0;
    int start = -1;       // a level to start at the next update
    bool native = false;  // asked for from the game's Options page (with its sound)
    // A level was started and the game has not left the boot program yet: the Planets page stays
    // on screen for a few frames while the flight loads, and a press on it then (a second Cross
    // or Space) reached the game's menu and left a black screen instead of the level.
    bool launched = false;
    std::uint16_t previous = 0xFFFF;  // pad 0 as last seen (active low)
    renderer::ui::PictureBlit blit;   // Draws the menu's picture over the frame.
};

State s;

// What --level N with OPENRAC_DIRECT does (rac1: port/game/rac1/host/boot.c, rac1_start_level).
void start_level(int level) {
    front_end()->start_level(level);
    log::info("debug menu: starting level {}", level);
}

// ---- Rac1: a "Planets" entry in the front end's Options list, opening a page of levels ----
//
// The front end's pages are data in the boot program (docs of ReRAC's menus.md section 3: a page is
// seq[14], parent, kind, focus, widget[14]; a list widget has its flags at +0x30, items at +0x34
// and cursor at +0x40; an item is {label, action, arg, sublabel, timer}). The Options list's enter
// (0x28dbb8 in ReRAC's level 1) puts the PAL list (Language, Video, Sound) in the widget each time,
// so its items pointer is pointed at a copy with a fourth entry whenever it shows the original.
// The entry opens (action 3) a copy of the Language page whose title and list are the game's own
// widgets with other data: the title "Planets" (text 20216) and one item per level, labelled with
// the game's planet names, drawn by the game's scroll list (func_0021E4B0) in the game's font. An
// item's action is one the list ignores (0x7F); Cross on it is taken here, with the game's confirm
// sound, and starts the level as start_level does. The addresses are the disc's own
// (openrac_front_end, from the game's program).
namespace page {

constexpr std::int16_t kIgnoredAction = 0x7F;
constexpr int kMaxItems = 8;  // the game's own Options items, at most (PAL: Language, Video, Sound)

struct Native {
    gaddr base = 0;           // the items the Options list had (the game's, or options_menu's)
    gaddr options_items = 0;  // those, then Planets
    gaddr page = 0, title = 0, list = 0, items = 0;
};

Native n;

void put_item(gaddr at, std::int16_t label, std::int16_t action, std::int32_t arg) {
    GREF(std::int16_t, at) = label;
    GREF(std::int16_t, at + 2) = action;
    GREF(std::int32_t, at + 4) = arg;
    GREF(std::int16_t, at + 8) = 0;
    GREF(std::int16_t, at + 10) = 0;
}

void build() {
    if (n.page != 0) {
        return;
    }

    const openrac_front_end& fe = *front_end();

    if (fe.language_page == 0) {
        return;  // a game whose pages are not described: the menu the port draws is the only one
    }

    n.page = openrac_guest_static(0xA0);
    n.title = openrac_guest_static(0x60);
    n.list = openrac_guest_static(0x60);
    n.items = openrac_guest_static(12 * static_cast<gaddr>(level_count() + 1));

    for (int i = 0; i < level_count(); ++i) {
        put_item(n.items + 12 * static_cast<gaddr>(i), fe.planet_texts[i], kIgnoredAction, i);
    }

    // The page, title and list: the Language page's, with this data.
    std::memcpy(G(n.page), G(fe.language_page), 0xA0);
    std::memcpy(G(n.title), G(fe.language_title), 0x60);
    std::memcpy(G(n.list), G(fe.language_list), 0x60);
    GREF(std::int32_t, n.title + 0x34) = fe.planets_text;
    // A scrolled list (0x8000: by whole rows), its items centred (0x400), wrapping (0x1000), rows
    // as high as the font (0x10); no language marker.
    GREF(std::uint32_t, n.list + 0x30) = 0x10u | 0x400u | 0x1000u | 0x8000u;
    GREF(std::uint32_t, n.list + 0x34) = n.items;
    GREF(std::uint32_t, n.list + 0x38) = 0;
    GREF(std::uint32_t, n.list + 0x3C) = 0;
    GREF(std::int32_t, n.list + 0x40) = 0;
    GREF(std::int32_t, n.list + 0x44) = 0;
    GREF(std::uint32_t, n.page + 0x44 + 4 * 2) = n.title;
    GREF(std::uint32_t, n.page + 0x44 + 4 * 3) = n.list;
    GREF(std::uint32_t, n.page + 0x40) = n.list;  // focus
    GREF(std::uint32_t, n.page + 0x80) = 0;
}

// The Options list's items `base` (up to the one without a label), then Planets.
void list_planets(gaddr base) {
    if (n.base == base) {
        return;
    }

    int count = 0;

    while (count < kMaxItems && GREF(std::int16_t, base + 12 * static_cast<gaddr>(count)) != 0) {
        ++count;
    }

    const gaddr own = 12 * static_cast<gaddr>(count);
    n.base = base;
    n.options_items = openrac_guest_static(own + 12 * 2);
    std::memcpy(G(n.options_items), G(base), own);
    put_item(
        n.options_items + own, front_end()->planets_text, 3, static_cast<std::int32_t>(n.page)
    );
}

// Once a frame over the front end: the Options list shows the entry.
void patch() {
    build();

    if (n.page == 0) {
        return;
    }

    const openrac_front_end& fe = *front_end();
    const gaddr shown = GREF(std::uint32_t, fe.options_list + 0x34);

    if (shown == fe.options_items || (shown != 0 && shown == options_menu::options_items())) {
        list_planets(shown);
        GREF(std::uint32_t, fe.options_list + 0x34) = n.options_items;
    }
}

bool on_page() {
    return n.page != 0 && front_end() != nullptr
           && GREF(std::uint32_t, front_end()->page_menu + 4) == n.page;
}

int cursor() {
    const int c = GREF(std::int32_t, n.list + 0x40);
    return c >= 0 && c < level_count() ? c : 0;
}

void confirm_sound() {
    if (front_end()->menu_sound != nullptr) {
        front_end()->menu_sound(0x11, GREF(std::uint32_t, n.list + 0x14));
    }
}

}  // namespace page

void move(int by) {
    s.cursor = (s.cursor + by + level_count()) % level_count();
}

// ---- Drawing: a picture made on the CPU in the chip's pixels, blitted over the frame ----

using renderer::ui::rgba;

// The game's menu colours (its panels' navy, khaki frame, light blue and yellow text).
constexpr std::uint32_t kNavy = rgba(14, 14, 30);
constexpr std::uint32_t kFrame = rgba(150, 146, 104);
constexpr std::uint32_t kText = rgba(136, 168, 255);
constexpr std::uint32_t kSelected = rgba(255, 255, 64);
constexpr std::uint32_t kTitle = rgba(230, 230, 230);
constexpr std::uint32_t kHint = rgba(120, 120, 140);

constexpr int kRow = 10;
constexpr int kPanelWidth = 232;
constexpr int kPanelHeight = 30 + kVisibleLevels * kRow + 18;

renderer::ui::Canvas make_picture(const openrac_front_end& fe) {
    renderer::ui::Canvas p(kPanelWidth, kPanelHeight, kFrame);
    p.fill(2, 2, p.width() - 4, p.height() - 4, kNavy);
    p.draw_text(10, 9, "OPENRAC DEBUG: START A LEVEL", kTitle);
    p.fill(8, 20, p.width() - 16, 1, kFrame);
    const int first = (s.cursor / kVisibleLevels) * kVisibleLevels;
    const int end = std::min(first + kVisibleLevels, level_count());

    for (int i = first; i < end; ++i) {
        const int y = 27 + (i - first) * kRow;
        const bool sel = i == s.cursor;

        if (sel) {
            p.fill(6, y - 2, p.width() - 12, 11, rgba(36, 36, 70));
            p.draw_text(10, y, ">", kSelected);
        }

        const std::string number = (i < 10 ? " " : "") + std::to_string(i);
        const char* name = fe.level_names[i] != nullptr ? fe.level_names[i] : "-";
        p.draw_text(20, y, number + "  " + name, sel ? kSelected : kText);
    }
#if defined(__ANDROID__)
    p.draw_text(10, p.height() - 12, "X START   TRIANGLE CLOSE", kHint);  // no keyboard to name
#else
    p.draw_text(10, p.height() - 12, "X/RETURN START   TRIANGLE/F1 CLOSE", kHint);
#endif
    return p;
}

}  // namespace

bool is_open() {
    return s.open;
}

bool event(const SDL_Event& e, int loaded_overlay) {
    if (e.type != SDL_EVENT_KEY_DOWN || e.key.repeat) {
        return false;
    }

    const SDL_Keycode key = e.key.key;

    if (!s.open) {
        if (key == SDLK_F1 && available(loaded_overlay) && !s.launched) {
            s.open = true;
            return true;
        }

        return false;
    }

    switch (key) {
        case SDLK_F1:
        case SDLK_BACKSPACE:
            s.open = false;
            return true;
        case SDLK_UP:
            move(-1);
            return true;
        case SDLK_DOWN:
            move(1);
            return true;
        case SDLK_PAGEUP:
            move(-5);
            return true;
        case SDLK_PAGEDOWN:
            move(5);
            return true;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            s.start = s.cursor;
            s.open = false;
            return true;
        default:
            // The game's keys stay with the menu while it is open.
            return true;
    }
}

void filter_pad(std::uint16_t& buttons, int loaded_overlay) {
    const std::uint16_t pressed = static_cast<std::uint16_t>(~buttons & s.previous);  // newly down
    s.previous = buttons;
    constexpr std::uint16_t kSelect = 0x0001, kUp = 0x0010, kDown = 0x0040, kCross = 0x4000,
                            kTriangle = 0x1000;

    if (s.launched) {
        // The game's menu must not act on presses while the level starts (a Cross there broke
        // the start on both discs); once the menu is gone the presses are the game's again, so
        // Start still skips the flight.
        if (page::on_page()) {
            buttons = 0xFFFF;
        }

        return;
    }

    if (!s.open && available(loaded_overlay) && page::on_page() && (pressed & kCross) != 0) {
        // Cross on a level of the Planets page: the game's list ignores the item's action.
        s.start = page::cursor();
        s.native = true;
        buttons = static_cast<std::uint16_t>(buttons | kCross);
        return;
    }

    if (!s.open) {
        if ((pressed & kSelect) != 0 && available(loaded_overlay)
            && loaded_overlay == OPENRAC_OVERLAY_EXE) {
            s.open = true;
            buttons = 0xFFFF;
        }

        return;
    }

    if (!available(loaded_overlay)) {
        s.open = false;
        return;
    }

    if ((pressed & kUp) != 0) {
        move(-1);
    }

    if ((pressed & kDown) != 0) {
        move(1);
    }

    if ((pressed & (kTriangle | kSelect)) != 0) {
        s.open = false;
    }

    if ((pressed & kCross) != 0) {
        s.start = s.cursor;
        s.open = false;
    }

    buttons = 0xFFFF;
}

void update(int loaded_overlay, std::uint64_t frame) {
    // Unset, the menu is never opened by script.
    static const std::int64_t open_at = env::integer(env::Var::DebugMenu, -1);
    static bool opened_by_script = false;
    // The program a level was started from: its leaving ends the launch.
    static int launched_from = OPENRAC_OVERLAY_EXE;

    if (!opened_by_script && open_at >= 0 && frame >= static_cast<std::uint64_t>(open_at)
        && available(loaded_overlay)) {
        opened_by_script = true;
        s.open = true;
    }

    if (s.open && !available(loaded_overlay)) {
        s.open = false;
    }

    const bool over_front_end = available(loaded_overlay);

    if (loaded_overlay != launched_from) {
        s.launched = false;
    }

    if (over_front_end && loaded_overlay == OPENRAC_OVERLAY_EXE) {
        page::patch();
    }

    if (s.start < 0 || s.launched) {
        s.start = -1;
        s.native = false;
        return;
    }

    const int level = s.start;
    const bool native = s.native;
    s.start = -1;
    s.native = false;

    if (over_front_end) {
        if (native) {
            page::confirm_sound();
        }

        start_level(level);
        s.launched = true;
        launched_from = loaded_overlay;
    }
}

void draw(unsigned framebuffer, int width, int height) {
    if (!s.open || width <= 0 || height <= 0) {
        return;
    }

    // The menu is only open in a game with a front end (available()).
    const renderer::ui::Canvas p = make_picture(*front_end());
    // The picture in the chip's pixels: the frame is the game's 512 x 448 scaled.
    const float sx = static_cast<float>(width) / 512.0f;
    const float sy = static_cast<float>(height) / 448.0f;
    const int w = static_cast<int>(static_cast<float>(p.width()) * sx);
    const int h = static_cast<int>(static_cast<float>(p.height()) * sy);
    const renderer::ui::PictureRect centred{(width - w) / 2, (height - h) / 2, w, h};
    s.blit.draw(framebuffer, {p.pixels(), p.width(), p.height()}, centred);
}

void release() {
    s.blit.release();
}

}  // namespace openrac::frontend::debug_menu
