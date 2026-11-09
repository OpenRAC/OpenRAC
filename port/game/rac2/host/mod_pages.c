/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The mods' pages in Going Commando's menus (openrac_api.ui_main_item, ui_page; game_host.h,
 * openrac_mod_pages_host): the port keeps what they show (game/common/mod_ui.cpp), this puts their
 * items in the title's main menu and draws them.
 *
 * The main menu's enter (func_002D5748) gives its list (the front end's block + 0x3E430, the big
 * list func_00341F38 draws, a row per item at a fixed step) the items at D_00261540, 20 bytes each
 * ({scale, text id, action, argument, 0}, ended by one without a text, as the Options items):
 * New Game (action 4), Load (5) and Options (3, the Options page). The main menu's tick
 * (func_002D55E0) hands the item under the cursor to MenuDoAction (func_002D5B58), whose action 3
 * makes its argument the page shown next (g_menuScreenBlock + 0x18). Here the list gets a copy of
 * the game's items with each mod's after the one it names, of action 3, its text an id no text of
 * the disc has (rac2_mod_pages_text below, through the text lookup's wrap in options.c), opening a
 * page of its own: a copy of the 16:9 page's description (the first Options item's, as the
 * developer mode's Planets page is, options.c), its parent (+0xE0) the main menu's page. While that
 * copy is the page shown, the 16:9 page's object draws the mod's page in its panel (options.c's
 * wraps of its update and draw call rac2_mod_pages_tick and rac2_mod_pages_draw): the title, a
 * window of its rows with the cursor's brighter, the footer, or a question. The port has the pad
 * then (mod_ui.cpp's filter_pad): the game sees no button, so its Triangle never takes the page
 * back; going back is the port's, the parent made the page shown next.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/openrac_mod.h"
#include "rac2.h"

/* g_menuScreenBlock: + 0x14 the page shown, + 0x18 the next. */
#define MENU_SCREEN OPENRAC_DATA(RAC2_ADDR(0x001F2840u))
#define GUI OPENRAC_DATA(RAC2_ADDR(0x001A8CFCu)) /* g_guiInstance: the front end's block */
#define MAIN_ITEMS OPENRAC_DATA(RAC2_ADDR(0x00261540u))
#define OPTIONS_ITEMS OPENRAC_DATA(RAC2_ADDR(0x00261590u))
#define MAIN_LIST_OFFSET RAC2_K(MAIN_LIST_OFFSET, 0x3E430u, func_002D5748)
#define ITEM_BYTES 20u
#define ITEM_TEXT 4u
#define ITEM_ACTION 8u
#define ITEM_ARGUMENT 12u
#define ACTION_PAGE 3
#define PAGE_BYTES 0x130u /* a page's description: 14 boxes, then parent, kind and objects */
#define PAGE_PARENT 0xE0u
#define GAME_ITEMS 3u
#define MAX_ITEMS (GAME_ITEMS + OPENRAC_UI_MAIN_ITEMS_MAX)
/* The texts' ids: past the disc's and below options.c's (0x7F01 on): the main menu's labels, the
 * pause menu's labels and their descriptions, a pause page's title. */
#define TEXT_FIRST 0x7E80
#define TEXT_PAUSE (TEXT_FIRST + OPENRAC_UI_MAIN_ITEMS_MAX)
#define TEXT_PAUSE_HELP (TEXT_PAUSE + OPENRAC_UI_PAUSE_ITEMS_MAX)
#define TEXT_PAUSE_TITLE (TEXT_PAUSE_HELP + OPENRAC_UI_PAUSE_ITEMS_MAX)
#define LABEL_BYTES 48u

/* func_002E5E58's sounds, as the menus' pages play them: the cursor, a choice, a refusal. */
#define SOUND_MOVE 3
#define SOUND_CHOOSE 4
#define SOUND_DENIED 5

/* The page's layout (the screen's 512 by 416, x the middle): the rows in the panel, every one of
 * them (a page has at most 12), in the menus' first font (func_0027FAA8) up to ROWS_LARGE of them,
 * closer in the second (func_0027FB40) past that; the footer in the hint box under the panel, whose
 * own lines are emptied (options.c), in the second font. */
#define PAGE_MIDDLE 0x100
#define ROWS_LARGE 8
#define ROW_TOP 112
#define ROW_STEP 20
#define ROWS_AREA 160 /* from ROW_TOP - 8 to the panel's foot */
#define FOOTER_TOP 300
#define FOOTER_STEP 16
#define ROW_CHOSEN 0x80F0F0F0ull
#define ROW_OTHER 0x70808080ull
#define ROW_DIM 0x40505050ull
#define ROW_TEXT 0x70B0B0A0ull
#define LINE_BYTES 160u

static gaddr main_items; /* the game's items and the mods', ended by a zero item */
static gaddr pages[OPENRAC_UI_MAIN_ITEMS_MAX];
static gaddr pause_pages[OPENRAC_UI_PAUSE_ITEMS_MAX];
static gaddr labels;      /* each mod item's label, LABEL_BYTES apiece */
static gaddr pause_texts; /* the pause items' labels, their descriptions, a page's title */
static gaddr line;        /* a line being printed */
static int mod_items;

static int title_up(void) {
    return openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE && GREF(gaddr, GUI) != 0;
}

/* A level's pause menu: its program loaded, its front end's block there. */
static int pause_up(void) {
    const int overlay = openrac_game_loaded_overlay();
    return overlay != OPENRAC_OVERLAY_EXE && overlay >= 0 && GREF(gaddr, GUI) != 0;
}

static gaddr pause_text(int id) {
    if (pause_texts == 0) {
        pause_texts = openrac_guest_static(LABEL_BYTES * (2 * OPENRAC_UI_PAUSE_ITEMS_MAX + 1));
        memset(G(pause_texts), 0, LABEL_BYTES * (2 * OPENRAC_UI_PAUSE_ITEMS_MAX + 1));
    }
    return pause_texts + LABEL_BYTES * (gaddr)(id - TEXT_PAUSE);
}

int rac2_mod_pause_items(void) {
    const int n = openrac_mod_pages_items(1);
    return n < OPENRAC_UI_PAUSE_ITEMS_MAX ? n : OPENRAC_UI_PAUSE_ITEMS_MAX;
}

int rac2_mod_pause_label_text(int index) {
    return TEXT_PAUSE + index;
}

int rac2_mod_pause_help_text(int index) {
    return TEXT_PAUSE_HELP + index;
}

void rac2_mod_pause_page(int index, gaddr page) {
    if (index >= 0 && index < OPENRAC_UI_PAUSE_ITEMS_MAX) {
        pause_pages[index] = page;
    }
}

uint32_t rac2_mod_pause_title(void) {
    return pause_text(TEXT_PAUSE_TITLE);
}

/* The game's items with the mods' after the ones they name, once (the mods' items are made at
 * their load, before the title). */
static void build(void) {
    if (main_items != 0) {
        return;
    }
    mod_items = openrac_mod_pages_items(0);
    if (mod_items > OPENRAC_UI_MAIN_ITEMS_MAX) {
        mod_items = OPENRAC_UI_MAIN_ITEMS_MAX;
    }
    if (mod_items <= 0) {
        return;
    }
    unsigned count = 0;
    while (count < GAME_ITEMS && GREF(int32_t, MAIN_ITEMS + ITEM_BYTES * count + ITEM_TEXT) != 0) {
        ++count;
    }
    const gaddr options = GREF(gaddr, OPTIONS_ITEMS + ITEM_ARGUMENT); /* the 16:9 page */
    const gaddr options_page = GREF(gaddr, MAIN_ITEMS + ITEM_BYTES * (count - 1) + ITEM_ARGUMENT);
    if (count != GAME_ITEMS || GREF(int32_t, OPTIONS_ITEMS + ITEM_ACTION) != ACTION_PAGE
        || GREF(int32_t, MAIN_ITEMS + ITEM_BYTES * 2 + ITEM_ACTION) != ACTION_PAGE || options == 0
        || options_page == 0) {
        openrac_game_warn("rac2 mod pages: the main menu's items are not the ones expected");
        mod_items = 0;
        return;
    }
    labels = openrac_guest_static(LABEL_BYTES * OPENRAC_UI_MAIN_ITEMS_MAX);
    line = openrac_guest_static(LINE_BYTES);
    main_items = openrac_guest_static(ITEM_BYTES * (MAX_ITEMS + 1));
    memset(G(main_items), 0, ITEM_BYTES * (MAX_ITEMS + 1));
    unsigned at = 0;
    for (unsigned game = 0; game < GAME_ITEMS; ++game) {
        memcpy(G(main_items + ITEM_BYTES * at), G(MAIN_ITEMS + ITEM_BYTES * game), ITEM_BYTES);
        ++at;
        for (int i = 0; i < mod_items; ++i) {
            if (openrac_mod_pages_after(i) != (int)game) {
                continue;
            }
            pages[i] = openrac_guest_static(PAGE_BYTES);
            memcpy(G(pages[i]), G(options), PAGE_BYTES);
            /* Back from it is to the main menu, the Options page's parent. */
            GREF(gaddr, pages[i] + PAGE_PARENT) = GREF(gaddr, options_page + PAGE_PARENT);
            const gaddr item = main_items + ITEM_BYTES * at;
            memcpy(G(item), G(MAIN_ITEMS + ITEM_BYTES * game), ITEM_BYTES); /* its scale */
            GREF(int32_t, item + ITEM_TEXT) = TEXT_FIRST + i;
            GREF(int32_t, item + ITEM_ACTION) = ACTION_PAGE;
            GREF(gaddr, item + ITEM_ARGUMENT) = pages[i];
            ++at;
        }
    }
}

/* The main menu's enter: its list gets the items with the mods'. */
int func_002D5748(void) {
    const int result = func_002D5748__game();
    if (title_up()) {
        build();
        if (main_items != 0) {
            func_00341EB0(GREF(gaddr, GUI) + MAIN_LIST_OFFSET, main_items);
        }
    }
    return result;
}

uint32_t rac2_mod_pages_text(int id) {
    if (id >= TEXT_PAUSE && id < TEXT_PAUSE_HELP + rac2_mod_pause_items()) {
        const int index = id < TEXT_PAUSE_HELP ? id - TEXT_PAUSE : id - TEXT_PAUSE_HELP;
        const gaddr text = pause_text(id);
        if (!openrac_mod_pages_label(1, index, (char*)G(text), (int)LABEL_BYTES)) {
            return 0;
        }
        return text;
    }
    if (labels == 0 || id < TEXT_FIRST || id >= TEXT_FIRST + mod_items) {
        return 0;
    }
    const gaddr label = labels + LABEL_BYTES * (gaddr)(id - TEXT_FIRST);
    if (!openrac_mod_pages_label(0, id - TEXT_FIRST, (char*)G(label), (int)LABEL_BYTES)) {
        return 0;
    }
    return label;
}

/* The page of item `index` is the page shown, or the next one (its switch). */
static int page_is(gaddr page, unsigned at) {
    return page != 0 && GREF(gaddr, MENU_SCREEN + at) == page;
}

static int shown(void) {
    if (pause_up()) {
        for (int i = 0; i < OPENRAC_UI_PAUSE_ITEMS_MAX; ++i) {
            if (page_is(pause_pages[i], 0x14u)) {
                return OPENRAC_MOD_PAGES_PAUSE + i;
            }
        }
        return -1;
    }
    if (!title_up()) {
        return -1;
    }
    for (int i = 0; i < mod_items; ++i) {
        if (page_is(pages[i], 0x14u)) {
            return i;
        }
    }
    return -1;
}

/* A mod's page drawn now: shown, or the next (as it flies in). */
static int drawn(void) {
    if (pause_up()) {
        for (int i = 0; i < OPENRAC_UI_PAUSE_ITEMS_MAX; ++i) {
            if (page_is(pause_pages[i], 0x14u) || page_is(pause_pages[i], 0x18u)) {
                return OPENRAC_MOD_PAGES_PAUSE + i;
            }
        }
        return -1;
    }
    if (!title_up()) {
        return -1;
    }
    for (int i = 0; i < mod_items; ++i) {
        if (page_is(pages[i], 0x14u) || page_is(pages[i], 0x18u)) {
            return i;
        }
    }
    return -1;
}

static int main_menu(void) {
    if (!title_up() || mod_items <= 0) {
        return 0;
    }
    const gaddr options_page = GREF(gaddr, MAIN_ITEMS + ITEM_BYTES * 2 + ITEM_ARGUMENT);
    const gaddr main = options_page != 0 ? GREF(gaddr, options_page + PAGE_PARENT) : 0;
    const gaddr next = GREF(gaddr, MENU_SCREEN + 0x18u);
    return main != 0 && page_is(main, 0x14u) && (next == 0 || next == main);
}

static void open_main(int index) {
    if (index >= 0 && index < mod_items && pages[index] != 0) {
        GREF(gaddr, MENU_SCREEN + 0x18u) = pages[index];
    }
}

static void go_back(void) {
    const int item = shown();
    const gaddr page = item >= OPENRAC_MOD_PAGES_PAUSE ? pause_pages[item - OPENRAC_MOD_PAGES_PAUSE]
                       : item >= 0                     ? pages[item]
                                                       : 0;
    if (page != 0) {
        GREF(gaddr, MENU_SCREEN + 0x18u) = GREF(gaddr, page + PAGE_PARENT);
    }
}

static void sound(int cue) {
    func_002E5E58(
        cue == OPENRAC_UI_SOUND_CURSOR || cue == OPENRAC_UI_SOUND_PAGE ? SOUND_MOVE
        : cue == OPENRAC_UI_SOUND_DENIED                               ? SOUND_DENIED
                                                                       : SOUND_CHOOSE,
        0,
        0
    );
}

static const openrac_mod_pages_host host = {
    .shown = shown,
    .main_menu = main_menu,
    .open_main = open_main,
    .go_back = go_back,
    .sound = sound,
};

void rac2_install_mod_pages(void) {
    openrac_game_mod_pages = &host;
}

/* The 16:9 page object's (the title's) or the Camera page object's (a level's pause menu) update
 * while a mod's page is drawn: 1 when it is (the panel's move only, the buttons are the port's). */
int rac2_mod_pages_tick(void) {
    return drawn() >= 0;
}

static void print(int x, int y, unsigned long long colour, const char* text, int small) {
    snprintf((char*)G(line), LINE_BYTES, "%s", text);
    if (small) {
        func_0027FB40(x, y, colour, line, -1);
    } else {
        func_0027FAA8(x, y, colour, line, -1);
    }
}

/* The page's draw while a mod's page is drawn: its title into `title` (the panel's), its rows and
 * footer, or the question. 1 when it is. */
int rac2_mod_pages_draw(char* title, unsigned title_bytes) {
    if (drawn() < 0) {
        return 0;
    }
    static openrac_mod_pages_view view;
    if (!openrac_mod_pages_view_of(&view)) {
        /* Flying in before the port has made it the page shown: the title only. */
        return 1;
    }
    snprintf(title, title_bytes, "%s", view.title);
    if (view.asking) {
        print(PAGE_MIDDLE, ROW_TOP + 2 * ROW_STEP, ROW_CHOSEN, view.question, 0);
        print(PAGE_MIDDLE, FOOTER_TOP + FOOTER_STEP / 2, ROW_TEXT, "\x10 Yes   \x12 No", 1);
        return 1;
    }
    const int large = view.rows <= ROWS_LARGE;
    const int step = large ? ROW_STEP : ROWS_AREA / (view.rows > 0 ? view.rows : 1);
    const int first = large ? ROW_TOP : ROW_TOP - 8 + step / 2;
    char text[LINE_BYTES];
    for (int at = 0; at < view.rows; ++at) {
        const openrac_mod_pages_row* row = &view.row[at];
        if (row->value[0] != 0) {
            snprintf(text, sizeof text, "%s: %s", row->label, row->value);
        } else {
            snprintf(text, sizeof text, "%s", row->label);
        }
        const unsigned long long colour = (row->flags & OPENRAC_MOD_PAGES_ROW_DISABLED) ? ROW_DIM
                                          : (row->flags & OPENRAC_MOD_PAGES_ROW_TEXT)   ? ROW_TEXT
                                          : at == view.cursor                           ? ROW_CHOSEN
                                                                                        : ROW_OTHER;
        print(PAGE_MIDDLE, first + step * at, colour, text, !large);
    }
    for (int i = 0; i < view.footer_lines; ++i) {
        print(PAGE_MIDDLE, FOOTER_TOP + FOOTER_STEP * i, ROW_TEXT, view.footer[i], 1);
    }
    /* A page without a footer still says how to go back, in the hint box the game's lines left. */
    if (view.footer_lines == 0) {
        print(PAGE_MIDDLE, FOOTER_TOP + FOOTER_STEP / 2, ROW_TEXT, "\x10 Select   \x12 Back", 1);
    }
    return 1;
}
