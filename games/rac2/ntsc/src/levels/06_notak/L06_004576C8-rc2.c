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

extern f32 ref_L06_004576C8_00 __attribute__((sda));
extern s32 ref_L06_004576C8_01;
extern s32 ref_L06_004576C8_02;
extern f32 ref_L06_004576C8_03(f32);
extern void ref_L06_004576C8_04(void *, f32, f32, f32);
extern s32 ref_L06_004576C8_05(void *, void *, s32, void *, s32, s32);

void func_L06_004576C8(void *arg0) {
    f32 *var_s2;
    f32 temp_f12;
    f32 temp_f20;
    s32 *var_s0;
    s8 *var_s1;
    s8 *temp_v0;
    s32 var_s4;

    var_s2 = &ref_L06_004576C8_00;
    var_s4 = 7;
    temp_v0 = (*(s8 **)((s8 *)(arg0) + (0x68)));
    var_s0 = (s32 *)(temp_v0 + 0x80);
    var_s1 = temp_v0 + 0xA0;
    do {
        var_s4 -= 1;
        temp_f20 = ref_L06_004576C8_03(var_s2[0] * 0.017453294f);
        temp_f12 = var_s2[1];
        var_s2 += 2;
        ref_L06_004576C8_04(var_s1, 1.0f, temp_f20, ref_L06_004576C8_03(temp_f12 * 0.017453294f));
        var_s1 += 0x10;
        *var_s0 = ref_L06_004576C8_05(&ref_L06_004576C8_01, &ref_L06_004576C8_02, 0x3C3C3C78, arg0, 0x19, 0);
        var_s0 += 1;
    } while (var_s4 >= 0);
}
