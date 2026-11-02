
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

void func_L14_003DE310(s8 *arg0) {
    s32 var_a1;
    s8 **var_a0;
    s8 *temp_v1;
    s8 *t2;

    var_a1 = 3;
    var_a0 = (s8 **)((*(s8 **)((char *)(arg0) + (0x68))) + 0xE0);
    do {
        temp_v1 = *var_a0;
        var_a1 -= 1;
        if (temp_v1 != 0) {
            t2 = (*(s8 **)((char *)(temp_v1) + (0x68)));
            if ((*(s8 *)((char *)(temp_v1) + (0x20))) < 0) {
                goto store;
            }
            if ((*(f32 *)((char *)(t2) + (0x40))) <= 0.0f) {
            store:
                *var_a0 = 0;
            }
        }
        var_a0 = var_a0 + 1;
    } while (var_a1 >= 0);
}
