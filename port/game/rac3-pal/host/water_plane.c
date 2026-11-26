/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal's open water: the rivers and pools of Florana, Marcadia, Daxx and the other
 * levels whose water is a plane at one height (class 2871 on Florana, rac3-decomp
 * src/levels/shared/L02_0030FA80.c). Its update (func_L02_0030FA80) adds the draw callback
 * func_L02_00310838 to DrawFrame's third list. On the console that callback works out a grid on the
 * plane, sends it to a VU1 program of its own (microprogram 10, the boot program's 0x00116070,
 * 470 instructions) with 0x2B quadwords of parameters and a GS packet (0x0026EC60, built by
 * func_L02_0030FF48: ALPHA_1/2, TEST_1/2, CLAMP_1/2, TEX1, FOGCOL), and the program draws it. The
 * port runs no VU1 program: this file is the callback, the grid and the program's per-vertex
 * arithmetic written out in C, drawn through the liquids' strip routine.
 *
 * What the callback and the program do (read from the program and checked against it vertex for
 * vertex with synthetic parameters):
 *
 * - The grid follows the view; what is on it does not. The callback casts the ray of a bottom
 *   corner of the view (by the camera's roll, in eighths) down onto the plane, runs from that point
 *   along the view's horizontal right axis to the other side plane of the view, and takes the
 *   middle of that line as the grid's first row centre. Rows go away from the camera along the
 *   view's horizontal forward axis: 50 rows 1 unit apart (0x001DC568), 50 rows 2 apart and 5 rows
 *   200 apart, 1150 units in all. Each row has 33 points to either side of its centre, the step
 *   the bottom line's length over 64 (at least 0.5), widened row by row as the view widens (so a
 *   row always spans the view). The waves, the textures and the fog are worked out at each point's
 *   place in the world, so they stay put while the grid moves with the camera.
 * - The height: the plane (pvar +0x4C; half a unit under the camera at most) plus the eight waves
 *   of the table at 0x0026EB80 (amplitude, direction, wavelength, phase, wave vector, speed,
 *   filled from the pvar by func_L02_0030FC68). Waves 4 to 7 fade out over the first 50 rows,
 *   waves 0 to 3 over the next 50, and the last 5 rows are flat.
 * - The first layer (context 1): effect texture +0x00 (none when -1), mapped in the world, one
 *   repeat every +0x24 units, scrolled +0x2C units a second towards +0x28 degrees. Its colour is
 *   bytes +0x30..+0x32 shaded by the waves' slope along the view's forward axis
 *   (1 - 0x001E00D4 * slope; func_L02_003102A8 makes the factor from +0x48 and the steepest slope),
 *   blended by FIX +0x33 (ALPHA 0x64, or 0x68, adding, when +0x39 is set), depth tested and
 *   written.
 * - The second layer (context 2): effect texture +0x04, coloured by bytes +0x34..+0x36 and blended
 *   by FIX +0x37, depth tested but never written (TEST_2's alpha test fails to the frame buffer
 *   only). It is the reflection. The view's ray to the point is reflected by the waves' normal
 *   and that direction is looked up in a paraboloid map: S, T, Q = R.x, R.y, 2(|R| + R.z), the
 *   zenith at ST (0, 0), repeated. Flat water reflects the zenith everywhere; only the waves'
 *   slope moves the lookup, so it does not swim with the camera.
 * - Both are fogged by their row's depth towards FOGCOL (+0x3A..+0x3C), from +0x3D percent at +0x40
 *   to +0x3E percent at +0x44 (func_L02_00310368's line, 0x001E00E0..0x001E00EC), not the level's
 *   fog.
 * - Each row goes to the GS as the reflection's strip and then the surface's over it.
 * - Nothing is drawn while the camera is under the water or the corner ray does not reach the
 *   plane.
 *
 * The program's mipmapped surface texture (TEX1 with MIPTBP1/2) is not passed on: its levels are
 * made by the window from the picture (TEX1 asks for them, the effect renderer builds the chain).
 */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"
#include "textures.h"

/* render.c: the recorder's place is the levels' display list cursor. */
void rac3_anchor_levels(void);

/* The strip routine (func_003D3A24, hand-written: count, positions, colours, ST, 1), translated from
 * its instructions; game_protos.h may declare it without its parameters. */
typedef void (*StripRoutine)(int count, int positions, int colours, int uvs, int flag);
#define STRIP_ROUTINE ((StripRoutine)func_L01_00408AAC)
/* The VU1 upload (func_003A9390: VU address, source, quadwords). */
#define VU_UPLOAD func_L01_004C5A98

/* Florana's globals, the ones the callback reads, in the loaded level: the view block (+0x140 the
 * eye, +0x160.. its axes), the view context (+0xB0, +0xB4 the half-angle tangents), the frame
 * count, the mirrored view (the block's +0x170 negated), the row step, func_L02_003102A8's slope
 * factor, the waves' amplitudes summed (func_L02_0030FC68), func_L02_00310368's fog (F at +0x40,
 * F at +0x44, F per unit of the view's w, its offset) and the wave table. */
#define WATER_VIEW OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E1140u))
#define WATER_CONTEXT OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E5E40u))
#define WATER_FRAME OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001DE4C0u))
#define WATER_MIRROR OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001D5477u))
#define WATER_ROW_STEP OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001DC568u))
#define WATER_SHADE OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E00D4u))
#define WATER_WAVE_REACH OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E00DCu))
#define WATER_FOG_NEAR OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E00E0u))
#define WATER_FOG_FAR OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E00E4u))
#define WATER_FOG_SLOPE OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E00E8u))
#define WATER_FOG_OFFSET OPENRAC_LDATA(2, RAC3_LADDR(2, 0x001E00ECu))
#define WATER_WAVES OPENRAC_LDATA(2, RAC3_LADDR(2, 0x0026EB80u))

enum {
    WAVES = 8,
    WAVE_SIZE = 0x1C,
    SIDE = 32, /* points either side of a row's centre */
    COLUMNS = 2 * SIDE + 1,
    STRIP = 2 * COLUMNS, /* a strip joins two rows */
    ROWS = 50 + 50 + 5,
    WHITE_TEXTURE = 0x16,
    EFFECT_BASE = 0x64, /* the callback's index base for GetEffectTex (func_00389598) */
    FOG_LINE = 0x3AC,   /* the strip program's fog line, which its setup unpacks (func_0038C750) */
};

/* The callback's time: the frame count in seconds (PAL's 0.02 a frame, NTSC-U's func_L02_00310500
 * 0.0166667). */
#ifdef RAC3_PAL
static const float kSecondsPerFrame = 0.02f;
#else
static const float kSecondsPerFrame = 0.0166667f;
#endif
static const float kPi = 3.14159265f;

/* GS registers (VU1_addGSregister's first argument). */
enum {
    GS_TEX0_1 = 0x06,
    GS_CLAMP_1 = 0x08,
    GS_TEX1_1 = 0x14,
    GS_FOGCOL = 0x3D,
    GS_ALPHA_1 = 0x42,
    GS_TEST_1 = 0x47,
};

/* The strip routine draws in the first context, so both layers are given to it there. The
 * surface's TEST_1 is the packet's (0x50000, depth GEQUAL) with an alpha test that passes every
 * pixel (GEQUAL 0), so that the window writes its depth as the console's ZBUF does; the
 * reflection's is the packet's TEST_2 (0x51001: every pixel to the frame buffer only). */
static const uint64_t kTestSurface = 0x5000Bu;
static const uint64_t kTestReflection = 0x51001u;
/* DrawFrame's TEST between callbacks, which the callback leaves too. */
static const uint64_t kTestRestore = 0x5360Bu;
/* Bilinear in a mipmap chain (MMIN 5, linear mipmap linear, MXL 6): the window makes the surface
 * texture's chain natively, so the distant water is not left to shimmer; the reflection's picture
 * stays at one level (a zenith lookup, not a tiled picture). */
static const uint64_t kTex1 = 0x20u | 5u << 6 | 6u << 2;
static const uint64_t kTex1Reflection = 0x60u; /* bilinear, one level */

typedef struct Wave {
    float amplitude, phase, kx, ky, speed;
} Wave;

typedef struct Vec3 {
    float x, y, z;
} Vec3;

static float f32_at(gaddr at) {
    float v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static Vec3 vec_at(gaddr at) {
    Vec3 v = {f32_at(at), f32_at(at + 4), f32_at(at + 8)};
    return v;
}

static Vec3 add(Vec3 a, Vec3 b) {
    Vec3 r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}

static Vec3 sub(Vec3 a, Vec3 b) {
    Vec3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

static Vec3 scale(Vec3 a, float s) {
    Vec3 r = {a.x * s, a.y * s, a.z * s};
    return r;
}

static float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float length(Vec3 a) {
    return sqrtf(dot(a, a));
}

static int finite3(Vec3 a) {
    return isfinite(a.x) && isfinite(a.y) && isfinite(a.z);
}

/* A direction in the view (x forward) in the world: the view block's axes as rows. */
static Vec3 to_world(const Vec3* axes, float x, float y, float z) {
    return add(add(scale(axes[0], x), scale(axes[1], y)), scale(axes[2], z));
}

static int read_waves(Wave* waves) {
    const gaddr table = WATER_WAVES;
    for (int i = 0; i < WAVES; ++i) {
        const gaddr at = table + (gaddr)(i * WAVE_SIZE);
        Wave w =
            {f32_at(at + 0x00),
             f32_at(at + 0x0C),
             f32_at(at + 0x10),
             f32_at(at + 0x14),
             f32_at(at + 0x18)};
        if (!isfinite(w.amplitude) || !isfinite(w.phase) || !isfinite(w.kx) || !isfinite(w.ky)
            || !isfinite(w.speed) || fabsf(w.amplitude) > 16.0f) {
            return 0;
        }
        waves[i] = w;
    }
    return 1;
}

/* Where a row is and how much of the waves it keeps. */
typedef struct Row {
    float distance;            /* from the first row, in unit rows */
    float keep_low, keep_high; /* waves 0-3, 4-7 */
} Row;

static int make_rows(Row* rows) {
    int n = 0;
    float distance = 0.0f;
    /* 50 rows a unit apart, all waves, 4-7 fading; 50 two apart, 0-3 fading; 5 flat, 200 apart */
    for (int r = 0; r < 50; ++r, ++n, distance += 1.0f) {
        rows[n] = (Row){distance, 1.0f, (float)(50 - r) / 50.0f};
    }
    for (int r = 0; r < 50; ++r, ++n, distance += 2.0f) {
        rows[n] = (Row){distance, (float)(50 - r) / 50.0f, 0.0f};
    }
    for (int r = 0; r < 5; ++r, ++n, distance += 200.0f) {
        rows[n] = (Row){distance, 0.0f, 0.0f};
    }
    return n;
}

typedef struct Point {
    float position[3];
    float surface_st[2];
    float reflection_st[2];
    uint32_t surface_rgba;
} Point;

typedef struct Plane {
    Vec3 eye;
    float height; /* the grid's plane */
    float time;
    Vec3 forward;      /* the view's horizontal forward axis (n1, -n0) */
    float shade;       /* 0x001E00D4 */
    uint8_t colour[3]; /* the surface's, +0x30..+0x32 */
    int untextured_surface;
    float surface_repeat; /* +0x24 */
    float scroll[2];      /* the surface's ST at the first row's centre */
    Vec3 centre;
    int wave_count;
    Wave waves[WAVES];
} Plane;

static uint8_t colour_byte(float v) {
    return (uint8_t)(v <= 0.0f ? 0.0f : v >= 255.0f ? 255.0f : v + 0.5f);
}

static void point_at(const Plane* p, const Row* row, float x, float y, Point* out) {
    float h = 0.0f, gx = 0.0f, gy = 0.0f;
    for (int i = 0; i < p->wave_count; ++i) {
        const float keep = i < 4 ? row->keep_low : row->keep_high;
        if (keep <= 0.0f) {
            continue;
        }
        const Wave* w = &p->waves[i];
        const float theta = w->kx * x + w->ky * y + w->speed * p->time + w->phase;
        const float a = w->amplitude * keep;
        h += a * sinf(theta);
        const float c = a * cosf(theta);
        gx += c * w->kx;
        gy += c * w->ky;
    }
    out->position[0] = x;
    out->position[1] = y;
    out->position[2] = p->height + h;

    /* the surface: the world mapping (u along -y, v along x), scrolled */
    out->surface_st[0] = p->scroll[0] - (y - p->centre.y) / p->surface_repeat;
    out->surface_st[1] = p->scroll[1] + (x - p->centre.x) / p->surface_repeat;
    const float shade = 1.0f - p->shade * (gx * p->forward.x + gy * p->forward.y);
    float rgb[3];
    for (int k = 0; k < 3; ++k) {
        rgb[k] = (float)p->colour[k] * shade;
        if (p->untextured_surface) {
            rgb[k] *= 0.5f; /* the white texture doubles a colour (0x80 is 1.0) */
        }
    }
    out->surface_rgba = (uint32_t)colour_byte(rgb[0]) | (uint32_t)colour_byte(rgb[1]) << 8
                        | (uint32_t)colour_byte(rgb[2]) << 16 | 0x80u << 24;

    /* the reflection: the ray from the eye reflected by the waves' normal, in a paraboloid map
     * (the program's S, T, Q). The eye's height is over the plane, not the wave. */
    const Vec3 e = {p->eye.x - x, p->eye.y - y, p->eye.z - p->height};
    const Vec3 n = {-gx, -gy, 1.0f};
    const float en = dot(e, n), slope = gx * gx + gy * gy;
    const Vec3 r = sub(scale(n, en), scale(e, 0.5f * (1.0f + slope)));
    const float q = 2.0f * (length(r) + r.z);
    if (q > 1e-6f && isfinite(q)) {
        out->reflection_st[0] = r.x / q;
        out->reflection_st[1] = r.y / q;
    } else {
        out->reflection_st[0] = out->reflection_st[1] = 0.0f;
    }
}

/* The callback's set-up: the first row's centre, the steps and the widening, as the console
 * works them out from the view (func_L02_00310838 up to its parameter block). 0 when the plane
 * is not in view. */
static int frame_grid(
    gaddr view,
    gaddr context,
    float plane,
    float wave_reach,
    float row_unit,
    Vec3* centre,
    Vec3* column_step,
    Vec3* row_step,
    float* widen,
    Vec3* forward
) {
    const Vec3 eye = vec_at(view + 0x140);
    Vec3 axes[3] = {vec_at(view + 0x160), vec_at(view + 0x170), vec_at(view + 0x180)};
    if (GREF(uint8_t, WATER_MIRROR) != 0) {
        axes[1] = scale(axes[1], -1.0f);
    }
    if (!finite3(eye) || !finite3(axes[0]) || !finite3(axes[1]) || !finite3(axes[2])) {
        return 0;
    }
    float cur = plane;
    if (eye.z <= cur) {
        return 0; /* under the water */
    }
    if (eye.z <= cur + 0.5f) {
        cur = eye.z - 0.5f;
    }
    cur += wave_reach;
    const float tan_x = f32_at(context + 0xB0), tan_y = f32_at(context + 0xB4);

    /* the roll in eighths (func_0038D160: the angle of (+0x188, +0x178), 0 upright) */
    const float roll = atan2f(axes[1].z, axes[2].z);
    const int k = (int)(roll * 1.2732395f + 8.0f) & 7;
    float n1 = axes[0].x, n0 = -axes[0].y;
    const float l = sqrtf(n1 * n1 + n0 * n0);
    if (!(l > 1e-6f)) {
        return 0;
    }
    n1 /= l;
    n0 /= l;

    /* the corner ray, down onto the plane (raised by the waves' reach) */
    static const float corner_y[8] = {-1, -1, -1, -1, 1, 1, 1, 1};
    static const float corner_z[8] = {-1, -1, 1, 1, 1, 1, -1, -1};
    const Vec3 ray = to_world(axes, 1.0f, corner_y[k] * tan_x, corner_z[k] * tan_y);
    if (!(ray.z < 0.0f)) {
        return 0;
    }
    const Vec3 start = add(eye, scale(ray, (cur - eye.z) / ray.z));
    /* the side plane of the view it runs to */
    Vec3 side_normal;
    switch (k) {
        case 0:
        case 3:
            side_normal = to_world(axes, tan_x, -1.0f, 0.0f);
            break;
        case 1:
        case 6:
            side_normal = to_world(axes, tan_y, 0.0f, -1.0f);
            break;
        case 2:
        case 5:
            side_normal = to_world(axes, tan_y, 0.0f, 1.0f);
            break;
        default:
            side_normal = to_world(axes, tan_x, 1.0f, 0.0f);
            break;
    }
    const Vec3 across = {n0, n1, 0.0f};
    const float meet = dot(side_normal, across);
    if (!(fabsf(meet) > 1e-9f)) {
        return 0;
    }
    const Vec3 end = add(start, scale(across, dot(side_normal, sub(eye, start)) / meet));
    float d = length(sub(end, start));
    Vec3 middle = scale(add(start, end), 0.5f);

    /* how the view widens: the line's length at its depth and 650 units further */
    const float near_depth = dot(sub(middle, eye), axes[0]);
    const Vec3 ahead = {n1 * 650.0f, -n0 * 650.0f, 0.0f};
    const float far_depth = dot(add(sub(middle, eye), ahead), axes[0]);
    const float far_length = d * far_depth / near_depth;
    if (d < 32.0f) {
        d = 32.0f;
    }
    *widen = (far_length - d) * row_unit / ((far_depth - near_depth) * d);
    middle.z -= wave_reach;
    if (!finite3(middle) || !isfinite(*widen)) {
        return 0;
    }
    *centre = middle;
    *column_step = scale(across, d * 0.015625f);

    /* the rows' direction: the view's horizontal forward, from the axis the roll leaves level */
    Vec3 f;
    switch (k) {
        case 0:
        case 7:
            f = (Vec3){axes[1].y, -axes[1].x, 0.0f};
            break;
        case 1:
        case 2:
            f = (Vec3){-axes[2].y, axes[2].x, 0.0f};
            break;
        case 3:
        case 4:
            f = (Vec3){-axes[1].y, axes[1].x, 0.0f};
            break;
        default:
            f = (Vec3){axes[2].y, -axes[2].x, 0.0f};
            break;
    }
    const float fl = sqrtf(f.x * f.x + f.y * f.y);
    if (!(fl > 1e-6f)) {
        return 0;
    }
    *row_step = scale(f, row_unit / fl);
    *forward = (Vec3){n1, -n0, 0.0f};
    return 1;
}

static void set_layer(uint64_t tex0, uint64_t alpha, uint64_t test, uint64_t tex1) {
    func_L01_004C5B90(GS_ALPHA_1, alpha);
    func_L01_004C5B90(GS_CLAMP_1, 0);
    func_L01_004C5B90(GS_TEX1_1, tex1);
    func_L01_004C5B90(GS_TEST_1, test);
    func_L01_004C5B90(GS_TEX0_1, tex0);
}

/* The strip program's fog line (VU 0x3AC..0x3AF) as its set-up unpacks it, with the given F
 * line: 0x3AC is a GIF tag with the slope in w, 0x3AD the view's screen scale, 0x3AE its offset
 * with the F offset in w, 0x3AF the two F bounds. */
static void upload_fog_line(
    gaddr block, gaddr context, float slope, float offset, float bound0, float bound1
) {
    static const uint32_t tag[3] = {0x8000u, 0x303EC000u, 0x412u};
    memcpy(G(block), tag, sizeof tag);
    memcpy(G(block + 0x0C), &slope, 4);
    memcpy(G(block + 0x10), G(context + 0x190), 16);
    memcpy(G(block + 0x20), G(context + 0x1A0), 12);
    memcpy(G(block + 0x2C), &offset, 4);
    const float bounds[4] = {bound0, bound1, 0.0f, 0.0f};
    memcpy(G(block + 0x30), bounds, 16);
    VU_UPLOAD(FOG_LINE, (int)block, 4);
}

/* The two layers' native materials (docs/port/TEXTURES.md section 4), from the state this routine
 * itself gives the GS above: the surface blends by its FIX (mix, or added when pv[0x39] is set), tests
 * depth against the world and writes it; the reflection mixes by its FIX over the world, tested
 * and never written (its alpha test fails every pixel to the frame buffer only). Both are
 * bilinear, repeated, the surface through a mip chain the window makes. */
static void layer_material(openrac_material* m, int surface, uint8_t fix, int additive) {
    memset(m, 0, sizeof *m);
    m->source_k = 1;
    m->source_m = 0;
    m->dest_k = additive ? 0 : -1;
    m->dest_m = 1;
    m->weight = OPENRAC_MATERIAL_WEIGHT_CONSTANT;
    m->constant = fix;
    m->depth_test = OPENRAC_MATERIAL_DEPTH_NEARER_OR_EQUAL;
    m->combine = OPENRAC_MATERIAL_MODULATE;
    m->writes = OPENRAC_MATERIAL_WRITE_COLOUR;
    m->flags = OPENRAC_MATERIAL_BLEND | OPENRAC_MATERIAL_TEXTURE_ALPHA | OPENRAC_MATERIAL_LINEAR_MAG
               | OPENRAC_MATERIAL_LINEAR_MIN | OPENRAC_MATERIAL_REPEAT_S
               | OPENRAC_MATERIAL_REPEAT_T;
    if (surface) {
        m->alpha_test = OPENRAC_MATERIAL_TEST_ALWAYS;
        m->flags |= OPENRAC_MATERIAL_DEPTH_WRITE | OPENRAC_MATERIAL_MIPMAP;
    } else {
        m->alpha_test = OPENRAC_MATERIAL_TEST_NEVER;
        m->alpha_fail = OPENRAC_MATERIAL_FAIL_COLOUR;
    }
}

/* The water plane's draw callback (class 2871's on Florana, the same routine in twelve levels):
 * its argument the moby, its pvar at +0x68. */
void func_L02_00310838(int moby) {
    const gaddr vars = GREF(gaddr, (gaddr)moby + 0x68u);
    if (vars == 0 || vars >= openrac_guest_ram_size - 0x70u) {
        return;
    }
    const uint8_t* pv = G(vars);
    const gaddr view = WATER_VIEW, context = WATER_CONTEXT;
    Plane p;
    memset(&p, 0, sizeof p);
    const float plane = f32_at(vars + 0x4C);
    const float row_unit = f32_at(WATER_ROW_STEP), reach = f32_at(WATER_WAVE_REACH);
    if (!isfinite(plane) || !isfinite(row_unit) || !(row_unit > 0.0f) || !isfinite(reach)) {
        return;
    }
    Vec3 column_step, row_step;
    float widen;
    if (!frame_grid(
            view,
            context,
            plane,
            reach,
            row_unit,
            &p.centre,
            &column_step,
            &row_step,
            &widen,
            &p.forward
        )) {
        return;
    }
    p.eye = vec_at(view + 0x140);
    p.height = p.centre.z;
    p.time = (float)GREF(int32_t, WATER_FRAME) * kSecondsPerFrame;
    p.wave_count = read_waves(p.waves) ? WAVES : 0;
    p.shade = f32_at(WATER_SHADE);
    if (!isfinite(p.shade)) {
        p.shade = 0.0f;
    }
    memcpy(p.colour, pv + 0x30, 3);

    /* the surface's mapping: one repeat every +0x24 units, scrolled +0x2C units a second towards
     * +0x28 degrees (the callback's 0x130 and 0x134) */
    p.surface_repeat = f32_at(vars + 0x24);
    if (!(fabsf(p.surface_repeat) > 1e-3f) || !isfinite(p.surface_repeat)) {
        p.surface_repeat = 1.0f;
    }
    const float angle = f32_at(vars + 0x28) * (kPi / 180.0f), speed = f32_at(vars + 0x2C);
    const float drift = isfinite(angle) && isfinite(speed) ? p.time * speed : 0.0f;
    const float su = (-p.centre.y + drift * sinf(angle)) / p.surface_repeat;
    const float sv = (p.centre.x - drift * cosf(angle)) / p.surface_repeat;
    p.scroll[0] = su - floorf(su);
    p.scroll[1] = sv - floorf(sv);

    const int32_t surface_texture = GREF(int32_t, vars + 0x00);
    const int32_t reflection_texture = GREF(int32_t, vars + 0x04);
    p.untextured_surface = surface_texture < 0;

    static Row rows[ROWS];
    const int row_count = make_rows(rows);
    static Point grid[ROWS][COLUMNS];
    for (int r = 0; r < row_count; ++r) {
        const Vec3 base = add(p.centre, scale(row_step, rows[r].distance));
        const Vec3 step = scale(column_step, 1.0f + widen * rows[r].distance);
        for (int j = -SIDE; j <= SIDE; ++j) {
            const Vec3 at = add(base, scale(step, (float)j));
            point_at(&p, &rows[r], at.x, at.y, &grid[r][j + SIDE]);
        }
    }

    GFRAME(frame, STRIP * (12 + 4 + 8) + 0x40 + 16);
    const gaddr positions = frame;
    const gaddr colours = positions + 12u * STRIP;
    const gaddr uvs = colours + 4u * STRIP;
    const gaddr fog_block = (uvs + 8u * STRIP + 15u) & ~15u;

    /* The effect textures' handles (EffectTex0 also pages the images into the list), and, when the
     * window draws natively, the images they name. */
    uint64_t reflection_tex0 = 0, surface_tex0 = 0;
    int native = openrac_frame_native();
    openrac_game_ui_image reflection_image, surface_image;
    if (native) {
        reflection_tex0 = (uint64_t)func_L01_00404040(
            reflection_texture >= 0 ? reflection_texture + EFFECT_BASE : WHITE_TEXTURE
        );
        surface_tex0 = (uint64_t)func_L01_00404040(
            surface_texture >= 0 ? surface_texture + EFFECT_BASE : WHITE_TEXTURE
        );
        native = rac3_effect_image(reflection_tex0, &reflection_image)
                 && rac3_effect_image(surface_tex0, &surface_image);
    }
    if (!native) {
        func_L01_00407588(); /* the strip routine's setup: its program, the view and the level's fog */
        /* the water's own fog: its F line and colour */
        upload_fog_line(
            fog_block,
            context,
            f32_at(WATER_FOG_SLOPE),
            f32_at(WATER_FOG_OFFSET),
            f32_at(WATER_FOG_FAR),
            f32_at(WATER_FOG_NEAR)
        );
        func_L01_004C5B90(
            GS_FOGCOL, (uint64_t)pv[0x3A] | (uint64_t)pv[0x3B] << 8 | (uint64_t)pv[0x3C] << 16
        );
        reflection_tex0 = (uint64_t)func_L01_00404040(
            reflection_texture >= 0 ? reflection_texture + EFFECT_BASE : WHITE_TEXTURE
        );
        surface_tex0 = (uint64_t)func_L01_00404040(
            surface_texture >= 0 ? surface_texture + EFFECT_BASE : WHITE_TEXTURE
        );
    }
    const uint64_t reflection_alpha = (uint64_t)pv[0x37] << 32 | 0x64u;
    const uint64_t surface_alpha = (uint64_t)pv[0x33] << 32 | (pv[0x39] ? 0x68u : 0x64u);
    uint32_t reflection_rgba =
        (uint32_t)pv[0x34] | (uint32_t)pv[0x35] << 8 | (uint32_t)pv[0x36] << 16 | 0x80u << 24;
    if (reflection_texture < 0) {
        reflection_rgba = (uint32_t)(pv[0x34] >> 1) | (uint32_t)(pv[0x35] >> 1) << 8
                          | (uint32_t)(pv[0x36] >> 1) << 16 | 0x80u << 24;
    }

    openrac_frame_strip_desc desc;
    memset(&desc, 0, sizeof desc);
    desc.position = (const float*)G(positions);
    desc.st = (const float*)G(uvs);
    desc.rgba = (const uint32_t*)G(colours);
    desc.flags = OPENRAC_STRIP_LIQUID | OPENRAC_STRIP_FOG;
    desc.fog[0] = f32_at(WATER_FOG_SLOPE);
    desc.fog[1] = f32_at(WATER_FOG_OFFSET);
    desc.fog[2] = f32_at(WATER_FOG_FAR);
    desc.fog[3] = f32_at(WATER_FOG_NEAR);
    desc.fog_colour = (uint32_t)pv[0x3A] | (uint32_t)pv[0x3B] << 8 | (uint32_t)pv[0x3C] << 16;
    if (native) {
        rac3_anchor_levels();
    }

    /* each pair of rows: the reflection, then the surface over it */
    for (int r = 1; r < row_count; ++r) {
        for (int layer = 0; layer < 2; ++layer) {
            if (!native) {
                if (layer == 0) {
                    set_layer(reflection_tex0, reflection_alpha, kTestReflection, kTex1Reflection);
                } else {
                    set_layer(surface_tex0, surface_alpha, kTestSurface, kTex1);
                }
            }
            for (int j = 0; j < COLUMNS; ++j) {
                for (int side = 0; side < 2; ++side) {
                    const Point* v = &grid[r - 1 + side][j];
                    const int n = 2 * j + side;
                    memcpy(G(positions + 12u * (gaddr)n), v->position, 12);
                    memcpy(G(uvs + 8u * (gaddr)n), layer ? v->surface_st : v->reflection_st, 8);
                    GREF(uint32_t, colours + 4u * (gaddr)n) =
                        layer ? v->surface_rgba : reflection_rgba;
                }
            }
            if (native) {
                desc.count = STRIP;
                layer_material(
                    &desc.material, layer, (uint8_t)(layer ? pv[0x33] : pv[0x37]), layer && pv[0x39]
                );
                desc.image = layer ? surface_image : reflection_image;
                /* A strip the window does not take (the call checked what it was given) is left out. */
                (void)openrac_frame_strip(&desc);
                continue;
            }
            STRIP_ROUTINE(STRIP, (int)positions, (int)colours, (int)uvs, 1);
        }
    }
    if (!native) {
        func_L01_004C5B90(GS_TEST_1, kTestRestore);
        /* the level's fog line back for the strips after this */
        upload_fog_line(
            fog_block,
            context,
            f32_at(context + 0x220),
            f32_at(context + 0x1AC),
            f32_at(context + 0x23C),
            f32_at(context + 0x238)
        );
    }
}
