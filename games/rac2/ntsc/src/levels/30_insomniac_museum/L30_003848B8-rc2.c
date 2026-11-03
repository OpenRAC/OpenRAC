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

extern s32 ref_L30_003848B8_00 __attribute__((sda));
extern s32 ref_L30_003848B8_01[];
extern s8 D_001A63A8[];

s32 func_L30_003848B8(void *arg0) {
    s8 *temp_a3;
    s8 *t0;
    s8 *t1;
    s32 temp_a1;
    s32 temp_a2;
    s32 temp_v1;
    s32 var_v1;
    s32 o1;
    s32 o2;

    t0 = D_001A63A8;
    temp_a3 = *(s8 **)((s8 *)arg0 + 0x68);
    temp_a2 = *(s32 *)(temp_a3 + 0x144);
    temp_a1 = ref_L30_003848B8_01[temp_a2];
    if (*(s32 *)(t0 + 0x68) == 0) {
        temp_v1 = *(s32 *)(t0 + 0x24);
        if ((temp_v1 == -1) && (temp_a2 != temp_v1)) {
            if (temp_a1 != 6) {
                if ((((s32) ref_L30_003848B8_00 >> temp_a1) ^ 1) & 1) {
                    o1 = temp_a1 * 4;
                    var_v1 = *(s32 *)(temp_a3 + o1 + 0xE0);
                    ref_L30_003848B8_00 |= 1 << temp_a1;
                } else {
                    o1 = temp_a1 * 4;
                    o2 = temp_a1 * 4;
                    var_v1 = *(s32 *)(temp_a3 + o2 + 0x10C);
                }
            } else if (temp_a2 == 1) {
                var_v1 = *(s32 *)(temp_a3 + 0xF8);
            } else {
                var_v1 = *(s32 *)(temp_a3 + 0x124);
            }
            t1 = D_001A63A8;
            *(s32 *)(t1 + 0x24) = (var_v1 & 0x3FFF) + 0xBB8;
            *(s32 *)(t1 + 0x28) = 0;
            return 1;
        }
    }
    return 0;
}
