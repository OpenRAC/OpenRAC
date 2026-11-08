/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: whose draw callback is running. The draw callback lists (wide_view.c runs them)
 * call each function with its argument, for most a moby: the ship's engine flame in the space
 * fights (level 5's func_L05_004186D8, on the ship's moby) and the other effects a moby draws for
 * itself. The world quads drawn meanwhile (render.c) carry it, so that a frame between ticks can move
 * those on the moby's body with it (openrac_game_quad::parent; the window picks which).
 */
#ifndef OPENRAC_RAC2_DRAW_OWNER_H
#define OPENRAC_RAC2_DRAW_OWNER_H

#include "openrac/guest.h"

/* The running callback's argument, 0 outside the lists' runs. */
extern gaddr rac2_draw_owner;

#endif
