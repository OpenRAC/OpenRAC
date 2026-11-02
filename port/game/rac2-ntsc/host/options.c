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

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define WIDESCREEN 0x001A7BB1u
#define SOUND 0x001A7B98u
#define SOUND_WORDS 3
#define SOUND_WORD(i) (SOUND + 4u * (i))
#define SOUND_PLAYBACK 0u
#define LOUDEST 0x400
#define LANGUAGE 0x001A7BB4u
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
 * A level's pause menu shows the same rows from its Camera page (below).
 */
#define TEXT_WIDESCREEN 0x31E4 /* "16:9 (widescreen)": the Options item, the page's title */
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

int func_0033C2D8(gaddr object, int buttons) {
    const unsigned pressed = (unsigned)buttons;
    video_rows_update(pressed, GREF(int, object + 0x2A8) == 0);
    /* The panel's move, without the 16:9 byte's Cross. */
    func_0033C2D8__game(object, (int)(pressed & ~MENU_CROSS));
    return (int)((pressed >> 6) & 1u);
}

void func_0033C378(gaddr object) {
    func_0033AF48(object + 8);
    video_rows_draw();
}

/*
 * The pause menu's Camera page (Options, Camera, in every level): func_0033C590 moves its cursor
 * (object + 0x2A8, four rows) and on Cross flips its row's option, func_0033C738 draws the panel
 * and the four lines, "Up/Down movement", "Left/Right movement", "Change camera rotation speed" and
 * "16:9 (widescreen)" (0x001A7BB1, as the title's). Each level has its own copy of both, of the
 * page's tick (func_002D2CE8: Triangle leaves the page, Start and Select the menu) and of the
 * object they work on; the port runs the executable's C for all of them, so these wraps hold in
 * every level. The 16:9 row is the port's Video page here too: the row reads "Video", and its Cross
 * shows the Video rows in the same panel, moved and drawn by the title page's code above, until
 * Triangle goes back to the camera's rows (or the menu closes).
 */
#define CAMERA_VIDEO_ROW 3
#define CAMERA_CURSOR 0x2A8u
/* The page's data, each level's own copy (OPENRAC_DATA). */
#define CAMERA_LINES OPENRAC_DATA(0x001ADE00u)  /* each line's x and y words, 8 bytes a line */
#define CAMERA_ON_OFF OPENRAC_DATA(0x001ADE20u) /* the text ids of "Normal" and "Inverted" */
#define CAMERA_SPEEDS OPENRAC_DATA(0x001ADE28u) /* the text ids of the three rotation speeds */
#define CAMERA_UP_DOWN 0x001A7BA4u
#define CAMERA_LEFT_RIGHT 0x001A7BA8u
#define CAMERA_SPEED 0x001A7BACu
#define CAMERA_SPEED_COUNT 3u
#define TEXT_UP_DOWN 0x2DE2
#define TEXT_LEFT_RIGHT 0x2DE1
#define TEXT_SPEED 0x2F18
#define MENU_TRIANGLE 0x10u
#define MENU_CLOSE 0x900u        /* Start and Select: the pause menu closes */
#define MENU_BUTTONS 0x00138344u /* the menus' buttons pressed this tick */
#define CAMERA_CHOSEN 0x80D0D0D0ull

/* Whether the Camera page shows the Video rows. */
static int camera_video;

int func_002D2CE8(void) {
    const unsigned pressed = GREF(unsigned, MENU_BUTTONS);
    if (camera_video && (pressed & MENU_CLOSE) == 0 && (pressed & MENU_TRIANGLE) != 0) {
        /* Back to the camera's rows, the page kept. */
        camera_video = 0;
        func_002E5E58(SOUND_CHOOSE, 0, 0);
        return 0;
    }
    if (pressed & (MENU_TRIANGLE | MENU_CLOSE)) {
        camera_video = 0;
    }
    return func_002D2CE8__game();
}

int func_0033C590(gaddr object, int buttons) {
    unsigned pressed = (unsigned)buttons;
    if (camera_video) {
        video_rows_update(pressed, 1);
        /* The panel's move only: the camera's cursor and options stay. */
        pressed &= ~(MENU_UP | MENU_DOWN | MENU_CROSS);
    } else if ((pressed & MENU_CROSS) != 0
               && GREF(int, object + CAMERA_CURSOR) == CAMERA_VIDEO_ROW) {
        camera_video = 1;
        func_002E5E58(SOUND_CHOOSE, 0, 0);
        pressed &= ~MENU_CROSS;
    }
    func_0033C590__game(object, (int)pressed);
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

void func_0033C738(gaddr object) {
    if (camera_video) {
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
    const gaddr name = page_buffers();
    openrac_game_option_word("Video", (char*)G(name), TEXT_BYTES);
    camera_line(object, CAMERA_VIDEO_ROW, name, 0);
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
