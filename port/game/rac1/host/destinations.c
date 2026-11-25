/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * An opt-in named ship destination. Menu rows belong to the host; guest code receives only
 * actual program IDs. The template and the original planet use different resource mounts. */
#include <stdio.h>
#include <string.h>

#include "rac1.h"

#define DATA(a) OPENRAC_DATA(a)
#define MENU_INDICES DATA(RAC1_ADDR(0x001602E0u)) /* menu_level_indices */
#define CUSTOM_TEXT 0x7B00

static gaddr items, indices, choices, widget, previous_indices, labels;
static int rows, pending_destination = -1, pending;

/* How a version declares the menu's builder (rac1_version.h can say otherwise): PAL's C takes the
 * menu and returns an int. */
#ifndef RAC1_SHIP_MENU_PARAMS
#define RAC1_SHIP_MENU_RESULT int
#define RAC1_SHIP_MENU_PARAMS gaddr menu
#define RAC1_SHIP_MENU_CALL(menu) func_00222848__game(menu)
#define RAC1_SHIP_MENU_RETURN(result) return result
#endif

/* The ship's list is rebuilt on each opening; no retail array is extended in place. */
RAC1_SHIP_MENU_RESULT func_00222848(RAC1_SHIP_MENU_PARAMS) {
    const int result = RAC1_SHIP_MENU_CALL(menu);
    if (openrac_game_destination_count == 0) {
        RAC1_SHIP_MENU_RETURN(result);
    }
    const int count = openrac_game_front_end->levels;
    if (items == 0) {
        items = openrac_guest_static((count + (int)openrac_game_destination_count + 1) * 12);
        indices = openrac_guest_static((count + (int)openrac_game_destination_count + 1) * 4);
        choices = openrac_guest_static((count + (int)openrac_game_destination_count + 1) * 4);
    }
    memset(G(items), 0, (count + (int)openrac_game_destination_count + 1) * 12);
    memset(G(indices), 0, (count + (int)openrac_game_destination_count + 1) * 4);
    rows = 0;
    memset(G(choices), 0xFF, (count + (int)openrac_game_destination_count + 1) * 4);
    for (int i = 0; i < count; ++i) {
        const int level = GREF(int, RAC1_ADDR(0x0013D618u) + 4u * i); /* gGalaxyList */
        if (level == 0) {
            break;
        }
        if (level < 0 || level >= count) {
            continue;
        }
        GREF(short, items + rows * 12) = GREF(short, DATA(RAC1_ADDR(0x001DE0C0u)) + level * 12);
        GREF(short, items + rows * 12 + 2) = 1;
        GREF(short, items + rows * 12 + 8) = GREF(short, DATA(RAC1_ADDR(0x001DE0C0u)) + level * 12 + 4);
        GREF(int, indices + rows * 4) = level;
        ++rows;
    }
    /* Keep each donor planet accessible, without adding duplicate rows for shared templates. */
    for (unsigned d = 0; d < openrac_game_destination_count; ++d) {
        const int level = openrac_game_destinations[d].level;
        int found = 0;
        for (int i = 0; i < rows; ++i) {
            found |= GREF(int, indices + i * 4) == level;
        }
        if (!found) {
            GREF(short, items + rows * 12) = GREF(short, DATA(RAC1_ADDR(0x001DE0C0u)) + level * 12);
            GREF(short, items + rows * 12 + 2) = 1;
            GREF(short, items + rows * 12 + 8) = GREF(short, DATA(RAC1_ADDR(0x001DE0C0u)) + level * 12 + 4);
            GREF(int, indices + rows * 4) = level;
            ++rows;
        }
    }
    /* A template planet's own row names it by a text of its own, so it keeps its name while the
     * planet texts name a destination (rac1_ship_destination_text). */
    for (int i = 0; i < rows; ++i) {
        const int level = GREF(int, indices + i * 4);
        for (unsigned d = 0; d < openrac_game_destination_count; ++d) {
            if (openrac_game_destinations[d].level == level) {
                GREF(short, items + i * 12) = (short)(CUSTOM_TEXT + 0x80 + level);
                GREF(short, items + i * 12 + 8) = (short)(CUSTOM_TEXT + 0x100 + level);
            }
        }
    }
    const int first_custom = rows;
    for (unsigned d = 0; d < openrac_game_destination_count; ++d) {
        GREF(short, items + rows * 12) = CUSTOM_TEXT + d;
        GREF(short, items + rows * 12 + 2) = 1;
        GREF(int, indices + rows * 4) = openrac_game_destinations[d].level;
        GREF(int, choices + rows * 4) = d;
        ++rows;
    }
    if (GREF(gaddr, MENU_INDICES) != indices) {
        previous_indices = GREF(gaddr, MENU_INDICES);
    }
    GREF(gaddr, MENU_INDICES) = indices;
    GREF(gaddr, menu + 0x34) = items;
    GREF(int, menu + 0x40) =
        openrac_game_active_destination >= 0 ? first_custom + openrac_game_active_destination : 0;
    widget = menu;
    pending = 0;
    fprintf(
        stderr,
        "[info] ship catalog: %u added destinations after %d original programs, %d available "
        "rows\n",
        openrac_game_destination_count,
        count,
        rows
    );
    RAC1_SHIP_MENU_RETURN(result);
}

/* Remember the logical selection before the ordinary menu converts it to a template ID. */
int func_0021BB90(gaddr menu) {
    const int ours =
        openrac_game_destination_count != 0 && menu == widget && GREF(gaddr, menu + 0x34) == items;
    const gaddr page = GREF(gaddr, DATA(RAC1_ADDR(0x001D5F70u)) + 4);
    if (ours && page != 0 && GREF(gaddr, page + 0x40) == menu
        && (GREF(int, RAC1_ADDR(0x0013CC04u)) & 0x40) != 0) {
        const int row = GREF(int, menu + 0x40);
        if (row >= 0 && row < rows) {
            pending_destination = GREF(int, choices + row * 4);
            pending = 1;
        }
    }
    const int result = func_0021BB90__game(menu);
    if (ours && result < 0) {
        pending = 0;
        GREF(gaddr, MENU_INDICES) = previous_indices;
    }
    return result;
}

/* The game has already closed the ship computer. Request a real level load even when two
 * destinations share a template; retail's same-planet branch would only land in the old world. */
void func_L00_00290030(int target) {
    if (openrac_game_destination_count == 0) {
        func_L00_00290030__game(target);
        return;
    }
    const int destination = pending ? pending_destination : -1;
    const int custom = destination >= 0 && (unsigned)destination < openrac_game_destination_count
                       && target == openrac_game_destinations[destination].level;
    const int leaving_map = openrac_game_map != NULL;
    const int program = custom ? openrac_game_destinations[destination].level : target;
    if (!openrac_game_has_level(program)) {
        openrac_game_crash("The ship selected an unavailable template program");
    }
    pending = 0;
    GREF(gaddr, MENU_INDICES) = previous_indices;
    if (!custom && !leaving_map) {
        func_L00_00290030__game(target);
        return;
    }
    openrac_game_select_destination(custom ? destination : -1);
    fprintf(
        stderr,
        "[info] ship travel: destination %d, template %d, map mount %s\n",
        custom ? openrac_game_front_end->levels + destination : target,
        program,
        custom ? "on" : "off"
    );
    func_0022F4A0(program);
}

/* The destination the planet texts are about now: on the ship's planet list the row under the
 * cursor (-1 for an original planet), then the one picked until Ratchet lands there, elsewhere
 * the planet Ratchet is on. */
static int text_destination(void) {
    const gaddr page = GREF(gaddr, DATA(RAC1_ADDR(0x001D5F70u)) + 4);
    if (widget != 0 && page != 0 && GREF(gaddr, page + 0x40) == widget
        && GREF(gaddr, widget + 0x34) == items) {
        const int row = GREF(int, widget + 0x40);
        return row >= 0 && row < rows ? GREF(int, choices + row * 4) : -1;
    }
    if (pending) {
        return pending_destination;
    }
    return openrac_game_active_destination;
}

/* A separate text ID keeps the original planet's label intact; the template planet's own area
 * and name (level_select_texts, D_001DE0C0: the area's text, then the planet's) read as nothing
 * and the destination's name while it is the planet in question, so its galaxy label and map
 * page name it (the flight's caption is a picture: flight_pictures.c). */
gaddr rac1_ship_destination_text(int id) {
    if (openrac_game_destination_count == 0) {
        return 0;
    }
    if (labels == 0) {
        labels = openrac_guest_static(openrac_game_destination_count * 128 + 4);
        for (unsigned i = 0; i < openrac_game_destination_count; ++i) {
            snprintf((char*)G(labels + i * 128), 128, "%s", openrac_game_destinations[i].name);
        }
        GREF(char, labels + openrac_game_destination_count * 128) = 0;
    }
    const int index = id - CUSTOM_TEXT;
    if (index >= 0 && (unsigned)index < openrac_game_destination_count) {
        return labels + index * 128;
    }
    const int d = text_destination();
    if (d < 0 || (unsigned)d >= openrac_game_destination_count) {
        return 0;
    }
    const gaddr row = DATA(RAC1_ADDR(0x001DE0C0u)) + openrac_game_destinations[d].level * 12;
    if (id == GREF(int, row + 4)) {
        return labels + d * 128;
    }
    if (id == GREF(int, row)) {
        return labels + openrac_game_destination_count * 128;
    }
    return 0;
}

/* The game's own text id behind a template planet's list row (above); any other id as it is. */
int rac1_ship_text_id(int id) {
    const int levels = openrac_game_front_end->levels;
    if (openrac_game_destination_count != 0 && id >= CUSTOM_TEXT + 0x80
        && id < CUSTOM_TEXT + 0x100 + levels) {
        const int area = id < CUSTOM_TEXT + 0x100;
        const int level = id - CUSTOM_TEXT - (area ? 0x80 : 0x100);
        if (level < levels) {
            return GREF(int, DATA(RAC1_ADDR(0x001DE0C0u)) + level * 12 + (area ? 0 : 4));
        }
    }
    return id;
}
