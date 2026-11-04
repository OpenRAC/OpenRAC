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

void func_L09_00408058(void *arg0, s32 arg1, s32 arg2, s32 arg3) {
    u16 temp_v0_2;
    u16 var_v0;
    u16 var_v0_2 ;
    s32 temp_v1 ;
    void *temp_v0;
    void *temp_v0_3;
    void *temp_v0_4;

    if ((arg0 != 0) &&
        (temp_v1 = (*(u8 *)((s8 *)(arg0) + (0x20))), temp_v1 != 0xFE) &&
        (temp_v1 != 0xFD)) {
        if ((arg1 != 0) &&
            ((temp_v0 = (*(void **)((s8 *)(arg0) + (0x24))), temp_v0 == 0) ||
             ((*(s16 *)((s8 *)(temp_v0) + (0x46))) != 0x12))) {
            temp_v0_2 = (*(u16 *)((s8 *)(arg0) + (0x34)));
            if (arg1 > 0) {
                var_v0 = temp_v0_2 & 0xFFFD;
            } else {
                var_v0 = temp_v0_2 | 2;
            }
            (*(u16 *)((s8 *)(arg0) + (0x34))) = var_v0;
        }
        if ((arg2 != 0) &&
            ((arg0 == 0) ||
             ((temp_v0_3 = (*(void **)((s8 *)(arg0) + (0x24))), temp_v0_3 == 0) ||
              ((*(s16 *)((s8 *)(temp_v0_3) + (0x46))) != 0x12)))) {
            temp_v1 = 1;
            if (arg2 > 0) {
                (*(s8 *)((s8 *)(arg0) + (0x31))) = temp_v1;
                var_v0_2 = (*(u16 *)((s8 *)(arg0) + (0x34))) & 0xFFFE;
            } else {
                (*(s8 *)((s8 *)(arg0) + (0x31))) = 0;
                var_v0_2 = (*(u16 *)((s8 *)(arg0) + (0x34))) | 1;
            }
            (*(u16 *)((s8 *)(arg0) + (0x34))) = var_v0_2;
        }
        if (arg3 != 0) {
            temp_v0_4 = (*(void **)((s8 *)(arg0) + (0x24)));
            if (temp_v0_4 != 0) {
                if (arg3 > 0) {
                    (*(s32 *)((s8 *)(arg0) + (0x98))) = (s32)(*(s32 *)((s8 *)(temp_v0_4) + (0x10)));
                    return;
                }
                (*(s32 *)((s8 *)(arg0) + (0x98))) = 0;
            }
        }
    }
}
