
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

s32 ref_L00_003DFDC0_00(s32, s32);
f32 ref_L00_003DFDC0_01(void);
void ref_L00_003DFDC0_02(void *, s32, f32, s32, f32, s32, s32, f32);

void func_L00_003DFDC0(void *arg0) {
    f32 temp_f20;
    f32 temp_f20_2;
    f32 temp_f20_3;
    f32 temp_f20_4;
    s32 temp_s0;
    s32 temp_s0_2;
    s32 temp_s0_3;
    s32 temp_s0_4;
    u8 temp_v1;
    void *temp_s1;
    void *temp_s1_2;
    void *temp_s3;

    temp_v1 = (*(u8 *)((char *)(arg0) + (0x20)));
    temp_s3 = (*(void **)((char *)(arg0) + (0x68)));
    switch (temp_v1) {                               
    case 0:
        temp_s1 = arg0 + 0x10;
        temp_f20 = (*(f32 *)((char *)(arg0) + (0x2C))) * ((*(f32 *)((char *)(temp_s3) + (0x20))) * 45.0f);
        temp_s0 = ref_L00_003DFDC0_00(0x5F, 0x6F);
        ref_L00_003DFDC0_02(temp_s1, 0x103080, temp_f20, temp_s0, ref_L00_003DFDC0_01(), 0xB, 2, 1.0f);
        temp_f20_2 = (*(f32 *)((char *)(arg0) + (0x2C))) * ((*(f32 *)((char *)(temp_s3) + (0x20))) * 22.5f);
        temp_s0_2 = ref_L00_003DFDC0_00(0x6F, 0x7F);
        ref_L00_003DFDC0_02(temp_s1, 4144959, temp_f20_2, temp_s0_2, ref_L00_003DFDC0_01(), 0xB, 2, 1.0f);
        return;
    case 1:
        temp_s1_2 = arg0 + 0x10;
        temp_f20_3 = (*(f32 *)((char *)(arg0) + (0x2C))) * ((*(f32 *)((char *)(temp_s3) + (0x20))) * 45.0f);
        temp_s0_3 = ref_L00_003DFDC0_00(0x5F, 0x6F);
        ref_L00_003DFDC0_02(temp_s1_2, 0x103080, temp_f20_3, temp_s0_3, ref_L00_003DFDC0_01(), 0xB, 2, 1.0f);
        temp_f20_4 = (*(f32 *)((char *)(arg0) + (0x2C))) * ((*(f32 *)((char *)(temp_s3) + (0x20))) * 22.5f);
        temp_s0_4 = ref_L00_003DFDC0_00(0x6F, 0x7F);
        ref_L00_003DFDC0_02(temp_s1_2, 4144959, temp_f20_4, temp_s0_4, ref_L00_003DFDC0_01(), 0xB, 2, 1.0f);
        return;
    }
}
