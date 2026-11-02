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

extern s32 ref_L22_00415A48_00(s32 *a, void *b);
extern s32 ref_L22_00415A48_01(s32 a, s32 b);
extern struct { s32 w0; s32 w1; s32 w2; } D_0018B6A8;

void func_L22_00415A48(void *arg0) {
    s32 *temp_s1;
    void *temp_s0;

    temp_s1 = (*(s32 **)((s8 *)(arg0) + (0x68)));
    if ((*(u8 *)((s8 *)(arg0) + (0x20))) == 0) {
        (*(u8 *)((s8 *)(arg0) + (0x20))) = 1U;
        (*(u8 *)((s8 *)(arg0) + (0x30))) = 0xFF;
        *temp_s1 = 0x708;
    }
    if ((*(u8 *)((s8 *)(arg0) + (0x20))) == 1) {
        temp_s0 = (void *) D_0018B6A8.w0;
        if ((ref_L22_00415A48_00(temp_s1, arg0) != 0) && ((*(s32 *)((s8 *)(temp_s0) + (8))) == 0)) {
            if ((*(s32 *)((s8 *)(temp_s0) + (0xC))) == 0) {
                ref_L22_00415A48_01(0x9FD, 0x54);
                *temp_s1 = 0x708;
            }
        }
    }
}
