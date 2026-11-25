/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * What a game's host C says about the picture its display lists make (openrac_game_dma_send in
 * openrac/game_host.h). Implemented in program/game_host.cpp; the game's thread alone calls it.
 * Apart from game_host.h, which every translated program includes, so that only the host C that
 * says it includes it. */
#ifndef OPENRAC_PICTURE_H
#define OPENRAC_PICTURE_H

#ifdef __cplusplus
extern "C" {
#endif

/* The display lists the game sends from now until its next vertical blank are one picture. A list
 * sent with no vertical blank since the last one otherwise ends the frame at the send (the loops
 * the DMA alone paces); a game whose frame sends several and then waits for its blank says so
 * each frame, before its second send: Up Your Arsenal's split-screen maps, which send each player's
 * view in a list of its own (host/mp_maps.c). The sends still run the blank's work for the game;
 * the window draws all the lists together at the blank (program/picture_lists.h). */
void openrac_game_picture_spans_sends(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_PICTURE_H */
