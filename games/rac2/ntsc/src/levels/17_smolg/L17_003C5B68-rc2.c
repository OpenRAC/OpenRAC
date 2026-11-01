
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
#ifndef NULL
#define NULL 0
#endif

extern struct Gl { s32 w0; s32 w1; s32 w2; } D_0018A160;
extern s32 ref_L17_003C5B68_00 __attribute__((sda));

s32 func_L17_003C5B68(void *arg0) {
    void *temp_a0;
    void *temp_v0;
    s32 temp_a1;
    s32 temp_v1;
    s32 temp_a0_2;
    s32 temp_v1_2;
    void *temp_v0_2;

    if (D_0018A160.w0 == 0) {
        return 0;
    }
    temp_a1 = (*(s16 *)((char *)(arg0) + (0xAA)));
    if (temp_a1 == 0x920) {
success:
        return 1;
    }
    if (temp_a1 != 0x929) {
        goto fail;
    }
    temp_a0 = (*(void **)((char *)(arg0) + (0x68)));
    temp_v0 = (*(void **)((char *)(temp_a0) + (0x9C)));
    if (temp_v0 != NULL) {
        temp_v1 = (*(u16 *)((char *)(temp_v0) + (0xAA)));
        if ((u32) (temp_v1 - 0x920) < 2U) {
            goto fail;
        }
        if ((s16) temp_v1 == temp_a1) {
            goto fail;
        }
    }
    if ((*(f32 *)((char *)(temp_a0) + (0x98))) != 0.0f) {
        goto fail;
    }
    temp_a0_2 = (*(s32 *)((char *)(temp_a0) + (0xA0)));
    if (temp_a0_2 == -1) {
        goto success;
    }
    temp_v0_2 = (void *) (ref_L17_003C5B68_00 + (temp_a0_2 << 8));
    if (temp_v0_2 == NULL) {
        goto success;
    }
    temp_v1_2 = (*(u8 *)((char *)(temp_v0_2) + (0x20)));
    if (temp_v1_2 == 0xFE) {
        goto success;
    }
    if (temp_v1_2 == 0xFD) {
        goto success;
    }
fail:
    return 0;
}
