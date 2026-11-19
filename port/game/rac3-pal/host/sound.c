/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's 989snd, EE side (rac3-decomp's core 0x0013B330-0x0013D420, SN-built). Every
 * command goes to the sound server on the IOP through sceSifCallRpc (func_0013C348 and
 * func_0013C578), then the EE waits for the server's reply in a buffer the IOP writes
 * (func_0013B818). The port has no IOP: the shared replacements of rac1 and rac2 (lib/snd.c) are
 * not bound here yet, so for now every command is answered at once with an empty reply, and the
 * game goes on without sound. */
#include <stdint.h>
#include <string.h>

#include "openrac/guest.h"

/* The reply wait (0x0013B818): 1 when the server's reply to the last command is in (the reply
 * buffer named at 0x001D4DA0 is cleared), 0 while it is still to come. With no server the reply is
 * the receive buffer as the call left it. */
int func_0013B818(void) {
    const uint32_t none = 0;
    memcpy(G(0x001D4DA0u), &none, sizeof none);
    return 1;
}

int openrac_lib_snd_FlushSoundCommands(void);

/* snd_FlushSoundCommands (0x0013B620): sends the queued commands and takes in the server's reply to
 * the last one sent without a wait (func_0013B8B0 names its buffer at 0x001D4DA0, which the next
 * command's send waits to see cleared). Here the reply is in at once, and the shared flush ends a
 * stream read the game started (lib/snd.c). */
int func_0013B620(void) {
    func_0013B818();
    return openrac_lib_snd_FlushSoundCommands();
}

unsigned int openrac_lib_snd_BankLoadByLoc(int sector, int offset);

/* snd_BankLoadByLoc with a callback (0x0013BAB8, rac2-ntsc's func_00132498 and rac2/host/sound.c):
 * the bank at a sector and offset of the disc, loaded at once; the callback is told its handle, as
 * the IOP's answer would. The level loader waits for it (func_0039A5C0, stages 3 and 5: the
 * level's banks, func_003A1598 and func_003A1638). */
void func_0013BAB8(int sector, int offset, int callback, long long argument) {
    const unsigned int handle = openrac_lib_snd_BankLoadByLoc(sector, offset);
    if (callback != 0) {
        GFN(void (*)(int, long long), (gaddr)callback)((int)handle, argument);
    }
}
