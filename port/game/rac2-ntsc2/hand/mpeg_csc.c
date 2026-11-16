/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's libmpeg DMA interrupt handlers, hand-written assembly in the decompilation (ASM_FUNC in src/core/0012E110.c), in
 * C, written from what the routine's instructions do and in their order; handdiff.py checks it
 * against hostgen's translation of the retail routine. Its own file because hostgen gives a hand
 * file to the decompilation unit that declares its functions (mpeg.c has the rest).
 *
 * In v1.01's names: hostgen renames this file for PAL and USA v2.00 ("primary_hand"). */
#include "common.h"

/* DMA hardware registers. */
#define MPEG_HW(addr) (*(volatile u32*)(addr))

/* _doCSC2's channel 3 (IPU out) interrupt handler: counts the finished chunk, and while the IPU is
 * idle starts the next 0x3FF-macroblock chunk (or the last, shorter one) of the colour conversion.
 * rec: chunks done, a busy flag, macroblocks left, the next destination, the number of chunks;
 * volatile: the thread that started the transfer waits on it (func_0012E378). */
int func_0012E238(int channel, volatile s32* rec) {
    MPEG_HW(0x1000E010) = 8;
    rec[0] = rec[0] + 1;
    if (MPEG_HW(0x1000B020) != 0 || (MPEG_HW(0x1000B000) & 0x100) != 0) {
        rec[1] = 1;
        return 0;
    }
    if (rec[0] < rec[4] - 1) {
        MPEG_HW(0x1000B010) = rec[3];
        MPEG_HW(0x1000B020) = 0xFFC0;
        MPEG_HW(0x1000B000) = 0x100;
        MPEG_HW(0x10002000) = 0x700003FF;
        rec[3] = (rec[3] + 0xFFC00) & 0x0FFFFFFF;
    } else if (rec[0] == rec[4] - 1) {
        s32 done = rec[0];
        s32 next = rec[3];
        rec[2] = rec[2] - (done * 0x3FF);
        MPEG_HW(0x1000B010) = next;
        MPEG_HW(0x1000B020) = rec[2] << 6;
        MPEG_HW(0x1000B000) = 0x100;
        MPEG_HW(0x10002000) = rec[2] | 0x70000000;
    }
    return 0;
}

/* Channel 4 (IPU in) interrupt handler: feeds the bitstream to the IPU 0xFFFF quadwords at a time.
 * rec: quadwords left, the next source; volatile, a record the interrupted thread reads. */
void func_0012E538(int channel, volatile s32* rec) {
    MPEG_HW(0x1000E010) = 0x10;
    if (rec[0] == 0) {
        return;
    }
    if ((u32)0xFFFF < (u32)rec[0]) {
        MPEG_HW(0x1000B410) = rec[1];
        MPEG_HW(0x1000B420) = 0xFFFF;
        MPEG_HW(0x1000B400) = 0x101;
        {
            u32 next = (rec[1] + 0xFFFF0) & 0x0FFFFFFF;
            rec[0] = rec[0] - 0xFFFF;
            rec[1] = next;
        }
    } else {
        MPEG_HW(0x1000B410) = rec[1];
        MPEG_HW(0x1000B420) = rec[0];
        MPEG_HW(0x1000B400) = 0x101;
        rec[0] = 0;
    }
}
