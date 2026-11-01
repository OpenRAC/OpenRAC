
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

extern void ref_L03_0038AA20_00(s8 *, s32, s32, s32, s32, s32, s32, f32, f32, f32, f32, f32);
extern s8 ref_L03_0038AA20_01[];

void func_L03_0038AA20(s8 *arg0) {
    s8 *s0;
    s32 i;
    s32 b1;
    s32 b2;

    i = 0;
    b1 = (s32)ref_L03_0038AA20_01;
    s0 = *(s8 **)(arg0 + 0x68);
    b2 = b1 + 0x400;
    do {
        ref_L03_0038AA20_00(arg0, ((*(s32 *)(s0 + 0xDC) + i) << 8) + b1, *(s32 *)(s0 + 0x38), *(s32 *)(s0 + 0x40), *(s32 *)(s0 + 0x60), *(s32 *)(s0 + 0x44), *(s32 *)(s0 + 0x64), *(f32 *)(s0 + 0x50), *(f32 *)(s0 + 0x70), *(f32 *)(s0 + 0x54), *(f32 *)(s0 + 0x74), 0.89999997615814208984375f);
        ref_L03_0038AA20_00(arg0, ((*(s32 *)(s0 + 0xDC) + i) << 8) + b2, *(s32 *)(s0 + 0x38), *(s32 *)(s0 + 0x48), *(s32 *)(s0 + 0x68), *(s32 *)(s0 + 0x4C), *(s32 *)(s0 + 0x6C), *(f32 *)(s0 + 0x58), *(f32 *)(s0 + 0x78), *(f32 *)(s0 + 0x5C), *(f32 *)(s0 + 0x7C), 0.100000001490116119384765625f);
        i += 1;
    } while (i < 4);
}
