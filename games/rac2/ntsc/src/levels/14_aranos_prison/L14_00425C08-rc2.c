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

extern s8 *ref_L14_00425C08_00(s32);
extern void ref_L14_00425C08_01(s8 *, s8 *, s32, s32);

s8 *func_L14_00425C08(s8 *arg0) {
    s8 *temp_s0;
    s8 *temp_v1;
    s8 *temp_v0;
    u8 temp_a0;

    temp_s0 = (*(s8 **)((s8 *)(arg0) + (0x68)));
    temp_v1 = (*(s8 **)((s8 *)(temp_s0) + (0x120)));
    if (temp_v1 == 0) {
        goto create;
    }
    temp_a0 = (*(u8 *)((s8 *)(temp_v1) + (0x20)));
    if (temp_a0 == 0xFE) {
        goto create;
    }
    if (temp_a0 != 0xFD) {
        return temp_v1;
    }
create:
    temp_v0 = ref_L14_00425C08_00(0x1142);
    (*(s8 **)((s8 *)(temp_s0) + (0x120))) = temp_v0;
    if (temp_v0 != 0) {
        (*(u16 *)((s8 *)(temp_v0) + (0x32))) = (*(u16 *)((s8 *)(arg0) + (0x32)));
        (*(u8 *)((s8 *)((*(s8 **)((s8 *)(temp_s0) + (0x120)))) + (0x30))) = (*(u8 *)((s8 *)(arg0) + (0x30)));
        (*(s64 *)((s8 *)((*(s8 **)((s8 *)(temp_s0) + (0x120)))) + (0x38))) = (*(s64 *)((s8 *)(arg0) + (0x38)));
        (*(s8 *)((s8 *)((*(s8 **)((s8 *)(temp_s0) + (0x120)))) + (0x31))) = 1;
        ref_L14_00425C08_01(arg0, (*(s8 **)((s8 *)(temp_s0) + (0x120))), 2, 0);
    }
    return (*(s8 **)((s8 *)(temp_s0) + (0x120)));
}
