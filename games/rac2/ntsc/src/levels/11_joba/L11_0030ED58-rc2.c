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
extern s8 ref_L11_0030ED58_00[];
extern s32 ref_L11_0030ED58_01 __attribute__((sda));
extern void ref_L11_0030ED58_02(s32 *, s32, s32, s32, s32);

void func_L11_0030ED58(void) {
    s8 *temp_v0;
    s32 a1;
    s32 a0r ;
    s32 k ;

    if (D_001A79F0 == 11) {
        temp_v0 = ref_L11_0030ED58_00;
        if ((*(s32 *)((s8 *)(temp_v0) + (0x30))) == 2) {
            a0r = (*(s32 *)((s8 *)(temp_v0) + (0x34)));
            a1 = -1;
            if ((u32)(a0r - 0x1B9) < 0xB3U) {
                a1 = 0x46;
            } else if ((u32)(a0r - 0x321) < 0x95U) {
                a1 = 0x47;
            } else {
                k = 0x44B;
                if (!(k < a0r)) {
                    a1 = 0x48;
                }
            }
            if (a1 != -1) {
                ref_L11_0030ED58_02(&ref_L11_0030ED58_01, a1, 2, 0x80808080, 0);
            }
        }
    }
}
