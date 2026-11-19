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
