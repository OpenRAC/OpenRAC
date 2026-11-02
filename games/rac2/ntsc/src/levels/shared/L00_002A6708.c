/* SPDX-License-Identifier: MIT
 * MIT License
 *
 * Copyright (c) 2026 RC2 decomp contributions
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

extern void ref_L00_002A6708_00(s32, void *);

typedef struct {
    s8 pad0[0x2290];
    s32 f2290;
    s8 pad1[0x198];
    s32 slots[9];
    void *ptrs[9];
} Ctx;

extern Ctx D_00189E20;
extern s8 D_00188660[];

void func_L00_002A6708(void) {
    Ctx *c = &D_00189E20;
    s8 *tb = D_00188660;
    void **pp = c->ptrs;
    s32 *var_s0 = c->slots;
    s32 off = 0;
    s32 var_s2 = 8;
    s32 temp_a0;
    void *var_a1;
    s8 *temp_v1;
    u8 var_v0;
    s32 m1 = -1;

    do {
        var_a1 = *(void **)(off + (s32)pp);
        temp_a0 = *var_s0;
        if (var_a1 != 0) {
            if (temp_a0 == m1) {
                *var_s0 = m1;
                goto L_inc;
            }
            temp_v1 = tb + temp_a0 * 0x70;
            if (*(s32 *)(temp_v1 + 0x88) != (s32)var_a1) {
                *var_s0 = m1;
                goto L_inc;
            }
            var_v0 = *(u8 *)(temp_v1 + 0x74);
        } else {
            if (temp_a0 == m1) {
                *var_s0 = m1;
                goto L_inc;
            }
            temp_v1 = tb + temp_a0 * 0x70;
            var_a1 = temp_v1;
            if (*(s32 *)(temp_v1 + 0x88) != c->f2290) {
                *var_s0 = m1;
                goto L_inc;
            }
            var_v0 = *(u8 *)(temp_v1 + 0x74);
        }
        if (var_v0 == 0) {
            *var_s0 = m1;
            goto L_inc;
        }
        ref_L00_002A6708_00(temp_a0, var_a1);
        *var_s0 = m1;
    L_inc:
        var_s0 += 1;
        var_s2 -= 1;
        off += 4;
    } while (var_s2 >= 0);
}
