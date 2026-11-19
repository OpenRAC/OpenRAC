/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's libmpeg scratchpad transfer, hand-written assembly in the decompilation (ASM_FUNC in src/core/0012DA98.c), in
 * C, written from what the routine's instructions do and in their order; handdiff.py checks it
 * against hostgen's translation of the retail routine. Its own file because hostgen gives a hand
 * file to the decompilation unit that declares its functions (mpeg.c has the rest).
 *
 * In v1.01's names: hostgen renames this file for PAL and USA v2.00 ("primary_hand"). */
#include "common.h"

/* DMA hardware registers. */
#define MPEG_HW(addr) (*(volatile u32*)(addr))

extern int func_0011F5E0(void);  /* DI: interrupts off, the previous state */
extern void func_0011F628(void); /* EI */
extern u8 D_001469C0[];          /* the scratchpad transfer's chain of DMA tags */

/* Sends a picture's slices to the scratchpad: one pair of reference tags per slice in the chain at
 * D_001469C0 (written through the uncached mirror), the last pair ending the chain, then the
 * toSPR channel (9) started on it with interrupts off. */
void func_0012DF18(u8* p) {
    s32 idx = *(s32*)(p + 0x810);
    s32 count = *(s32*)(p + idx * 0x140 + 0x6BC);
    u32 tag = ((u32)D_001469C0 & 0x0FFFFFFF) | 0x20000000;
    s32 i = 0;
    int intr;

    if (count > 0) {
        do {
            s32 off = i * 4 + idx * 0x140;
            u64 last = (i ^ (count - 1)) == 0 ? 0 : 3;
            u64 a, b;
            i++;
            a = *(u32*)(p + 0x5A8 + off) & 0x0FFFFFFF;
            b = *(u32*)(p + 0x598 + off) & 0x0FFFFFFF;
            *(u64*)tag = (b << 32) | 0x30000030;
            *(u64*)(tag + 16) = (a << 32) | (last << 28) | 0x30;
            tag += 32;
        } while (i < count);
    }
    intr = func_0011F5E0();
    idx = *(s32*)(p + 0x810);
    MPEG_HW(0x1000D480) = *(u32*)(p + idx * 0x140 + 0x590);
    MPEG_HW(0x1000D430) = (u32)D_001469C0;
    MPEG_HW(0x1000D420) = 0;
    MPEG_HW(0x1000D400) = 0x105;
    if (intr != 0) {
        func_0011F628();
    }
}
