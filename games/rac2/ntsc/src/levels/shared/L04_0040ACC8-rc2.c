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

extern s8 *ref_L04_0040ACC8_00 __attribute__((sda));
extern s32 ref_L04_0040ACC8_01 __attribute__((sda));
extern s32 ref_L04_0040ACC8_02 __attribute__((sda));
extern s32 ref_L04_0040ACC8_03 __attribute__((sda));

void func_L04_0040ACC8(void) {
    s8 *temp_a0;
    s32 temp_v0;
    s32 temp_v1;

    temp_a0 = *(s8 **)(ref_L04_0040ACC8_00 + 0x68);
    if (*(s32 *)(temp_a0 + 0x744) < 5) {
        temp_v1 = *(s32 *)(temp_a0 + 0x724);
        ref_L04_0040ACC8_01 = 1;
        if (temp_v1 != 0) {
            temp_v0 = ref_L04_0040ACC8_03 + 1;
            ref_L04_0040ACC8_03 = temp_v0;
            if (temp_v0 == 3) {
                ref_L04_0040ACC8_02 = 1;
            }
        }
    } else if (ref_L04_0040ACC8_03 < 4) {
        ref_L04_0040ACC8_03 = 0;
    }
    *(s32 *)(temp_a0 + 0x724) = 0;
    *(s32 *)(temp_a0 + 0x744) = 0;
}
