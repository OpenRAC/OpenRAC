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

extern u16 ref_L03_00408068_00 __attribute__((sda));
extern u16 ref_L03_00408068_01 __attribute__((sda));
extern s32 ref_L03_00408068_02 __attribute__((sda));
extern f32 ref_L03_00408068_03 __attribute__((sda));
extern f32 ref_L03_00408068_04 __attribute__((sda));
extern f32 ref_L03_00408068_05 __attribute__((sda));
extern f32 ref_L03_00408068_06 __attribute__((sda));
extern s8 ref_L03_00408068_07;

extern void ref_L03_00408068_08(void *, s32, s32);
extern s32 ref_L03_00408068_09(void *, s32, s32);
extern void ref_L03_00408068_10(void *, s32, s8 *, s32, f32, f32, f32, f32);
extern s32 ref_L03_00408068_11(void *);
extern void ref_L03_00408068_12(void *, s32);
extern void ref_L03_00408068_13(s32, s32, void *);

void func_L03_00408068(void *arg0) {
    u8 temp_v1;

    temp_v1 = *(u8 *)((u8 *)arg0 + 0x20);
    switch (temp_v1) {
    case 0:
        *(u16 *)((u8 *)arg0 + 0x7C) = ref_L03_00408068_00;
        ref_L03_00408068_08(arg0, 1, -1);
        return;
    case 1:
        if (ref_L03_00408068_09(arg0, 0x10000, 0) != 0) {
            ref_L03_00408068_13(0, 0, arg0);
            ref_L03_00408068_10(arg0, ref_L03_00408068_02, &ref_L03_00408068_07, 0xFF, ref_L03_00408068_03, ref_L03_00408068_04, ref_L03_00408068_05, ref_L03_00408068_06);
            ref_L03_00408068_08(arg0, 2, -1);
        }
        return;
    case 2:
        if (ref_L03_00408068_11(arg0) != 0) {
            *(u16 *)((u8 *)arg0 + 0x7C) = ref_L03_00408068_01;
            ref_L03_00408068_12(arg0, 1);
            *(s32 *)((u8 *)arg0 + 0x9C) = 0xF;
        }
        break;
    }
}
