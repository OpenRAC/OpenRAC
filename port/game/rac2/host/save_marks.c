/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The memory card page's "MODS" badges, as Ratchet & Clank's (rac1/host/save_marks.c): a slot whose
 * save was written while mods were loaded says so, so a player telling two saves apart does not have
 * to open the launcher.
 *
 * The mark is a small file beside the save, `save<N>.bin.mods`, which the port's libmc writes and
 * removes as the game saves (port/game/common/lib/mc.c). The save file itself is never changed.
 *
 * save_slot_list_draw (func_002DDA10, matched C in src/game/002D5E98-g8.c) draws the four slots of
 * the Load and Save pages, at the title and in a level's pause menu, and is wrapped here
 * (libraries.tsv, port = wrap): the game's own draw runs first, then one badge per marked slot, on
 * the slot's first line (its play time), at the panel's right. Every slot is three lines high and a
 * gap below, as the game's loop steps it, so a badge's place is arithmetic.
 *
 * v1.01 only (RAC2_PRIMARY) until the PAL and v2.00 tables are written again: their copies of the
 * draw (func_PAL_002DE710, func_US2_002DE768) are their own functions.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

#if defined(RAC2_PRIMARY)

int func_002DDA10__game(gaddr menu);
void func_0027C540(int);
void func_0027C660(void);
unsigned long long func_0027C878(int index);
int func_0027F198(gaddr text, int length);
void func_0027F640(
    float x,
    float y,
    unsigned long long colour,
    gaddr text,
    int limit,
    float scale,
    unsigned long long tex0,
    gaddr glyphs
);

/* The page's state, as save_slot_list_draw reads it. */
#define SAVE_MENU_SCREEN RAC2_ADDR(0x001A7420u) /* g_save_menu_screen: 10 draws no slots */
#define CARD_STATE RAC2_ADDR(0x001393E0u)       /* g_memcard_state */
#define CARD_PHASE 0x8u                         /* 2 once the card is read */
#define CARD_MODE 0x15Cu                        /* the slots are drawn below 3 */
#define CARD_SELECTED 0x164u                    /* -1 while no slot is being saved or loaded */
#define CARD_ROWS RAC2_ADDR(0x00139410u)        /* the four slots' summaries, 28 bytes each */
#define CARD_ROW_BYTES 28u
#define CARD_ROW_EMPTY (-1)                /* a summary's first word for an empty slot */
#define PAUSE_STATE RAC2_ADDR(0x001F2840u) /* g_pause_menu_state */
#define PAUSE_OPEN_FRAMES 0x164u           /* the page's frames open: the slots show past 10 */

/* The panel's layout (small data, as the draw reads it). */
#define LINE_HEIGHT RAC2_ADDR(0x001ABBF0u)
#define SLOT_GAP RAC2_ADDR(0x001ABBF4u)
#define LINE_TOP RAC2_ADDR(0x001ABC08u)

/* The menu widget: its left, top and width. */
#define MENU_X 0x18u
#define MENU_Y 0x1Cu
#define MENU_WIDTH 0x20u

/* The small font's glyph table, which the slots' lines are printed in (font.c, texture 2). */
#define FONT_SMALL RAC2_ADDR(0x00263EF0u)
#define FONT_SMALL_TEXTURE 2

#define SLOTS 4
#define BADGE_SCALE 0.7f
#define BADGE_PAST_MENU 10 /* its right edge, past the widget's: in the slot panel's corner */
#define BADGE_RISE 9       /* up from the first line's top, into the panel's top edge */
#define BADGE_COLOUR 0x60E0C890ull /* the panels' light blue, see-through (A, B, G, R) */
#define FRAMES_BETWEEN_LOOKS 60 /* the page is drawn every frame, a save written far less often */

static const char kBadge[] = "MODS";

static gaddr badge_text;
static int marks_known[SLOTS];
static int marks_until_look;

static int file_is_there(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

/* Which slots have a mark beside their save, looked for once every FRAMES_BETWEEN_LOOKS draws. */
static void look_for_marks(void) {
    if (marks_until_look-- > 0) {
        return;
    }
    marks_until_look = FRAMES_BETWEEN_LOOKS;
    const openrac_game_play* play = openrac_game_play_host;
    const char* cards = openrac_game_card_dir != NULL ? openrac_game_card_dir : "memcard";
    for (int s = 0; s < SLOTS; ++s) {
        marks_known[s] = 0;
        if (play == NULL || play->save_folder == NULL || play->save_file == NULL) {
            continue;
        }
        char file[64];
        char mark[1100];
        snprintf(file, sizeof file, play->save_file, s);
        if (snprintf(mark, sizeof mark, "%s/%s/%s.mods", cards, play->save_folder, file)
            < (int)sizeof mark) {
            marks_known[s] = file_is_there(mark);
        }
    }
}

/* Whether the game drew the slots' summaries this frame, as save_slot_list_draw decides. */
static int slots_shown(void) {
    const gaddr card = OPENRAC_DATA(CARD_STATE);
    return GREF(int32_t, OPENRAC_DATA(SAVE_MENU_SCREEN)) != 10
           && GREF(int32_t, card + CARD_MODE) < 3 && GREF(int32_t, card + CARD_SELECTED) < 0
           && GREF(int32_t, OPENRAC_DATA(PAUSE_STATE) + PAUSE_OPEN_FRAMES) > 10
           && GREF(int32_t, card + CARD_PHASE) == 2;
}

/* save_slot_list_draw: the game's four slots, then a badge on each one written with mods. */
int func_002DDA10(gaddr menu) {
    const int drawn = func_002DDA10__game(menu);
    if (menu == 0 || !slots_shown()) {
        return drawn;
    }
    look_for_marks();
    if (badge_text == 0) {
        badge_text = openrac_guest_static(sizeof kBadge);
        memcpy(G(badge_text), kBadge, sizeof kBadge);
    }
    const int line = GREF(int32_t, OPENRAC_DATA(LINE_HEIGHT));
    const int gap = GREF(int32_t, OPENRAC_DATA(SLOT_GAP));
    const int top = GREF(int32_t, OPENRAC_DATA(LINE_TOP));
    const float width = (float)func_0027F198(badge_text, -1) * BADGE_SCALE;
    const float x =
        (float)(GREF(int32_t, menu + MENU_X) + GREF(int32_t, menu + MENU_WIDTH) + BADGE_PAST_MENU)
        - width;
    int y = GREF(int32_t, menu + MENU_Y);
    func_0027C540(0);
    for (int s = 0; s < SLOTS; ++s) {
        const gaddr row = OPENRAC_DATA(CARD_ROWS) + CARD_ROW_BYTES * (gaddr)s;
        if (marks_known[s] && GREF(int32_t, row) != CARD_ROW_EMPTY) {
            func_0027F640(
                x,
                (float)(y + top - BADGE_RISE),
                BADGE_COLOUR,
                badge_text,
                -1,
                BADGE_SCALE,
                func_0027C878(FONT_SMALL_TEXTURE),
                OPENRAC_DATA(FONT_SMALL)
            );
        }
        y += line * 3 + gap;
    }
    func_0027C660();
    return drawn;
}

#endif /* RAC2_PRIMARY */
