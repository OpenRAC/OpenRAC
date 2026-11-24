/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's VU0 microprograms as native functions, and the routines that started them.
 *
 * At start-up the game sends VU0 a program (the VIF0 chain at 0x0010F1A0, its MPG at 0x0010F1AC: 96
 * instruction pairs at 0xC80, sent by func_003983C8 through KickVif0Chain), and hand-written
 * routines start it with vcallms: entry 0xC80 turns Euler angles into rotation rows, 0xDA0 and 0xDB0
 * are the cosine and the sine, 0xE98 turns a quaternion into rotation rows (0xF10, an arc cosine,
 * has only callers the port does not run). The port has no VU0: what each entry computes is a
 * function here with its arguments and results (trig.h), and every routine that started one is
 * host C that calls it (libraries.tsv, mp/libraries.tsv and mp/maps.py bind every copy: the boot
 * program's, frontbin's, the levels', the multiplayer executable's and its maps'), so no VU0
 * register is left for them to pass anything through.
 *
 * The program's constants are its LOI immediates, written here as numbers: they are data, the
 * console's words. Its sine folds the angle into [-pi/2, pi/2] and sums its series in two halves,
 * every product and sum the console's (common/lib/vu0_sine.h, OPENRAC_VU0_SINE_HALVES_EE); the
 * matrices are the program's sums in the program's order, in the host's floats. */
#include "trig.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "game/common/lib/vu0_sine.h"
#include "game_protos.h"
#include "mp/game_protos.h"
#include "openrac/guest.h"
#include "openrac/ps2float.h"
#include "rac3_version.h"

/* The program's sine constants: pi/2 at 0xDA0 (0x3FC90FDB), pi at 0xDB0 (0x40490FDB), negated for
 * the lower fold, and c3, c5, c7, c9 at 0xDF8, 0xE08, 0xDE8, 0xDF0 (0xBE2AAAA4, 0x3C08873E,
 * 0xB94FB21F, 0x362E9C14). */
static const openrac_vu0_sine_program kSine = {
    .half_pi = 0x1.921fb6p+0f,
    .pi = 0x1.921fb6p+1f,
    .minus_pi = -0x1.921fb6p+1f,
    .series = {-0x1.555548p-3f, 0x1.110e7cp-7f, -0x1.9f643ep-13f, 0x1.5d3828p-19f},
    .order = OPENRAC_VU0_SINE_HALVES_EE,
};

/* The quaternion program's sqrt(2), at 0xE98 (0x3FB504F3). */
static const float kSqrt2 = 0x1.6a09e6p+0f;

float rac3_cos(float angle) {
    return openrac_vu0_cosine(&kSine, angle);
}

float rac3_sin(float angle) {
    return openrac_vu0_sine(&kSine, angle);
}

/* The biased exponent of a float: VU0 reads one of 0 as zero. */
static unsigned exponent_of(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits >> 23 & 0xFFu;
}

/* rows = rows . m, row by row: each row r becomes r.x m0 + r.y m1 + r.z m2, w included (the
 * program's subroutine at 0xE50). */
static void times_rows(float rows[3][4], const float m[3][4]) {
    for (int i = 0; i < 3; ++i) {
        float out[4];
        for (int j = 0; j < 4; ++j) {
            out[j] = m[0][j] * rows[i][0] + m[1][j] * rows[i][1] + m[2][j] * rows[i][2];
        }
        memcpy(rows[i], out, sizeof out);
    }
}

/* From the identity the program turns about x (its rows written directly), then multiplies by the
 * turns about y and z, each step left out when its angle's exponent is 0. */
void rac3_euler_rows(float rows[3][4], const float angles[3]) {
    static const float kIdentity[3][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
    memcpy(rows, kIdentity, sizeof kIdentity);
    if (exponent_of(angles[0]) != 0) {
        const float c = rac3_cos(angles[0]), s = rac3_sin(angles[0]);
        rows[1][1] = c;
        rows[1][2] = s;
        rows[2][1] = 0.0f - s;
        rows[2][2] = c;
    }
    if (exponent_of(angles[1]) != 0) {
        const float c = rac3_cos(angles[1]), s = rac3_sin(angles[1]);
        const float turn[3][4] = {{c, 0, 0.0f - s, 0}, {0, 1, 0, 0}, {s, 0, c, 0}};
        times_rows(rows, turn);
    }
    if (exponent_of(angles[2]) != 0) {
        const float c = rac3_cos(angles[2]), s = rac3_sin(angles[2]);
        const float turn[3][4] = {{c, s, 0, 0}, {0.0f - s, c, 0, 0}, {0, 0, 1, 0}};
        times_rows(rows, turn);
    }
}

/* With q' = sqrt(2) q, the cross terms are q'y q'z, q'z q'x, q'x q'y less and plus q'x..z q'w, and
 * the diagonal is 1 less two of the squares 2 x^2, 2 y^2, 2 z^2, in the program's order. */
void rac3_quaternion_rows(float rows[3][4], const float q[4]) {
    const float x = q[0] * kSqrt2, y = q[1] * kSqrt2, z = q[2] * kSqrt2, w = q[3] * kSqrt2;
    const float xx = 2.0f * (q[0] * q[0]), yy = 2.0f * (q[1] * q[1]), zz = 2.0f * (q[2] * q[2]);
    const float yz = y * z, zx = z * x, xy = x * y;
    rows[0][0] = (1.0f - yy) - zz;
    rows[0][1] = xy - z * w;
    rows[0][2] = zx + y * w;
    rows[1][0] = xy + z * w;
    rows[1][1] = (1.0f - xx) - zz;
    rows[1][2] = yz - x * w;
    rows[2][0] = zx - y * w;
    rows[2][1] = yz + x * w;
    rows[2][2] = (1.0f - xx) - yy;
    rows[0][3] = rows[1][3] = rows[2][3] = 0.0f;
}

/* The quaternion (axis sin(a/2) / |axis|, cos(a/2)) as rows, applied to v, as func_0038DC70 does it
 * with VU0's macro instructions around the program's sine and cosine. */
void rac3_turn_about_axis(float out[4], const float v[4], const float axis[3], float angle) {
    if ((int)exponent_of(angle) - 0x71 <= 0) {
        memcpy(out, v, 4 * sizeof *out);
        return;
    }
    const float length2 = (axis[0] * axis[0] + axis[1] * axis[1]) + axis[2] * axis[2];
    const float half = angle * 0.5f;
    /* VU0's rsqrt: the sine over the axis's length (an axis of length 0 gives a zero vector). */
    const float scale = exponent_of(length2) != 0 ? rac3_sin(half) / sqrtf(length2) : 0.0f;
    const float q[4] = {axis[0] * scale, axis[1] * scale, axis[2] * scale, rac3_cos(half)};
    const float d[3] = {q[0] + q[0], q[1] + q[1], q[2] + q[2]};
    const float dw[3] = {d[0] * q[3], d[1] * q[3], d[2] * q[3]};
    const float dx[3] = {d[0] * q[0], d[1] * q[0], d[2] * q[0]};
    const float dy1 = d[1] * q[1], dy2 = d[2] * q[1], dz2 = d[2] * q[2];
    const float r0[3] = {(1.0f - dy1) - dz2, dw[2] + dx[1], (0.0f - dw[1]) + dx[2]};
    const float r1[3] = {dx[1] - dw[2], (1.0f - dx[0]) - dz2, dw[0] + dy2};
    const float r2[3] = {dx[2] + dw[1], dy2 - dw[0], (1.0f - dx[0]) - dy1};
    float turned[4];
    for (int j = 0; j < 3; ++j) {
        turned[j] = (r0[j] * v[0] + r1[j] * v[1]) + r2[j] * v[2];
    }
    turned[3] = v[3];
    memcpy(out, turned, sizeof turned);
}

/* The rows of the turn about axis a by angle t: a a^T (1 - cos t) + cos t I + sin t [a]x, each
 * element as func_003D1DD8 sums it. */
void rac3_axis_angle_rows(float rows[3][3], const float axis[3], float angle) {
    const float s = rac3_sin(angle), c = rac3_cos(angle);
    const float t = 1.0f - c;
    const float as[3] = {axis[0] * s, axis[1] * s, axis[2] * s};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            rows[i][j] = axis[j] * axis[i] * t;
        }
    }
    rows[0][0] += c;
    rows[0][1] -= as[2];
    rows[0][2] += as[1];
    rows[1][0] += as[2];
    rows[1][1] += c;
    rows[1][2] -= as[0];
    rows[2][0] -= as[1];
    rows[2][1] += as[0];
    rows[2][2] += c;
}

/* ---- Game memory: VU0's quadword loads and stores (lqc2, sqc2) ignore the low four bits. ---- */

static void load_quad(float v[4], gaddr address) {
    memcpy(v, G(address & ~15u), 4 * sizeof *v);
}

static void store_quad(gaddr address, const float v[4]) {
    memcpy(G(address & ~15u), v, 4 * sizeof *v);
}

static void store_rows(gaddr address, const float rows[3][4]) {
    for (int i = 0; i < 3; ++i) {
        store_quad(address + 16u * (unsigned)i, rows[i]);
    }
}

/* ---- The routines that start the program ---- */

/* func_0038D478, func_0038D490: the angle in, its cosine or sine out. */
static float cos_routine(float angle) {
    return rac3_cos(angle);
}

static float sin_routine(float angle) {
    return rac3_sin(angle);
}

/* func_0038D6E8 and func_0038D708: the Euler angles at `angles`, three rows to `out`. */
static void euler_routine(gaddr out, gaddr angles) {
    float a[4];
    float rows[3][4];
    load_quad(a, angles);
    rac3_euler_rows(rows, a);
    store_rows(out, rows);
}

/* func_0038DBF0: the quaternion at `q`, three rows to `out`; func_0038DC10 adds (0, 0, 0, 1). */
static void quaternion_routine(gaddr q, gaddr out, int fourth_row) {
    static const float kW[4] = {0, 0, 0, 1};
    float in[4];
    float rows[3][4];
    load_quad(in, q);
    rac3_quaternion_rows(rows, in);
    store_rows(out, rows);
    if (fourth_row) {
        store_quad(out + 0x30u, kW);
    }
}

/* func_0038DC70: `v` turned about `axis` by `angle`, to `out`. */
static void turn_routine(gaddr out, gaddr v, gaddr axis, float angle) {
    float in[4], about[4], turned[4];
    load_quad(in, v);
    load_quad(about, axis);
    rac3_turn_about_axis(turned, in, about, angle);
    store_quad(out, turned);
}

/* func_003D1DD8: a ring of `count` points (a0 + 4) about the centre at a0 + 0x10 (its w the radius)
 * into the scratchpad, two quadwords each: the point, and the point plus the offset `offset_at`
 * holds. The first point is `start_at`'s vector times the radius, each next one turned by
 * 6.28 / count about the axis `axis_at` holds negated; the first two quadwords go again at the
 * end, and the count of pairs (count + 1) to `count_out`. The points' w is 0 (the console's is
 * whatever VU0's vf5.w held). A count below 1 (the console would go round 2^32 times) is 1. */
static void ring_routine(
    gaddr ring, gaddr offset_at, gaddr axis_at, gaddr start_at, gaddr count_out
) {
    const int32_t count = GREF(int32_t, ring + 4u);
    float centre[4], offset[4], axis[4], point[4];
    float rows[3][3];
    load_quad(centre, ring + 0x10u);
    load_quad(offset, offset_at);
    load_quad(axis, axis_at);
    load_quad(point, start_at);
    for (int j = 0; j < 3; ++j) {
        axis[j] = 0.0f - axis[j];
        point[j] *= centre[3];
    }
    const int32_t steps = count < 1 ? 1 : count;
    /* 6.28 (0x40C8F5C3) over the count, as the EE's div.s. */
    rac3_axis_angle_rows(rows, axis, openrac_ps2_div(0x1.91eb86p+2f, openrac_ps2_itof(steps)));
    gaddr to = 0x70000000u;
    for (int32_t k = 0; k < steps; ++k, to += 0x20u) {
        float turned[3];
        for (int j = 0; j < 3; ++j) {
            turned[j] = (rows[0][j] * point[0] + rows[1][j] * point[1]) + rows[2][j] * point[2];
        }
        memcpy(point, turned, sizeof turned);
        float pair[2][4] = {{0}, {0}};
        for (int j = 0; j < 3; ++j) {
            pair[0][j] = point[j] + centre[j];
            pair[1][j] = pair[0][j] + offset[j];
        }
        memcpy(G(to), pair, sizeof pair);
    }
    memmove(G(to), G(0x70000000u), 0x20u);
    GREF(int32_t, count_out) = steps + 1;
}

/* A copy of the moby routines below: the default pose's offsets (indexed by the moby's +0x40) and
 * the copy's cell relink, func_003C7D10, which it ends in. */
typedef struct MobyCopy {
    gaddr poses;
    void (*relink)(gaddr moby, int cells);
} MobyCopy;

/* func_003C7FF0 (kept 0: the rows from the moby's Euler angles at +0xF0, or those at +0xC0 when
 * +0x34 has 0x100) and func_003C8190 (kept 1: the rows at +0xC0 always): a moby's rows to +0xC0,
 * mirrored in y when +0x34 has 0x8000, its bounding sphere (the pose's offset, scaled by +0x2C and
 * turned, plus the position at +0x10 times 1024; w the radius) to +0x00, +0xAC counted up, and when
 * +0x98 says it lives in the grid, the grid cells it covers (+0xA4) changed by the copy's relink. The
 * pose's offset is the class's frame +0x42 (+0x48 of the class at +0x24, by index), blended towards
 * frame +0x43 by +0x44 while they differ, the default pose +0x40 in place of frame 0xFF. */
static void moby_routine(gaddr moby, int kept, const MobyCopy* copy) {
    if (GREF(int8_t, moby + 0x20u) < 0) {
        return;
    }
    const uint16_t flags = GREF(uint16_t, moby + 0x34u);
    const gaddr frames = GREF(uint32_t, moby + 0x24u) + 0x48u;
    const unsigned from = GREF(uint8_t, moby + 0x42u), to = GREF(uint8_t, moby + 0x43u);
    const float scale = GREF(float, moby + 0x2Cu);
    int mirror = flags & 0x8000u;
    float rows[3][4];
    if (!kept && (flags & 0x100u) != 0) {
        kept = 1;
        mirror = 0;
    }
    if (kept) {
        for (int i = 0; i < 3; ++i) {
            load_quad(rows[i], moby + 0xC0u + 16u * (unsigned)i);
        }
    } else {
        float angles[4];
        load_quad(angles, moby + 0xF0u);
        rac3_euler_rows(rows, angles);
    }
    float pose[4];
    if (from != to) {
        float next[4];
        load_quad(
            pose,
            from == 0xFFu ? copy->poses + 16u * GREF(uint8_t, moby + 0x40u)
                          : GREF(uint32_t, frames + 4u * from)
        );
        load_quad(next, GREF(uint32_t, frames + 4u * to));
        const float t = GREF(float, moby + 0x44u);
        for (int j = 0; j < 4; ++j) {
            pose[j] = (next[j] * t + pose[j]) - pose[j] * t;
        }
    } else if (from != GREF(uint8_t, moby + 0x61u)) {
        GREF(uint8_t, moby + 0x61u) = (uint8_t)from;
        load_quad(pose, GREF(uint32_t, frames + 4u * from));
        store_quad(moby + 0x80u, pose);
    } else {
        load_quad(pose, moby + 0x80u);
    }
    if (mirror) {
        for (int j = 0; j < 3; ++j) {
            rows[1][j] = 0.0f - rows[1][j];
        }
    }
    store_rows(moby + 0xC0u, rows);
    float sphere[4], position[4];
    for (int j = 0; j < 4; ++j) {
        sphere[j] = pose[j] * scale;
    }
    load_quad(position, moby + 0x10u);
    for (int j = 0; j < 3; ++j) {
        position[j] *= 1024.0f;
    }
    const float offset[3] = {sphere[0], sphere[1], sphere[2]};
    for (int j = 0; j < 3; ++j) {
        sphere[j] = ((rows[0][j] * offset[0] + rows[1][j] * offset[1]) + rows[2][j] * offset[2])
                    + position[j];
    }
    store_quad(moby, sphere);
    GREF(uint16_t, moby + 0xACu) = (uint16_t)(GREF(uint16_t, moby + 0xACu) + 1u);
    if (GREF(uint32_t, moby + 0x98u) == 0) {
        return;
    }
    /* The cells the sphere covers, 16384 units each: the bytes of the low x, low y, high x, high y. */
    const int32_t radius = openrac_ps2_ftoi(sphere[3]);
    const int32_t x = openrac_ps2_ftoi(sphere[0]), y = openrac_ps2_ftoi(sphere[1]);
    const int32_t ends[4] =
        {(int32_t)((uint32_t)x - (uint32_t)radius) >> 14,
         (int32_t)((uint32_t)y - (uint32_t)radius) >> 14,
         (int32_t)((uint32_t)x + (uint32_t)radius) >> 14,
         (int32_t)((uint32_t)y + (uint32_t)radius) >> 14};
    uint32_t cells = 0;
    for (int k = 0; k < 4; ++k) {
        cells |= ((uint32_t)ends[k] & 0xFFu) << (8 * k);
    }
    /* The console compares the packed word with the old one sign-extended: one with its top bit set
     * never equals it. */
    const uint32_t old = GREF(uint32_t, moby + 0xA4u);
    if ((cells == old && (cells & 0x80000000u) == 0) || (cells & 0xC0C0u) != 0) {
        return;
    }
    copy->relink(moby, (int)cells);
}

/* ---- Every copy of the routines: the boot program's, frontbin's and the levels' ---- */

float func_0038D478(float angle) {
    return cos_routine(angle);
}

float func_0038D490(float angle) {
    return sin_routine(angle);
}

float func_0038D098__frontbin(float angle) {
    return cos_routine(angle);
}

float func_0038D0B0__frontbin(float angle) {
    return sin_routine(angle);
}

float func_L01_0040E8F8(float angle) {
    return cos_routine(angle);
}

float func_L01_0040E910(float angle) {
    return sin_routine(angle);
}

void func_0038D6E8(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void func_0038D708(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void func_0038D308__frontbin(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void func_0038D328__frontbin(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void func_L01_0040EB68(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void func_L01_0040EB88(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void func_0038DBF0(gaddr q, gaddr out) {
    quaternion_routine(q, out, 0);
}

void func_0038DC10(gaddr q, gaddr out) {
    quaternion_routine(q, out, 1);
}

void func_0038D810__frontbin(gaddr q, gaddr out) {
    quaternion_routine(q, out, 0);
}

void func_0038D830__frontbin(gaddr q, gaddr out) {
    quaternion_routine(q, out, 1);
}

void func_L01_0040F0B0(gaddr q, gaddr out) {
    quaternion_routine(q, out, 0);
}

void func_L01_0040F0D0(gaddr q, gaddr out) {
    quaternion_routine(q, out, 1);
}

void func_0038DC70(gaddr out, gaddr v, gaddr axis, float angle) {
    turn_routine(out, v, axis, angle);
}

void func_0038D890__frontbin(gaddr out, gaddr v, gaddr axis, float angle) {
    turn_routine(out, v, axis, angle);
}

void func_L01_0040F1D0(gaddr out, gaddr v, gaddr axis, float angle) {
    turn_routine(out, v, axis, angle);
}

/* The ring's globals: the boot program's and frontbin's (D_001DA6D0, D_001DA6B0, D_001DA6C0, and
 * $gp - 0x21C0), Veldin's for every level's copy. */
void func_003D1DD8(gaddr ring) {
    ring_routine(
        ring,
        RAC3_ADDR(0x001DA6D0u),
        RAC3_ADDR(0x001DA6B0u),
        RAC3_ADDR(0x001DA6C0u),
        RAC3_ADDR(0x001DA6F0u)
    );
}

void func_003D1A20__frontbin(gaddr ring) {
    ring_routine(
        ring,
        RAC3_ADDR(0x001DA6D0u),
        RAC3_ADDR(0x001DA6B0u),
        RAC3_ADDR(0x001DA6C0u),
        RAC3_ADDR(0x001DA6F0u)
    );
}

#define VELDIN(a) OPENRAC_LDATA(1, (gaddr)(a))

void func_L01_004A4700(gaddr ring) {
    ring_routine(
        ring,
        VELDIN(RAC3_LADDR(1, 0x001E0D90u)),
        VELDIN(RAC3_LADDR(1, 0x001E0D70u)),
        VELDIN(RAC3_LADDR(1, 0x001E0D80u)),
        VELDIN(RAC3_LADDR(1, 0x001E0DB0u))
    );
}

static void relink_boot(gaddr moby, int cells) {
    func_003C7D10((int)moby, cells);
}

static void relink_frontbin(gaddr moby, int cells) {
    func_003C7958__frontbin((int)moby, cells);
}

static void relink_levels(gaddr moby, int cells) {
    func_L01_0043C430((int)moby, cells);
}

void func_003C7FF0(gaddr moby) {
    const MobyCopy copy = {RAC3_ADDR(0x002DA100u), relink_boot};
    moby_routine(moby, 0, &copy);
}

void func_003C8190(gaddr moby) {
    const MobyCopy copy = {RAC3_ADDR(0x002DA100u), relink_boot};
    moby_routine(moby, 1, &copy);
}

void func_003C7C38__frontbin(gaddr moby) {
    const MobyCopy copy = {RAC3_ADDR(0x002DA100u), relink_frontbin};
    moby_routine(moby, 0, &copy);
}

void func_003C7DD8__frontbin(gaddr moby) {
    const MobyCopy copy = {RAC3_ADDR(0x002DA100u), relink_frontbin};
    moby_routine(moby, 1, &copy);
}

void func_L01_0043C710(gaddr moby) {
    const MobyCopy copy = {VELDIN(RAC3_LADDR(1, 0x001FCB40u)), relink_levels};
    moby_routine(moby, 0, &copy);
}

void func_L01_0043C8B0(gaddr moby) {
    const MobyCopy copy = {VELDIN(RAC3_LADDR(1, 0x001FCB40u)), relink_levels};
    moby_routine(moby, 1, &copy);
}

/* ---- The multiplayer executable's copies (its C names start with mp_): its own engine's Euler
 * rows, level 39's (the menus) and each map's, whose globals are level 39's relocated to the map
 * loaded (OPENRAC_LDATA(39, ...)). ---- */

void mp_func_00280670(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

#define MENUS(a) OPENRAC_LDATA(39, (gaddr)(a))

static void ring_multiplayer(gaddr ring) {
    ring_routine(
        ring,
        MENUS(RAC3_LADDR(39, 0x0024C8C0u)),
        MENUS(RAC3_LADDR(39, 0x0024C8A0u)),
        MENUS(RAC3_LADDR(39, 0x0024C8B0u)),
        MENUS(RAC3_LADDR(39, 0x0024C8E0u))
    );
}

/* One map's copies, by the names of its functions (mp_func_L<map>_<place>). The
 * arguments are the places of the cosine, the sine, the two Euler rows, the quaternion rows and
 * matrix, the turn about an axis, the moby matrix and its kept form, the ring, and the grid's
 * relink the moby matrix ends in. */
#define MP_VU0_COPIES(p, c_at, s_at, e_at, e2_at, q_at, q4_at, t_at, m_at, k_at, r_at, g_at)       \
    float p##c_at(float angle) {                                                                   \
        return cos_routine(angle);                                                                 \
    }                                                                                              \
    float p##s_at(float angle) {                                                                   \
        return sin_routine(angle);                                                                 \
    }                                                                                              \
    void p##e_at(gaddr out, gaddr angles) {                                                        \
        euler_routine(out, angles);                                                                \
    }                                                                                              \
    void p##e2_at(gaddr out, gaddr angles) {                                                       \
        euler_routine(out, angles);                                                                \
    }                                                                                              \
    void p##q_at(gaddr q, gaddr out) {                                                             \
        quaternion_routine(q, out, 0);                                                             \
    }                                                                                              \
    void p##q4_at(gaddr q, gaddr out) {                                                            \
        quaternion_routine(q, out, 1);                                                             \
    }                                                                                              \
    void p##t_at(gaddr out, gaddr v, gaddr axis, float angle) {                                    \
        turn_routine(out, v, axis, angle);                                                         \
    }                                                                                              \
    static void relink_##p##g_at(gaddr mob, int cells) {                                           \
        p##g_at((int)mob, cells);                                                                  \
    }                                                                                              \
    void p##m_at(gaddr mob) {                                                                      \
        const MobyCopy copy = {MENUS(RAC3_LADDR(39, 0x00271500u)), relink_##p##g_at};              \
        moby_routine(mob, 0, &copy);                                                               \
    }                                                                                              \
    void p##k_at(gaddr mob) {                                                                      \
        const MobyCopy copy = {MENUS(RAC3_LADDR(39, 0x00271500u)), relink_##p##g_at};              \
        moby_routine(mob, 1, &copy);                                                               \
    }                                                                                              \
    void p##r_at(gaddr points) {                                                                   \
        ring_multiplayer(points);                                                                  \
    }

/* Level 39's, by name (the release's names replace them: rac3-ntsc/host/rac3_names.tsv). */
float mp_func_L39_00590A68(float angle) {
    return cos_routine(angle);
}

float mp_func_L39_00590A80(float angle) {
    return sin_routine(angle);
}

void mp_func_L39_00590CD8(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void mp_func_L39_00590CF8(gaddr out, gaddr angles) {
    euler_routine(out, angles);
}

void mp_func_L39_00591220(gaddr q, gaddr out) {
    quaternion_routine(q, out, 0);
}

void mp_func_L39_00591240(gaddr q, gaddr out) {
    quaternion_routine(q, out, 1);
}

void mp_func_L39_005912A0(gaddr out, gaddr v, gaddr axis, float angle) {
    turn_routine(out, v, axis, angle);
}

static void relink_menus(gaddr moby, int cells) {
    mp_func_L39_005ACFD0((int)moby, cells);
}

void mp_func_L39_005AD2B0(gaddr moby) {
    const MobyCopy copy = {MENUS(RAC3_LADDR(39, 0x00271500u)), relink_menus};
    moby_routine(moby, 0, &copy);
}

void mp_func_L39_005AD450(gaddr moby) {
    const MobyCopy copy = {MENUS(RAC3_LADDR(39, 0x00271500u)), relink_menus};
    moby_routine(moby, 1, &copy);
}

void mp_func_L39_005ECB30(gaddr points) {
    ring_multiplayer(points);
}

/* Each map's (mp_maps.h, written by mp/maps.py with each map's places). */
#define MP_MAP(...)
#define MP_MAP_UI(...)
#define MP_MAP_HUD(...)
#define MP_MAP_VU0(m, ...) MP_VU0_COPIES(mp_func_L##m##_, __VA_ARGS__)
#include "mp_maps.h"
#undef MP_MAP
#undef MP_MAP_UI
#undef MP_MAP_HUD
#undef MP_MAP_VU0
