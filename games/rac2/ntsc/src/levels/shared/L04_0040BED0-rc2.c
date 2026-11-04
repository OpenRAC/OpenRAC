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

typedef int rc2_qword __attribute__((mode(TI)));
typedef int TI __attribute__((mode(TI)));
typedef struct {
    f32 f0;
    f32 f4;
    f32 f8;
    s32 padC;
    TI v10;
} Stack_0040BED0;
extern void ref_L04_0040BED0_00(void *, void *, TI *);
extern void ref_L04_0040BED0_01(void *, s32, s32);
extern void ref_L04_0040BED0_02(TI *, TI *, f32);
extern void ref_L04_0040BED0_03(void *, void *, f32);
extern s32 ref_L04_0040BED0_04(f32);
extern s32 ref_L04_0040BED0_05(s32, s32);
extern f32 ref_L04_0040BED0_06(f32, f32);
extern f32 ref_L04_0040BED0_07(f32, f32);
extern void ref_L04_0040BED0_08(s32, void *, s32, f32, f32, s32);
extern void *ref_L04_0040BED0_09(s32, void *, s32, s32, s32);

void func_L04_0040BED0(s32 arg0, rc2_qword *arg1, s32 arg2, s32 arg3, f32 fparg0) {
    Stack_0040BED0 sp;
    TI copyValue  = *(TI *)arg1;
    s32 var_s1 = 0;
    s32 var_s1_2;
    void *temp_s0;

    sp.v10 = copyValue;
    ref_L04_0040BED0_01(&sp, arg0, arg3 + 0x10);
    ref_L04_0040BED0_03(&sp, &sp, 0.01f);
    while (var_s1 < ref_L04_0040BED0_04(fparg0 * 30.0f)) {
        var_s1 += 1;
        sp.f0 += ref_L04_0040BED0_07(0.01f, 0.03f);
        sp.f4 += ref_L04_0040BED0_07(0.01f, 0.015f);
        sp.f8 += ref_L04_0040BED0_07(0.01f, 0.02f);
        ref_L04_0040BED0_02(&sp.v10, &sp.v10, 0.6f);
        ref_L04_0040BED0_00(&sp, &sp, &sp.v10);
        ref_L04_0040BED0_08(arg0, &sp, 0xF, 0.0027777778f, fparg0, 0x7FFFFFFF);
    }
    if (arg2 == 0) {
        var_s1_2 = 0;
        while (var_s1_2 < ref_L04_0040BED0_04(fparg0 * 5.0f)) {
            var_s1_2 += 1;
            temp_s0 = ref_L04_0040BED0_09(arg0, &sp, ref_L04_0040BED0_05(30, 60), 0, 0);
            *(f32 *)((u8 *)temp_s0 + 0x2C) *= ref_L04_0040BED0_06(0.2f, 0.4f);
        }
    }
}
