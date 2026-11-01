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

extern void *ref_L22_003A3368_00[];
f32 ref_L22_003A3368_01(void *, void *);

void func_L22_003A3368(void *arg0) {
    s32 i;
    s32 j;
    s32 k;
    s32 *base ;
    s32 *indices;
    void *item0;
    void *item1;
    f32 *point0;
    f32 *point1;
    f32 distances[4];
    f32 last_distance;

    base = *(s32 **)((s8 *)arg0 + 0x68);
    indices = base + 0xA8;
    i = 0;
    do {
        j = 0;
        item0 = ref_L22_003A3368_00[(u32)*(s32 *)((s32)indices + (i << 2))];
        *(s32 *)((( *(s32 *)item0) << 4) + (s32)item0 + 0xC) = 0;
        *(s32 *)((s8 *)item0 + 0x1C) = 0;
        do {
            item1 = ref_L22_003A3368_00[(u32)*(s32 *)((s32)indices + (j << 2))];
            if (item0 != item1) {
                point0 = (f32 *)((s8 *)item0 + 0x10);
                point1 = (f32 *)((s8 *)item1 + 0x10);
                distances[0] = ref_L22_003A3368_01(point0, point1);
                distances[1] = ref_L22_003A3368_01(point0, (f32 *)((s8 *)item1 + ((*(s32 *)item1) << 4)));
                distances[2] = ref_L22_003A3368_01((f32 *)((s8 *)item0 + ((*(s32 *)item0) << 4)), point1);
                last_distance = ref_L22_003A3368_01((f32 *)((s8 *)item0 + ((*(s32 *)item0) << 4)), (f32 *)((s8 *)item1 + ((*(s32 *)item1) << 4)));
                {
                    f32 limit_first ;
                    s32 mask_first ;
                    s32 count0 ;
                    f32 *scan0;
                    limit_first = 1.0f;
                    mask_first = (s32)(1u << j);
                    distances[3] = last_distance;
                    scan0 = &distances[0];
                    count0 = 1;
                    do {
                        count0--;
                        if (*scan0 < limit_first) {
                            *(s32 *)((s8 *)item0 + 0x1C) |= mask_first;
                        }
                        scan0++;
                    } while (count0 >= 0);
                }
                k = 2;
                do {
                    if (distances[k] < 1.0f) {
                        *(s32 *)((( *(s32 *)item0) << 4) + (s32)item0 + 0xC) |= (s32)(1u << j);
                    }
                    k++;
                } while (k < 4);
            }
            j++;
        } while (j < 32);
        i++;
    } while (i < 32);
}
