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
#ifndef NULL
#define NULL 0
#endif

extern void *ref_L10_004176C0_00();
extern void ref_L10_004176C0_01(s32, s32, s8 *, s32);
extern void ref_L10_004176C0_02(s8 *, s32, s32, f32, s32, f32, f32, f32, s32, s32, s32);
extern s32 ref_L10_004176C0_03(s32, s32);

void func_L10_004176C0(s8 *arg0) {
    s8 *s1;
    s8 *v;

    s1 = *(s8 **)(arg0 + 0x68);
    v = (s8 *)ref_L10_004176C0_00();
    if (v != NULL && *(s32 *)(v + 0x120) != 0 && *(s32 *)(s1 + 0x2E4) == 0) {
        ref_L10_004176C0_01(0, 0, arg0, 0xF86);
        ref_L10_004176C0_02(arg0, 0x6000FF00, 1, 0.5f, 0, 2.0f, 0.9f, 1.3f, 0x1E, 2, *(s32 *)(s1 + 0x2C0));
        ref_L10_004176C0_02(arg0, 0x6000FF00, 1, 0.5f, 0, 2.0f, 0.9f, 1.3f, 0x1E, 2, *(s32 *)(s1 + 0x2C4));
        *(s32 *)(s1 + 0x2E4) = ref_L10_004176C0_03(0x32, 0x78);
    }
}
