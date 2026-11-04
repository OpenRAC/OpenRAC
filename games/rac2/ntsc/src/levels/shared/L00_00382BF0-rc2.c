
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

extern volatile s32 D_001A7344;
extern void ref_L00_00382BF0_00(void *, s32, s8 *);
extern f32 ref_L00_00382BF0_01(void *, s8 *);
extern void ref_L00_00382BF0_02(s32, s32 *, s32 *);
extern s32 ref_L00_00382BF0_03(s32, u16, u16, s32, s32, s32, f32 *);
extern s32 D_001A7340 __attribute__((sda));
extern f32 ref_L00_00382BF0_04 __attribute__((sda));
extern s8 ref_L00_00382BF0_05[];

f32 func_L00_00382BF0(void *arg0) {
    s8 buf[0x10];
    s32 sp10;
    s32 sp14;
    f32 temp_f20;
    s32 temp_s1;

    temp_s1 = *(s32 *)((s8 *)arg0 + 0x68);
    ref_L00_00382BF0_00(buf, temp_s1, ref_L00_00382BF0_05);
    temp_f20 = ref_L00_00382BF0_01(buf, ref_L00_00382BF0_05 + 0x230);
    ref_L00_00382BF0_02(temp_s1, &sp10, &sp14);
    if ((sp10 < 0) || (sp10 >= (D_001A7340 - 0x10)) || (sp14 < 0) || (sp14 >= (D_001A7344 - 8)) || !(temp_f20 > 0.0f) || (ref_L00_00382BF0_03(-1, (u16) sp10, (u16) sp14, 0x10, 8, 0, &ref_L00_00382BF0_04) == -1)) {
        ref_L00_00382BF0_04 = -1.0f;
    }
    return ref_L00_00382BF0_04;
}
