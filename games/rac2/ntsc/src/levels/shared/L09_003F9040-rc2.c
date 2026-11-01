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

extern void ref_L09_003F9040_00(s8 *, s32, s32, s32);
extern s32 D_001A79F0;
extern s32 ref_L09_003F9040_01 __attribute__((sda));
extern s32 ref_L09_003F9040_02 __attribute__((sda));
extern s32 ref_L09_003F9040_03 __attribute__((sda));
extern s32 ref_L09_003F9040_04 __attribute__((sda));

void func_L09_003F9040(s8 *arg0) {
    s32 v;
    s32 m;
    s32 w;

    switch (D_001A79F0) {
    case 8:
        w = ref_L09_003F9040_01;
        m = 0xFF000000;
        if ((w & m) != 0) {
            ref_L09_003F9040_00(arg0, w & 0xFF, (w >> 8) & 0xFF, (w >> 16) & 0xFF);
        }
        v = ref_L09_003F9040_02;
        goto block_9;
    case 20:
        w = ref_L09_003F9040_03;
        m = 0xFF000000;
        if ((w & m) != 0) {
            ref_L09_003F9040_00(arg0, w & 0xFF, (w >> 8) & 0xFF, (w >> 16) & 0xFF);
        }
        v = ref_L09_003F9040_04;
        goto block_9;
    }
    return;
block_9:
    if ((v & m) != 0) {
        (*(s32 *)((s8 *)(arg0) + (0x78))) = v;
        (*(u16 *)((s8 *)(arg0) + (0x34))) = (*(u16 *)((s8 *)(arg0) + (0x34))) | 0x10;
    }
}
