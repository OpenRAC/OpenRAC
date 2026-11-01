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

extern void *ref_L06_00457920_00(void *, s32, s16, s32, void *, void *, void *, s32, f32, f32);
extern void ref_L06_00457920_01(s32, s32, f32);
extern s32 ref_L06_00457920_02;
extern s8 ref_L06_00457920_03[];

void func_L06_00457920(void *arg0) {
    void *temp_s0;
    s32 temp_a0;
    s32 var_s2;

    var_s2 = 0;
    do {
        temp_s0 = ref_L06_00457920_00(arg0, 2, (*(s16 *)((s8 *)(arg0) + (0xAA))), var_s2, (s8 *)arg0 + 0x10, (s8 *)arg0 + 0xF0, &ref_L06_00457920_02, 0xFF, 0.0f, 10.0f);
        var_s2 += 1;
        if (temp_s0 != 0) {
            temp_a0 = (*(s32 *)((s8 *)(temp_s0) + (0x68)));
            ref_L06_00457920_01(temp_a0, temp_a0 + 0x20, 0.00200000009499490261077880859375f);
            (*(s32 *)((s8 *)(temp_s0) + (0x64))) = (s32)ref_L06_00457920_03;
        }
    } while (var_s2 < 0xF);
}
