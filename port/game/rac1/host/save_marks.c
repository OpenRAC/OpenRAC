/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The memory card page's "MODS" badges: a slot whose save was written while mods were loaded says
 * so, so a player telling two saves apart does not have to open the launcher.
 *
 * The mark is a small file beside the save, `save<N>.bin.mods`, which the port's libmc writes and
 * removes as the game saves (port/game/common/lib/mc.c). The save file itself is never changed:
 * its header has no room, every section carries a CRC, and a real memory card must still take it.
 *
 * draw_save_slot_list (func_00224C30, matched C) draws the five slots and is wrapped here
 * (libraries.tsv, port = wrap): the game's own draw runs first, then one badge per marked slot.
 * The rows it draws are a fixed height, so a badge's place is arithmetic, not a read of the menu's
 * state: the first row of slot i is at y = 4 + i * (0x30 + stride), stride 0x24 in PAL and 0x1B in
 * NTSC, as the game's own loop steps it.
 */
#include <stdio.h>
#include <string.h>

#include "openrac/guest.h"
#include "rac1.h"

#if !RAC1_OMIT(func_00224C30)

/* The game's small font print (FontPrintSmall), and the flag that says this is the PAL game. */
#define PAL_LINES RAC1_ADDR(0x0015EE80u)

/* MenuScreen (games/rac1/include/menu.h): its width at +0x20. */
#define MENU_WIDTH 0x20u

/* The badge, where the game's own first row of a slot ends, in the slot's own colour. */
#define BADGE_COLOUR 0x80FFA888ull
#define BADGE_RIGHT 0x34 /* how far from the panel's right edge the badge starts */

#define SLOTS 5

/* "MODS" in game memory, made once: the game's font print takes a pointer the game can read. */
static gaddr badge_text(void) {
    static gaddr text;
    if (text == 0) {
        static const char word[] = "MODS";
        text = openrac_guest_static((uint32_t)sizeof word);
        for (unsigned i = 0; i < sizeof word; ++i) {
            GREF(char, text + i) = word[i];
        }
    }
    return text;
}

/* Whether slot `i`'s save has a mark beside it. The page is drawn every frame and a save is
 * written far less often, so the marks are looked for once every FRAMES_BETWEEN_LOOKS draws. */
#define FRAMES_BETWEEN_LOOKS 60

static int marks_known[SLOTS];
static int marks_until_look;

static void look_for_marks(void) {
    if (marks_until_look-- <= 0) {
        marks_until_look = FRAMES_BETWEEN_LOOKS;
        for (int s = 0; s < SLOTS; ++s) {
            char path[1024];
            const char* cards = openrac_game_card_dir != NULL ? openrac_game_card_dir : "memcard";
            marks_known[s] = 0;
            if (snprintf(path, sizeof path, "%s/%s/save%d.bin.mods", cards, RAC1_SAVE_FOLDER, s)
                < (int)sizeof path) {
                FILE* f = fopen(path, "rb");
                if (f != NULL) {
                    marks_known[s] = 1;
                    fclose(f);
                }
            }
        }
    }
}

static int slot_is_modded(int i) {
    return i >= 0 && i < SLOTS ? marks_known[i] : 0;
}

/* draw_save_slot_list: the game's five slots, then a badge on each one written with mods. */
int func_00224C30(gaddr menu) {
    const int drawn = func_00224C30__game(menu);
    const gaddr screen = menu;
    if (screen == 0) {
        return drawn;
    }
    const int width = GREF(int, screen + MENU_WIDTH);
    const int stride = GREF(int, PAL_LINES) != 0 ? 0x24 : 0x1B;
    const int x = width - BADGE_RIGHT;
    if (x <= 0) {
        return drawn;
    }
    look_for_marks();
    for (int i = 0; i < SLOTS; ++i) {
        if (slot_is_modded(i)) {
            func_001F6968(x, 4 + i * (0x30 + stride), BADGE_COLOUR, badge_text(), -1);
        }
    }
    return drawn;
}

#endif /* !RAC1_OMIT(func_00224C30) */
