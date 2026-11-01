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

extern s32 ref_L09_003F7C50_00(s8 *, s32, s32, s32);
extern s32 ref_L09_003F7C50_01(s8 *, s32, f32);

s32 func_L09_003F7C50(s8 *arg0, s32 arg1, s32 arg2) {
    s8 *temp_s2;
    s32 t;

    temp_s2 = (*(s8 **)((s8 *)(arg0) + (0x68)));
    (*(s32 *)((s8 *)(temp_s2) + (0x2DC))) = arg1;
    if (arg1 == -1) {
        (*(s16 *)((s8 *)(temp_s2) + (0x1C8))) = (s16) arg1;
        return -1;
    }
    t = ref_L09_003F7C50_01(arg0 + 0x10, arg1, 0.0f);
    if (ref_L09_003F7C50_00(arg0, arg1, arg2, t) != 0) {
        return (*(s16 *)((s8 *)(temp_s2) + (0x1C8)));
    }
    (*(s16 *)((s8 *)(temp_s2) + (0x1C8))) = 0;
    return -1;
}
