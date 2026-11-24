/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The level's collision triangles in a box, for mods (openrac_api.collision_triangles). Going
 * Commando's world collision is Ratchet & Clank's format (extractor/collision.py reads both): a tree
 * of 4-unit cells (Z, then Y, then X) whose leaves hold packed vertices and faces. The tree is the
 * first word of the collision queries' output block (g_collision_hit, 0x001BAF80 on USA v1.01; its
 * +4 is the moby mesh cache, g_collMobyMeshCache), as in rac1/host/collision.c. Only the console's
 * layout is read here: Going Commando has no expanded collision. Mobys are not in it yet. */
#include <math.h>
#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

#define COLL_OUT RAC2_ADDR(0x001BAF80u)

#define U8(a) GREF(uint8_t, (a))
#define U16(a) GREF(uint16_t, (a))
#define U32(a) GREF(uint32_t, (a))

typedef struct {
    float corners[3][3];
    float normal[3];
    uint32_t type;
    int32_t moby;
} CollTriangle; /* openrac_collision_triangle's layout */

typedef struct {
    float lo[3], hi[3];
    CollTriangle* out;
    int max;
    int count;
} Sink;

/* The leaf word of cell (x, y, z), 0 for none (the cell lookup of the collision queries). */
static uint32_t cell_word(gaddr mesh, int x, int y, int z) {
    int i = z - U16(mesh);
    if (i < 0 || (int)U16(mesh + 2) - i <= 0) {
        return 0;
    }
    gaddr slab = (gaddr)U16(mesh + 4 + 2 * (gaddr)i) * 4u;
    if (slab == 0) {
        return 0;
    }
    slab += mesh;
    i = y - U16(slab);
    if (i < 0 || (int)U16(slab + 2) - i <= 0) {
        return 0;
    }
    gaddr row = U32(slab + 4 + 4 * (gaddr)i);
    if (row == 0) {
        return 0;
    }
    row += mesh;
    i = x - U16(row);
    if (i < 0 || (int)U16(row + 2) - i <= 0) {
        return 0;
    }
    return U32(row + 4 + 4 * (gaddr)i);
}

/* A packed vertex (x and y 10 bits at 1/16, z 12 bits at 1/64 of a unit from the cell's centre). */
static void cell_vertex(gaddr vertices, int i, const float centre[3], float out[3]) {
    const uint32_t w = U32(vertices + 4 * (gaddr)i);
    const int32_t f[3] =
        {((int32_t)(w << 22)) >> 22, ((int32_t)(w << 12)) >> 22, ((int32_t)w) >> 20};
    out[0] = centre[0] + (float)f[0] / 16.0f;
    out[1] = centre[1] + (float)f[1] / 16.0f;
    out[2] = centre[2] + (float)f[2] / 64.0f;
}

/* Adds the triangle when its bounds meet the box; its normal from its corners. */
static void sink_triangle(Sink* s, const float p[3][3], uint32_t type) {
    for (int k = 0; k < 3; ++k) {
        const float lo = fminf(p[0][k], fminf(p[1][k], p[2][k]));
        const float hi = fmaxf(p[0][k], fmaxf(p[1][k], p[2][k]));
        if (hi < s->lo[k] || lo > s->hi[k]) {
            return;
        }
    }
    if (s->count < s->max) {
        CollTriangle* t = &s->out[s->count];
        float e1[3], e2[3];
        for (int k = 0; k < 3; ++k) {
            for (int c = 0; c < 3; ++c) {
                t->corners[c][k] = p[c][k];
            }
            e1[k] = p[1][k] - p[0][k];
            e2[k] = p[2][k] - p[0][k];
        }
        const float n[3] =
            {e1[1] * e2[2] - e1[2] * e2[1],
             e1[2] * e2[0] - e1[0] * e2[2],
             e1[0] * e2[1] - e1[1] * e2[0]};
        const float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        for (int k = 0; k < 3; ++k) {
            t->normal[k] = len > 0.0f ? n[k] / len : 0.0f;
        }
        t->type = type;
        t->moby = -1;
    }
    ++s->count;
}

/* A box larger than 1024 units each way is cut to its low corner's 1024. */
#define CELLS_MAX 256
#define CELLS_TOP (1024 / 4 - 1)

static void level_triangles(gaddr mesh, Sink* s) {
    int lo[3], hi[3];
    for (int k = 0; k < 3; ++k) {
        lo[k] = s->lo[k] <= 0.0f ? 0 : (int)(s->lo[k] / 4.0f);
        hi[k] = s->hi[k] <= 0.0f ? -1 : (int)(s->hi[k] / 4.0f);
        if (hi[k] > CELLS_TOP) {
            hi[k] = CELLS_TOP;
        }
        if (hi[k] - lo[k] >= CELLS_MAX) {
            hi[k] = lo[k] + CELLS_MAX - 1;
        }
    }
    for (int z = lo[2]; z <= hi[2]; ++z) {
        for (int y = lo[1]; y <= hi[1]; ++y) {
            for (int x = lo[0]; x <= hi[0]; ++x) {
                const uint32_t word = cell_word(mesh, x, y, z);
                if (word == 0) {
                    continue;
                }
                /* The leaf: u16 faces, u8 vertices, u8 quads; the vertices, the faces (v0, v1, v2,
                 * type), then each quad's fourth index (a quad is face j again as v0, v2, v3). */
                const gaddr leaf = mesh + (word >> 8);
                const int nf = U16(leaf), nv = U8(leaf + 2), nq = U8(leaf + 3);
                if (nv == 0) {
                    continue;
                }
                const gaddr vertices = leaf + 4;
                const gaddr faces = vertices + 4 * (gaddr)nv;
                const gaddr fourth = faces + 4 * (gaddr)nf;
                const float centre[3] = {x * 4.0f + 2.0f, y * 4.0f + 2.0f, z * 4.0f + 2.0f};
                for (int j = 0; j < nf + nq; ++j) {
                    const gaddr f = faces + 4 * (gaddr)(j < nf ? j : j - nf);
                    const int v[3] =
                        {U8(f),
                         j < nf ? U8(f + 1) : U8(f + 2),
                         j < nf ? U8(f + 2) : U8(fourth + (gaddr)(j - nf))};
                    if (v[0] >= nv || v[1] >= nv || v[2] >= nv) {
                        continue;
                    }
                    float p[3][3];
                    for (int c = 0; c < 3; ++c) {
                        cell_vertex(vertices, v[c], centre, p[c]);
                    }
                    sink_triangle(s, p, U8(f + 3));
                }
            }
        }
    }
}

int rac2_collision_triangles(const float lo[3], const float hi[3], void* out, int max) {
    if (lo == NULL || hi == NULL || openrac_game_play_host == NULL
        || openrac_game_play_host->hero_moby == NULL || openrac_game_play_host->hero_moby() == 0) {
        return 0;
    }
    Sink s;
    for (int k = 0; k < 3; ++k) {
        if (!isfinite(lo[k]) || !isfinite(hi[k]) || hi[k] < lo[k]) {
            return 0;
        }
        s.lo[k] = lo[k];
        s.hi[k] = hi[k];
    }
    s.out = (CollTriangle*)out;
    s.max = out != NULL && max > 0 ? max : 0;
    s.count = 0;
    const gaddr mesh = U32(OPENRAC_DATA(COLL_OUT));
    if (mesh != 0) {
        level_triangles(mesh, &s);
    }
    return s.count;
}
