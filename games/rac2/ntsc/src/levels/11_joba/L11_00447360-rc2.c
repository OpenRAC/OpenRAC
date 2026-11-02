
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

extern s8 *ref_L11_00447360_00;
extern s8 ref_L11_00447360_01[];

extern void ref_L11_00447360_02(void *, s32, s32, void *, s8 *, f32);
extern void ref_L11_00447360_03(void *);
extern void ref_L11_00447360_04(void *);
extern void ref_L11_00447360_05(void *);
extern void ref_L11_00447360_06(void *);
extern void ref_L11_00447360_07(void *);
extern void ref_L11_00447360_08(void *);
extern void ref_L11_00447360_09(void *);
extern void ref_L11_00447360_10(void *);
extern void ref_L11_00447360_11(void *);
extern void ref_L11_00447360_12(void *);

void func_L11_00447360(s32 arg0) {
    s8 *temp_s0;
    s16 var_v1;

    temp_s0 = ref_L11_00447360_00;
    if (temp_s0 != 0) {
        do {
            var_v1 = *(s16 *)(temp_s0 + 0xAA);
            switch (var_v1) {
            case 0xCB:
                ref_L11_00447360_02(temp_s0, arg0, 0x10000, temp_s0 + 0x10, ref_L11_00447360_01, 100.0f);
                break;
            case 0xE7D:
                ref_L11_00447360_11(temp_s0);
                break;
            case 0xEB0:
                ref_L11_00447360_12(temp_s0);
                break;
            case 0xCB2:
                ref_L11_00447360_06(temp_s0);
                break;
            case 0xB02:
                ref_L11_00447360_05(temp_s0);
                break;
            case 0x1366:
                ref_L11_00447360_07(temp_s0);
                break;
            case 0x1367:
                ref_L11_00447360_08(temp_s0);
                break;
            case 0x326:
                ref_L11_00447360_03(temp_s0);
                break;
            case 0xA5C:
            case 0x1192:
                ref_L11_00447360_04(temp_s0);
                break;
            case 0xE20:
                ref_L11_00447360_10(temp_s0);
                break;
            case 0xAE0:
            case 0xAE1:
            case 0xAE2:
            case 0xAEC:
            case 0xB06:
            case 0xB09:
            case 0xCC0 ... 0xCC9:
            case 0xCDB:
                ref_L11_00447360_09(temp_s0);
                break;
            default:
                break;
            }
            temp_s0 = *(s8 **)(temp_s0 + 0x28);
        } while (temp_s0 != 0);
    }
}
