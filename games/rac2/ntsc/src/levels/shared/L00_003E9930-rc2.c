
/* SPDX-License-Identifier: MIT
 * MIT License
 *
 * Copyright (c) 2026 Forja contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "common.h"

extern f32 ref_L00_003E9930_00(s32);
extern f32 ref_L00_003E9930_01(f32, f32);
extern f32 ref_L00_003E9930_02(f32);
extern f32 ref_L00_003E9930_03(f32);
extern void ref_L00_003E9930_04(void *, void *, void *);
extern void ref_L00_003E9930_05(void *, void *, void *);
extern void ref_L00_003E9930_06(void *, void *, void *);
extern void ref_L00_003E9930_07(void *, void *, void *);
extern void ref_L00_003E9930_08(void *, void *, f32);

struct Ent {
    s32 x0;
    f32 x4;
    f32 x8;
    s32 xc;
};

struct Fr {
    struct Ent e[2];
    s8 pad20[0xB0];
    s8 b0[0x10];
    s8 b1[0x10];
};

void func_L00_003E9930(s8 *arg0, s8 *arg1, s8 *arg2, f32 fparg) {
    struct Fr fr;
    s32 cnt;
    f32 f20v;
    f32 f22v;
    f32 t;
    f32 u;
    struct Ent *s0;

    f20v = 0.0f;
    f22v = 6.283185482025146484375f / ref_L00_003E9930_00(11);
    cnt = 1;
    s0 = &fr.e[0];
    do {
        f20v = ref_L00_003E9930_01(f20v, f22v);
        s0->xc = 0;
        t = ref_L00_003E9930_02(f20v) * fparg;
        s0->x0 = 0;
        s0->x4 = t;
        u = ref_L00_003E9930_03(f20v);
        s0->x8 = -u * fparg;
        ref_L00_003E9930_04(s0, s0, arg0 + 0xC0);
        ref_L00_003E9930_05(s0, s0, arg1);
        s0 += 1;
        cnt -= 1;
    } while (cnt >= 0);
    ref_L00_003E9930_06(fr.b0, &fr.e[0], arg1);
    ref_L00_003E9930_06(fr.b1, &fr.e[1], arg1);
    ref_L00_003E9930_07(arg2, fr.b0, fr.b1);
    ref_L00_003E9930_08(arg2, arg2, 1.0f);
}
