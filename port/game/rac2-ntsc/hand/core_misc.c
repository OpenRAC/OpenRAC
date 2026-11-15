/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Library routines of the boot program that the decompilation keeps as assembly (ASM_FUNC), in C:
 * written from what each routine's instructions do; handdiff.py checks each against hostgen's
 * translation of the retail routine. In v1.01's names ("primary_hand"). */
#include "common.h"

extern int func_0011F5E0(void);  /* DIntr: the interrupt enable before, nonzero if it was on */
extern int func_0011F628(void);  /* EIntr */
extern void func_0011B010(void); /* sceSifSetDChain */
extern int func_0011AC50(int);
extern u32 D_0013CF58[]; /* sceSif's command data: {the packet buffer, ..., the system table, its size, the user table, its size} */

/* Waits for the next vertical blank: acknowledges INTC_STAT's VBLANK_START bit (0x1000F000, bit 2;
 * writing 1 clears it) with interrupts off, waits until the bit is set again, and acknowledges it
 * once more, interrupts back on after each write if they were on. The routine's two `sync` have no
 * host counterpart. */
void func_0011B0B0(void)
{
    int enabled = func_0011F5E0();
    *(volatile u32 *)0x1000F000 = 4;
    if (enabled != 0) {
        func_0011F628();
    }
    while ((*(volatile u32 *)0x1000F000 & 4) == 0) {
    }
    enabled = func_0011F5E0();
    *(volatile u32 *)0x1000F000 = 4;
    if (enabled != 0) {
        func_0011F628();
    }
}

/* A wrapper that passes its third argument on (0x00124540). */
int func_00124540(int unused0, int unused1, int arg)
{
    return func_0011AC50(arg);
}

/* The SIF command handler (sceSifCmdIntrHdlr, DMA channel 5's handler, installed by func_0011C8D8):
 * takes the packet the IOP sent from the buffer whose address is the first word of D_0013CF58, its
 * size the packet header's first byte (0: nothing came), clears that byte, copies the packet whole
 * quadwords at a time onto the stack (the IOP sends at most 112 bytes), and re-arms the channel (sceSifSetDChain).
 * Then calls the handler the packet's command id names: a system command (bit 31 set) from the
 * table at +12 (+16 its size), a user one from +20 (+24 its size), each entry {function, argument},
 * as function(packet, argument). Returns 0. */
int func_0011CDA0(void)
{
    /* The routine's whole 144-byte frame: the copy is bounded only by the header's size byte, so a
     * packet over 112 bytes runs over the saved registers into the caller's frame, as on the EE. */
    u32 packet[36] __attribute__((aligned(16)));
    u32 *data = D_0013CF58;
    u8 *from;
    int size;
    int quads;
    int i;
    int id;
    u32 *entry;

    func_0011F628();
    from = (u8 *)data[0];
    size = from[0];
    if (size == 0) {
        return 0;
    }
    quads = (size + 15) >> 4;
    from[0] = 0;
    /* lq ignores the address's low four bits: whole quadwords from the one the header is in. */
    for (i = 0; i < quads * 4; i++) {
        packet[i] = ((u32 *)((u32)from & ~15))[i];
    }
    func_0011B010();
    id = (int)packet[2];
    if (id < 0) {
        id &= 0x7FFFFFFF;
        if (id >= (int)data[4]) {
            return 0;
        }
        entry = (u32 *)(data[3] + id * 8);
    } else {
        if (id >= (int)data[6]) {
            return 0;
        }
        entry = (u32 *)(data[5] + id * 8);
    }
    if (entry[0] != 0) {
        ((int (*)(void *, u32))entry[0])(packet, entry[1]);
    }
    return 0;
}
