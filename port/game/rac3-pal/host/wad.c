/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's WAD decompressor (hand-written assembly, as Going Commando's 0x0029D4D0:
 * rac2/host/wad.c): the boot program's 0x003A0B60, frontbin's 0x003A0760 and each level's copy
 * (rac3-decomp's catalogue names those after frontbin's, func_003A0760). It unpacks the stream at
 * its first argument to its second, through the scratchpad by DMA. Translated from its
 * instructions it wrote another picture of the stream (the level loading screen's pack at
 * 0x006F3000 began with zeros where the stream's own table is), so the port's routine every game
 * shares does it in memory. */
#include "game/common/lib/wad_routine.h"

int func_003A0B60(int source, int destination) {
    return openrac_wad_routine(source, destination, "Up Your Arsenal");
}

int func_003A0760__frontbin(int source, int destination) {
    return openrac_wad_routine(source, destination, "Up Your Arsenal");
}

int func_003A0760(int source, int destination) {
    return openrac_wad_routine(source, destination, "Up Your Arsenal");
}
