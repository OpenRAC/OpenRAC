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

extern f32 ref_L03_00384340_00(s32);
extern void ref_L03_00384340_01(void *, s32, s32);
extern s32 ref_L03_00384340_02(void *, s32);
extern void ref_L03_00384340_03(void *);
extern s32 ref_L03_00384340_04;
extern s32 g_001B2548 __attribute__((sda));
extern s32 ref_L03_00384340_05;
extern s32 g_001B2B0C __attribute__((sda));
extern s8 ref_L03_00384340_06[];


void func_L03_00384340(void *arg0) {
    void *var_s1  = arg0;
    s32 *var_s0  = (*(s32 **)((s8 *)(arg0) + (0x68)));
    s32 temp_v1  = (*(u8 *)((s8 *)(arg0) + (0x20)));
    s32 var_s2 ;
    s32 var_s3 ;
    s32 addr_v0 ;
    f32 temp_f20 ;
    f32 result_f0 ;
    f32 product_f1 ;
    s32 temp_a0;

    switch (temp_v1) {
    case 0:
        var_s3 = -1;
        var_s2 = 0xF;
        do {
            temp_a0 = *var_s0;
            if (temp_a0 != var_s3) {
                temp_f20 = ref_L03_00384340_00(g_001B2B0C + (temp_a0 << 7));
                result_f0 = ref_L03_00384340_00(ref_L03_00384340_05 + (*var_s0 << 7) + 0x10);
                addr_v0 = *var_s0;
                product_f1 = temp_f20 * result_f0;
                temp_v1 = ref_L03_00384340_05;
                result_f0 = (result_f0 < temp_f20) ? temp_f20 : result_f0;
                addr_v0 <<= 7;
                addr_v0 += temp_v1;
                (*(f32 *)((s8 *)(addr_v0) + (0x7C))) = product_f1;
                addr_v0 = *var_s0;
                addr_v0 <<= 7;
                addr_v0 += temp_v1;
                (*(f32 *)((s8 *)(addr_v0) + (0x3C))) = result_f0;
                ref_L03_00384340_01(var_s1, 2, -1);
            }
            var_s2 -= 1;
            var_s0 += 1;
        } while (var_s2 >= 0);
        break;
    case 1:

        ref_L03_00384340_03(var_s1);
        if ((ref_L03_00384340_04 & 0xF) == (((s32)var_s1 >> 8) & 0xF)) {
            if (ref_L03_00384340_02(ref_L03_00384340_06, (*(s32 *)((s8 *)(var_s0) + (0x44)))) == 0) {
                ref_L03_00384340_01(var_s1, 2, -1);
            }
        }
        break;
    case 2:
        if (((g_001B2548 & 0xF) == (((s32)var_s1 >> 8) & 0xF)) &&
            (ref_L03_00384340_02(ref_L03_00384340_06, (*(s32 *)((s8 *)(var_s0) + (0x44)))) != 0)) {
            ref_L03_00384340_01(var_s1, 1, -1);
        }
        break;
    }
}
