/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The scratchpad's DMA channels for Going Commando's hand-written kernels. CollLine (0x002761F8)
 * and about twenty others stream their data into the scratchpad by writing the channel's registers
 * and polling CHCR's STR (0x100) until it clears; the port has no DMA controller, so nothing would
 * clear it and they would poll for ever. hostgen sends those kernels' word stores here
 * (hostgen.json's asm.dma_stores): each store happens as usual, and one that starts a channel does
 * its transfer at once, leaving the registers as a finished transfer leaves them (as
 * rac1-pal/host/spr_dma.c does for its WadDecompress). */
#include <stdio.h>
#include <string.h>

#include "openrac/ee.h"
#include "openrac/guest.h"

#define D8_CHCR 0x1000D000u /* fromSPR */
#define D9_CHCR 0x1000D400u /* toSPR */
#define MADR(chcr) ((chcr) + 0x10u)
#define QWC(chcr) ((chcr) + 0x20u)
#define SADR(chcr) ((chcr) + 0x80u)
#define STR 0x100u
#define MODE(chcr) (((chcr) >> 2) & 3u)
#define MODE_NORMAL 0u
#define QUADWORD 16u

void openrac_ee_sw_dma(openrac_u32 address, openrac_u64 value) {
    openrac_ee_sw(address, value);

    if ((address != D8_CHCR && address != D9_CHCR) || !(value & STR)) {
        return;
    }

    const uint32_t chcr = (uint32_t)value;

    if (MODE(chcr) != MODE_NORMAL) {
        static int warned;

        if (!warned) {
            warned = 1;
            fprintf(
                stderr,
                "[warning] DMA channel at %08X started in mode %u: not transferred\n",
                address,
                MODE(chcr)
            );
        }
    } else {
        const uint32_t madr = GREF(volatile uint32_t, MADR(address));
        const uint32_t sadr = GREF(volatile uint32_t, SADR(address));
        const uint32_t qwc = GREF(volatile uint32_t, QWC(address));
        const uint32_t memory = madr & 0x01FFFFF0u;
        const uint32_t offset = sadr & 0x3FF0u;
        uint32_t bytes = qwc * QUADWORD;

        if (offset + bytes > GUEST_SCRATCHPAD_SIZE) {
            bytes = GUEST_SCRATCHPAD_SIZE - offset;
        }

        if (address == D9_CHCR) {
            memmove(G(GUEST_SCRATCHPAD + offset), G(memory), bytes);
        } else {
            memmove(G(memory), G(GUEST_SCRATCHPAD + offset), bytes);
        }

        GREF(volatile uint32_t, MADR(address)) = madr + qwc * QUADWORD;
        GREF(volatile uint32_t, SADR(address)) = sadr + qwc * QUADWORD;
        GREF(volatile uint32_t, QWC(address)) = 0;
    }

    GREF(volatile uint32_t, address) = chcr & ~STR;
}
