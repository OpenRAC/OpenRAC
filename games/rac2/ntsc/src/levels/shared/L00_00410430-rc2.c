
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

extern s32 ref_L00_00410430_00(s32, s32);
extern s32 ref_L00_00410430_01(s32, s32, void *);
extern void *ref_L00_00410430_02(void *, s32, s32, void *, f32, f32, f32, f32);
extern s32 ref_L00_00410430_03(void *, s32);
extern u8 ref_L00_00410430_04[];
extern volatile s32 ref_L00_00410430_05;

void func_L00_00410430(void *arg0, s32 arg1, s32 arg2) {
    u8 *state = *(u8 **)((u8 *)arg0 + 0x68);
    void *result;
    s32 temp;
    if (arg2 == 0) {
        ref_L00_00410430_01(5, 0, arg0);
        result = ref_L00_00410430_02(arg0, arg1, *(s32 *)(state + 0), ref_L00_00410430_04 - 0x70E0, *(f32 *)(state + 8), 2.0f, 5.0f, 0.6f);
        if (result != 0) {
            ref_L00_00410430_00(-1, 1);
            ref_L00_00410430_05 = 1;
            *(void **)((u8 *)result + 0xB8) = arg0;
        }
    } else {
        temp = arg2 - 1;
        *(s32 *)(state + 0x34) = arg2;
        *(s32 *)(state + 0x28) = temp;
        *(s32 *)(state + 0x2C) = temp;
        ref_L00_00410430_01(5, 0, arg0);
        ref_L00_00410430_03(arg0, arg1);
    }
}
