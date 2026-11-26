/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * RAC1's list-build edge: retain the real reset/swap operation and announce the cursor it selected.
 * No packet contents or header hashes stand in for the producer's construction lifetime.
 */
#ifndef OPENRAC_RAC1_FRAME_LIST_OWNER_H
#define OPENRAC_RAC1_FRAME_LIST_OWNER_H

#include "openrac/frame.h"

/* Runs the game's void reset/swap exactly once, then announces its new list root.
 * `cursor` is the readable game word that operation updates, not the old cursor value.
 * The retail operations do not yield or record commands between selecting a buffer and returning. */
static inline void rac1_frame_list_rebuilt(void (*rebuild)(void), gaddr cursor) {
    rebuild();
    gaddr root;
    memcpy(&root, G(cursor), sizeof(root));
    openrac_frame_list_begin(root);
}

#endif /* OPENRAC_RAC1_FRAME_LIST_OWNER_H */
