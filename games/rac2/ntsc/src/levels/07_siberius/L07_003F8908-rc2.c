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

extern s32 ref_L07_003F8908_00(s32 a0);
extern s32 ref_L07_003F8908_01(s32 a0, s32 a1);
extern void ref_L07_003F8908_02(void *a0, s32 a1, s32 a2);
extern s32 ref_L07_003F8908_03(s32 a0);
extern s32 ref_L07_003F8908_04 __attribute__((sda));
extern s32 ref_L07_003F8908_05 __attribute__((sda));
extern s32 ref_L07_003F8908_06 __attribute__((sda));
extern s32 ref_L07_003F8908_07 __attribute__((sda));
extern s32 ref_L07_003F8908_08 __attribute__((sda));
extern s32 ref_L07_003F8908_09 __attribute__((sda));

s32 func_L07_003F8908(void *arg0) {
    s32 one  = 1;
    u8 *temp_s0 ;
    s32 var_s1 ;
    s32 temp_v1 ;
    s32 temp_v0;
    s32 var_v0;
    s32 var_v1;
    s32 var_a0;
    s32 var_a1;

    temp_s0 = *(u8 **)((u8 *)arg0 + 0x68);
    temp_v1 = *(s32 *)(temp_s0 + 0x420);
    var_s1 = 7;
    if (temp_v1 != one) {
        var_s1 = 7;
        if (temp_v1 < 2) {
            if (temp_v1 != 0) {
                goto state_default;
            }
        } else {
            goto state_default;
        }
        goto dispatch;
    }

    one = ref_L07_003F8908_00(0xFF) & 1;
    temp_v1 = 6;
    if (one != 0) {
        var_s1 = temp_v1;
    }
    goto dispatch;

state_default:
    temp_v0 = ref_L07_003F8908_00(*(s32 *)(temp_s0 + 0x420) + 1);
    switch (temp_v0) {
    case 0:
        var_s1 = 7;
        break;
    case 1:
        var_s1 = 6;
        break;
    case 2:
        if (*(s32 *)(temp_s0 + 0x458) != 0) {
            var_s1 = 8;
            ref_L07_003F8908_02((void *)(temp_s0 + 0x458), 0, 1);
        } else {
            {
                    s32 arg0  = 2;

                    var_s1 = 6;
                    temp_v0 = ref_L07_003F8908_00(arg0);
                    temp_v1 = 7;
                    if (temp_v0 != 0) {
                        var_s1 = temp_v1;
                    }
                }
        }
        break;
    case 3:
        var_s1 = 9;
        break;
    }

dispatch:
switch (var_s1) {
    case 7:
        var_v1 = ref_L07_003F8908_05;
        var_a0 = 3;
        var_v0 = ref_L07_003F8908_08;
        var_a1 = 5;
        goto block_35;
    case 6:
        var_v1 = ref_L07_003F8908_04;
        var_a0 = 2;
        var_v0 = ref_L07_003F8908_07;
        var_a1 = 3;
        goto block_35;
    case 8:
        *(s32 *)(temp_s0 + 0x438) = 1;
        *(s32 *)(temp_s0 + 0x41C) = ref_L07_003F8908_03(*(s32 *)(temp_s0 + 0x180));
        goto block_36;
    case 9:
        var_v1 = ref_L07_003F8908_06;
        var_a0 = 1;
        var_v0 = ref_L07_003F8908_09;
        var_a1 = 3;
        goto block_35;
    default:
        return var_s1;
    }

block_35:
    *(s32 *)(temp_s0 + 0x444) = var_v1;
    *(s32 *)(temp_s0 + 0x448) = var_v0;
    *(s32 *)(temp_s0 + 0x438) = ref_L07_003F8908_01(var_a0, var_a1);

block_36:
    return var_s1;
}
