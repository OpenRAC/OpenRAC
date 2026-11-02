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

extern f32 ref_L04_003A0C40_00;
extern f32 ref_L04_003A0C40_01;
extern f32 ref_L04_003A0C40_02;
extern f32 ref_L04_003A0C40_03;

void func_L04_003A0C40(void *arg0) {
    s32 var_a1;
    s32 var_a2;
    f32 *var_a1_2;
    f32 *var_v1;
    f32 temp_f0;
    f32 var_f2;
    f32 var_f3;
    s32 temp_v0;

    var_v1 = (f32 *)0x70002000;
    var_a1 = 0;
    var_f3 = (*(f32 *)((s8 *)(arg0) + (4))) + ref_L04_003A0C40_01;
    do {
        temp_v0 = var_a1 + 1;
        var_f2 = (*(f32 *)((s8 *)(arg0) + (0))) + ref_L04_003A0C40_00;
        var_a1_2 = (f32 *)((s8 *)0x70000010 + temp_v0 * 0x60);
        var_a2 = 0x10;
        do {
            *var_v1 = var_f2;
            var_a2 -= 1;
            var_v1 += 1;
            *var_v1 = var_f3;
            var_v1 += 1;
            temp_f0 = *var_a1_2;
            var_a1_2 += 1;
            *var_v1 = temp_f0 + (*(f32 *)((s8 *)(arg0) + (8)));
            var_v1 += 1;
            var_f2 += ref_L04_003A0C40_02;
        } while (var_a2 >= 0);
        var_a1 = temp_v0;
        var_f3 += ref_L04_003A0C40_03;
    } while (var_a1 < 0x11);
}
