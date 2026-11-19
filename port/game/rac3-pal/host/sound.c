/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's 989snd, EE side (rac3-decomp's core 0x0013B330-0x0013D420, SN-built): Going
 * Commando's client at new addresses. Every command goes to the sound server on the IOP through
 * sceSifCallRpc, either at once and waited for (snd_SendCommandSync, 0x0013C348) or queued in a
 * ring of batches that snd_Pump (0x0013B620) sends and whose answers it hands to the commands'
 * callbacks (snd_QueueCommandToRing, 0x0013C578). The port has no IOP: both go to the ring rac2
 * shares (lib/snd_ring.c), which the native sound engine answers: banks, sounds, volumes, and the
 * music's VAG streams from the disc. The banks, loaded by their own RPCs, are read from the disc at
 * once and their callbacks told, as the IOP's answer would. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"

/* The reply wait (0x0013B818): 1 when the server's reply to the last command is in (the reply
 * buffer named at 0x001D4DA0 is cleared), 0 while it is still to come. snd_StartSoundSystem
 * (0x0013B3F8, run from its instructions) waits on it; with no server the reply is in at once. */
int func_0013B818(void) {
    const uint32_t none = 0;
    memcpy(G(0x001D4DA0u), &none, sizeof none);
    return 1;
}

/* snd_Pump (0x0013B620; IDENTIFIED.tsv's snd_FlushSoundCommands): called by every waiting loop of
 * the loader and the sound code. Clears the reply wait, then the shared ring's pump: the callbacks
 * of the batch out, the end of a snd_StreamSafeCdRead, the next batch. Whether a batch is still out. */
int func_0013B620(void) {
    func_0013B818();
    return openrac_snd_ring_pump();
}

/* snd_SendCommandSync (0x0013C348): (command, size, data), the server's reply word. Command 0x2A
 * (snd_InitVAGStreamingEx, func_0013CA30) answers 1: streams are ready. */
int func_0013C348(int command, int bytes, int data) {
    return openrac_snd_ring_send_sync((unsigned int)command, (gaddr)data, bytes);
}

/* snd_QueueCommandToRing (0x0013C578): (command, size, data, callback, argument), queued and
 * committed: play (0x11), set a sound's parameters (0x21), volumes, stops, the music's streams
 * (0x2C, func_0013CB20) and their questions. */
int func_0013C578(int command, int bytes, int data, int callback, long long argument) {
    return openrac_snd_ring_queue(
        (unsigned int)command, (gaddr)data, bytes, (gaddr)callback, argument
    );
}

/* snd_BankLoadAsync (0x0013BAB8): (sector, offset, callback, argument), the bank at that place of
 * the disc, loaded at once; the callback is told its handle, as the IOP's answer at the next pump
 * would. The boot loads the front end's banks so (func_003A1598, func_003A1638), the callback
 * (func_003A1580) storing the handle where the menu's sounds find it. */
void func_0013BAB8(int sector, int offset, int callback, long long argument) {
    const unsigned int handle = openrac_lib_snd_BankLoadByLoc(sector, offset);
    if (openrac_env_string("OPENRAC_SOUND_LOG") != NULL) {
        fprintf(stderr, "[debug] sound bank at sector %#x + %#x: %#x\n", sector, offset, handle);
    }
    if (callback != 0) {
        GFN(void (*)(int, long long), (gaddr)callback)((int)handle, argument);
    }
}

/* snd_BankLoadFromIOP (0x0013BD58, RPC 0x59): a bank the game sent to the IOP's memory, its handle. */
int func_0013BD58(int bank) {
    return openrac_snd_ring_bank_from_iop(bank);
}
}
