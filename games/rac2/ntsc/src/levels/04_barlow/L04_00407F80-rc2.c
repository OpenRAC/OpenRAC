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

extern s8 ref_L04_00407F80_00[];
extern s8 ref_L04_00407F80_01[];
extern void *ref_L04_00407F80_02 __attribute__((sda));
extern void *ref_L04_00407F80_03 __attribute__((sda));
extern void *ref_L04_00407F80_04 __attribute__((sda));
extern void ref_L04_00407F80_05(void *, void *, void *, void *);
extern s32 ref_L04_00407F80_06(s8 *, s32);
extern void ref_L04_00407F80_07(s8 *, void *);

void func_L04_00407F80(void *arg0) {
    void *p68;
    s32 temp_a1;

    p68 = (*(void **)((s8 *)(arg0) + (0x68)));
    if ((*(u8 *)((s8 *)(arg0) + (0x20))) == 0) {
        ref_L04_00407F80_05(arg0, ref_L04_00407F80_02, ref_L04_00407F80_03, ref_L04_00407F80_04);
        (*(u8 *)((s8 *)(arg0) + (0x30))) = 0xC4;
        (*(u8 *)((s8 *)(arg0) + (0x20))) = 1;
        (*(u16 *)((s8 *)(arg0) + (0x34))) = (u16) ((*(u16 *)((s8 *)(arg0) + (0x34))) | 1);
        (*(s16 *)((s8 *)(arg0) + (0x32))) = 0;
        return;
    }
    temp_a1 = (*(s32 *)((s8 *)(p68) + (8)));
    if ((temp_a1 == -1) || (ref_L04_00407F80_06(ref_L04_00407F80_00, temp_a1) != 0)) {
        ref_L04_00407F80_05(arg0, ref_L04_00407F80_02, ref_L04_00407F80_03, ref_L04_00407F80_04);
        ref_L04_00407F80_07(ref_L04_00407F80_01, arg0);
    }
}
