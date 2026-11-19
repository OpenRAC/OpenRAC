/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A census of the display lists Up Your Arsenal sends to VIF1, for finding which of its draw paths
 * the native renderer reads (GIF packets sent DIRECT) and which it does not yet (data unpacked for a
 * VU1 microprogram, started with MSCAL/MSCNT). OPENRAC_RAC3_CHAINS=N prints the VIF codes of the
 * first N chains sent, one line each, to stderr. Development only: nothing reads it otherwise. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/guest.h"
#include "openrac/host_env.h"

static uint32_t word(gaddr at) {
    uint32_t v;
    memcpy(&v, G(at & 0x01FFFFFFu), sizeof v);
    return v;
}

/* The size in words of an UNPACK's data (VIF1 codes 0x60..0x7F), with no write mask filling. */
static uint32_t unpack_words(uint32_t code) {
    const uint32_t num = ((code >> 16) & 0xFFu) ? ((code >> 16) & 0xFFu) : 256u;
    const uint32_t vn = (code >> 26) & 3u, vl = (code >> 24) & 3u;
    const uint32_t bits = (32u >> vl) * (vn + 1u);
    return (num * bits + 31u) / 32u;
}

typedef struct {
    unsigned direct_qw, unpacks, unpack_words, mscal, mpg, other, tags;
    unsigned mscal_addr[8];
    unsigned mscal_n;
} census;

static void vif_codes(census* c, gaddr at, uint32_t words) {
    uint32_t k = 0;
    while (k < words) {
        const uint32_t code = word(at + 4u * k);
        const uint32_t cmd = (code >> 24) & 0x7Fu;
        k++;
        if (cmd == 0x50u || cmd == 0x51u) { /* DIRECT, DIRECTHL */
            uint32_t qw = code & 0xFFFFu;
            if (qw == 0) {
                qw = 0x10000u;
            }
            k = (k + 3u) & ~3u;
            c->direct_qw += qw;
            k += qw * 4u;
        } else if (cmd >= 0x60u) {
            const uint32_t n = unpack_words(code);
            c->unpacks++;
            c->unpack_words += n;
            k += n;
        } else if (cmd == 0x4Au) { /* MPG */
            const uint32_t num = ((code >> 16) & 0xFFu) ? ((code >> 16) & 0xFFu) : 256u;
            k = (k + 1u) & ~1u;
            k += num * 2u;
            c->mpg++;
        } else if (cmd == 0x14u || cmd == 0x15u || cmd == 0x17u) { /* MSCAL, MSCALF, MSCNT */
            if (c->mscal_n < 8u) {
                c->mscal_addr[c->mscal_n++] = (code & 0xFFFFu) * 8u;
            }
            c->mscal++;
        } else if (cmd == 0x20u) { /* STMASK: one word follows */
            k += 1u;
        } else if (cmd == 0x30u || cmd == 0x31u) { /* STROW, STCOL */
            k += 4u;
        } else if (cmd != 0u && cmd != 0x01u && cmd != 0x02u && cmd != 0x03u && cmd != 0x04u
                   && cmd != 0x05u && cmd != 0x06u && cmd != 0x07u && cmd != 0x10u && cmd != 0x11u
                   && cmd != 0x13u) {
            c->other++;
        }
    }
}

void rac3_chain_census(gaddr channel, gaddr tag_addr) {
    static long left = -1;
    if (left < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_CHAINS");
        left = v != NULL ? strtol(v, NULL, 10) : 0;
    }
    if (left <= 0 || channel != 0x10009000u) {
        return;
    }
    left--;
    census c;
    memset(&c, 0, sizeof c);
    gaddr at = tag_addr & 0x0FFFFFF0u;
    gaddr stack[2] = {0, 0};
    int depth = 0;
    for (int guard = 0; guard < 100000; guard++) {
        const uint32_t w0 = word(at), w1 = word(at + 4u);
        const uint32_t qwc = w0 & 0xFFFFu, id = (w0 >> 28) & 7u;
        c.tags++;
        vif_codes(&c, at + 8u, 2u);
        gaddr data = at + 16u, next = at + 16u + qwc * 16u;
        switch (id) {
        case 0: /* refe */
            vif_codes(&c, w1, qwc * 4u);
            next = 0;
            break;
        case 1: /* cnt */
            vif_codes(&c, data, qwc * 4u);
            break;
        case 2: /* next */
            vif_codes(&c, data, qwc * 4u);
            next = w1;
            break;
        case 3: /* ref */
        case 4: /* refs */
            vif_codes(&c, w1, qwc * 4u);
            next = at + 16u;
            break;
        case 5: /* call */
            vif_codes(&c, data, qwc * 4u);
            if (depth < 2) {
                stack[depth++] = at + 16u + qwc * 16u;
            }
            next = w1;
            break;
        case 6: /* ret */
            vif_codes(&c, data, qwc * 4u);
            next = depth > 0 ? stack[--depth] : 0;
            break;
        default: /* end */
            vif_codes(&c, data, qwc * 4u);
            next = 0;
            break;
        }
        if (next == 0) {
            break;
        }
        at = next & 0x0FFFFFF0u;
    }
    fprintf(stderr,
            "[rac3 chain %08x] %u tags, DIRECT %u qw, %u UNPACKs (%u words), %u MPG, %u MSCAL",
            tag_addr, c.tags, c.direct_qw, c.unpacks, c.unpack_words, c.mpg, c.mscal);
    for (unsigned k = 0; k < c.mscal_n; k++) {
        fprintf(stderr, " %#x", c.mscal_addr[k]);
    }
    fprintf(stderr, ", %u other; mode %d (0x001D5B94), next %d (0x001D5B90)\n", c.other,
            (int)word(0x001D5B94u), (int)word(0x001D5B90u));
    fprintf(stderr, "[rac3 boot] state %d (0x001D5458), frames %d (0x001D5688), 0x001D568C %d, screen %d (0x001D5690), timer %d (0x001D5694)\n",
            (int)word(0x001D5458u), (int)word(0x001D5688u), (int)word(0x001D568Cu), (int)word(0x001D5690u), (int)word(0x001D5694u));
}
