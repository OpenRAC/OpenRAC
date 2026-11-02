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

extern s32 D_001A79F0;
extern s32 ref_L01_00367F78_00 __attribute__((sda));
extern u8 D_001395B8[];

void func_L01_00367F78(void) {
    u8 *base;
    s32 g;
    s32 *var_a1;
    u32 var_a0;
    u32 one;

    base = D_001395B8;
    g = D_001A79F0;
    var_a0 = 1;
    one = 1;
    var_a1 = &ref_L01_00367F78_00;
    for (; (s32) var_a0 < 5; var_a0++) {
        if (g == *var_a1) {
            if (var_a0 < 8U) {
                base[0x8D] = (u8) (base[0x8D] | (one << var_a0));
            }
        }
        var_a1 += 1;
    }
}
