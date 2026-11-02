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
 * (func_002E48E8) both; the remembered values go in before its first run. The language is not
 * remembered here: its word (0x001A7BB4, a byte, set by the menus' action type 9) is set at the start
 * by code still in assembly, not read yet.
 */
#include <stdint.h>
#include <stdio.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define WIDESCREEN 0x001A7BB1u
#define SOUND 0x001A7B98u
#define SOUND_WORDS 3
#define SOUND_WORD(i) (SOUND + 4u * (i))
#define SOUND_PLAYBACK 0u
#define LOUDEST 0x400

static const char* const sound_keys[SOUND_WORDS] = {"game_playback", "game_music_volume", "game_sound_volume"};

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
 */
#define TEXT_WIDESCREEN 0x31E4    /* "16:9 (widescreen)": the Options item, the page's title */
#define MENU_CROSS 0x40u           /* the menus' buttons (the pad block's word at +0x1C4) */
#define MENU_UP 0x1000u
#define MENU_RIGHT 0x2000u
#define MENU_DOWN 0x4000u
#define MENU_LEFT 0x8000u
#define SOUND_MOVE 3               /* func_002E5E58's sounds, as the menus' pages play them */
#define SOUND_CHOOSE 4
#define PAGE_ROWS 7
#define PAGE_TOP 118               /* the first row's y, and each next one's step */
#define PAGE_STEP 21
#define PAGE_MIDDLE 0x100
#define ROW_CHOSEN 0x80F0F0F0ull   /* the row under the cursor, the others (func_0033C738's) */
#define ROW_OTHER 0x70808080ull
#define TEXT_BYTES 96

static int page_cursor;
static int page_top;
/* The page's name and one row's line, in game memory for the game's print. */
static gaddr page_text;

static gaddr page_buffers(void) {
    if (page_text == 0) {
        page_text = openrac_guest_static(2 * TEXT_BYTES);
    }
    return page_text;
}

/* The game's text lookup: the 16:9 page's name is the port page's in the boot program, whose
 * Options page it is in. A level's pages are its own (their 16:9 row says what the byte holds). */
int func_00288FB8(int id) {
    if (id == TEXT_WIDESCREEN && openrac_game_loaded_overlay() == OPENRAC_OVERLAY_EXE) {
        const gaddr name = page_buffers();
        openrac_game_option_word("Video", (char*)G(name), TEXT_BYTES);
        return (int)name;
    }
    return func_00288FB8__game(id);
}

int func_0033C2D8(gaddr object, int buttons) {
    const int rows = openrac_game_option_rows();
    const unsigned pressed = (unsigned)buttons;
    if (rows > 0) {
        const int was = page_cursor;
        if (pressed & MENU_UP) {
            page_cursor = page_cursor > 0 ? page_cursor - 1 : rows - 1;
        } else if (pressed & MENU_DOWN) {
            page_cursor = page_cursor < rows - 1 ? page_cursor + 1 : 0;
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
        if (by != 0 && GREF(int, object + 0x2A8) == 0) {
            openrac_game_option_step(page_cursor, by);
            func_002E5E58(SOUND_CHOOSE, 0, 0);
        }
    }
    /* The panel's move, without the 16:9 byte's Cross. */
    func_0033C2D8__game(object, (int)(pressed & ~MENU_CROSS));
    return (int)((pressed >> 6) & 1u);
}

void func_0033C378(gaddr object) {
    func_0033AF48(object + 8);
    const int rows = openrac_game_option_rows();
    const gaddr line = page_buffers() + TEXT_BYTES;
    char label[TEXT_BYTES];
    char value[TEXT_BYTES];
    for (int i = 0; i < PAGE_ROWS && page_top + i < rows; ++i) {
        const int row = page_top + i;
        openrac_game_option_row_text(row, label, value, TEXT_BYTES);
        snprintf((char*)G(line), TEXT_BYTES, value[0] != 0 ? "%s: %s" : "%s", label, value);
        func_0027FAA8(PAGE_MIDDLE, PAGE_TOP + PAGE_STEP * i, row == page_cursor ? ROW_CHOSEN : ROW_OTHER, line, -1);
    }
}

/* Each frame (tools.c's on_frame): the game's 16:9 off, and the Sound page as the player left it,
 * for the next run, once it has the remembered values. */
void rac2_options_frame(void) {
    GREF(uint8_t, WIDESCREEN) = 0;
    if (sound_put) {
        for (unsigned i = 0; i < SOUND_WORDS; ++i) {
            openrac_game_set_option(sound_keys[i], GREF(int, SOUND_WORD(i)));
        }
    }
}
