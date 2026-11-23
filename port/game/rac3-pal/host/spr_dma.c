/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The scratchpad's DMA channels for Up Your Arsenal: its kernels and libraries
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

#define MODE_CHAIN 1u
#define TADR(chcr) ((chcr) + 0x30u)
#define TTE 0x40u /* CHCR: the tag's own quadword is transferred too */
#define TIE 0x80u /* CHCR: a tag with its IRQ bit ends the transfer */
#define CHAIN_TAGS 0x10000

static uint32_t word_at(uint32_t at) {
    return GREF(volatile uint32_t, at);
}

static void to_scratchpad(uint32_t* sadr, uint32_t from, uint32_t qwc) {
    for (uint32_t k = 0; k < qwc; k++) {
        memmove(
            G(GUEST_SCRATCHPAD + (*sadr & 0x3FF0u)),
            G((from + k * QUADWORD) & 0x01FFFFF0u),
            QUADWORD
        );
        *sadr += QUADWORD;
    }
}

static void from_scratchpad(uint32_t* sadr, uint32_t to, uint32_t qwc) {
    for (uint32_t k = 0; k < qwc; k++) {
        memmove(
            G((to + k * QUADWORD) & 0x01FFFFF0u), G(GUEST_SCRATCHPAD + (*sadr & 0x3FF0u)), QUADWORD
        );
        *sadr += QUADWORD;
    }
}

/* Chain mode. toSPR is a source chain: the tags are in memory from TADR (refe 0, cnt 1, next 2,
 * ref 3, refs 4, call 5, ret 6, end 7), each moving its data to the scratchpad. fromSPR is a
 * destination chain: each tag is read from the scratchpad, ahead of its data, and says where in
 * memory the data goes (cnt 0 and 1, end 7). Up Your Arsenal's joint pose kernel (0x003CA858, its
 * worker 0x003CA9C0) feeds the scratchpad this way and polls for the data to land. */
static void chain(uint32_t channel, uint32_t chcr) {
    uint32_t sadr = word_at(SADR(channel));
    uint32_t tadr = word_at(TADR(channel)) & 0x01FFFFF0u;
    uint32_t stack[2];
    int depth = 0;
    uint32_t tag0 = 0, tag1 = 0;
    for (int guard = 0; guard < CHAIN_TAGS; guard++) {
        int last = 0;
        if (channel == D9_CHCR) {
            tag0 = word_at(tadr);
            tag1 = word_at(tadr + 4u);
            const uint32_t qwc = tag0 & 0xFFFFu, id = (tag0 >> 28) & 7u, addr = tag1 & 0x01FFFFF0u;
            const uint32_t after = tadr + QUADWORD;
            if (chcr & TTE) {
                to_scratchpad(&sadr, tadr, 1);
            }
            switch (id) {
                case 0: /* refe */
                    to_scratchpad(&sadr, addr, qwc);
                    last = 1;
                    break;
                case 1: /* cnt */
                    to_scratchpad(&sadr, after, qwc);
                    tadr = after + qwc * QUADWORD;
                    break;
                case 2: /* next */
                    to_scratchpad(&sadr, after, qwc);
                    tadr = addr;
                    break;
                case 3: /* ref */
                case 4: /* refs */
                    to_scratchpad(&sadr, addr, qwc);
                    tadr = after;
                    break;
                case 5: /* call */
                    to_scratchpad(&sadr, after, qwc);
                    if (depth < 2) {
                        stack[depth++] = after + qwc * QUADWORD;
                    }
                    tadr = addr;
                    break;
                case 6: /* ret */
                    to_scratchpad(&sadr, after, qwc);
                    if (depth > 0) {
                        tadr = stack[--depth];
                    } else {
                        last = 1;
                    }
                    break;
                default: /* end */
                    to_scratchpad(&sadr, after, qwc);
                    last = 1;
                    break;
            }
        } else {
            const uint32_t at = GUEST_SCRATCHPAD + (sadr & 0x3FF0u);
            tag0 = word_at(at);
            tag1 = word_at(at + 4u);
            const uint32_t qwc = tag0 & 0xFFFFu, id = (tag0 >> 28) & 7u, addr = tag1 & 0x01FFFFF0u;
            sadr += QUADWORD;
            from_scratchpad(&sadr, addr, qwc);
            last = id == 7u;
        }
        if (last || ((chcr & TIE) && (tag0 & 0x80000000u))) {
            break;
        }
    }
    GREF(volatile uint32_t, SADR(channel)) = sadr;
    GREF(volatile uint32_t, TADR(channel)) = tadr;
    GREF(volatile uint32_t, QWC(channel)) = 0;
    /* CHCR's upper half holds the last tag's upper half, as the DMAC leaves it. */
    GREF(volatile uint32_t, channel) = (chcr & 0xFFFFu & ~STR) | (tag0 & 0xFFFF0000u);
}

void openrac_ee_sw_dma(openrac_u32 address, openrac_u64 value) {
    openrac_ee_sw(address, value);

    if ((address != D8_CHCR && address != D9_CHCR) || !(value & STR)) {
        return;
    }

    const uint32_t chcr = (uint32_t)value;

    if (MODE(chcr) == MODE_CHAIN) {
        chain(address, chcr);
        return;
    } else if (MODE(chcr) != MODE_NORMAL) {
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
        const uint32_t qwc = GREF(volatile uint32_t, QWC(address)) & 0xFFFFu;
        uint32_t at = sadr;

        /* SADR wraps round the scratchpad's 16 KB, as the DMAC's does. */
        if (address == D9_CHCR) {
            to_scratchpad(&at, madr, qwc);
        } else {
            from_scratchpad(&at, madr, qwc);
        }

        GREF(volatile uint32_t, MADR(address)) = madr + qwc * QUADWORD;
        GREF(volatile uint32_t, SADR(address)) = sadr + qwc * QUADWORD;
        GREF(volatile uint32_t, QWC(address)) = 0;
    }

    GREF(volatile uint32_t, address) = chcr & ~STR;
}
