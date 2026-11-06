/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando's options: the game's own 16:9, which the port keeps off, and the Sound page, which
 * a console forgets at power off and the port remembers between runs.
 *
 * The 16:9 option (the title's Options page, "16:9 (widescreen)", 0x001A7BB1): on, UpdateViewContext
 * narrows the vertical view by a quarter for a television that stretches the picture back. The
 * window's picture has the player's shape (settings `aspect`, the Graphics menu's Picture) and keeps
 * the game's vertical view, wider pictures showing more to the sides; on as well, the world would be
 * squeezed. So the flag is kept off, each frame.
 *
 * The Sound page (func_002D7DB8 draws it): the playback mode (0x001A7B98, a flag: stereo or mono),
 * the music volume (0x001A7B9C) and the sound volume (0x001A7BA0), both 0 to 0x400, the same three
 * words in the same order as Ratchet & Clank's (rac1/host/options.c), under the same keys of the
 * settings file. func_002E4860 hands the volumes to the mixer's groups and the sound's reset
 * (func_002E48E8) both; the remembered values go in before its first run.
 *
 * The language (0x001A7BB4, a byte: the game's own index, which the menus' action type 9 sets) holds
 * its data's value, 0, from the start: no code writes it before a menu does (a watch of it from the
 * first frame through the title and its menus). The remembered index goes in with the Sound page.
 *
 * A loaded save carries all of them: the memory card's load (func_00298F00) copies each block of
 * the save into its place (func_0029B168, rac1's memcard_restore_data, the table at D_00256138),
 * the options as block 0x26, 0x28 bytes from 0x001A7B90 (the Sound page at +8, the language at
 * +0x24). The player's language and Sound page stay, as in rac1.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac2.h"

#define WIDESCREEN RAC2_ADDR(0x001A7BB1u)
#define SOUND RAC2_ADDR(0x001A7B98u)
#define SOUND_WORDS 3
#define SOUND_WORD(i) (SOUND + 4u * (i))
#define SOUND_PLAYBACK 0u
#define LOUDEST 0x400
#define LANGUAGE RAC2_ADDR(0x001A7BB4u)
#define LANGUAGES 6 /* the indexes the sound and text tables have room for (func_002B6120's rows) */

static const char* const sound_keys[SOUND_WORDS] =
    {"game_playback", "game_music_volume", "game_sound_volume"};

/* Whether the Sound page has the remembered values: from the sound's first reset on. */
static int sound_put;

/* The sound's reset: at the game's start it gives the mixer the Sound page's values, so the
 * remembered ones go in first. */
void func_002E48E8(void) {
    if (!sound_put) {
        sound_put = 1;
        for (unsigned i = 0; i < SOUND_WORDS; ++i) {
            const int value = openrac_game_option(sound_keys[i], -1);
            /* None remembered (-1), or a volume past the loudest in a hand-edited file: the game's
             * own value stays. */
            if (value >= 0 && (i == SOUND_PLAYBACK || value <= LOUDEST)) {
                GREF(int, SOUND_WORD(i)) = value;
            }
        }
        const int language = openrac_game_option("game_language_index", -1);
        if (language >= 0 && language < LANGUAGES) {
            GREF(uint8_t, LANGUAGE) = (uint8_t)language;
        }
    }
    func_002E48E8__game();
}

/*
 * The port's Video page, in the 16:9 page's place on the title's Options page (Going Commando's
 * menus are not the front end's widgets of options_menu.cpp, so the page is drawn here, with the
 * same rows, through game_host.h's openrac_game_option_*). The 16:9 option is kept off (above), so
 * its page has nothing to show; as rac1's PAL page gave its 50/60 Hz row's place to the port's rows,
 * this page gives its place to them, and its name, in the Options list, the page's title and here,
 * is "Video".
 *
 * The page is an object of the boot program's menus (the front end's block at D_001A8CFC + 0x3DE60;
 * func_0029C8B0 updates it with the menus' buttons this tick, func_0029C8F0 draws it). Its update,
 * func_0033C2D8, moves its panel (object + 8, to the place at *(object + 0x2AC)) and on Cross,
 * once the panel is in place (object + 0x2A8 zero), flips the 16:9 byte; its draw, func_0033C378,
 * draws the panel (func_0033AF48) and one line centred, "16:9 (widescreen): Off" (func_0027FAA8,
 * the menus' font, x the line's middle, the page's 512 by 416). Here the panel moves as before, and
 * the port's rows are drawn in it, PAGE_ROWS at a time, the one under the cursor brighter: Up and
 * Down move the cursor, Cross and Right step its row to the next value, Left to the one before.
 * A level's pause menu shows the same rows from a Video item of its own in Options (below).
 */
#define TEXT_WIDESCREEN RAC2_K(TEXT_WIDESCREEN, 0x31E4, func_0033C738) /* "16:9 (widescreen)": the Options item, the page's title */
#define TEXT_PLANETS 0x7F01    /* no text of the disc: the developer mode's Planets (below) */
#define TEXT_VIDEO 0x7F02      /* no text of the disc: the pause Options list's Video row (below) */
#define TEXT_VIDEO_HELP 0x7F03 /* no text of the disc: that row's description */
#define MENU_CROSS 0x40u       /* the menus' buttons (the pad block's word at +0x1C4) */
#define MENU_UP 0x1000u
#define MENU_RIGHT 0x2000u
#define MENU_DOWN 0x4000u
#define MENU_LEFT 0x8000u
#define SOUND_MOVE 3 /* func_002E5E58's sounds, as the menus' pages play them */
#define SOUND_CHOOSE 4
#define PAGE_ROWS 7
#define PAGE_TOP 118 /* the first row's y, and each next one's step */
#define PAGE_STEP 21
#define PAGE_MIDDLE 0x100
#define ROW_CHOSEN 0x80F0F0F0ull /* the row under the cursor, the others (func_0033C738's) */
#define ROW_OTHER 0x70808080ull
#define TEXT_BYTES 96
/* The page object's cursor, 0 once its panel is in place (func_0033C2D8, func_0033C738). */
#define CAMERA_CURSOR RAC2_K(CAMERA_CURSOR, 0x2A8u, func_0033C738)

static int page_cursor;
static int page_top;
/* The page's name, one row's line, the Planets item's name and the Video row's description, in game memory for the game's
 * print. */
static gaddr page_text;

static gaddr page_buffers(void) {
    if (page_text == 0) {
        page_text = openrac_guest_static(4 * TEXT_BYTES);
    }
    return page_text;
}

/* The game's text lookup: the 16:9 page's name is the port page's in the boot program, whose
 * Options page it is in. A level's pages are its own (their 16:9 row says what the byte holds). */
/* The pause Options list's description of the row under its cursor: the game reads it from a table
 * of eight, by the row (below). */
static int video_text(int id);

int func_00288FB8(int id) {
    id = video_text(id);
    if (id == TEXT_VIDEO_HELP) {
        const gaddr help = page_buffers() + 3 * TEXT_BYTES;
        snprintf((char*)G(help), TEXT_BYTES, "Display, picture and frame rate settings.");
        return (int)help;
    }
    if (id == TEXT_PLANETS) {
        const gaddr name = page_buffers() + 2 * TEXT_BYTES;
        snprintf((char*)G(name), TEXT_BYTES, "Planets");
        return (int)name;
    }
    if (id == TEXT_VIDEO) {
        const gaddr name = page_buffers();
        openrac_game_option_word("Video", (char*)G(name), TEXT_BYTES);
        return (int)name;
    }
    if (id == TEXT_WIDESCREEN && openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE) {
        const gaddr name = page_buffers();
        openrac_game_option_word("Video", (char*)G(name), TEXT_BYTES);
        return (int)name;
    }
    return func_00288FB8__game(id);
}

/* The Video rows' cursor: Up and Down move it (wrapping), the window of PAGE_ROWS follows it, and when
 * `can_step` Cross and Right step its row to the next value, Left to the one before. */
static void video_rows_update(unsigned pressed, int can_step) {
    const int rows = openrac_game_option_rows();
    if (rows <= 0) {
        return;
    }
    const int was = page_cursor;
    if (pressed & MENU_UP) {
        page_cursor = page_cursor > 0 ? page_cursor - 1 : rows - 1;
    } else if (pressed & MENU_DOWN) {
        page_cursor = page_cursor < rows - 1 ? page_cursor + 1 : 0;
    }
    if (page_cursor >= rows) {
        page_cursor = rows - 1;
    }
    if (page_cursor != was) {
        func_002E5E58(SOUND_MOVE, 0, 0);
    }
    /* The window follows the cursor. */
    if (page_cursor < page_top) {
        page_top = page_cursor;
    } else if (page_cursor >= page_top + PAGE_ROWS) {
        page_top = page_cursor - PAGE_ROWS + 1;
    }
    const int by = (pressed & (MENU_CROSS | MENU_RIGHT)) ? 1 : (pressed & MENU_LEFT) ? -1 : 0;
    if (by != 0 && can_step) {
        openrac_game_option_step(page_cursor, by);
        func_002E5E58(SOUND_CHOOSE, 0, 0);
    }
}

/* The Video rows, PAGE_ROWS of them from the window's top, the one under the cursor brighter, each
 * centred on the page's middle. */
static void video_rows_draw(void) {
    const int rows = openrac_game_option_rows();
    const gaddr line = page_buffers() + TEXT_BYTES;
    char label[TEXT_BYTES];
    char value[TEXT_BYTES];
    for (int i = 0; i < PAGE_ROWS && page_top + i < rows; ++i) {
        const int row = page_top + i;
        openrac_game_option_row_text(row, label, value, TEXT_BYTES);
        snprintf((char*)G(line), TEXT_BYTES, value[0] != 0 ? "%s: %s" : "%s", label, value);
        func_0027FAA8(
            PAGE_MIDDLE,
            PAGE_TOP + PAGE_STEP * i,
            row == page_cursor ? ROW_CHOSEN : ROW_OTHER,
            line,
            -1
        );
    }
}

/*
 * The developer mode's level select (OPENRAC_DEVELOPER, the launcher's developer tools), as Ratchet
 * & Clank's port has it: the title's Options list gets a "Planets" item, which opens a page listing
 * the levels by name; Cross on one starts a new game in it (tools.c, the developer menu's
 * start_level). Without the developer mode the menus are the game's own.
 *
 * The Options page's enter (func_002D58A0) gives its list (the front end's block + 0x3E1D8, a list
 * widget: func_00342380 sets its items) the items at D_00261590, 20 bytes each ({scale, text id,
 * action, argument, 0}, ended by one without a text) of which action 3 opens the page its argument
 * describes (func_002D5E98: g_menuScreenBlock + 0x18, the page shown next; + 0x14 the page shown).
 * Planets is one more item of action 3, its text an id no text of the disc has (the lookup's wrap
 * above names it), opening a copy of the 16:9 page's description: the same panel and object
 * (func_0033C2D8 and func_0033C378 above), which show the levels while that copy is the page.
 */
#define OPTIONS_LIST_OFFSET RAC2_K(OPTIONS_LIST_OFFSET, 0x3E1D8u, func_002D58A0)
#define OPTIONS_ITEMS OPENRAC_DATA(RAC2_ADDR(0x00261590u))
/* g_menuScreenBlock: + 0x14 the page shown, + 0x18 the next. */
#define MENU_SCREEN OPENRAC_DATA(RAC2_ADDR(0x001F2840u))
#define GUI OPENRAC_DATA(RAC2_ADDR(0x001A8CFCu)) /* g_guiInstance: the front end's block */
#define ITEM_BYTES 20u
#define ITEM_TEXT 4u
#define ITEM_ACTION 8u
#define ITEM_ARGUMENT 12u
#define ACTION_PAGE 3
#define PAGE_BYTES 0x130u /* a page's description: 14 boxes, then parent, kind and objects */
#define MAX_ITEMS 8u

static gaddr planets_page;
static gaddr planets_items;
static int planets_cursor;
static int planets_top;
static int planets_level = -1;

static int developer(void) {
    const char* value = openrac_env_string("OPENRAC_DEVELOPER");
    return value != NULL && value[0] != 0 && !(value[0] == '0' && value[1] == 0);
}

/* The levels the front end lists (tools.c), by the game's numbers: the unnamed ones are skipped. */
static int planets_count(void) {
    const openrac_front_end* fe = openrac_game_front_end;
    int count = 0;
    for (int i = 0; fe != NULL && i < fe->levels; ++i) {
        count += fe->level_names[i] != NULL;
    }
    return count;
}

static int planets_nth(int n) {
    const openrac_front_end* fe = openrac_game_front_end;
    for (int i = 0; fe != NULL && i < fe->levels; ++i) {
        if (fe->level_names[i] != NULL && n-- == 0) {
            return i;
        }
    }
    return -1;
}

static int planets_shown(void) {
    return planets_page != 0 && openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE
           && (GREF(gaddr, MENU_SCREEN + 0x14u) == planets_page
               || GREF(gaddr, MENU_SCREEN + 0x18u) == planets_page);
}

/* The Options items, then Planets, opening a copy of the first item's page (the 16:9 page). */
static void planets_build(void) {
    if (planets_items != 0) {
        return;
    }
    unsigned count = 0;
    while (count < MAX_ITEMS
           && GREF(int32_t, OPTIONS_ITEMS + ITEM_BYTES * count + ITEM_TEXT) != 0) {
        ++count;
    }
    if (count == 0 || GREF(int32_t, OPTIONS_ITEMS + ITEM_ACTION) != ACTION_PAGE) {
        return;
    }
    planets_page = openrac_guest_static(PAGE_BYTES);
    memcpy(G(planets_page), G(GREF(gaddr, OPTIONS_ITEMS + ITEM_ARGUMENT)), PAGE_BYTES);
    planets_items = openrac_guest_static(ITEM_BYTES * (count + 2));
    memcpy(G(planets_items), G(OPTIONS_ITEMS), ITEM_BYTES * (count + 1));
    const gaddr item = planets_items + ITEM_BYTES * count;
    memcpy(G(item), G(item - ITEM_BYTES), ITEM_BYTES); /* the last item's scale */
    GREF(int32_t, item + ITEM_TEXT) = TEXT_PLANETS;
    GREF(int32_t, item + ITEM_ACTION) = ACTION_PAGE;
    GREF(gaddr, item + ITEM_ARGUMENT) = planets_page;
    memset(G(item + ITEM_BYTES), 0, ITEM_BYTES);
}

int func_002D58A0(void) {
    const int result = func_002D58A0__game();
    const gaddr gui = GREF(gaddr, GUI);
    /* The title's page only: a level's pause menu has its own copy of the enter. */
    if (gui != 0 && developer() && openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE) {
        planets_build();
        if (planets_items != 0) {
            func_00342380(gui + OPTIONS_LIST_OFFSET, planets_items);
        }
    }
    return result;
}

/* The level a Cross on the page chose, once (tools.c starts it at the next frame), or -1. */
int rac2_planets_take(void) {
    const int level = planets_level;
    planets_level = -1;
    return level;
}

static void planets_update(unsigned pressed, int can_choose) {
    const int rows = planets_count();
    if (rows <= 0) {
        return;
    }
    const int was = planets_cursor;
    if (pressed & MENU_UP) {
        planets_cursor = planets_cursor > 0 ? planets_cursor - 1 : rows - 1;
    } else if (pressed & MENU_DOWN) {
        planets_cursor = planets_cursor < rows - 1 ? planets_cursor + 1 : 0;
    } else if (pressed & MENU_LEFT) {
        planets_cursor = planets_cursor >= PAGE_ROWS ? planets_cursor - PAGE_ROWS : 0;
    } else if (pressed & MENU_RIGHT) {
        planets_cursor = planets_cursor + PAGE_ROWS < rows ? planets_cursor + PAGE_ROWS : rows - 1;
    }
    if (planets_cursor != was) {
        func_002E5E58(SOUND_MOVE, 0, 0);
    }
    if (planets_cursor < planets_top) {
        planets_top = planets_cursor;
    } else if (planets_cursor >= planets_top + PAGE_ROWS) {
        planets_top = planets_cursor - PAGE_ROWS + 1;
    }
    if ((pressed & MENU_CROSS) != 0 && can_choose && planets_level < 0) {
        planets_level = planets_nth(planets_cursor);
        func_002E5E58(SOUND_CHOOSE, 0, 0);
    }
}

static void planets_draw(void) {
    const openrac_front_end* fe = openrac_game_front_end;
    const gaddr line = page_buffers() + TEXT_BYTES;
    const int rows = planets_count();
    for (int i = 0; fe != NULL && i < PAGE_ROWS && planets_top + i < rows; ++i) {
        const int row = planets_top + i;
        const int level = planets_nth(row);
        snprintf((char*)G(line), TEXT_BYTES, "%d  %s", level, fe->level_names[level]);
        func_0027FAA8(
            PAGE_MIDDLE,
            PAGE_TOP + PAGE_STEP * i,
            row == planets_cursor ? ROW_CHOSEN : ROW_OTHER,
            line,
            -1
        );
    }
}

int func_0033C2D8(gaddr object, int buttons) {
    const unsigned pressed = (unsigned)buttons;
    if (planets_shown()) {
        planets_update(pressed, GREF(int, object + CAMERA_CURSOR) == 0);
        func_0033C2D8__game(object, (int)(pressed & ~MENU_CROSS));
        return (int)((pressed >> 6) & 1u);
    }
    video_rows_update(pressed, GREF(int, object + CAMERA_CURSOR) == 0);
    /* The panel's move, without the 16:9 byte's Cross. */
    func_0033C2D8__game(object, (int)(pressed & ~MENU_CROSS));
    return (int)((pressed >> 6) & 1u);
}

void func_0033C378(gaddr object) {
    func_0033AF48(object + 8);
    /* The page's title prints the name the text lookup gave it when the page opened, which is in
     * page_buffers(): the Planets page is a copy of this one, so the name is put there each frame. */
    const gaddr name = page_buffers();
    if (planets_shown()) {
        snprintf((char*)G(name), TEXT_BYTES, "Planets");
        planets_draw();
        return;
    }
    openrac_game_option_word("Video", (char*)G(name), TEXT_BYTES);
    video_rows_draw();
}

/*
 * The pause menu's Options list (func_00341610's list 1) has a Video row after Camera: a copy of the
 * game's items and icons with one more, and the rows drawn closer, so that nine fit in the panel
 * made for eight. The row opens a copy of the Camera page's description, whose tick and draw
 * (func_002D2CE8, func_0033C590, func_0033C738: each level has its own copy of them, and of the
 * object they work on, and the port runs the executable's C for all of them, so these wraps hold
 * in every level) show the port's rows when that copy is the page on screen. Triangle goes back to
 * Options as from any page. The Camera page keeps its three camera rows; the game's fourth, 16:9,
 * is kept off (above) and is not shown or reached.
 */
#define PAUSE_OPTIONS_LIST 1
#define PAUSE_OPTIONS_ROWS RAC2_K(PAUSE_OPTIONS_ROWS, 8, func_00341610) /* PAL's list has 7 */
#define PAUSE_LIST_ICONS RAC2_K(PAUSE_LIST_ICONS, 0x1C8u, func_00341610)
#define PAUSE_LIST_COUNT RAC2_K(PAUSE_LIST_COUNT, 0x1CCu, func_00341610)
#define TEXT_CAMERA_ROW RAC2_K(TEXT_CAMERA_ROW, 0x2D98, func_0033C468)
#define VIDEO_ROW_AFTER 5 /* Camera's row */
/* The title's Options list's icons: their sheet and their table (func_00341CE0 sets them). */
#define TITLE_ICON_SHEET RAC2_K(TITLE_ICON_SHEET, 0xEAA6, func_00342118)
#define TITLE_ICONS OPENRAC_DATA(RAC2_ADDR(0x001AE138u))
/* The list's spacing: the cursor's, the rows' and the icons'. */
#define LIST_CURSOR_STEP OPENRAC_DATA(RAC2_ADDR(0x001AE0E0u))
#define LIST_ROW_STEP OPENRAC_DATA(RAC2_ADDR(0x001AE0F4u))
#define LIST_ICON_STEP OPENRAC_DATA(RAC2_ADDR(0x001AE128u))

static gaddr video_page, video_items, video_icons;
static float list_steps[3];
static const float list_tighter = (float)PAUSE_OPTIONS_ROWS / (PAUSE_OPTIONS_ROWS + 1);

#define PAUSE_LIST_OFFSET RAC2_K(PAUSE_LIST_OFFSET, 0x3BE08u, func_0029C4C0)
#define PAUSE_LIST_WHICH RAC2_K(PAUSE_LIST_WHICH, 0x2FCu, func_00341610)
#define PAUSE_LIST_CURSORS RAC2_K(PAUSE_LIST_CURSORS, 0x1B8u, func_00341610)
/* The description's text id of the row the game's cursor is on, its eight rows and the row past them. */
static const int options_help[9] = /* PAUSE_OPTIONS_ROWS + 1 of them read */
    {RAC2_K(HELP_0, 0x2DA6, D_002652B0), RAC2_K(HELP_1, 0x2DA7, D_002652B0),
     RAC2_K(HELP_2, 0x2DA8, D_002652B0), RAC2_K(HELP_3, 0x2D6C, D_002652B0),
     RAC2_K(HELP_4, 0x2DAA, D_002652B0), RAC2_K(HELP_5, 0x2DAB, D_002652B0),
     RAC2_K(HELP_6, 0x2DAC, D_002652B0), RAC2_K(HELP_7, 0x2DAE, D_002652B0),
     RAC2_K(HELP_8, 0x2E1C, D_002652B0)};

static int video_shown(void) {
    return video_page != 0
           && (GREF(gaddr, MENU_SCREEN + 0x14u) == video_page
               || GREF(gaddr, MENU_SCREEN + 0x18u) == video_page);
}

static int video_text(int id) {
    const gaddr gui = GREF(gaddr, GUI);
    if (gui == 0 || video_items == 0) {
        return id;
    }
    const gaddr list = gui + PAUSE_LIST_OFFSET;
    if (GREF(int, list + PAUSE_LIST_WHICH) != PAUSE_OPTIONS_LIST
        || GREF(int, list + PAUSE_LIST_COUNT) != PAUSE_OPTIONS_ROWS + 1) {
        return id;
    }
    const int row = GREF(int, list + PAUSE_LIST_CURSORS + 4u * PAUSE_OPTIONS_LIST);
    if (row <= VIDEO_ROW_AFTER || row > PAUSE_OPTIONS_ROWS || id != options_help[row]) {
        return id;
    }
    return row == VIDEO_ROW_AFTER + 1 ? TEXT_VIDEO_HELP : options_help[row - 1];
}

/* The Options items with Video after Camera, or the game's where they are not what is expected. */
static gaddr video_items_of(gaddr items) {
    if (GREF(int32_t, items + ITEM_BYTES * VIDEO_ROW_AFTER + ITEM_TEXT) != TEXT_CAMERA_ROW
        || GREF(int32_t, items + ITEM_BYTES * VIDEO_ROW_AFTER + ITEM_ACTION) != ACTION_PAGE) {
        return items;
    }
    if (video_items == 0) {
        video_page = openrac_guest_static(PAGE_BYTES);
        video_items = openrac_guest_static(ITEM_BYTES * (PAUSE_OPTIONS_ROWS + 2));
    }
    const gaddr camera = items + ITEM_BYTES * VIDEO_ROW_AFTER;
    memcpy(G(video_page), G(GREF(gaddr, camera + ITEM_ARGUMENT)), PAGE_BYTES);
    const unsigned before = VIDEO_ROW_AFTER + 1;
    memcpy(G(video_items), G(items), ITEM_BYTES * before);
    const gaddr row = video_items + ITEM_BYTES * before;
    memcpy(G(row), G(camera), ITEM_BYTES);
    GREF(int32_t, row + ITEM_TEXT) = TEXT_VIDEO;
    GREF(gaddr, row + ITEM_ARGUMENT) = video_page;
    /* The rest and the zero item that ends the table. */
    memcpy(
        G(row + ITEM_BYTES), G(camera + ITEM_BYTES), ITEM_BYTES * (PAUSE_OPTIONS_ROWS - before + 1)
    );
    return video_items;
}

void func_00341540(gaddr list, int main_items, int options_items, int third_items) {
    func_00341540__game(list, main_items, (int)video_items_of((gaddr)options_items), third_items);
}

void func_00341610(gaddr list, int which) {
    const gaddr steps[3] = {LIST_CURSOR_STEP, LIST_ROW_STEP, LIST_ICON_STEP};
    const int video = which == PAUSE_OPTIONS_LIST && video_items != 0;
    for (int i = 0; i < 3; ++i) {
        if (list_steps[i] == 0.0f) {
            list_steps[i] = GREF(float, steps[i]);
        }
        GREF(float, steps[i]) = video ? list_steps[i] * list_tighter : list_steps[i];
    }
    func_00341610__game(list, which);
    if (!video) {
        return;
    }
    if (video_icons == 0) {
        video_icons = openrac_guest_static(4u * (PAUSE_OPTIONS_ROWS + 1));
    }
    const gaddr icons = GREF(gaddr, list + PAUSE_LIST_ICONS);
    const unsigned before = VIDEO_ROW_AFTER + 1;
    memcpy(G(video_icons), G(icons), 4u * before);
    /* The title list's first icon, its Video row's (list_icons draws it from that list's sheet). */
    GREF(int32_t, video_icons + 4u * before) = GREF(int32_t, TITLE_ICONS);
    memcpy(
        G(video_icons + 4u * (before + 1)),
        G(icons + 4u * before),
        4u * (PAUSE_OPTIONS_ROWS - before)
    );
    GREF(gaddr, list + PAUSE_LIST_ICONS) = video_icons;
    GREF(int, list + PAUSE_LIST_COUNT) = PAUSE_OPTIONS_ROWS + 1;
}

/*
 * The panel's art has its rows baked in, one frame for each list, and none for nine. With Video
 * shown it is squeezed to the rows' spacing and drawn twice: down to the gap above the last two rows,
 * where its sides are straight, then one row lower from there, which gives the ninth row and the
 * panel's bottom edge.
 */
#define PANEL_SPRITE_ROWS 3
#define LIST_ON RAC2_K(LIST_ON, 0x220u, func_00341C20)
#define LIST_ORIGIN RAC2_K(LIST_ORIGIN, 0x218u, func_00341A88)
#define LIST_TITLE_BAR RAC2_K(LIST_TITLE_BAR, 0x98u, func_00341C20)
#define LIST_TITLE_TEXT RAC2_K(LIST_TITLE_TEXT, 0x1D0u, func_00341C20)
#define LIST_CURSOR RAC2_K(LIST_CURSOR, 0x130u, func_00342118)
#define SPRITE_POSITION 0u
#define SPRITE_SCALE 4u
#define LIST_ICON RAC2_K(LIST_ICON, 0x17Cu, func_00341A88)
#define LIST_ICON_SHEET RAC2_K(LIST_ICON_SHEET, 0x1C4u, func_00341A88)
#define LIST_ICON_LEFT OPENRAC_DATA(RAC2_ADDR(0x001AE120u))
#define LIST_ICON_FIRST OPENRAC_DATA(RAC2_ADDR(0x001AE124u))
#define GS_SCISSOR_1 0x40ull
#define GS_LAST_LINE 447
/* The art's rows are a little closer than the text's: by here that is this many lines. */
#define PANEL_LOWER_DROP 2.0f
/* The icons keep their size in the squeezed slots: this many lines up centres them. */
#define PANEL_ICON_RISE 1.0f
#define LIST_ICON_HALF                                                                             \
    16.0f /* LIST_ICON_FIRST is an icon's top, and the row's middle is below it */

static void panel_clip(int top, int bottom) {
    func_002FC560(
        GS_SCISSOR_1,
        (511ull << 16) | ((unsigned long long)top << 32) | ((unsigned long long)bottom << 48)
    );
}

/* Draws the sprite SQUEEZED about the first row, moved down by BELOW. */
static void panel_sprite(gaddr sprite, float first_row, float below) {
    const gaddr position = GREF(gaddr, sprite + SPRITE_POSITION);
    const gaddr scale = GREF(gaddr, sprite + SPRITE_SCALE);
    const float y = GREF(float, position + 4u);
    const float height = GREF(float, scale + 4u);
    GREF(float, position + 4u) = y + (1.0f - list_tighter) * (first_row - y) + below;
    GREF(float, scale + 4u) = height * list_tighter;
    func_00336320(sprite);
    GREF(float, position + 4u) = y;
    GREF(float, scale + 4u) = height;
}

/* func_00341A88 for this list: its icons, a line higher, and on the Video row the icon the title's
 * Options list has there, from that list's sheet. */
static void list_icons(gaddr list) {
    const gaddr icon = list + LIST_ICON;
    const gaddr origin = GREF(gaddr, list + LIST_ORIGIN);
    const int chosen = GREF(int, list + PAUSE_LIST_CURSORS + 4u * PAUSE_OPTIONS_LIST);
    const int rows = GREF(int, list + PAUSE_LIST_COUNT);
    const gaddr icons = GREF(gaddr, list + PAUSE_LIST_ICONS);
    const float x = GREF(float, origin) + GREF(float, LIST_ICON_LEFT);
    const float y = GREF(float, origin + 4u) + GREF(float, LIST_ICON_FIRST) - PANEL_ICON_RISE;
    for (int i = 0; i < rows; ++i) {
        const float row_y = y + GREF(float, LIST_ICON_STEP) * (float)i;
        func_00335E38(icon, x, row_y, 0.0f, 0.0f);
        func_00336920(
            icon,
            i == VIDEO_ROW_AFTER + 1 ? TITLE_ICON_SHEET : GREF(int, list + LIST_ICON_SHEET),
            GREF(int, icons + 4u * (gaddr)i)
        );
        GREF(unsigned int, func_00335E20(icon)) = i == chosen ? 0x70FFFEEDu : 0x60F0F0B0u;
        func_00336828(icon);
    }
}

void func_00341C20(gaddr list) {
    if (GREF(int, list + PAUSE_LIST_WHICH) != PAUSE_OPTIONS_LIST
        || GREF(int, list + PAUSE_LIST_COUNT) != PAUSE_OPTIONS_ROWS + 1) {
        func_00341C20__game(list);
        return;
    }
    if (GREF(int, list + LIST_ON) == 0) {
        return;
    }
    static const unsigned sprites[PANEL_SPRITE_ROWS] =
        {0u, RAC2_K(LIST_SPRITE_1, 0x4Cu, func_00341C20), RAC2_K(LIST_SPRITE_2, 0xE4u, func_00341C20)};
    const gaddr origin = GREF(gaddr, list + LIST_ORIGIN);
    const float step = GREF(float, LIST_ICON_STEP);
    const float first_row = GREF(float, origin + 4u) + GREF(float, LIST_ICON_FIRST);
    const int split = (int)(first_row + LIST_ICON_HALF + step * (PAUSE_OPTIONS_ROWS - 1.5f));

    func_00336320(list + LIST_TITLE_BAR);
    panel_clip(0, split - 1);
    for (int i = 0; i < PANEL_SPRITE_ROWS; ++i) {
        panel_sprite(list + sprites[i], first_row, 0.0f);
    }
    panel_clip(split, GS_LAST_LINE);
    for (int i = 0; i < PANEL_SPRITE_ROWS; ++i) {
        panel_sprite(list + sprites[i], first_row, step + PANEL_LOWER_DROP);
    }
    panel_clip(0, GS_LAST_LINE);

    /* The bar around the page's name is in the art: the name goes with it. */
    const gaddr title = GREF(gaddr, list + LIST_TITLE_TEXT + SPRITE_POSITION);
    const float title_y = GREF(float, title + 4u);
    GREF(float, title + 4u) = title_y + (1.0f - list_tighter) * (first_row - title_y);
    func_00336A78(list + LIST_TITLE_TEXT);
    GREF(float, title + 4u) = title_y;
    const gaddr cursor_scale = GREF(gaddr, list + LIST_CURSOR + SPRITE_SCALE);
    const float cursor_height = GREF(float, cursor_scale + 4u);
    GREF(float, cursor_scale + 4u) = cursor_height * list_tighter;
    func_00341A48(list);
    GREF(float, cursor_scale + 4u) = cursor_height;
    func_00341A68(list);
    list_icons(list);
}

#define CAMERA_HIDDEN_ROW 3 /* 16:9: the page's fourth row */
/* The page's data, each level's own copy (OPENRAC_DATA). */
#define CAMERA_LINES OPENRAC_DATA(RAC2_ADDR(0x001ADE00u))  /* each line's x and y words, 8 bytes a line */
#define CAMERA_ON_OFF OPENRAC_DATA(RAC2_ADDR(0x001ADE20u)) /* the text ids of "Normal" and "Inverted" */
#define CAMERA_SPEEDS OPENRAC_DATA(RAC2_ADDR(0x001ADE28u)) /* the text ids of the three rotation speeds */
#define CAMERA_UP_DOWN RAC2_ADDR(0x001A7BA4u)
#define CAMERA_LEFT_RIGHT RAC2_ADDR(0x001A7BA8u)
#define CAMERA_SPEED RAC2_ADDR(0x001A7BACu)
#define CAMERA_SPEED_COUNT 3u
#define TEXT_UP_DOWN RAC2_K(TEXT_UP_DOWN, 0x2DE2, func_0033C738)
#define TEXT_LEFT_RIGHT RAC2_K(TEXT_LEFT_RIGHT, 0x2DE1, func_0033C738)
#define TEXT_SPEED RAC2_K(TEXT_SPEED, 0x2F18, func_0033C738)
#define CAMERA_CHOSEN 0x80D0D0D0ull

int func_0033C590(gaddr object, int buttons) {
    unsigned pressed = (unsigned)buttons;
    if (video_shown()) {
        video_rows_update(pressed, 1);
        /* The panel's move only: the camera's cursor and options stay. */
        pressed &= ~(MENU_UP | MENU_DOWN | MENU_CROSS);
    }
    const int row = GREF(int, object + CAMERA_CURSOR);
    func_0033C590__game(object, (int)pressed);
    /* Past the third row: round to the other end, as the game's cursor does at its fourth. */
    if (GREF(int, object + CAMERA_CURSOR) == CAMERA_HIDDEN_ROW) {
        GREF(int, object + CAMERA_CURSOR) = row == 0 ? CAMERA_HIDDEN_ROW - 1 : 0;
    }
    return (int)(((unsigned)buttons >> 6) & 1u);
}

/* One of the camera's lines, as func_0033C738 prints it ("name: value", the menus' second font). */
static void camera_line(gaddr object, int row, gaddr name, gaddr value) {
    const gaddr line = page_buffers() + TEXT_BYTES;
    if (value != 0) {
        snprintf((char*)G(line), TEXT_BYTES, "%s: %s", (const char*)G(name), (const char*)G(value));
    } else {
        snprintf((char*)G(line), TEXT_BYTES, "%s", (const char*)G(name));
    }
    func_0027FB40(
        GREF(int, CAMERA_LINES + 8u * (unsigned)row),
        GREF(int, CAMERA_LINES + 8u * (unsigned)row + 4u),
        GREF(int, object + CAMERA_CURSOR) == row ? CAMERA_CHOSEN : ROW_OTHER,
        line,
        -1
    );
}

/* The page's title, which the game sets once when it builds the page (func_0033C468), follows what
 * it shows: Camera or Video. */
#define TEXT_CAMERA_TITLE RAC2_K(TEXT_CAMERA_TITLE, 0x2D98, func_0033C468)
#define TEXT_PAGE_TOGGLE RAC2_K(TEXT_PAGE_TOGGLE, 0x2D4E, func_0033C468)
#define TEXT_PAGE_EXIT RAC2_K(TEXT_PAGE_EXIT, 0x2D4F, func_0033C468)

static gaddr titled_video;

static void camera_title(gaddr object) {
    const int shown = video_shown();
    if (shown == (titled_video == object)) {
        return;
    }
    titled_video = shown ? object : 0;
    func_0033B050(
        object + 8,
        func_00288FB8(shown ? TEXT_VIDEO : TEXT_CAMERA_TITLE),
        func_00288FB8(TEXT_PAGE_TOGGLE),
        func_00288FB8(TEXT_PAGE_EXIT)
    );
}

void func_0033C738(gaddr object) {
    camera_title(object);
    if (video_shown()) {
        func_0033AF48(object + 8);
        video_rows_draw();
        return;
    }
    func_0033AF48(object + 8);
    const int up_down = GREF(int, CAMERA_ON_OFF + (GREF(int, CAMERA_UP_DOWN) != 0 ? 4u : 0u));
    camera_line(object, 0, (gaddr)func_00288FB8(TEXT_UP_DOWN), (gaddr)func_00288FB8(up_down));
    const int left_right = GREF(int, CAMERA_ON_OFF + (GREF(int, CAMERA_LEFT_RIGHT) != 0 ? 4u : 0u));
    camera_line(object, 1, (gaddr)func_00288FB8(TEXT_LEFT_RIGHT), (gaddr)func_00288FB8(left_right));
    const unsigned mode = (unsigned)GREF(int, CAMERA_SPEED);
    const int speed = GREF(int, CAMERA_SPEEDS + 4u * (mode < CAMERA_SPEED_COUNT ? mode : 0u));
    camera_line(object, 2, (gaddr)func_00288FB8(TEXT_SPEED), (gaddr)func_00288FB8(speed));
}

/* A save's blocks into their places (func_0029B168: the memory card's load, func_00298F00, and
 * func_00298D28 both restore through it), the language and the Sound page kept. */
int func_0029B168(gaddr save, int slot, gaddr blocks) {
    const uint8_t language = GREF(uint8_t, LANGUAGE);
    int sound[SOUND_WORDS];
    for (unsigned i = 0; i < SOUND_WORDS; ++i) {
        sound[i] = GREF(int, SOUND_WORD(i));
    }
    const int errors = func_0029B168__game(save, slot, blocks);
    GREF(uint8_t, LANGUAGE) = language;
    for (unsigned i = 0; i < SOUND_WORDS; ++i) {
        GREF(int, SOUND_WORD(i)) = sound[i];
    }
    return errors;
}

/* Each frame (tools.c's on_frame): the game's 16:9 off, and the Sound page and the language as the
 * player left them, for the next run, once they have the remembered values. */
void rac2_options_frame(void) {
    GREF(uint8_t, WIDESCREEN) = 0;
    if (sound_put) {
        for (unsigned i = 0; i < SOUND_WORDS; ++i) {
            openrac_game_set_option(sound_keys[i], GREF(int, SOUND_WORD(i)));
        }
        openrac_game_set_option("game_language_index", GREF(uint8_t, LANGUAGE));
    }
}
