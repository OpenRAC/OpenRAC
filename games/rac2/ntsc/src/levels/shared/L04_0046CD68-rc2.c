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

typedef struct {
    s8 pad0[0x70];
    f32 f70;
    f32 f74;
    f32 f78;
    f32 f7C;
    f32 f80;
    s8 pad84[0x0C];
    f32 f90;
    s32 f94;
    s32 f98;
} Entity;

typedef struct {
    s8 pad0[0x70];
    Entity * volatile entity;
} Outer;

extern f32 ref_L04_0046CD68_00(f32 *);
extern void ref_L04_0046CD68_01(Outer *);
extern f32 ref_L04_0046CD68_02 __attribute__((sda));
extern f32 ref_L04_0046CD68_03 __attribute__((sda));
extern f32 ref_L04_0046CD68_04 __attribute__((sda));
extern f32 ref_L04_0046CD68_05 __attribute__((sda));
extern s32 ref_L04_0046CD68_06 __attribute__((sda));

void func_L04_0046CD68(Outer *arg0, s32 arg1) {
    Entity *ent = arg0->entity;
    f32 *base = &ent->f70;
    Entity *entv0 ;

    ent->f70 = ref_L04_0046CD68_02;
    if (arg1 != 0) {
        if (ref_L04_0046CD68_06 != 0) {
            ent->f70 = -ref_L04_0046CD68_02;
        }
    }
    base[1] = ref_L04_0046CD68_03;
    base[2] = ref_L04_0046CD68_04;
    base[3] = 1.0f;
    base[4] = ref_L04_0046CD68_00(base);

    entv0 = arg0->entity;
    entv0->f90 = ref_L04_0046CD68_05;
    if (arg1 != 0) {
        entv0->f90 = ref_L04_0046CD68_05 * 1.1f;
    }

    entv0 = arg0->entity;
    entv0->f94 = 0;
    entv0->f98 = 0;
    ref_L04_0046CD68_01(arg0);
}
