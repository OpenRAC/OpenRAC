
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

extern s8 D_00189E20[];
extern f32 ref_L00_003DC1C0_00 __attribute__((sda));
extern f32 ref_L00_003DC1C0_01 __attribute__((sda));
extern f32 ref_L00_003DC1C0_02 __attribute__((sda));
extern f32 ref_L00_003DC1C0_03 __attribute__((sda));
extern s32 ref_L00_003DC1C0_04(s32, s32, s32, s32, f32, f32, f32, f32);
extern f32 ref_L00_003DC1C0_05(f32, f32);
extern void ref_L00_003DC1C0_06(s32, s32, s32, f32);
extern void ref_L00_003DC1C0_07(s32, s32, s32, s32, f32, f32, f32, f32);

s32 func_L00_003DC1C0(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s8 *b;
    s32 temp_s0;
    s32 temp_s1;

    b = D_00189E20;
    temp_s1 = ref_L00_003DC1C0_04(*(s32 *)(b + 0x2290) + 0xC0, 5, 0, arg3, ref_L00_003DC1C0_01, *(f32 *)(b + 0x98), 0.39269909f, 0.39269909f);
    ref_L00_003DC1C0_07(arg1, arg0 + 0x10, temp_s1, 0, ref_L00_003DC1C0_02, ref_L00_003DC1C0_05(ref_L00_003DC1C0_02, ref_L00_003DC1C0_01), ref_L00_003DC1C0_00 * 0.016666668f, ref_L00_003DC1C0_03 * 0.00027777778f);
    if (temp_s1 == 0) {
        temp_s0 = *(s32 *)(b + 0x2290) + 0xE0;
        ref_L00_003DC1C0_06(arg1, arg1, temp_s0, ref_L00_003DC1C0_05(-0.19634955f, 0.19634955f));
    }
    return temp_s1;
}
