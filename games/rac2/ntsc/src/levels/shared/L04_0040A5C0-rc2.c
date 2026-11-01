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

extern s8 *ref_L04_0040A5C0_00;

void func_L04_0040A5C0(s8 *arg0) {
    s8 *base;
    s32 *var_a0;
    s32 temp_v0;
    s32 var_a1;
    s32 km1;
    s32 k1;
    s32 k2;

    base = *(s8 **)(arg0 + 0x68);
    km1 = -1;
    k1 = 1;
    k2 = 2;

    var_a0 = (s32 *)(base + 0x1E4);
    var_a1 = 0x14;
    do {
        temp_v0 = *var_a0;
        var_a1 -= 1;
        if (temp_v0 != km1) {
            *(s8 *)(*(s8 **)((s32)((temp_v0 << 8) + (s32)ref_L04_0040A5C0_00) + 0x68) + 0xA0) = k1;
            *(s8 *)((s32)((*var_a0 << 8) + (s32)ref_L04_0040A5C0_00) + 0x20) = k2;
        }
        var_a0 += 1;
    } while (var_a1 >= 0);
}
