
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

extern f32 g_001B1B24 __attribute__((sda));
extern void ref_L01_0041C890_00(void *, void *, void *);
extern f32 ref_L01_0041C890_01(f32, f32);
extern void ref_L01_0041C890_02(void *);

void func_L01_0041C890(s8 *arg0) {
    s8 *s0;
    s8 *a0;
    s8 *a2;
    s32 v;

    a0 = arg0 + 0x10;
    s0 = *(s8 **)(arg0 + 0x68);
    *(f32 *)(arg0 + 0x2C) = *(f32 *)(*(s8 **)(arg0 + 0x24) + 0x24) * g_001B1B24;
    *(f32 *)(s0 + 0x8) = *(f32 *)(s0 + 0x8) - 0.0041666668839752674103f;
    ref_L01_0041C890_00(a0, a0, s0);
    a2 = arg0 + 0xF0;
    ref_L01_0041C890_00(a2, a2, s0 + 0x10);
    *(f32 *)(arg0 + 0xF0) = ref_L01_0041C890_01(*(f32 *)(arg0 + 0xF0), *(f32 *)(s0 + 0x10));
    *(f32 *)(arg0 + 0xF4) = ref_L01_0041C890_01(*(f32 *)(arg0 + 0xF4), *(f32 *)(s0 + 0x14));
    *(f32 *)(arg0 + 0xF8) = ref_L01_0041C890_01(*(f32 *)(arg0 + 0xF8), *(f32 *)(s0 + 0x18));
    v = *(u8 *)(arg0 + 0xBC) - 1;
    *(u8 *)(arg0 + 0xBC) = v;
    if ((v & 0xFF) == 0) {
        ref_L01_0041C890_02(arg0);
    }
}
