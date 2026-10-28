/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * An opt-in named ship destination. Menu rows belong to the host; guest code receives only
 * actual program IDs. The template and the original planet use different resource mounts. */
#include <stdio.h>
#include <string.h>

#include "rac1.h"

#define DATA(a) OPENRAC_DATA(a)
#define MENU_INDICES DATA(0x001602E0u)
#define CUSTOM_TEXT 0x7B00

static gaddr items, indices, choices, widget, previous_indices, labels;
static int rows, pending_destination = -1, pending;

/* The ship's list is rebuilt on each opening; no retail array is extended in place. */
int func_00222848(gaddr menu) {
    const int result = func_00222848__game(menu);
    if (openrac_game_destination_count == 0) {
        return result;
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
        const int level = GREF(int, 0x0013D618u + 4u * i);
        if (level == 0) {
            break;
        }
        if (level < 0 || level >= count) {
            continue;
        }
        GREF(short, items + rows * 12) = GREF(short, DATA(0x001DE0C0u) + level * 12);
        GREF(short, items + rows * 12 + 2) = 1;
        GREF(short, items + rows * 12 + 8) = GREF(short, DATA(0x001DE0C0u) + level * 12 + 4);
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
            GREF(short, items + rows * 12) = GREF(short, DATA(0x001DE0C0u) + level * 12);
            GREF(short, items + rows * 12 + 2) = 1;
            GREF(short, items + rows * 12 + 8) = GREF(short, DATA(0x001DE0C0u) + level * 12 + 4);
            GREF(int, indices + rows * 4) = level;
            ++rows;
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
    return result;
}

/* Remember the logical selection before the ordinary menu converts it to a template ID. */
int func_0021BB90(gaddr menu) {
    const int ours =
        openrac_game_destination_count != 0 && menu == widget && GREF(gaddr, menu + 0x34) == items;
    const gaddr page = GREF(gaddr, DATA(0x001D5F70u) + 4);
    if (ours && page != 0 && GREF(gaddr, page + 0x40) == menu
        && (GREF(int, 0x0013CC04u) & 0x40) != 0) {
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

/* A separate text ID keeps the original planet's label intact. */
gaddr rac1_ship_destination_text(int id) {
    const int index = id - CUSTOM_TEXT;
    if (index < 0 || (unsigned)index >= openrac_game_destination_count) {
        return 0;
    }
    if (labels == 0) {
        labels = openrac_guest_static(openrac_game_destination_count * 128);
        for (unsigned i = 0; i < openrac_game_destination_count; ++i) {
            snprintf((char*)G(labels + i * 128), 128, "%s", openrac_game_destinations[i].name);
        }
    }
    return labels + index * 128;
}
