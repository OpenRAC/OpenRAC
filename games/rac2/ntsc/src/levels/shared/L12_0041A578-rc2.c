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

extern s32 *ref_L12_0041A578_00[];

s32 func_L12_0041A578(s8 *arg0, s32 arg1) {
    s32 i  = arg1 + 1;
    f32 z  = 0.0f;
    s32 **tb  = (s32 **)ref_L12_0041A578_00;
    s32 q  = (i << 4) + 0x1C;
    s8 *t;
    s32 n;
    s32 r;
    f32 *p;

    t = (s8 *)tb[*(s32 *)(*(s8 **)(arg0 + 0x68) + 0x110)];
    n = *(s32 *)t;
    r = n - 1;
    p = (f32 *)(q + (s32)t);
loop:
    if (i >= n) {
        return r;
    }
    if (*p == z) {
        return i;
    }
    p += 4;
    i += 1;
    goto loop;
}
