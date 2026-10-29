// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's Exit item (exit_menu.h): its texts, the main menu's question page and the close. The
// pause menu's question is options_menu.cpp's, on that page's own list.

#include "exit_menu.h"

#include <cstdio>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "common/log.h"
#include "options_menu.h"

namespace openrac::frontend::exit_menu {

namespace {

// The texts' ids: past the mods' pages' (0x7C00 on, mod_ui.cpp) and below the options page's
// (0x7E00 on, options_menu.cpp).
constexpr int kTextFirst = 0x7D00;
constexpr gaddr kTextBytes = 64;  // a text's room, its end byte included

// The page menu (rac1-decomp pause.c, include/menu.h; mod_ui.cpp describes it): its state (1 while
// a page switch runs), the page shown and the page asked for.
constexpr gaddr kState = 0x00;
constexpr gaddr kCurrent = 0x04;
constexpr gaddr kNext = 0x08;
constexpr std::int32_t kSwitching = 1;

// A MenuPage: its back, focus, screens (the title in slot 2, the list in slot 3) and pending focus.
constexpr gaddr kBack = 0x38;
constexpr gaddr kFocus = 0x40;
constexpr gaddr kScreens = 0x44;
constexpr gaddr kPending = 0x80;
constexpr gaddr kTitleSlot = 2;
constexpr gaddr kListSlot = 3;

// A MenuScreen: enter, leave and its moby; a text list's flags, items, up and down, cursor and
// scroll; a label's text.
constexpr gaddr kEnter = 0x08;
constexpr gaddr kLeave = 0x0C;
constexpr gaddr kMoby = 0x14;
constexpr gaddr kListFlags = 0x30;
constexpr gaddr kListItems = 0x34;
constexpr gaddr kListUp = 0x38;
constexpr gaddr kListDown = 0x3C;
constexpr gaddr kListCursor = 0x40;
constexpr gaddr kListScroll = 0x44;
constexpr gaddr kLabelText = 0x34;

// What of the Options page and of the Language page's title and list the question page copies (as
// a mod's page, mod_ui.cpp). The Options page's seqs, because going back plays the page's own
// backwards: the Options page's land on the main menu's rows, the Language page's on the Options
// page's frames, which the main menu then kept.
constexpr gaddr kPageCopy = 0x88;
constexpr gaddr kWidgetCopy = 0x60;

// A text list item {label, action, argument, sublabel, timer}, and the action the game's list
// update (update_menu_entry_actions) has no case for.
constexpr gaddr kItemBytes = 12;
constexpr std::int16_t kIgnoredAction = 0x7F;

// The list's flags: rows as high as the font (0x10), centred (0x400), wrapping (0x1000), scrolled
// by whole rows (0x8000), as Planets'.
constexpr std::uint32_t kListLayout = 0x10u | 0x400u | 0x1000u | 0x8000u;

// The first pad's buttons pressed this tick (openrac_front_end.pad + 0x1C4) as the game's menus
// test them: Cross (update_menu_entry_actions' confirm).
constexpr gaddr kPadPressed = 0x1C4;
constexpr std::uint32_t kPadCross = 0x40;

// The menus' confirm sound (openrac_front_end.menu_sound).
constexpr int kConfirmSound = 0x11;

// The question's rows.
constexpr std::int32_t kYesRow = 0;
constexpr std::int32_t kNoRow = 1;

// What the module made in game memory, and the main menu's question's Cross.
struct State {
    gaddr texts = 0;  // kTextCount texts of kTextBytes
    gaddr page = 0, title = 0, list = 0, items = 0;
    CrossPress cross;
};

State s;

// Returns the 32-bit word of game memory at `at`.
gaddr word(gaddr at) {
    return GREF(std::uint32_t, at);
}

}  // namespace

int text_id(Text text) {
    return kTextFirst + static_cast<int>(text);
}

void prepare() {
    // Made once: the room stays the texts' for the whole run.
    if (s.texts == 0) {
        s.texts = openrac_guest_static(kTextBytes * kTextCount);
    }
}

std::uint32_t text(int id) {
    // Another module's id, or the game's.
    if (id < kTextFirst || id >= kTextFirst + kTextCount) {
        return 0;
    }

    prepare();
    const int index = id - kTextFirst;
    const gaddr at = s.texts + kTextBytes * static_cast<gaddr>(index);
    // Written at each lookup: the player may have changed the language since.
    const std::string shown = options_menu::game_text(kWords[index].text[options_menu::language()]);
    std::snprintf(static_cast<char*>(G(at)), kTextBytes, "%s", shown.c_str());
    return at;
}

void put_item(gaddr at, Text label) {
    GREF(std::int16_t, at) = static_cast<std::int16_t>(text_id(label));
    GREF(std::int16_t, at + 2) = kIgnoredAction;
    GREF(std::uint32_t, at + 4) = 0;
    GREF(std::int16_t, at + 8) = 0;
    GREF(std::int16_t, at + 10) = 0;
}

void cue(const openrac_front_end& fe, gaddr widget) {
    // A front end without the menus' sounds stays silent.
    if (fe.menu_sound != nullptr) {
        fe.menu_sound(kConfirmSound, word(widget + kMoby));
    }
}

bool CrossPress::take(const openrac_front_end& fe, bool ready) {
    // Not ready: a release seen before does not count for the page that comes next.
    if (!ready) {
        m_armed = false;
        return false;
    }

    // Let go: the next press is the player's.
    if ((word(fe.pad + kPadPressed) & kPadCross) == 0) {
        m_armed = true;
        return false;
    }

    const bool pressed = m_armed;
    m_armed = false;
    return pressed;
}

gaddr title_page(const openrac_front_end& fe) {
    // Made once, in the boot program's memory, which stays loaded with the title.
    if (s.page != 0) {
        return s.page;
    }

    prepare();
    s.page = openrac_guest_static(kPageCopy);
    s.title = openrac_guest_static(kWidgetCopy);
    s.list = openrac_guest_static(kWidgetCopy);
    s.items = openrac_guest_static(kItemBytes * 3);
    std::memcpy(G(s.page), G(fe.options_page), kPageCopy);
    std::memcpy(G(s.title), G(fe.language_title), kWidgetCopy);
    std::memcpy(G(s.list), G(fe.language_list), kWidgetCopy);
    std::memset(G(s.items), 0, kItemBytes * 3);
    put_item(s.items, kYes);
    put_item(s.items + kItemBytes, kNo);
    GREF(std::int32_t, s.title + kLabelText) = text_id(kAsk);
    // The list keeps the game's update (cursor, Triangle back) but not its enter, which would put
    // the cursor on the language's row.
    GREF(std::uint32_t, s.list + kListFlags) = kListLayout;
    GREF(std::uint32_t, s.list + kListItems) = s.items;
    GREF(std::uint32_t, s.list + kListUp) = 0;
    GREF(std::uint32_t, s.list + kListDown) = 0;
    GREF(std::int32_t, s.list + kListCursor) = kNoRow;
    GREF(std::int32_t, s.list + kListScroll) = 0;
    GREF(std::uint32_t, s.list + kEnter) = 0;
    GREF(std::uint32_t, s.list + kLeave) = 0;
    GREF(std::uint32_t, s.page + kScreens + 4 * kTitleSlot) = s.title;
    GREF(std::uint32_t, s.page + kScreens + 4 * kListSlot) = s.list;
    GREF(std::uint32_t, s.page + kFocus) = s.list;
    GREF(std::uint32_t, s.page + kPending) = 0;
    GREF(std::uint32_t, s.page + kBack) = fe.main_page;
    return s.page;
}

void update_title(const openrac_front_end& fe) {
    // No Exit row in the main menu: nothing to answer.
    if (s.page == 0) {
        return;
    }

    const gaddr menu = fe.page_menu;

    // Another page: the question opens on No next time.
    if (word(menu + kCurrent) != s.page) {
        GREF(std::int32_t, s.list + kListCursor) = kNoRow;
        s.cross.take(fe, false);
        return;
    }

    const bool ready =
        GREF(std::int32_t, menu + kState) != kSwitching && word(s.page + kFocus) == s.list;

    // No new Cross on a settled page.
    if (!s.cross.take(fe, ready)) {
        return;
    }

    cue(fe, s.list);

    // Yes closes the game; No goes back to the main menu, as Triangle does.
    if (GREF(std::int32_t, s.list + kListCursor) == kYesRow) {
        quit("the main menu");
    } else {
        GREF(std::uint32_t, menu + kNext) = fe.main_page;
    }
}

void quit(const char* where) {
    log::info("exit: chosen in {}, closing the game", where);
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}

}  // namespace openrac::frontend::exit_menu
