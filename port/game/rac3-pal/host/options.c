/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's options: the port's Video page in the title's Options, the game's own 16:9
 * kept off, and the options a console forgets at power off remembered between runs, as Going
 * Commando's port has them (rac2/host/options.c).
 *
 * The title's Options page (the boot program's screen at D_001DA168: build func_003B55D8, update
 * func_003B5890) is a list widget (D_00280CF0, list 0: +4 the first row shown, +8 the cursor's row
 * among those shown, +0xC the rows, +0x10 how many are shown, +0x14 the columns, +0xB0 the cells,
 * 0x44 bytes each: the text, then +0x40 its state, 4 one the cursor skips) of two columns, a name
 * and a value, with the items at D_0037F830 (16 bytes each: the name's text id, enabled, the
 * action, the preview's animation; ended by a zero name): 16:9 (0x508, the byte +0xAD of the
 * options at D_00143950, NTSC-U's option_widescreen), Subtitles (0x148, the byte +6), the sound's
 * Stereo or Mono (0x146, the word +8) and the language (0x14F, the byte +0xB7, 1 to 5). Each
 * frame func_003B63D0 writes every row's name and value into its cells (the values at
 * D_0037F790, 0x20 bytes each), func_003B6090 follows the cursor (D_001D89B8) with the preview
 * (widget 0x1C0003, an animation a row) and func_003B5F78 acts on the menus' buttons: Cross the
 * row's action, Triangle back to the main menu.
 *
 * The window's picture has the player's shape (the Video page's Picture), so the game's 16:9,
 * which narrows the view for a television that stretches it back, is kept off, and its row gives
 * its place to the port's Video page, as Going Commando's 16:9 page does: Cross on it shows the
 * port's rows (game_host.h, openrac_game_option_*) in the same list, its four shown rows moving
 * with the cursor as the game's own lists scroll; Cross or Right steps the row under the cursor to
 * its next value, Left to the one before, Triangle goes back to the game's rows.
 */
#include <stdint.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define OPTIONS 0x00143950u
#define OPTION_SUBTITLES (OPTIONS + 0x06u) /* a byte */
#define OPTION_SOUND (OPTIONS + 0x08u)     /* a word: 0 mono, else stereo */
#define OPTION_WIDESCREEN (OPTIONS + 0xADu)
/* A byte: 1 English, 2 French, 3 German, 4 Spanish, 5 Italian. */
#define OPTION_LANGUAGE (OPTIONS + 0xB7u)

#define LIST 0x00280CF0u /* the Options page's list (func_003B9F88(0)) */
#define LIST_TOP 0x04u
#define LIST_CURSOR 0x08u
#define LIST_ROWS 0x0Cu
#define LIST_COLUMNS 0x14u
#define LIST_SCROLL 0x1Cu
#define LIST_CELLS 0xB0u
#define CELL_BYTES 0x44u
#define CELL_TEXT 0x40u
#define LIST_CAPACITY 0x9000u /* the cells' room (func_003B6128 gives the list 0x9000 bytes) */

#define CURSOR_ROW 0x001D89B8u  /* the Options row under the cursor (func_003B6090) */
#define PREVIEW_ROW 0x001D89BCu /* the row whose preview shows, -1 none yet */
#define PREVIEW 0x001C0003      /* the preview's widget */
#define ROW_VIDEO 0             /* the 16:9 row, the port's Video page in its place */

#define MENU_TRIANGLE 0x10u /* the menus' buttons (the pad record's word at +0x1C4) */
#define MENU_CROSS 0x40u
#define MENU_RIGHT 0x2000u
#define MENU_LEFT 0x8000u
#define SOUND_CHOOSE 4 /* func_003A5748's sounds, as the menus play them */

static int video_open;
static int video_count; /* the Video page's rows */
static int game_rows;   /* the list's rows while the Video page shows its own */

static gaddr cell(int column, int row) {
    const gaddr cells = GREF(gaddr, LIST + LIST_CELLS);
    const int columns = GREF(int32_t, LIST + LIST_COLUMNS);
    return cells + (gaddr)((columns * row + column) * (int)CELL_BYTES);
}

static int list_row(void) {
    return GREF(int32_t, LIST + LIST_TOP) + GREF(int32_t, LIST + LIST_CURSOR);
}

/* The list's rows, from the first, with their cells emptied and none skipped. */
static void list_reset(int rows) {
    const int columns = GREF(int32_t, LIST + LIST_COLUMNS);
    memset(G(cell(0, 0)), 0, (size_t)(rows * columns) * CELL_BYTES);
    GREF(int32_t, LIST + LIST_ROWS) = rows;
    GREF(int32_t, LIST + LIST_TOP) = 0;
    GREF(int32_t, LIST + LIST_CURSOR) = 0;
    GREF(int32_t, LIST + LIST_SCROLL) = 0;
}

/* The game's menus are in capitals: so are the port's texts in them (ASCII letters; the accented
 * ones are left as the game's text has them). */
static void capitals(gaddr text) {
    for (char* c = (char*)G(text); *c != 0; ++c) {
        if (*c >= 'a' && *c <= 'z') {
            *c = (char)(*c - 'a' + 'A');
        }
    }
}

static int video_rows(void) {
    const int columns = GREF(int32_t, LIST + LIST_COLUMNS);
    const int rows = openrac_game_option_rows();
    const int room = columns > 0 ? (int)(LIST_CAPACITY / CELL_BYTES) / columns : 0;
    return rows < room ? rows : room;
}

static void video_show(int open) {
    if (open) {
        const int rows = video_rows();
        if (rows <= 0) {
            return;
        }
        game_rows = GREF(int32_t, LIST + LIST_ROWS);
        video_count = rows;
        list_reset(rows);
        func_003E7DD0(PREVIEW, 0);
    } else {
        list_reset(game_rows);
        GREF(int32_t, PREVIEW_ROW) = -1; /* the preview again, the row's */
    }
    video_open = open;
}

/* The Options rows' texts, each frame: the port's while its page shows, else the game's with the
 * 16:9 row named Video, its value none. */
int func_003B63D0(void) {
    /* The page built again (the screen left another way than Triangle): the game's rows. */
    if (video_open && GREF(int32_t, LIST + LIST_ROWS) != video_count) {
        video_open = 0;
    }
    if (!video_open) {
        const int r = func_003B63D0__game();
        openrac_game_option_word("Video", (char*)G(cell(0, ROW_VIDEO)), (int)CELL_TEXT);
        capitals(cell(0, ROW_VIDEO));
        GREF(char, cell(1, ROW_VIDEO)) = 0;
        return r;
    }
    const int rows = GREF(int32_t, LIST + LIST_ROWS);
    for (int row = 0; row < rows; ++row) {
        openrac_game_option_row_text(
            row, (char*)G(cell(0, row)), (char*)G(cell(1, row)), (int)CELL_TEXT
        );
        capitals(cell(0, row));
        capitals(cell(1, row));
    }
    return 0;
}

/* The preview follows the cursor among the game's rows; the Video row and page have none. */
void func_003B6090(void) {
    if (video_open) {
        func_003E7DD0(PREVIEW, 0);
        return;
    }
    func_003B6090__game();
    if (GREF(int32_t, CURSOR_ROW) == ROW_VIDEO) {
        func_003E7DD0(PREVIEW, 0);
    }
}

/* The menus' buttons on the Options page: Cross on the Video row opens the port's page; on it,
 * Cross, Right and Left step the row under the cursor and Triangle goes back. */
void func_003B5F78(int screen, int buttons) {
    if (!video_open) {
        if ((buttons & MENU_CROSS) != 0 && GREF(int32_t, CURSOR_ROW) == ROW_VIDEO) {
            func_003A5748(SOUND_CHOOSE, 0, 0);
            video_show(1);
            return;
        }
        func_003B5F78__game(screen, buttons);
        return;
    }
    if ((buttons & MENU_TRIANGLE) != 0) {
        func_003A5748(SOUND_CHOOSE, 0, 0);
        video_show(0);
        return;
    }
    const int by = (buttons & (MENU_CROSS | MENU_RIGHT)) != 0 ? 1
                   : (buttons & MENU_LEFT) != 0               ? -1
                                                              : 0;
    if (by != 0) {
        openrac_game_option_step(list_row(), by);
        func_003A5748(SOUND_CHOOSE, 0, 0);
    }
}

/* The 16:9 row's action: the port's page has its place (above); the byte stays off. */
void func_003B6588(void) {
    GREF(uint8_t, OPTION_WIDESCREEN) = 0;
}

/* The console's language code for the game's index and back (func_003983C8's table: the game's
 * 3 is German, the console's 4; its 4 Spanish, the console's 3), 0 for none. */
static int console_language(int game) {
    static const int codes[6] = {0, 1, 2, 4, 3, 5};
    return game >= 0 && game < 6 ? codes[game] : 0;
}

static void on_frame(unsigned frame);
static void (*g_previous_on_frame)(unsigned frame);

/* The front end's start-up: it sets the language from the console's (openrac_game_language, the
 * one the port remembered); the options it leaves at their defaults get the remembered ones. */
int func_003983C8(void) {
    const int r = func_003983C8__game();
    const int subtitles = openrac_game_option("game_subtitles", -1);
    if (subtitles == 0 || subtitles == 1) {
        GREF(uint8_t, OPTION_SUBTITLES) = (uint8_t)subtitles;
    }
    const int sound = openrac_game_option("game_playback", -1);
    if (sound >= 0) {
        GREF(int32_t, OPTION_SOUND) = sound;
    }
    /* The hook another part set (menu_log.c's) runs first, then this one. */
    if (openrac_game_on_frame != on_frame) {
        g_previous_on_frame = openrac_game_on_frame;
    }
    openrac_game_on_frame = on_frame;
    return r;
}

/* Each frame: the 16:9 off, and the options as the player left them, for the next run. */
static void on_frame(unsigned frame) {
    if (g_previous_on_frame != NULL) {
        g_previous_on_frame(frame);
    }
    GREF(uint8_t, OPTION_WIDESCREEN) = 0;
    const int language = console_language(GREF(uint8_t, OPTION_LANGUAGE));
    if (language > 0) {
        openrac_game_set_option("game_language", language);
        openrac_game_language = language;
    }
    openrac_game_set_option("game_subtitles", GREF(uint8_t, OPTION_SUBTITLES) != 0);
    openrac_game_set_option("game_playback", GREF(int32_t, OPTION_SOUND));
}
