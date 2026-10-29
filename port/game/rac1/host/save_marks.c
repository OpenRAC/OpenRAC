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
#include <stdint.h>
#include <stdio.h>

#include "openrac/guest.h"
#include "rac1.h"

#if !RAC1_OMIT(func_00224C30)

/* The game's three fonts are drawn at a fixed 16 pixels a glyph (FontPrint, func_001F6668), so a
 * badge smaller than the rows beside it is drawn here glyph by glyph instead: the small font's
 * table and texture, each quad BADGE_SIZE pixels and the advances scaled with it. */
#define SMALL_FONT RAC1_ADDR(0x001DF770u) /* small_font_metrics: {u, v, top, advance} a glyph */
#define FONT_PIXELS 16 /* what a glyph is drawn at unscaled */
#define BADGE_SIZE 11  /* and what this badge is drawn at */

/* The flag that says this is the PAL game (its rows are further apart). */
#define PAL_LINES RAC1_ADDR(0x0015EE80u)

/* MenuScreen (games/rac1/include/menu.h): its width at +0x20. */
#define MENU_WIDTH 0x20u

/* The badge, in the colour the game gives a slot it is not pointing at. */
#define BADGE_COLOUR 0x80FFA888ull
#define BADGE_RIGHT 6 /* how far its right edge sits from the panel's */

#define SLOTS 5

/* save_info only reads the head of a save (the sizes and the first four blocks). */
#define SAVE_BYTES_READ 64u

static const char kBadge[] = "MODS";

#define GLYPH(c) (SMALL_FONT + 4u * (unsigned char)(c))
#define GLYPH_U(c) GREF(uint8_t, GLYPH(c) + 0u)
#define GLYPH_V(c) GREF(uint8_t, GLYPH(c) + 1u)
#define GLYPH_TOP(c) GREF(int8_t, GLYPH(c) + 2u)
#define GLYPH_ADVANCE(c) GREF(int8_t, GLYPH(c) + 3u)

#define SCALED(n) ((n) * BADGE_SIZE / FONT_PIXELS)

/* How wide the badge is once drawn, to put its right edge where it belongs. */
static int badge_width(void) {
    int width = 0;
    for (const char* c = kBadge; *c != '\0'; ++c) {
        width += SCALED(GLYPH_ADVANCE(*c));
    }
    return width;
}

/* The badge at (`x`, `y`), the game's own small font shrunk to BADGE_SIZE. */
static void draw_badge(int x, int y) {
    const unsigned long long texture = func_001F4868(2); /* the small font's */
    for (const char* c = kBadge; *c != '\0'; ++c) {
        func_001F5800(
            x,
            y + SCALED(GLYPH_TOP(*c)),
            BADGE_SIZE,
            BADGE_SIZE,
            GLYPH_U(*c),
            GLYPH_V(*c),
            FONT_PIXELS,
            FONT_PIXELS,
            BADGE_COLOUR,
            texture
        );
        x += SCALED(GLYPH_ADVANCE(*c));
    }
}

/* Whether slot `i`'s save has a mark beside it. The page is drawn every frame and a save is
 * written far less often, so the marks are looked for once every FRAMES_BETWEEN_LOOKS draws. */
#define FRAMES_BETWEEN_LOOKS 60

static int marks_known[SLOTS];
static int marks_until_look;

/* Whether the file at `path` exists. */
static int file_is_there(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

/* Whether slot `s` holds a game, as the port's own save reader says (game_host.h, play.c's
 * save_info). A slot the player has not saved into is written as a blank by the game and must
 * show no badge, whatever a mark left beside it by an older build says. */
static int slot_has_a_save(const char* path) {
    if (openrac_game_play_host == NULL || openrac_game_play_host->save_info == NULL) {
        return 0;
    }
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    /* save_info checks the sizes against the whole file, so it needs the file's length; the bytes
     * it reads are the head (the two sizes and the first four blocks). */
    long size = 0;
    uint8_t head[SAVE_BYTES_READ];
    size_t read = 0;
    if (fseek(f, 0, SEEK_END) == 0) {
        size = ftell(f);
        if (fseek(f, 0, SEEK_SET) == 0) {
            read = fread(head, 1, sizeof head, f);
        }
    }
    fclose(f);
    if (size <= 0 || read < sizeof head) {
        return 0;
    }
    int32_t level = 0, bolts = 0, gold_bolts = 0, seconds = 0;
    return openrac_game_play_host->save_info(
               head, (uint32_t)size, &level, &bolts, &gold_bolts, &seconds
           )
           != 0;
}

static void look_for_marks(void) {
    if (marks_until_look-- <= 0) {
        marks_until_look = FRAMES_BETWEEN_LOOKS;
        for (int s = 0; s < SLOTS; ++s) {
            char save[1024];
            char mark[1100];
            const char* cards = openrac_game_card_dir != NULL ? openrac_game_card_dir : "memcard";
            marks_known[s] = 0;
            if (snprintf(save, sizeof save, "%s/%s/save%d.bin", cards, RAC1_SAVE_FOLDER, s)
                    < (int)sizeof save
                && snprintf(mark, sizeof mark, "%s.mods", save) < (int)sizeof mark) {
                marks_known[s] = file_is_there(mark) && slot_has_a_save(save);
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
    const int x = width - BADGE_RIGHT - badge_width();
    if (x <= 0) {
        return drawn;
    }
    look_for_marks();
    for (int i = 0; i < SLOTS; ++i) {
        if (slot_is_modded(i)) {
            draw_badge(x, 4 + i * (0x30 + stride));
        }
    }
    return drawn;
}

#endif /* !RAC1_OMIT(func_00224C30) */
