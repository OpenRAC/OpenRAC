
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

extern s32 ref_L07_003F90E8_00 __attribute__((sda));
extern f32 ref_L07_003F90E8_01(s32);

f32 func_L07_003F90E8(s32 arg0, s32 arg1, f32 fparg0, f32 fparg1) {
    f32 temp_f20;
    f32 temp_f21;
    f32 temp_f22;

    if (arg0 >= ref_L07_003F90E8_00) {
        return fparg0;
    }
    if (ref_L07_003F90E8_00 >= arg1) {
        return fparg1;
    }
    temp_f21 = ref_L07_003F90E8_01(ref_L07_003F90E8_00);
    temp_f22 = ref_L07_003F90E8_01(arg0);
    temp_f20 = ref_L07_003F90E8_01(arg1);
    {
        f32 den = temp_f20 - ref_L07_003F90E8_01(arg0);
        f32 num = temp_f21 - temp_f22;
        f32 dif = fparg1 - fparg0;
        num = num / den;
        return fparg0 + (dif * num);
    }
}
