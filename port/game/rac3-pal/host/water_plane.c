/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal's open water: the rivers and pools of Florana, Marcadia, Daxx and the other
 * levels whose water is a plane at one height (class 2871 on Florana, rac3-decomp
 * src/levels/shared/L02_0030FA80.c). Its update (func_L02_0030FA80) adds the draw callback
 * func_L02_00310838 to DrawFrame's third list; on the console that callback loads a VU1 program of
 * its own (microprogram 10, func_003A9338 with 0x00116070), sends it a prebuilt GS packet
 * (0x0026EC60: ALPHA_1/2, TEST_1/2, CLAMP, TEX1, FOGCOL, built by func_L02_0030FF48) and 0x2B
 * quadwords of parameters, and the program tessellates the whole plane in front of the camera and
 * draws it twice. The port runs no VU1 program, so the water was not drawn at all (swimming and
 * its collision, which are the game's C, were there).
 *
 * The callback is drawn here natively from the same inputs, through the routines the game's other
 * liquids use (LiquidSurfaceDraw, func_L02_003A2EC8): VU1_addGSregister for the GS state and the
 * world-space strip routine (func_003D3A24, Veldin's 0x00408AAC) after its setup (func_0038C750,
 * Veldin's 0x00407588), whose strips the window draws with the liquids (renderer/effects). The
 * plane is a disc of rings around the camera, finer near it, and the two layers are the console's,
 * the second drawn first (below):
 *
 * - the first: effect texture pvar +0x00 (+0x64; none, -1, is untextured: the white effect
 *   texture 0x16 stands for it), coloured by bytes +0x30..+0x32, blended by FIX +0x33 (ALPHA 0x64,
 *   or 0x68 added when +0x39 is set), mapped in the world every +0x24 units and scrolled +0x2C
 *   units a second along +0x28 degrees (the callback's 0x130..0x154 parameters); it writes depth;
 * - the second: effect texture pvar +0x04 (+0x64; white when -1), coloured by +0x34..+0x36 and
 *   blended by FIX +0x37, colour only (TEST_2 0x51001: the alpha test fails every pixel, frame
 *   buffer only). The parameters it is drawn with are the camera's axes and the waves' slope
 *   normalised by their largest (func_L02_0030FC68, 0x001E00D8): a reflection. Florana's is effect
 *   texture 100, a sphere map of a cloudy sky; it is read here by the view's reflection off the
 *   waves.
 *
 * The eight waves are the game's own table (0x0026EB80, 0x1C bytes each, which func_L02_0030FC68
 * fills from the pvar at the class's first update: amplitude, wavelength, phase, the wave vector
 * and its speed), at the callback's time (the frame count 0x001DE4C0 in seconds). A wave shorter
 * than twice the rings' spacing where a vertex is fades out, so far rings do not alias.
 *
 * Not the console's to the vertex: the program's own tessellation (a 0x32 x 0x32 grid in the view)
 * and its exact reflection mapping are in the VU1 program, which is not read here. The plane's
 * extent (kOuter) is the fog's reach and more; the camera under the water draws nothing, as the
 * callback returns then.
 */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

/* The strip routine (func_003D3A24, hand-written: count, positions, colours, ST, 1), translated from
 * its instructions; game_protos.h may declare it without its parameters. */
typedef int (*StripRoutine)(int count, int positions, int colours, int uvs, int flag);
#define STRIP_ROUTINE ((StripRoutine)func_L01_00408AAC)

/* Florana's globals, the ones the callback reads, in the loaded level. */
#define WATER_CAMERA OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E1140u)) /* +0x140 the eye */
#define WATER_FRAME OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001DE4C0u))
#define WATER_WAVES OPENRAC_LDATA(2, RAC3_LADDR(2, 0x0026EB80u))

enum {
    WAVES = 8,
    WAVE_SIZE = 0x1C,
    SEGMENTS = 30, /* a strip of two rings: 2 * 31 = 62 vertices, under the routine's 64 */
    RINGS = 20,
    STRIP = 2 * (SEGMENTS + 1),
    WHITE_TEXTURE = 0x16,
    EFFECT_BASE = 0x64, /* the callback's index base for GetEffectTex (func_00389598) */
};

static const float kInner = 0.75f;  /* the first ring's radius */
static const float kOuter = 320.0f; /* the last ring's */
static const float kSnap = 0.5f;    /* the rings' centre moves in steps of this */
static const float kReflectionReach = 0.45f; /* the map's disc has a radius of 0.5 */
static const float kPi = 3.14159265f;
/* The callback's time: the frame count in seconds (PAL's 0.02 a frame, NTSC-U's func_L02_00310500
 * 0.0166667). */
#ifdef RAC3_PAL
static const float kSecondsPerFrame = 0.02f;
#else
static const float kSecondsPerFrame = 0.0166667f;
#endif

/* GS registers (VU1_addGSregister's first argument) and their values, as the console's packet. */
enum {
    GS_TEX0_1 = 0x06,
    GS_CLAMP_1 = 0x08,
    GS_TEX1_1 = 0x14,
    GS_ALPHA_1 = 0x42,
    GS_TEST_1 = 0x47,
};

/* The first layer's TEST: depth GEQUAL as the packet's TEST_1 (0x50000), with an alpha test that
 * passes every pixel (GEQUAL 0), so that the window writes its depth as the console's ZBUF does. */
static const uint64_t kTestSurface = 0x5000Bu;
/* The second's: TEST_2 0x51001, every pixel written to the frame buffer only. */
static const uint64_t kTestLayer = 0x51001u;
/* DrawFrame's TEST between callbacks, which LiquidSurfaceDraw leaves too. */
static const uint64_t kTestRestore = 0x5360Bu;
static const uint64_t kTex1 = 0x60u; /* the packet's TEX1: bilinear, no mipmaps */

typedef struct Wave {
    float amplitude, wavelength, phase, kx, ky, speed;
} Wave;

static float f32_at(gaddr at) {
    float v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static int read_waves(Wave* waves) {
    const gaddr table = WATER_WAVES;
    for (int i = 0; i < WAVES; ++i) {
        const gaddr at = table + (gaddr)(i * WAVE_SIZE);
        Wave w = {
            f32_at(at + 0x00),
            f32_at(at + 0x08),
            f32_at(at + 0x0C),
            f32_at(at + 0x10),
            f32_at(at + 0x14),
            f32_at(at + 0x18),
        };
        if (!isfinite(w.amplitude) || !isfinite(w.wavelength) || !isfinite(w.phase)
            || !isfinite(w.kx) || !isfinite(w.ky) || !isfinite(w.speed) || !(w.wavelength > 0.0f)
            || fabsf(w.amplitude) > 4.0f) {
            return 0;
        }
        waves[i] = w;
    }
    return 1;
}

static float fraction(float x) {
    return x - floorf(x);
}

static void normalise(float* v) {
    const float l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (l > 1e-6f) {
        v[0] /= l;
        v[1] /= l;
        v[2] /= l;
    }
}

static float dot3(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

typedef struct Plane {
    float height, time;
    float centre[2];
    float eye[3];
    float right[3], up[3]; /* the view's axes in the world */
    int wave_count;
    Wave waves[WAVES];
    /* the first layer's world mapping */
    float scale, scroll[2];
} Plane;

typedef struct Vertex {
    float position[3];
    float surface_uv[2];
    float reflection_uv[2];
} Vertex;

static float ring_radius(int k) {
    if (k <= 0) {
        return 0.0f;
    }
    const float q = powf(kOuter / kInner, 1.0f / (float)(RINGS - 1));
    return kInner * powf(q, (float)(k - 1));
}

static void vertex_at(const Plane* p, float x, float y, float spacing, Vertex* out) {
    float z = p->height, gx = 0.0f, gy = 0.0f;
    for (int i = 0; i < p->wave_count; ++i) {
        const Wave* w = &p->waves[i];
        /* the wave fades out where the rings are too far apart to show it */
        float keep = (w->wavelength / spacing - 2.0f) * 0.5f;
        keep = keep < 0.0f ? 0.0f : keep > 1.0f ? 1.0f : keep;
        if (keep <= 0.0f) {
            continue;
        }
        const float theta = w->kx * x + w->ky * y + w->speed * p->time + w->phase;
        const float a = w->amplitude * keep;
        z += a * sinf(theta);
        const float c = a * cosf(theta);
        gx += c * w->kx;
        gy += c * w->ky;
    }
    out->position[0] = x;
    out->position[1] = y;
    out->position[2] = z;

    /* first layer: the world mapping, kept small around the centre for the floats */
    out->surface_uv[0] = p->scroll[0] - (y - p->centre[1]) / p->scale;
    out->surface_uv[1] = p->scroll[1] + (x - p->centre[0]) / p->scale;

    /* second layer: the view's reflection off the waves, its direction across the view (right,
     * up) read inside the sphere map's disc: a reflection along the view's axis (the water seen
     * edge on, or from straight above) is its centre, one across the view its rim; never the
     * rim's singular ring, where the map is dark */
    float n[3] = {-gx, -gy, 1.0f};
    normalise(n);
    float v[3] = {x - p->eye[0], y - p->eye[1], z - p->eye[2]};
    normalise(v);
    const float d = dot3(v, n);
    const float r[3] = {v[0] - 2.0f * d * n[0], v[1] - 2.0f * d * n[1], v[2] - 2.0f * d * n[2]};
    out->reflection_uv[0] = 0.5f + kReflectionReach * dot3(r, p->right);
    out->reflection_uv[1] = 0.5f - kReflectionReach * dot3(r, p->up);
}

/* One layer: the GS state, then each pair of rings as a strip. */
static void draw_layer(
    const Vertex* grid,
    uint64_t tex0,
    uint64_t alpha,
    uint64_t test,
    uint32_t rgba,
    int reflection,
    gaddr positions,
    gaddr colours,
    gaddr uvs
) {
    func_L01_004C5B90(GS_ALPHA_1, alpha);
    func_L01_004C5B90(GS_CLAMP_1, 0);
    func_L01_004C5B90(GS_TEX1_1, kTex1);
    func_L01_004C5B90(GS_TEST_1, test);
    func_L01_004C5B90(GS_TEX0_1, tex0);
    for (int j = 0; j < STRIP; ++j) {
        GREF(uint32_t, colours + 4u * (gaddr)j) = rgba;
    }
    for (int k = 0; k < RINGS; ++k) {
        for (int j = 0; j <= SEGMENTS; ++j) {
            for (int side = 0; side < 2; ++side) {
                const Vertex* v = &grid[(k + side) * (SEGMENTS + 1) + j];
                const int n = 2 * j + side;
                memcpy(G(positions + 12u * (gaddr)n), v->position, 12);
                memcpy(G(uvs + 8u * (gaddr)n), reflection ? v->reflection_uv : v->surface_uv, 8);
            }
        }
        STRIP_ROUTINE(STRIP, (int)positions, (int)colours, (int)uvs, 1);
    }
}

static uint32_t colour_at(const uint8_t* rgb) {
    return (uint32_t)rgb[0] | (uint32_t)rgb[1] << 8 | (uint32_t)rgb[2] << 16 | 0x80u << 24;
}

/* The water plane's draw callback (class 2871's on Florana, the same routine in twelve levels):
 * its argument the moby, its pvar at +0x68. */
void func_L02_00310838(int moby) {
    const gaddr vars = GREF(gaddr, (gaddr)moby + 0x68u);
    if (vars == 0 || vars >= openrac_guest_ram_size - 0x70u) {
        return;
    }
    const uint8_t* pv = G(vars);
    Plane p;
    memset(&p, 0, sizeof p);
    p.height = f32_at(vars + 0x4C);
    const gaddr camera = WATER_CAMERA;
    for (int i = 0; i < 3; ++i) {
        p.eye[i] = f32_at(camera + 0x140u + 4u * (gaddr)i);
    }
    if (!isfinite(p.height) || !isfinite(p.eye[0]) || !isfinite(p.eye[1]) || !isfinite(p.eye[2])
        || p.eye[2] <= p.height) {
        return; /* the camera under the water: the callback draws nothing */
    }
    /* the view's forward axis, as MobyProc's test reads it from the view block */
    float forward[3] = {f32_at(camera + 0x08), f32_at(camera + 0x18), f32_at(camera + 0x28)};
    normalise(forward);
    const float world_up[3] = {0.0f, 0.0f, 1.0f};
    p.right[0] = forward[1] * world_up[2] - forward[2] * world_up[1];
    p.right[1] = forward[2] * world_up[0] - forward[0] * world_up[2];
    p.right[2] = forward[0] * world_up[1] - forward[1] * world_up[0];
    if (dot3(p.right, p.right) < 1e-8f) {
        p.right[0] = 1.0f;
        p.right[1] = p.right[2] = 0.0f;
    }
    normalise(p.right);
    p.up[0] = p.right[1] * forward[2] - p.right[2] * forward[1];
    p.up[1] = p.right[2] * forward[0] - p.right[0] * forward[2];
    p.up[2] = p.right[0] * forward[1] - p.right[1] * forward[0];

    p.time = (float)GREF(int32_t, WATER_FRAME) * kSecondsPerFrame;
    p.wave_count = read_waves(p.waves) ? WAVES : 0;
    p.centre[0] = floorf(p.eye[0] / kSnap) * kSnap;
    p.centre[1] = floorf(p.eye[1] / kSnap) * kSnap;

    /* the first layer's mapping: every +0x24 units, scrolled +0x2C units a second along +0x28
     * degrees (the callback's 0x130 and 0x134) */
    p.scale = f32_at(vars + 0x24);
    if (!(fabsf(p.scale) > 1e-3f) || !isfinite(p.scale)) {
        p.scale = 1.0f;
    }
    const float angle = f32_at(vars + 0x28) * (kPi / 180.0f), speed = f32_at(vars + 0x2C);
    const float drift = isfinite(angle) && isfinite(speed) ? p.time * speed : 0.0f;
    p.scroll[0] = fraction((-p.centre[1] + drift * cosf(angle)) / p.scale);
    p.scroll[1] = fraction((p.centre[0] - drift * sinf(angle)) / p.scale);

    static Vertex grid[(RINGS + 1) * (SEGMENTS + 1)];
    for (int k = 0; k <= RINGS; ++k) {
        const float r = ring_radius(k);
        const float radial = k == 0 ? kInner : r - ring_radius(k - 1);
        const float around = r * 2.0f * kPi / (float)SEGMENTS;
        const float spacing = radial > around ? radial : around;
        for (int j = 0; j <= SEGMENTS; ++j) {
            const float a = (float)(j % SEGMENTS) * (2.0f * kPi / (float)SEGMENTS);
            vertex_at(
                &p,
                p.centre[0] + r * cosf(a),
                p.centre[1] + r * sinf(a),
                spacing > 1e-3f ? spacing : 1e-3f,
                &grid[k * (SEGMENTS + 1) + j]
            );
        }
    }

    GFRAME(frame, STRIP * (12 + 4 + 8) + 48);
    const gaddr positions = frame;
    const gaddr colours = positions + 12u * STRIP;
    const gaddr uvs = colours + 4u * STRIP;

    const int32_t first_texture = GREF(int32_t, vars + 0x00);
    const int32_t second_texture = GREF(int32_t, vars + 0x04);
    func_L01_00407588(); /* the strip routine's setup: its program, the view and the fog */
    /* The reflection first, colour only, then the surface blended over it, writing depth. The
     * other order cannot be the console's: the reflection is opaque (FIX 0x80) in eleven of the
     * twelve levels, and would hide the textured surface most of them blend over it (FIX 0x28 to
     * 0x40); its frame-buffer-only test leaves the depth for the surface to write. */
    {
        const uint64_t tex0 = (uint64_t)func_L01_00404040(
            second_texture >= 0 ? second_texture + EFFECT_BASE : WHITE_TEXTURE
        );
        const uint64_t alpha = (uint64_t)pv[0x37] << 32 | 0x64u;
        const uint32_t rgba = colour_at(pv + 0x34);
        draw_layer(grid, tex0, alpha, kTestLayer, rgba, 1, positions, colours, uvs);
    }
    {
        const uint64_t tex0 = (uint64_t)func_L01_00404040(
            first_texture >= 0 ? first_texture + EFFECT_BASE : WHITE_TEXTURE
        );
        const uint64_t alpha = (uint64_t)pv[0x33] << 32 | (pv[0x39] ? 0x68u : 0x64u);
        const uint32_t rgba = colour_at(pv + 0x30);
        draw_layer(grid, tex0, alpha, kTestSurface, rgba, 0, positions, colours, uvs);
    }
    func_L01_004C5B90(GS_TEST_1, kTestRestore);
}
