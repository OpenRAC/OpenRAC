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

extern s8 ref_L05_0037AA88_00[];

s32 func_L05_0037AA88(s32 arg0) {
    s32 k ;
    s32 val ;
    s8 *a2;
    s8 *a3;
    s32 var_a0;
    s32 temp_a1;

    k = 0x28;
    a2 = ref_L05_0037AA88_00;
    a3 = a2 - 4;
    var_a0 = arg0;
loop_1:
    if (var_a0 < 0) {
        goto end_1;
    }
    temp_a1 = var_a0 * k;
    val = *(s32 *)(temp_a1 + (s32)a3);
    if (val != 0) {
        return var_a0;
    }
    var_a0 = *(s32 *)(temp_a1 + (s32)a2);
    goto loop_1;
end_1:
    return -1;
}
