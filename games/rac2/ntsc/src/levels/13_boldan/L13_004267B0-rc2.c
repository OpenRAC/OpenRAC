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

extern s32 D_001A79F0;
extern s32 ref_L13_004267B0_00;
extern u8 D_0019B2E8[];
extern void ref_L13_004267B0_01();

void func_L13_004267B0(s8 *arg0) {
    s32 *temp_a1;

    temp_a1 = (*(s32 **)((s8 *)(arg0) + (0x68)));
    if (D_0019B2E8[(*(u8 *)((s8 *)(arg0) + (0xB0))) + (D_001A79F0 << 4)] & 0x80) {
        ref_L13_004267B0_01(ref_L13_004267B0_00 + (*temp_a1 << 8), temp_a1);
        ref_L13_004267B0_01(arg0);
    }
}
