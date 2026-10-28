// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "mod_menu.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "common/log.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/openrac_mod.h"
#include "renderer/ui/canvas.h"
#include "renderer/ui/picture_blit.h"

namespace openrac::frontend::mod_menu {

namespace {

// The entries' text ids: past the game's (under 0x7000) and the options page's (0x7E00 on).
constexpr int kTextFirst = 0x7F00;
// A label's room in game memory, its end included (menu_entry keeps 31 characters).
constexpr gaddr kTextBytes = 32;
// An Options list item: label, action, page, sublabel, timer.
constexpr gaddr kItemBytes = 12;
// The Options list's items before ours, at most (Language, Video, Sound, Planets).
constexpr int kMaxItems = 8;
// The game's list actions: open the page in the item's word; one the list ignores.
constexpr std::int16_t kOpenPage = 3;
constexpr std::int16_t kIgnoredAction = 0x7F;
// What is copied of the Language page, its title and its list (debug_menu.cpp's sizes).
constexpr gaddr kPageBytes = 0xA0;
constexpr gaddr kWidgetBytes = 0x60;
// A page's focus and widgets; a list's flags, items and cursor; a title's text (debug_menu.cpp).
constexpr gaddr kPageFocus = 0x40;
constexpr gaddr kPageWidgets = 0x44;
constexpr gaddr kListFlags = 0x30;
constexpr gaddr kListItems = 0x34;
constexpr gaddr kListCursor = 0x40;
constexpr gaddr kTitleText = 0x34;
// The list's flags: rows as high as the font (0x10), centred (0x400).
constexpr std::uint32_t kListRows = 0x10u | 0x400u;
// Pad 0's Triangle, active low, and how many ticks a press the game missed waits to be tried again.
constexpr std::uint16_t kTriangle = 0x1000;
constexpr int kRetryTicks = 60;
// The page's colour where the mod draws nothing.
constexpr std::uint32_t kBlank = renderer::ui::rgba(0, 0, 0);

// One entry's copy of the Language page, and its label in game memory.
struct Page {
    gaddr page = 0, title = 0, list = 0, item = 0, label = 0;
};

struct State {
    std::array<Page, OPENRAC_MENU_ENTRIES_MAX> pages{};
    int made = 0;                     // how many of pages are made
    gaddr items = 0;                  // the Options list's items with the entries after them
    gaddr items_for = 0;              // the items those were made from
    int open = -1;                    // the entry whose page is shown, or -1
    bool opened = false;              // the open page's callback has not run yet
    bool closing = false;             // the mod closed its page: Triangle is pressed for the game
    int press_in = 0;                 // ticks until Triangle is pressed (again)
    bool pressing = false;            // Triangle is held for the game this tick
    std::uint16_t previous = 0xFFFF;  // pad 0 as last seen (active low)
    std::uint32_t held = 0;           // the OPENRAC_PAD_* buttons down now
    std::uint32_t pressed = 0;        // those that went down since the callback last ran
    renderer::ui::Canvas canvas{OPENRAC_MENU_WIDTH, OPENRAC_MENU_HEIGHT, kBlank};
    // The last page drawn, for the window's draw (which may run on another thread).
    std::mutex shown_lock;
    std::vector<std::uint8_t> shown;
    renderer::ui::PictureBlit blit;
};

State s;

// The game's front end when it has the Language page the entries' pages are copied from.
const openrac_front_end* front_end() {
    const openrac_front_end* fe = openrac_game_front_end;
    return fe != nullptr && fe->language_page != 0 && fe->options_list != 0 ? fe : nullptr;
}

// An entry's label as the game's text: ASCII, anything else a question mark.
void put_label(gaddr at, const std::string& label) {
    std::string ascii = label;
    std::replace_if(ascii.begin(), ascii.end(), [](char c) { return c < ' ' || c > '~'; }, '?');
    std::snprintf(static_cast<char*>(G(at)), kTextBytes, "%s", ascii.c_str());
}

void put_item(gaddr at, std::int16_t label, std::int16_t action, std::uint32_t arg) {
    GREF(std::int16_t, at) = label;
    GREF(std::int16_t, at + 2) = action;
    GREF(std::uint32_t, at + 4) = arg;
    GREF(std::int16_t, at + 8) = 0;
    GREF(std::int16_t, at + 10) = 0;
}

// Makes the pages of the entries not made yet: the Language page, title and list copied, the title
// the entry's label, the list one item the list ignores.
void build(const openrac_front_end& fe) {
    const int count = std::min(mods::menu_entry_count(), OPENRAC_MENU_ENTRIES_MAX);

    for (; s.made < count; ++s.made) {
        Page& p = s.pages[static_cast<std::size_t>(s.made)];
        const auto id = static_cast<std::int16_t>(kTextFirst + s.made);
        p.page = openrac_guest_static(kPageBytes);
        p.title = openrac_guest_static(kWidgetBytes);
        p.list = openrac_guest_static(kWidgetBytes);
        p.item = openrac_guest_static(kItemBytes * 2);
        p.label = openrac_guest_static(kTextBytes);
        put_label(p.label, mods::menu_entry_label(s.made));
        std::memcpy(G(p.page), G(fe.language_page), kPageBytes);
        std::memcpy(G(p.title), G(fe.language_title), kWidgetBytes);
        std::memcpy(G(p.list), G(fe.language_list), kWidgetBytes);
        std::memset(G(p.item), 0, kItemBytes * 2);
        put_item(p.item, id, kIgnoredAction, 0);
        GREF(std::int32_t, p.title + kTitleText) = id;
        GREF(std::uint32_t, p.list + kListFlags) = kListRows;
        GREF(std::uint32_t, p.list + kListItems) = p.item;

        // No list above or below, the cursor on the item, as debug_menu.cpp's Planets list.
        for (gaddr at = kListItems + 4; at <= kListCursor + 4; at += 4) {
            GREF(std::uint32_t, p.list + at) = 0;
        }

        GREF(std::uint32_t, p.page + kPageWidgets + 4 * 2) = p.title;
        GREF(std::uint32_t, p.page + kPageWidgets + 4 * 3) = p.list;
        GREF(std::uint32_t, p.page + kPageFocus) = p.list;
    }
}

// Whether the items at `items` already open the first entry's page.
bool lists_entries(gaddr items) {
    for (int i = 0; i < kMaxItems + OPENRAC_MENU_ENTRIES_MAX; ++i) {
        const gaddr at = items + kItemBytes * static_cast<gaddr>(i);

        // The list ends at an item without a label.
        if (GREF(std::int16_t, at) == 0) {
            return false;
        }

        // The item that opens the first entry's page is ours.
        if (GREF(std::int16_t, at + 2) == kOpenPage
            && GREF(std::uint32_t, at + 4) == s.pages[0].page) {
            return true;
        }
    }

    return false;
}

// The Options list shows the entries after whatever items it was given (the game's, or the port's
// with Video and Planets): a copy of those with ours after them, made again when they change.
void list_entries(const openrac_front_end& fe) {
    const gaddr shown = GREF(std::uint32_t, fe.options_list + kListItems);

    // A list given nothing, or ours, needs nothing.
    if (shown == 0 || shown == s.items || lists_entries(shown)) {
        return;
    }

    if (s.items_for != shown) {
        int count = 0;

        while (count < kMaxItems
               && GREF(std::int16_t, shown + kItemBytes * static_cast<gaddr>(count)) != 0) {
            ++count;
        }

        const gaddr own = kItemBytes * static_cast<gaddr>(count);
        s.items_for = shown;
        s.items = openrac_guest_static(own + kItemBytes * static_cast<gaddr>(s.made + 1));
        std::memcpy(G(s.items), G(shown), own);

        for (int i = 0; i < s.made; ++i) {
            const Page& p = s.pages[static_cast<std::size_t>(i)];
            put_item(
                s.items + own + kItemBytes * static_cast<gaddr>(i),
                static_cast<std::int16_t>(kTextFirst + i),
                kOpenPage,
                p.page
            );
        }

        std::memset(G(s.items + own + kItemBytes * static_cast<gaddr>(s.made)), 0, kItemBytes);
    }

    GREF(std::uint32_t, fe.options_list + kListItems) = s.items;
}

// The entry whose page the game shows now, or -1.
int shown_entry(const openrac_front_end& fe) {
    const gaddr page = GREF(std::uint32_t, fe.page_menu + 4);

    for (int i = 0; i < s.made; ++i) {
        // The game shows one of the entries' pages.
        if (page != 0 && page == s.pages[static_cast<std::size_t>(i)].page) {
            return i;
        }
    }

    return -1;
}

// Keeps the page drawn for the window's draw; nothing when no page is open.
void publish(bool open) {
    const std::lock_guard lock(s.shown_lock);

    if (open) {
        s.shown = s.canvas.pixels();
    } else {
        s.shown.clear();
    }
}

}  // namespace

void update(int loaded_overlay) {
    const openrac_front_end* fe = front_end();

    // Only the boot program (the title and its menus) has the Options list the entries are in.
    if (fe == nullptr || loaded_overlay != OPENRAC_OVERLAY_EXE || mods::menu_entry_count() == 0) {
        if (s.open >= 0) {
            publish(false);
        }

        s.open = -1;
        s.closing = false;
        return;
    }

    build(*fe);
    list_entries(*fe);
    const int now = shown_entry(*fe);

    // Another page (or none of ours) since the last frame: a new page starts afresh.
    if (now != s.open) {
        s.open = now;
        s.opened = true;
        s.closing = false;
        s.pressed = 0;

        if (now >= 0) {
            log::info("mod menu: page \"{}\" opened", mods::menu_entry_label(now));
        }
    }

    // No page of a mod is shown, or its mod closed it and the game is going back.
    if (s.open < 0 || s.closing) {
        publish(s.open >= 0);
        return;
    }

    openrac_menu_frame frame{};
    frame.size = sizeof frame;
    frame.held = s.held;
    frame.pressed = s.pressed;
    frame.opened = s.opened ? 1u : 0u;
    s.pressed = 0;
    s.opened = false;
    s.canvas.clear(kBlank);

    // The mod closes its page: Triangle, at the next tick, takes the game back.
    if (mods::run_menu_page(s.open, frame)) {
        s.closing = true;
        s.press_in = 0;
    }

    publish(true);
}

void filter_pad(std::uint16_t& buttons) {
    const auto pressed = static_cast<std::uint16_t>(~buttons & s.previous);  // newly down
    s.previous = buttons;

    // With no page of a mod open, the pad is the game's.
    if (s.open < 0) {
        return;
    }

    s.held = static_cast<std::uint16_t>(~buttons);
    s.pressed |= pressed;
    buttons = 0xFFFF;

    // Closing: Triangle down for one tick, then up; pressed again if the game has not gone back.
    if (s.closing) {
        if (s.pressing) {
            s.pressing = false;
        } else if (s.press_in <= 0) {
            buttons = static_cast<std::uint16_t>(~kTriangle);
            s.pressing = true;
            s.press_in = kRetryTicks;
        } else {
            --s.press_in;
        }
    }
}

bool is_open() {
    return s.open >= 0;
}

void draw(unsigned framebuffer, int width, int height) {
    const std::lock_guard lock(s.shown_lock);

    // Nothing to draw without an open page, or on an empty frame.
    if (s.shown.empty() || width <= 0 || height <= 0) {
        return;
    }

    // The page covers the whole frame, stretched.
    s.blit.draw(
        framebuffer, {s.shown, OPENRAC_MENU_WIDTH, OPENRAC_MENU_HEIGHT}, {0, 0, width, height}
    );
}

void release() {
    s.blit.release();
}

std::uint32_t text(int id) {
    const int i = id - kTextFirst;

    // An id of an entry made has its label; any other id is someone else's.
    if (i < 0 || i >= s.made) {
        return 0;
    }

    return s.pages[static_cast<std::size_t>(i)].label;
}

void rect(int x, int y, int width, int height, std::uint32_t rgba) {
    s.canvas.fill(x, y, width, height, rgba);
}

void text_at(int x, int y, int size, std::uint32_t rgba, const char* text) {
    s.canvas.draw_text(x, y, text != nullptr ? text : "", rgba, size);
}

}  // namespace openrac::frontend::mod_menu
