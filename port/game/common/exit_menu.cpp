// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "exit_menu.h"

#include <cstdio>
#include <cstring>

#include <SDL3/SDL.h>

#include "common/log.h"
#include "options_menu.h"

namespace openrac::frontend::exit_menu {

namespace {

constexpr int kTextFirst = 0x7D00;
constexpr gaddr kTextBytes = 64;

// The page menu (mod_ui.cpp describes it): its state (1 while a page switch runs), the page shown
// and the page asked for; a page's back, focus and screens; a text list's flags, items, up and
// down, cursor and scroll; a label's text.
constexpr gaddr kState = 0x00;
constexpr gaddr kCurrent = 0x04;
constexpr gaddr kNext = 0x08;
constexpr std::int32_t kSwitching = 1;
constexpr gaddr kBack = 0x38;
constexpr gaddr kFocus = 0x40;
constexpr gaddr kScreens = 0x44;
constexpr gaddr kPending = 0x80;
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
constexpr gaddr kItemBytes = 12;
constexpr std::int16_t kIgnoredAction = 0x7F;  // drawn as any item; the game does nothing on it

// The first pad's buttons pressed this tick (openrac_front_end.pad + 0x1C4), as the game's menus
// test them: Cross.
constexpr gaddr kPadPressed = 0x1C4;
constexpr std::uint32_t kPadCross = 0x40;
constexpr int kConfirmSound = 0x11;

constexpr int kYesRow = 0;
constexpr int kNoRow = 1;

struct State {
    gaddr texts = 0;  // kTextCount strings of kTextBytes
    gaddr page = 0, title = 0, list = 0, items = 0;
    bool armed = false;  // Cross let go since the page showed: a press is the player's answer
};
State s;

gaddr word(gaddr at) {
    return GREF(std::uint32_t, at);
}

void put_item(gaddr at, int label) {
    GREF(std::int16_t, at) = static_cast<std::int16_t>(label);
    GREF(std::int16_t, at + 2) = kIgnoredAction;
    GREF(std::uint32_t, at + 4) = 0;
    GREF(std::int16_t, at + 8) = 0;
    GREF(std::int16_t, at + 10) = 0;
}

}  // namespace

int text_id(Text text) {
    return kTextFirst + static_cast<int>(text);
}

std::uint32_t text(int id) {
    if (id < kTextFirst || id >= kTextFirst + kTextCount) {
        return 0;
    }
    if (s.texts == 0) {
        s.texts = openrac_guest_static(kTextBytes * kTextCount);
    }
    // In the language the game has now: the player may have changed it since.
    const int index = id - kTextFirst;
    const gaddr at = s.texts + kTextBytes * static_cast<gaddr>(index);
    const std::string shown = options_menu::game_text(kWords[index].text[options_menu::language()]);
    std::snprintf(static_cast<char*>(G(at)), kTextBytes, "%s", shown.c_str());
    return at;
}

// The Language page with other data, as the developer menu's Planets page (debug_menu.cpp): the
// title the question, the list Yes and No under the main menu. The list keeps the game's update
// (the cursor, Triangle back to the main menu) but not its enter, which would put its cursor on the
// language's row.
gaddr title_page(const openrac_front_end& fe) {
    if (s.page != 0) {
        return s.page;
    }
    s.page = openrac_guest_static(0xA0);
    s.title = openrac_guest_static(0x60);
    s.list = openrac_guest_static(0x60);
    s.items = openrac_guest_static(kItemBytes * 3);
    std::memcpy(G(s.page), G(fe.language_page), 0xA0);
    std::memcpy(G(s.title), G(fe.language_title), 0x60);
    std::memcpy(G(s.list), G(fe.language_list), 0x60);
    std::memset(G(s.items), 0, kItemBytes * 3);
    put_item(s.items, text_id(kYes));
    put_item(s.items + kItemBytes, text_id(kNo));
    GREF(std::int32_t, s.title + kLabelText) = text_id(kAsk);
    // Rows as high as the font (0x10), centred (0x400), wrapping (0x1000), scrolled by whole rows
    // (0x8000), as Planets'.
    GREF(std::uint32_t, s.list + kListFlags) = 0x10u | 0x400u | 0x1000u | 0x8000u;
    GREF(std::uint32_t, s.list + kListItems) = s.items;
    GREF(std::uint32_t, s.list + kListUp) = 0;
    GREF(std::uint32_t, s.list + kListDown) = 0;
    GREF(std::int32_t, s.list + kListCursor) = kNoRow;
    GREF(std::int32_t, s.list + kListScroll) = 0;
    GREF(std::uint32_t, s.list + kEnter) = 0;
    GREF(std::uint32_t, s.list + kLeave) = 0;
    GREF(std::uint32_t, s.page + kScreens + 4 * 2) = s.title;
    GREF(std::uint32_t, s.page + kScreens + 4 * 3) = s.list;
    GREF(std::uint32_t, s.page + kFocus) = s.list;
    GREF(std::uint32_t, s.page + kPending) = 0;
    GREF(std::uint32_t, s.page + kBack) = fe.main_page;
    return s.page;
}

void update_title(const openrac_front_end& fe) {
    if (s.page == 0) {
        return;
    }
    const gaddr menu = fe.page_menu;
    // Elsewhere: the page opens on No, and waits for Cross to be let go.
    if (word(menu + kCurrent) != s.page) {
        GREF(std::int32_t, s.list + kListCursor) = kNoRow;
        s.armed = false;
        return;
    }
    if (GREF(std::int32_t, menu + kState) == kSwitching || word(s.page + kFocus) != s.list) {
        return;
    }
    if ((word(fe.pad + kPadPressed) & kPadCross) == 0) {
        s.armed = true;
        return;
    }
    if (!s.armed) {
        return;
    }
    s.armed = false;
    if (fe.menu_sound != nullptr) {
        fe.menu_sound(kConfirmSound, word(s.list + kMoby));
    }
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
