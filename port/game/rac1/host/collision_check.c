/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the collision queries checked against the kernels' own instructions
 * (OPENRAC_COLL_CHECK, a developer's switch).
 *
 * The kernels are Insomniac's hand-written EE and VU0 code: both decompilations keep them as
 * assembly, and collision.c is the port's C for them. hostgen also translates that assembly
 * (asmc.py, from the player's disc, hostgen.json "asm" "shadows") into openrac_shadow_<kind>, which
 * the game never calls. With the switch on, each query runs both ways from the same state:
 *
 * 1. the translation first: every block of game memory it reads or writes (16 bytes each) is kept
 *    as it was before its first touch (openrac_shadow_ptr, which its loads and stores go through),
 *    and so are the CollOutput's pose cache, the scratchpad and VU0;
 * 2. what it left is kept, then everything is put back (collision.c's own state too);
 * 3. collision.c runs, and the result, those blocks, the pose cache and the hit registers are
 *    compared with the translation's. The game goes on with collision.c's.
 *
 * OPENRAC_COLL_CHECK=N also compares all of main memory every Nth query (the slow way, for writes
 * outside the translation's blocks). Differences go to stderr, "[coll-check]", with a tally.
 */
#include "collision_check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/ee.h"
#include "openrac/host_env.h"

/* hostgen's translations, 0 where it had no assembly (asm_shadows.c). */
extern void* const openrac_shadow_coll_line_fn;
extern void* const openrac_shadow_coll_line_boot_fn;
extern void* const openrac_shadow_coll_sphere_fn;
extern void* const openrac_shadow_coll_capsule_fn;
extern void* const openrac_shadow_coll_sphere_mobys_fn;
extern void* const openrac_shadow_coll_hero_groups_fn;

typedef int (*LineFn)(uint32_t, uint32_t, int, int, int);
typedef int (*SphereFn)(float, uint32_t, int, uint32_t);
typedef int (*CapsuleFn)(float, float, uint32_t, int, int);
typedef int (*MobysFn)(float, uint32_t, int, uint32_t, uint32_t);
typedef int (*GroupsFn)(float, uint32_t);

static const char* const NAMES[RAC_COLL_KINDS] =
    {"line", "sphere", "capsule", "sphere-mobys", "hero-groups", "boot line"};

static void* shadow_of(int kind) {
    switch (kind) {
        case RAC_COLL_LINE:
            return openrac_shadow_coll_line_fn;
        case RAC_COLL_LINE_BOOT:
            return openrac_shadow_coll_line_boot_fn;
        case RAC_COLL_SPHERE:
            return openrac_shadow_coll_sphere_fn;
        case RAC_COLL_CAPSULE:
            return openrac_shadow_coll_capsule_fn;
        case RAC_COLL_SPHERE_MOBYS:
            return openrac_shadow_coll_sphere_mobys_fn;
        case RAC_COLL_HERO_GROUPS:
            return openrac_shadow_coll_hero_groups_fn;
        default:
            return NULL;
    }
}

static int run_shadow(int kind, const RacCollArgs* a) {
    void* fn = shadow_of(kind);
    switch (kind) {
        case RAC_COLL_LINE:
        case RAC_COLL_LINE_BOOT:
            return ((LineFn)fn)(a->u[0], a->u[1], (int)a->u[2], (int)a->u[3], (int)a->u[4]);
        case RAC_COLL_SPHERE:
            return ((SphereFn)fn)(a->f[0], a->u[0], (int)a->u[1], a->u[2]);
        case RAC_COLL_CAPSULE:
            return ((CapsuleFn)fn)(a->f[0], a->f[1], a->u[0], (int)a->u[1], (int)a->u[2]);
        case RAC_COLL_SPHERE_MOBYS:
            return ((MobysFn)fn)(a->f[0], a->u[0], (int)a->u[1], a->u[2], a->u[3]);
        case RAC_COLL_HERO_GROUPS:
            return ((GroupsFn)fn)(a->f[0], a->u[0]);
        default:
            return 0;
    }
}

/* ---- The blocks the translation touches ---- */

typedef struct {
    gaddr at;
    uint8_t before[16];
    uint8_t after[16];
} Block;

static int g_logging;
static uint8_t* g_seen; /* a bit for each 16-byte block of main memory */
static Block* g_blocks;
static size_t g_count, g_cap;

/* ---- The scratchpad's DMA ----
 *
 * The line and volume kernels bring the collision tree's cells into the scratchpad by DMA (to SPR,
 * channel 9: CHCR 0x1000D400, MADR +0x10, QWC +0x20, SADR +0x80) and wait for the start bit to
 * clear. The port has no DMA controller, so a transfer started here finishes at the translation's
 * next access to memory, before it can look. (collision.c reads the tree where it is.) */
#define DMA_TO_SPR 0x1000D400u
#define DMA_FROM_SPR 0x1000D000u

static void dma_channel(gaddr ch, int to_spr) {
    uint32_t* chcr = &GREF(uint32_t, ch);
    if (!(*chcr & 0x100u)) {
        return;
    }
    if ((*chcr >> 2 & 3u) != 0) {
        static int said;
        if (!said++) {
            fprintf(stderr, "[coll-check] a chained DMA to the scratchpad: not modelled\n");
        }
    }
    uint32_t qwc = GREF(uint32_t, ch + 0x20) & 0xFFFFu;
    uint32_t madr = GREF(uint32_t, ch + 0x10) & 0x01FFFFF0u;
    uint32_t sadr = GREF(uint32_t, ch + 0x80) & 0x3FF0u;
    for (; qwc > 0; --qwc, madr += 16, sadr = (sadr + 16) & 0x3FF0u) {
        if (to_spr) {
            memcpy(G(GUEST_SCRATCHPAD + sadr), G(madr), 16);
        } else {
            memcpy(G(madr), G(GUEST_SCRATCHPAD + sadr), 16);
        }
    }
    GREF(uint32_t, ch + 0x10) = madr;
    GREF(uint32_t, ch + 0x80) = sadr;
    GREF(uint32_t, ch + 0x20) = 0;
    *chcr &= ~0x100u;
}

/* asm_shadows.c reaches game memory through this (its OPENRAC_EE_PTR). */
void* openrac_shadow_ptr(gaddr a) {
    if (g_logging) {
        dma_channel(DMA_TO_SPR, 1);
        dma_channel(DMA_FROM_SPR, 0);
    }
    if (g_logging && a < GUEST_RAM_SIZE) {
        const gaddr b = a >> 4;
        if (!(g_seen[b >> 3] & (1u << (b & 7)))) {
            g_seen[b >> 3] |= (uint8_t)(1u << (b & 7));
            if (g_count == g_cap) {
                g_cap = g_cap ? 2 * g_cap : 4096;
                g_blocks = realloc(g_blocks, g_cap * sizeof *g_blocks);
            }
            Block* k = &g_blocks[g_count++];
            k->at = b << 4;
            memcpy(k->before, G(k->at), 16);
        }
    }
    return G(a);
}

/* ---- Regions written outside the translation's own stores (its callees') ---- */

enum {
    REGIONS = 5
};

typedef struct {
    const char* name;
    gaddr at;
    uint32_t size;
    uint8_t* before;
    uint8_t* after;
} Region;

static Region g_regions[REGIONS];

static void regions_set(int kind) {
    const gaddr out = rac1_coll_output(kind == RAC_COLL_LINE_BOOT);
    const gaddr spec[REGIONS][2] = {
        {out, 0x80},                          /* CollOutput */
        {GREF(uint32_t, out + 8), 0x40},      /* the pose cache's entries */
        {GREF(uint32_t, out + 4), 8 * 0x800}, /* their joints */
        {0x1000D000u, 0x500},                 /* the scratchpad's DMA channels */
        {GUEST_SCRATCHPAD, GUEST_SCRATCHPAD_SIZE},
    };
    static const char* const names[REGIONS] =
        {"output", "pose tags", "pose joints", "dma", "scratchpad"};
    for (int r = 0; r < REGIONS; ++r) {
        Region* g = &g_regions[r];
        g->name = names[r];
        g->at = spec[r][0];
        g->size = spec[r][0] != 0 ? spec[r][1] : 0;
        if (g->before == NULL) {
            g->before = malloc(GUEST_SCRATCHPAD_SIZE);
            g->after = malloc(GUEST_SCRATCHPAD_SIZE);
        }
        memcpy(g->before, G(g->at), g->size);
    }
}

/* A pose cache entry's second word: the kernel's $7 when it stores it, which after a pose is what
 * the pose routine (C here) left in that register: known to neither side. No code reads it. */
static int unknown_word(gaddr a) {
    const gaddr tags = g_regions[1].at;
    return g_regions[1].size != 0 && a >= tags && a < tags + 0x40 && (a - tags) % 8 == 4;
}

/* ---- The tally ---- */

static unsigned long g_calls[RAC_COLL_KINDS], g_bad[RAC_COLL_KINDS], g_unchecked[RAC_COLL_KINDS];
static unsigned long g_why[8];
static const char* const WHY[8] =
    {"result", "memory", "output", "pose tags", "pose joints", "scratchpad", "registers", "deep"};
static unsigned long g_printed[RAC_COLL_KINDS], g_total;
static unsigned long g_deep_every;
static unsigned long g_print_max = 400;

static void tally(void) {
    fprintf(stderr, "[coll-check] tally:");
    for (int k = 0; k < RAC_COLL_KINDS; ++k) {
        fprintf(stderr, " %s %lu/%lu", NAMES[k], g_bad[k], g_calls[k]);
        if (g_unchecked[k]) {
            fprintf(stderr, " (%lu unchecked)", g_unchecked[k]);
        }
    }
    fprintf(stderr, "; differing in:");
    for (int w = 0; w < 8; ++w) {
        if (g_why[w]) {
            fprintf(stderr, " %s %lu", WHY[w], g_why[w]);
        }
    }
    fprintf(stderr, "\n");
}

int rac1_coll_check_on(void) {
    const char* v = openrac_env_string("OPENRAC_COLL_CHECK");
    if (v == NULL || *v == '\0' || strcmp(v, "0") == 0) {
        return 0;
    }
    g_deep_every = strtoul(v, NULL, 10);
    if (g_deep_every == 1) {
        g_deep_every = 0; /* "1" is on, without the deep comparison */
    }
    const char* m = openrac_env_string("OPENRAC_COLL_CHECK_PRINT");
    if (m != NULL) {
        g_print_max = strtoul(m, NULL, 10);
    }
    g_seen = calloc(GUEST_RAM_SIZE / 16 / 8, 1);
    atexit(tally);
    fprintf(
        stderr,
        "[coll-check] on: collision.c against the kernels' translations (%s)%s\n",
        openrac_shadow_coll_capsule_fn != NULL ? "built from the disc" : "none built: no assembly",
        g_deep_every ? ", all memory every few queries" : ""
    );
    return 1;
}

static void say_args(int kind, const RacCollArgs* a) {
    fprintf(stderr, "[coll-check]   %s #%lu", NAMES[kind], g_calls[kind]);
    switch (kind) {
        case RAC_COLL_LINE:
        case RAC_COLL_LINE_BOOT:
            fprintf(
                stderr,
                " a (%.4f %.4f %.4f) b (%.4f %.4f %.4f) flags %#x ignore %#x template %#x\n",
                (double)GREF(float, a->u[0]),
                (double)GREF(float, a->u[0] + 4),
                (double)GREF(float, a->u[0] + 8),
                (double)GREF(float, a->u[1]),
                (double)GREF(float, a->u[1] + 4),
                (double)GREF(float, a->u[1] + 8),
                a->u[2],
                a->u[3],
                a->u[4]
            );
            break;
        default: {
            const gaddr c = a->u[0];
            fprintf(
                stderr,
                " r %.4f h %.4f at (%.4f %.4f %.4f) words %#x %#x %#x\n",
                (double)a->f[0],
                (double)a->f[1],
                (double)GREF(float, c),
                (double)GREF(float, c + 4),
                (double)GREF(float, c + 8),
                a->u[1],
                a->u[2],
                a->u[3]
            );
            break;
        }
    }
}

/* One difference, said (up to the print limit) and counted once per query and kind of place. */
static int differ(int kind, const RacCollArgs* a, int* said, int why, const char* fmt, ...)
    __attribute__((format(printf, 5, 6)));

#include <stdarg.h>

static int differ(int kind, const RacCollArgs* a, int* said, int why, const char* fmt, ...) {
    if (!(said[0] & (1 << why))) {
        said[0] |= 1 << why;
        g_why[why]++;
    }
    if (g_printed[kind] < g_print_max && said[1] < 6) {
        if (said[1] == 0) {
            say_args(kind, a);
        }
        said[1]++;
        g_printed[kind]++;
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "[coll-check]     ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }
    return 1;
}

static void compare_bytes(
    int kind,
    const RacCollArgs* a,
    int* said,
    int why,
    const char* what,
    gaddr at,
    const uint8_t* want,
    const uint8_t* got,
    uint32_t size
) {
    for (uint32_t o = 0; o < size; o += 4) {
        uint32_t w, g;
        memcpy(&w, want + o, 4);
        memcpy(&g, got + o, 4);
        if (w != g && !unknown_word(at + o)) {
            float wf, gf;
            memcpy(&wf, &w, 4);
            memcpy(&gf, &g, 4);
            differ(
                kind,
                a,
                said,
                why,
                "%s %#x: kernel %08x (%g), port %08x (%g)",
                what,
                at + o,
                w,
                (double)wf,
                g,
                (double)gf
            );
        }
    }
}

int rac1_coll_check(int kind, const RacCollArgs* a) {
    g_calls[kind]++;
    g_total++;
    if (shadow_of(kind) == NULL) {
        g_unchecked[kind]++;
        return rac1_coll_host(kind, a);
    }
    size_t state_size;
    rac1_coll_state(NULL, NULL, &state_size);
    uint8_t state[1024];
    if (state_size > sizeof state) {
        g_unchecked[kind]++;
        return rac1_coll_host(kind, a);
    }
    rac1_coll_state(state, NULL, &state_size);
    const openrac_vu0_state vu_before = openrac_vu0;
    regions_set(kind);
    const int deep = g_deep_every && g_total % g_deep_every == 0;
    static uint8_t *ram_before, *ram_kernel;
    if (deep && ram_before == NULL) {
        ram_before = malloc(GUEST_RAM_SIZE);
        ram_kernel = malloc(GUEST_RAM_SIZE);
    }
    if (deep) {
        memcpy(ram_before, G(0), GUEST_RAM_SIZE);
    }

    /* The kernel's own instructions. A start bit the port's other code left set (nothing clears it
     * there) is no transfer of this kernel's: cleared (the channels are put back after). */
    GREF(uint32_t, DMA_TO_SPR) &= ~0x100u;
    GREF(uint32_t, DMA_FROM_SPR) &= ~0x100u;
    g_count = 0;
    g_logging = 1;
    const int want = run_shadow(kind, a);
    g_logging = 0;
    const openrac_vu0_state vu_kernel = openrac_vu0;
    for (size_t i = 0; i < g_count; ++i) {
        memcpy(g_blocks[i].after, G(g_blocks[i].at), 16);
    }
    for (int r = 0; r < REGIONS; ++r) {
        memcpy(g_regions[r].after, G(g_regions[r].at), g_regions[r].size);
    }
    if (deep) {
        memcpy(ram_kernel, G(0), GUEST_RAM_SIZE);
    }

    /* Put back, newest first (a region can hold a block). */
    for (int r = REGIONS - 1; r >= 0; --r) {
        memcpy(G(g_regions[r].at), g_regions[r].before, g_regions[r].size);
    }
    for (size_t i = g_count; i-- > 0;) {
        memcpy(G(g_blocks[i].at), g_blocks[i].before, 16);
        const gaddr b = g_blocks[i].at >> 4;
        g_seen[b >> 3] &= (uint8_t)~(1u << (b & 7));
    }
    if (deep) {
        memcpy(G(0), ram_before, GUEST_RAM_SIZE);
    }
    openrac_vu0 = vu_before;
    rac1_coll_state(NULL, state, &state_size);

    /* The port's. */
    const int got = rac1_coll_host(kind, a);

    int said[2] = {0, 0};
    if (want != got) {
        differ(kind, a, said, 0, "result: kernel %d, port %d", want, got);
    }
    for (size_t i = 0; i < g_count; ++i) {
        compare_bytes(
            kind, a, said, 1, "memory", g_blocks[i].at, g_blocks[i].after, G(g_blocks[i].at), 16
        );
    }
    /* The scratchpad is the kernels' working space; the port does not keep it (collision.c). */
    for (int r = 0; r < REGIONS - 2; ++r) {
        compare_bytes(
            kind,
            a,
            said,
            2 + r,
            g_regions[r].name,
            g_regions[r].at,
            g_regions[r].after,
            G(g_regions[r].at),
            g_regions[r].size
        );
    }
    /* The hit registers the port keeps: vf25..vf29 whole, and vf2's w. */
    const uint32_t (*vf)[4] = rac1_coll_vf();
    for (int r = 2; r < 30; ++r) {
        if (r > 2 && r < 25) {
            continue;
        }
        for (int l = r == 2 ? 3 : 0; l < 4; ++l) {
            uint32_t k;
            memcpy(&k, &vu_kernel.vf[r][l], 4);
            if (k != vf[r][l]) {
                differ(
                    kind, a, said, 6, "vf%d.%c: kernel %08x, port %08x", r, "xyzw"[l], k, vf[r][l]
                );
            }
        }
    }
    if (deep) {
        const uint8_t* now = G(0);
        for (uint32_t o = 0; o < GUEST_RAM_SIZE; o += 4) {
            if (memcmp(now + o, ram_kernel + o, 4) != 0) {
                compare_bytes(kind, a, said, 7, "all memory", o, ram_kernel + o, now + o, 4);
            }
        }
    }
    if (said[0]) {
        g_bad[kind]++;
    }
    if (g_total % 5000 == 0) {
        tally();
    }
    return got;
}
