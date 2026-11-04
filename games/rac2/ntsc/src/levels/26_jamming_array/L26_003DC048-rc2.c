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

extern s8 ref_L26_003DC048_00[];
extern void ref_L26_003DC048_01(s8 *);
extern void ref_L26_003DC048_02(s8 *, s8 *);
extern f32 ref_L26_003DC048_03(f32, f32);
extern void ref_L26_003DC048_04(s8 *, s8 *, s32, s8 *, f32);

void func_L26_003DC048(s8 *arg0) {
    s8 *temp_s0;
    f32 temp_f0;
    u16 temp_v1;

    temp_s0 = *(s8 **)(arg0 + 0x68);
    ref_L26_003DC048_01(temp_s0 + 0x220);
    ref_L26_003DC048_01(temp_s0 + 0x222);
    ref_L26_003DC048_02(arg0, temp_s0 + 0x278);
    if (*(u8 *)(arg0 + 0x95) == 2) {
        temp_f0 = ref_L26_003DC048_03(*(f32 *)(temp_s0 + 0x27C), 0.209439546f);
        *(f32 *)(temp_s0 + 0x27C) = temp_f0;
        ref_L26_003DC048_04(arg0, temp_s0 + 0x270, 1, ref_L26_003DC048_00, temp_f0);
    } else {
        temp_f0 = ref_L26_003DC048_03(*(f32 *)(temp_s0 + 0x27C), 0.0116355307f);
        *(f32 *)(temp_s0 + 0x27C) = temp_f0;
        ref_L26_003DC048_04(arg0, temp_s0 + 0x270, 0, ref_L26_003DC048_00, temp_f0);
    }
    temp_v1 = *(u16 *)(arg0 + 0x96);
    if (temp_v1 <= 0xFFFE) {
        *(u16 *)(arg0 + 0x96) = temp_v1 + 1;
    }
}
