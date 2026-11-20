/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's DMA hand-offs. The native renderer consumes a chain when it is sent, so every
 * send here completes at once (as port/game/rac2/host/dma.c does for Going Commando's), and what
 * the game's completion handlers then read must be what the DMAC leaves after the whole chain. */
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"

void rac3_chain_census(gaddr channel, gaddr tag); /* chain_census.c */
void rac3_program_probe(void);                    /* program.c */
void rac3_picture_frame_sent(void);               /* picture.c */
void rac3_view_frame_sent(void);                  /* view.c */

static uint32_t reg(gaddr at) {
    uint32_t v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static void set_reg(gaddr at, uint32_t v) {
    memcpy(G(at), &v, sizeof v);
}

/* sceDmaSend (0x0012D938, the instructions of rac2-ntsc's func_00127220): as the shared
 * replacement, but the channel's CHCR is left as the DMAC leaves it after the chain's end tag: tag
 * ID 7 (end) in its upper half, the start bit clear, and the channel's interrupt handlers run.
 * Up Your Arsenal's VIF1 handler (0x003A9EF8) reads that tag field as Going Commando's does: an
 * end tag retires every stage of the frame's wait mask (0x001D8014, set by 0x003A9588 and waited
 * on by 0x003A96A8); another value retires one stage. */
void func_0012D938(gaddr channel, gaddr tag) {
    if ((tag >> 28) == 7u) {
        tag = (tag & 0x0FFFFFFFu) | 0x80000000u; /* libdma's scratchpad form, as the shared one */
    }
    if (reg(channel + 0x30u) != 0xFFFFFFFFu) {
        set_reg(channel + 0x30u, tag);
    }
    set_reg(channel + 0x20u, 0);
    set_reg(channel, (reg(channel) & ~0xCu) | 0x105u);

    rac3_chain_census(channel, tag);
    if (channel == 0x10009000u) {
        rac3_program_probe();
        rac3_picture_frame_sent();
        rac3_view_frame_sent();
    }
    openrac_game_dma_send(channel, tag);

    const uint32_t done = (reg(channel) & ~0x100u & 0x0000FFFFu) | 0x70000000u;
    set_reg(channel, done);
    openrac_game_run_dmac_handlers((int)((channel >> 12) & 0xF) - 8);
}

/* KickVif0Chain (0x003A9348, matched C in the decompilation; rac2-ntsc's func_002FC070): waits until
 * VIF0's channel is idle, writes QWC 0 and TADR, starts the channel in source-chain mode (CHCR
 * 0x145) and waits again. With the synchronous hand-off neither wait has anything to wait for. */
void func_003A9348(unsigned int chain) {
    func_0012D938(0x10008000u, (gaddr)chain & 0x0FFFFFFFu);
}

/* frontbin's and the levels' copies of KickVif0Chain (frontbin's 0x003A8F90, which rac3-decomp's
 * catalogue places in every level: hostgen names those copies func_L01_004C5698). Translated from
 * their instructions they polled VIF0's CHCR for ever, as there is no DMAC to clear it. */
void func_003A8F90__frontbin(unsigned int chain) {
    func_003A9348(chain);
}

void func_L01_004C5698(unsigned int chain) {
    func_003A9348(chain);
}
