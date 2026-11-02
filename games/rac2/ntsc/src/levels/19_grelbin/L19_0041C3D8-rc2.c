
/* SPDX-License-Identifier: MIT
 * MIT License
 * 
 * Copyright (c) 2026 Forja contributors
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

extern s32 func_001234F0(f32);
extern s32 func_00131430(s64);
extern s32 func_00122B00(s32, s64);
extern s32 func_00122DA8(s32, s32);
extern f32 func_00123298(s32);
extern void ref_L19_0041C3D8_00(void *, s32, s32);

void func_L19_0041C3D8(void *arg0) {
    void *s2;
    s32 t;

    s2 = (*(void **)((char *)(arg0) + (0x68)));
    if ((*(u8 *)((char *)(arg0) + (0x20))) == 0) {
        t = func_001234F0((*(f32 *)((char *)(arg0) + (0xC))));
        (*(f32 *)((char *)(s2) + (0x10))) = func_00123298(func_00122DA8(t, func_00122B00(func_00131430((s64)0x8000 << 47), (s64)0x8120 << 47)));
        ref_L19_0041C3D8_00(arg0, 1, -1);
    }
}
