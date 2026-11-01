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

extern s32 ref_L22_003A4490_00(void *, void *, void *);
extern s32 ref_L22_003A4490_01(void *, s32 *, void *);
extern s32 ref_L22_003A4490_02(void *, s32, void *);
extern s32 ref_L22_003A4490_03(void *, void *, s32, s32);
extern s32 ref_L22_003A4490_04 __attribute__((sda));


void func_L22_003A4490(void *arg0) {
    void *obj_a1 ;
    u8 stack_buffer[0x10];
    u8 temp_v1;
    u8 temp_v1_2;
    u8 temp_v1_3;
    void *temp_a0;
    void *temp_s0;

    temp_s0 = (*(void **)((s8 *)(arg0) + (0x68)));
    obj_a1 = (*(void **)((s8 *)(temp_s0) + (0x440)));
    if ((obj_a1 != NULL) && ((*(s16 *)((s8 *)(obj_a1) + (0xAA))) == 0xE0A)) {
        temp_v1 = (*(u8 *)((s8 *)(obj_a1) + (0x20)));
        if (temp_v1 != 0xFE) {
            if (temp_v1 != 0xFD) {
                if (((u32)((*(u8 *)((s8 *)(obj_a1) + (0x43))) - 1) < 3U) ||
                    ((*(s16 *)((s8 *)(temp_s0) + (0x460))) != 0)) {
                    ref_L22_003A4490_02(arg0, 1, obj_a1 + 0x10);
                } else {
                    ref_L22_003A4490_03(arg0, obj_a1, 1, 0);
                }
                (*(f32 *)((s8 *)((*(void **)((s8 *)(temp_s0) + (0x440)))) + (0x48))) =
                    (f32)(*(f32 *)((s8 *)(arg0) + (0x48)));
            }
        }
    }

    obj_a1 = (*(void **)((s8 *)(temp_s0) + (0x444)));
    if ((obj_a1 != NULL) && ((*(s16 *)((s8 *)(obj_a1) + (0xAA))) == 0xE0C)) {
        temp_v1_2 = (*(u8 *)((s8 *)(obj_a1) + (0x20)));
        if (temp_v1_2 != 0xFE) {
            if (temp_v1_2 != 0xFD) {
                ref_L22_003A4490_03((*(void **)((s8 *)(temp_s0) + (0x440))), obj_a1, 5, 0);
            }
        }
    }

    obj_a1 = (*(void **)((s8 *)(temp_s0) + (0x448)));
    if ((obj_a1 != NULL) && ((*(s16 *)((s8 *)(obj_a1) + (0xAA))) == 0xF8F)) {
        temp_v1_3 = (*(u8 *)((s8 *)(obj_a1) + (0x20)));
        if (temp_v1_3 != 0xFE) {
            if (temp_v1_3 != 0xFD) {
                if (((*(u16 *)((s8 *)(arg0) + (0x34))) ^ 1) & 1) {
                    ref_L22_003A4490_03((*(void **)((s8 *)(temp_s0) + (0x440))), obj_a1, 5, 0);
                    ref_L22_003A4490_01(stack_buffer, &ref_L22_003A4490_04,
                                  (*(void **)((s8 *)(temp_s0) + (0x448))) + 0xC0);
                    temp_a0 = (*(void **)((s8 *)(temp_s0) + (0x448))) + 0x10;
                    ref_L22_003A4490_00(temp_a0, temp_a0, stack_buffer);
                }
            }
        }
    }
}
