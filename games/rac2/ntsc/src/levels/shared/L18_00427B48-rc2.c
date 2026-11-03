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

extern s32 ref_L18_00427B48_00;
extern u16 *ref_L18_00427B48_01[];

void func_L18_00427B48(s32 unused, s32 arg1) {
    u16 *a;
    u16 *v;
    s8 *tmp;
    s32 t;

    if (arg1 != 0xFF) {
        v = ref_L18_00427B48_01[arg1];
        if (v != 0) {
            a = v;
            do {
                tmp = (s8 *)ref_L18_00427B48_00 + ((*a & 0x7FFF) << 8);
                if (tmp != 0) {
                    if (*(s16 *)(tmp + 0xAA) == 0xEC0) {
                        s32 b = *(u8 *)(tmp + 0x20);
                        if (b != 0xFE) {
                            if (b != 0xFD) {
                                *(s8 *)(*(s8 **)(tmp + 0x68) + 0x39A) = 1;
                            }
                        }
                    }
                }
                t = *(s16 *)a;
                a += 1;
            } while (t >= 0);
        }
    }
}
