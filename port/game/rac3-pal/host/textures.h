/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Up Your Arsenal's images by the game's own tables (textures.c, docs/port/TEXTURES.md). */
#pragma once

#include "openrac/guest.h"
#include "openrac/ui_item.h"

/*
 * HUD texture `n` (the number the HUD's sprite routines take, HudTextureTex0's argument) of the HUD
 * bank whose three table pointers start at `tables`: entries ({u16 palette, u16 image}), images and
 * palettes ({data, u16, u8 log2 width, u8 log2 height}). The boot program's and frontbin's at
X
 * The image is 8-bit with a 256-colour palette in the stored order. Its pixels are the game's,
 * as the game sent them when it sends them once (openrac_game_image_keep). 0 when the tables name
 * no image there.
 */
int rac3_hud_image(gaddr tables, int n, openrac_game_ui_image* out);
