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

extern const s32 D_001A7A00;
extern s8 D_00189EA0[];
extern s32 ref_L16_003D6D00_00;
extern struct { s32 w0; s32 w1; s32 w2; } ref_L16_003D6D00_01;
extern s32 ref_L16_003D6D00_02(s8 *, s32);
extern void ref_L16_003D6D00_03(s8 *, s32, s8 *);
extern void ref_L16_003D6D00_04(s8 *, s8 *, f32);
extern void ref_L16_003D6D00_05(s8 *, s8 *, s8 *);
extern s32 ref_L16_003D6D00_06(s8 *, s8 *, s32, s32, s32);
extern s32 ref_L16_003D6D00_07();
extern void ref_L16_003D6D00_08(s8 *, s32, s32);
extern void ref_L16_003D6D00_09(s32, s32);

void func_L16_003D6D00(s8 *arg0) {
    s32 *temp_s2;
    s8 *s1;
    s8 *s0;
    s8 *s2;
    s8 buf[0x10];

    temp_s2 = (*(s32 **)((s8 *)(arg0) + (0x68)));
    if ((*(u8 *)((s8 *)(arg0) + (0x20))) == 0) {
        (*(u8 *)((s8 *)(arg0) + (0x30))) = 0xFF;
        (*(u8 *)((s8 *)(arg0) + (0x20))) = 1;
    }
    s1 = D_00189EA0;
    if (ref_L16_003D6D00_02(s1, *temp_s2) != 0) {
        s0 = s1 - 0x80;
        if ((*(s32 *)((s8 *)(s0) + (0x229C))) != 0xF) {
            s2 = s1 + 0x270;
            ref_L16_003D6D00_03(s2, ref_L16_003D6D00_00 + (*temp_s2 << 7) + 0x30, s1);
            ref_L16_003D6D00_04(s2, s2, 1.0f);
            (*(f32 *)((s8 *)(s0) + (0x2FC))) = 2.0f;
            (*(s16 *)((s8 *)(s0) + (0x218))) = 5;
            (*(s32 *)((s8 *)(s0) + (0x2F8))) = 0;
            if ((*(s32 *)((s8 *)(s0) + (0x340))) != 0 && (*(s32 *)((s8 *)(s0) + (0x2294))) != 0x38) {
                ref_L16_003D6D00_05(buf, s1, s2);
                if (ref_L16_003D6D00_06(s1 + 0x60, buf, 0, (*(s32 *)((s8 *)(s0) + (0x2290))), 0) != 0 && ref_L16_003D6D00_07() != 2 && ref_L16_003D6D00_01.w0 == 0) {
                    ref_L16_003D6D00_08(s1 + 0x244C, 0, (s32)((f32)D_001A7A00 * 0.333f));
                    if ((*(s32 *)((s8 *)(s0) + (0x24CC))) != 0) {
                        ref_L16_003D6D00_09(0x38, 1);
                    }
                }
            }
        }
    }
}
