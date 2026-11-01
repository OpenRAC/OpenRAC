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

typedef int TI __attribute__((mode(TI)));

extern f32 ref_L02_003CD1E8_00(f32);
extern f32 ref_L02_003CD1E8_01(f32);
extern void ref_L02_003CD1E8_02(void *, void *, void *);
extern f32 ref_L02_003CD1E8_03(f32, f32);
extern f32 ref_L02_003CD1E8_04(void);
extern s32 ref_L02_003CD1E8_05(void *, void *, void *, s32, f32);

void func_L02_003CD1E8(void *arg0, void *arg1) {
    f32 temp_f0;
    f32 temp_f0_2;
    f32 temp_f21;
    void *temp_s1;
    TI temp_s128;
    TI temp_v0 ;

    temp_s1 = (*(void **)((s8 *)(arg0) + (0x68)));
    temp_f21 = ref_L02_003CD1E8_03(2.0f, 5.0f);
    temp_f0 = ref_L02_003CD1E8_04();
    (*(f32 *)((s8 *)(arg1) + (0))) = ref_L02_003CD1E8_00(temp_f0) * temp_f21;
    temp_f0_2 = ref_L02_003CD1E8_01(temp_f0) * temp_f21;
    (*(s32 *)((s8 *)(arg1) + (8))) = 0;
    (*(f32 *)((s8 *)(arg1) + (4))) = temp_f0_2;
    ref_L02_003CD1E8_02(arg1, arg1, temp_s1 + 0x340);
    temp_v0 = *(volatile TI *)((char *)temp_s1 + 0x340);
    temp_s128 = temp_v0;
    ref_L02_003CD1E8_05(&temp_s128, arg1, arg1, (*(s32 *)((s8 *)(temp_s1) + (0x318))), (*(f32 *)((s8 *)(temp_s1) + (0x100))));
}
