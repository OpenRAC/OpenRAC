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

extern s8 D_00189EA0[];
extern s32 ref_L09_00406BF8_00(s8 *, s32);
extern void ref_L09_00406BF8_01(s32, s32);

s32 func_L09_00406BF8(s32 arg0, s32 arg1) {
    s32 var_a1;
    s32 var_s0;

    var_a1 = 0;
    var_s0 = 0;
    if (arg0 != -1) {
        if (ref_L09_00406BF8_00(D_00189EA0, arg0) != 0) {
            var_a1 = var_s0;
            var_s0 = 1;
        } else {
            var_a1 = 2;
        }
    }
    ref_L09_00406BF8_01(arg1, var_a1);
    return var_s0;
}
