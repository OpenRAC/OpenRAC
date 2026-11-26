/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank PAL v2.00: the frame the game copies out of its frame buffer and puts back,
 * without the chip's image transfers (docs/port/SCREEN_EFFECTS.md, section 2).
 *
 * Two places keep a copy of the frame. The pause menu (func_L00_00277208's page open) reads the
 * frame back to main memory when it opens and writes it back under the menu every frame; the
 * vendor does the same through capture_draw_buffer (func_L00_001F9D40): it draws the world once,
 * grabs the buffer, and every frame after puts the grab under the monitors it builds. Both go
 * through transfer_draw_buffer_chunks (func_L00_002A21A8, the read: 64 x 64 tiles of the buffer
 * through GS image stores) and append_draw_buffer_upload_packets (func_L00_002A2258, the write: a
 * DMA tag pair and a GS image load per tile, appended to the level's packet cursor). The game
 * reads none of the tiles but to write them back, so the window keeps its own frame instead: the
 * read is the last frame it drew with its world in, kept as a port picture
 * (openrac_frame_snapshot_world), and the write is that picture drawn over the whole target at the
 * place the packets would have taken in the list, opaque as the image load is. No frame-buffer
 * block is read or written and no packet is appended.
 *
 * OPENRAC_UI_SKIP naming either function, OPENRAC_UI_REPLAY and no window keep the game's own.
 */
#include <stdio.h>
#include <string.h>

#include "openrac/frame.h"
#include "openrac/host_env.h"
#include "rac1.h"

#if !RAC1_OMIT(func_L00_002A21A8) && !RAC1_OMIT(func_L00_002A2258)
/* The port picture the frame is kept as. */
#define FRAME_COPY_PICTURE 0x52310001u
/* The level's packet cursor the upload appends through (dma_packet_cursor, D_L00_00161280). */
#define COPY_CURSOR OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00161280u))

static int copy_skipped(const char* name) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL && strstr(list, name) != NULL;
}

/* The picture is the frame's own while the read ran natively: the write keeps the packets of the
 * game when its read kept the store (no window at the read, a switch changed between). */
static int s_native_copy;

void func_L00_002A21A8(int to, int frame, int bytes) {
    if (!openrac_frame_native() || copy_skipped(RAC1_NAME(func_L00_002A21A8))) {
        s_native_copy = 0;
        func_L00_002A21A8__game(to, frame, bytes);
        return;
    }
    s_native_copy = 1;
    openrac_frame_snapshot_world(FRAME_COPY_PICTURE);
    if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
        fprintf(stderr, "screen_copy: frame read kept as a picture\n");
    }
}

void func_L00_002A2258(int from, int frame, int bytes) {
    if (!s_native_copy || !openrac_frame_native() || copy_skipped(RAC1_NAME(func_L00_002A2258))) {
        func_L00_002A2258__game(from, frame, bytes);
        return;
    }
    if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
        static int logged;
        if (logged++ < 3) {
            fprintf(
                stderr,
                "screen_copy: frame written as a picture, cursor %#x (list cursor %#x)\n",
                (unsigned)GREF(gaddr, COPY_CURSOR),
                (unsigned)GREF(gaddr, OPENRAC_DATA(RAC1_ADDR(0x00161000u)))
            );
        }
    }
    openrac_frame_anchor_at(COPY_CURSOR);
    openrac_frame_picture(
        FRAME_COPY_PICTURE, NULL, 0x80808080u, OPENRAC_BLEND_OPAQUE | OPENRAC_PICTURE_WHOLE
    );
}
#endif
