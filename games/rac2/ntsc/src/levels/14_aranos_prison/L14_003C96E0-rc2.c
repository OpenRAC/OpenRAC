
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

extern void ref_L14_003C96E0_00(s8 *, s32, s32);
extern f32 ref_L14_003C96E0_01(f32);
extern void ref_L14_003C96E0_02(s8 *, s8 *, s8 *);
extern void ref_L14_003C96E0_03(s8 *, s8 *, f32);

void func_L14_003C96E0(s8 *arg0, s8 *arg1, f32 fparg0, f32 fparg1, f32 fparg2) {
    f32 temp_f0;
    f32 temp_f21;
    f32 temp_f22;
    f32 temp_f23;
    s8 *temp_s1;
    s8 *temp_s3;

    temp_s1 = *(s8 **)(arg0 + 0x68);
    ref_L14_003C96E0_00(arg0, 0x16, 7);
    temp_f21 = fparg0 - *(f32 *)(arg0 + 0x18);
    temp_f21 = temp_f21 + temp_f21; temp_s3 = temp_s1 + 0x1B0; temp_f22 = ref_L14_003C96E0_01(temp_f21 / fparg1);
    temp_f22 = temp_f22 + ref_L14_003C96E0_01((2.0f * (fparg0 - *(f32 *)(arg1 + 8))) / fparg2);
    ref_L14_003C96E0_02(temp_s3, arg1, arg0 + 0x10);
    ref_L14_003C96E0_03(temp_s3, temp_s3, 1.0f / temp_f22);
    temp_f0 = ref_L14_003C96E0_01(temp_f21 * fparg1);
    *(f32 *)(temp_s1 + 0x394) = fparg1;
    *(f32 *)(temp_s1 + 0x398) = fparg2;
    *(f32 *)(temp_s1 + 0x1B8) = temp_f0;
    temp_f23 = *(f32 *)(arg1 + 8) + 0.333f;
    *(s32 *)(temp_s1 + 0x14C) = -1;
    *(f32 *)(temp_s1 + 0x3A0) = temp_f23;
}
