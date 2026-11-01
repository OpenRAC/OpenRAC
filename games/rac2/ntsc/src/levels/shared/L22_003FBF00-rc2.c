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

extern s8 ref_L22_003FBF00_00[];
extern f32 ref_L22_003FBF00_01 __attribute__((sda));
extern void ref_L22_003FBF00_02(void *a, s32 b, void *c, f32 d);
extern void ref_L22_003FBF00_03(void *a, void *b, f32 c);
extern void ref_L22_003FBF00_04(void *a, void *b, f32 c);
extern void ref_L22_003FBF00_05(void *a, void *b, void *c);

void func_L22_003FBF00(void *arg0, void *arg1, s32 arg2) {
    s32 ix ;
    void *t0 ;
    s32 sp[4];
    s32 sp10[4];
    s32 sp20[4];
    void *temp_v1;

    ix = arg2;
    t0 = (*(void **)((s8 *)(arg0) + (0x68)));
    ref_L22_003FBF00_02(sp, (ix * 0x10) + (s32)ref_L22_003FBF00_00, (s8 *)t0 + 0x10, (*(f32 *)((s8 *)(t0) + (0x1C))));
    t0 = (s8 *)t0 + (ix * 4);
    temp_v1 = (*(void **)((s8 *)(t0) + (0x60)));
    if (temp_v1 != 0) {
        if ((*(u8 *)((s8 *)(arg0) + (0x20))) == 0) {
            ref_L22_003FBF00_03(sp, sp, (*(f32 *)((s8 *)(temp_v1) + (0x28))));
        }
    }
    ref_L22_003FBF00_04(sp20, (s8 *)arg0 + 0xE0, ref_L22_003FBF00_01);
    ref_L22_003FBF00_05(sp10, (s8 *)arg0 + 0x10, sp20);
    ref_L22_003FBF00_05(arg1, sp10, sp);
}
