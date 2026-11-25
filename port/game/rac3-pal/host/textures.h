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
 * 0x001D9EA8, the levels' at Veldin's 0x001DFD20, the multiplayer core's at 0x00244428, level 39's at
 * 0x0024BD00.
 * The image is 8-bit with a 256-colour palette in the stored order. Its pixels are the game's,
 * as the game sent them when it sends them once (openrac_game_image_keep). 0 when the tables name
 * no image there.
 */
int rac3_hud_image(gaddr tables, int n, openrac_game_ui_image* out);

/*
 * The image a texture handle the game handed out names. The game's 2D routines are given their
 * texture as the handle its texture getters return (HudTextureTex0, EffectTex0, PictureTex0: a
 * 64-bit word the console's graphics chip reads), never as the image. Each getter is wrapped
 * (textures.c): it runs as the game's, and the image it named by its own tables (an index into its
 * table, or a picture's header) is kept against the handle it returned, until the getter hands the
 * same handle out for another image. rac3_texture_image gives the image kept for `handle`, or, for a
 * HUD handle, the HUD bank's entry whose stored places are the handle's (`hud_tables` as
 * rac3_hud_image's; 0 for none). A handle the game cut to its low word (a caller declared 32-bit) is
 * matched by that word. 0: no image known for it (the frame buffer's handles, EffectTex0's
 * negative numbers, are none).
 */
int rac3_texture_image(unsigned long long handle, gaddr hud_tables, openrac_game_ui_image* out);

/*
 * Another executable starts over the game's memory (LoadExecPS2: mp.c, entry.c): the images every
 * SendImage copy handed over (openrac_game_images_forget) and the images kept against handles were
 * the old program's, and are let go. The new program sends its own and names its own handles.
 */
void rac3_textures_forget(void);
