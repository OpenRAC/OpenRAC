/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The collision triangles in a box, for mods (openrac_api.collision_triangles). The level's
 * collision uses Ratchet & Clank's format (rac1/host/collision.c): a tree of 4-unit cells whose
 * leaves hold packed vertices and faces. Its root is the first word of g_collision_hit. After the
 * level come the collision meshes of the live mobys near the box (crates, ships, doors). */
#include <math.h>
#include <stdint.h>

#include "game_protos.h"
/* hostgen names a release's function in the .c files only (moby_checks.h). */
#define RAC2_MOBY_TARGET_RECORD func_002AB1F0
#include "moby_checks.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

#define COLL_OUT RAC2_ADDR(0x001BAF80u) /* g_collision_hit */

/* A moby: its bounding sphere (+0, radius +0xC, 1/1024 units), place, scale, state, collision
 * blob and rows. */
#define MOBY_SIZE 0x100u
#define MOBY_RADIUS 0x0Cu
#define MOBY_POS 0x10u
#define MOBY_STATE 0x20u
#define MOBY_SCALE 0x2Cu
#define MOBY_COLLISION 0x98u
#define MOBY_ROWS 0xC0u

#define U8(a) GREF(uint8_t, (a))
#define U16(a) GREF(uint16_t, (a))
#define U32(a) GREF(uint32_t, (a))
#define S16(a) GREF(int16_t, (a))
#define S32(a) GREF(int32_t, (a))
#define F32(a) GREF(float, (a))

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
static void sink_triangle(Sink* s, const float p[3][3], uint32_t type, int32_t moby) {
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
        t->moby = moby;
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
                    sink_triangle(s, p, U8(f + 3), -1);
                }
            }
        }
    }
}

/* A moby's mesh. Its blob holds the primitives' size (+4, high bit: u16 indices), the faces' size
 * (+8) and the vertices' size (+0xC). From +0x10, after the primitives: the vertices (three s16 and
 * a pad, in 1/1024 of the moby's scaled rows), then the faces (v0, v1, v2, type). */
static void moby_mesh(gaddr m, int32_t slot, Sink* s) {
    const gaddr blob = U32(m + MOBY_COLLISION);
    if (!rac2_ram_at(blob, 0x10u)) {
        return;
    }
    const uint32_t primitives = U32(blob + 4);
    const int wide = (primitives & 0x80000000u) != 0;
    const int32_t face_bytes = S32(blob + 8);
    const int32_t vertex_bytes = S32(blob + 0xC);
    if (vertex_bytes <= 0 || face_bytes <= 0) {
        return;
    }
    const gaddr verts = blob + 0x10u + (primitives & 0x7fffffffu);
    const gaddr faces = verts + (gaddr)vertex_bytes;
    if (!rac2_ram_at(verts, (uint32_t)vertex_bytes + (uint32_t)face_bytes)) {
        return;
    }
    const int nverts = vertex_bytes / 8;
    const int nfaces = face_bytes / (wide ? 8 : 4);
    float rows[3][3];
    for (int r = 0; r < 3; ++r) {
        for (int k = 0; k < 3; ++k) {
            rows[r][k] =
                F32(m + MOBY_ROWS + 0x10u * (gaddr)r + 4u * (gaddr)k) * F32(m + MOBY_SCALE);
        }
    }
    const float pos[3] = {F32(m + MOBY_POS), F32(m + MOBY_POS + 4u), F32(m + MOBY_POS + 8u)};
    for (int i = 0; i < nfaces; ++i) {
        const gaddr f = faces + (wide ? 8u : 4u) * (gaddr)i;
        const int v[3] =
            {wide ? U16(f) : U8(f), wide ? U16(f + 2) : U8(f + 1), wide ? U16(f + 4) : U8(f + 2)};
        if (v[0] >= nverts || v[1] >= nverts || v[2] >= nverts) {
            continue;
        }
        float p[3][3];
        for (int c = 0; c < 3; ++c) {
            const gaddr a = verts + 8u * (gaddr)v[c];
            const float l[3] = {(float)S16(a), (float)S16(a + 2), (float)S16(a + 4)};
            for (int k = 0; k < 3; ++k) {
                p[c][k] =
                    pos[k] + (rows[0][k] * l[0] + rows[1][k] * l[1] + rows[2][k] * l[2]) / 1024.0f;
            }
        }
        sink_triangle(s, p, U8(f + (wide ? 6u : 3u)), slot);
    }
}

/* The live mobys whose bounding sphere meets the box, except the hero. */
static void moby_triangles(gaddr hero, Sink* s) {
    const gaddr base = GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableBase));
    const gaddr end = GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableEnd));
    if (!rac2_moby_slot(base)) {
        return;
    }
    for (gaddr m = base; m < end; m += MOBY_SIZE) {
        if (U8(m + MOBY_STATE) >= 0x80u || m == hero || U32(m + MOBY_COLLISION) == 0) {
            continue;
        }
        const float radius = F32(m + MOBY_RADIUS) / 1024.0f;
        if (!isfinite(radius)) {
            continue;
        }
        float d2 = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float c = F32(m + 4u * (gaddr)k) / 1024.0f;
            const float e = c < s->lo[k] ? s->lo[k] - c : (c > s->hi[k] ? c - s->hi[k] : 0.0f);
            d2 += e * e;
        }
        if (!(d2 <= radius * radius)) {
            continue;
        }
        moby_mesh(m, (int32_t)((m - base) / MOBY_SIZE), s);
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
    moby_triangles(openrac_game_play_host->hero_moby(), &s);
    return s.count;
}
