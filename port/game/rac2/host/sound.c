/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's 989snd command boundary, without the IOP.
 *
 * The game's sound client sends each request to the IOP's sound driver by
 * RPC: snd_SendCommandSync (0x00132C48: command, size, data) copies the
 * data to its send buffer and waits for the reply, and snd_QueueCommandToRing
 * (0x00132DF0: command, size, data and an optional callback) queues it for
 * the next flush. There is no IOP in the port: both go to the ring every game of this engine shares
 * (lib/snd_ring.c), answered by the native sound engine as an IOP that answers within a tick would.
 */
#include <stdint.h>

#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "rac2.h"

/* snd_Pump (0x00132028): the answers of the batch out, the disc read's end (the common library's
 * flush), then the next batch (the shared ring, lib/snd_ring.c). Returns whether a batch is still
 * out, as retail's (an RPC or a disc read in flight). */
int func_00132028(void) {
    return openrac_snd_ring_pump();
}

/* snd_SendIOPCommandNoWait (0x00132DF0): one command queued on the ring and committed. */
int func_00132DF0(int command_arg, int bytes, int data_arg, int callback, long long callback_data) {
    return openrac_snd_ring_queue(
        (unsigned int)command_arg,
        (gaddr)data_arg,
        bytes,
        (gaddr)callback,
        (long long)(unsigned int)callback_data
    );
}

/* snd_SendCommandSync (0x00132C48): waits, pumping, until no batch is out (the callbacks of the
 * one out run), has the IOP answer this command, then sends the batch queued meanwhile. */
int func_00132C48(int command_arg, int bytes, int data_arg) {
    return openrac_snd_ring_send_sync((unsigned int)command_arg, (gaddr)data_arg, bytes);
}

/* snd_BankLoadByLoc with a callback (0x00132498): the bank at a sector and offset of the disc, loaded
 * at once; the callback is told its handle, as the IOP's answer would. The level loader waits for it
 * (func_00293A60, stages 3 and 5). */
void func_00132498(int sector, int offset, int callback, long long argument) {
    const unsigned int handle = openrac_lib_snd_BankLoadByLoc(sector, offset);
    if (callback != 0) {
        GFN(void (*)(int, long long), (gaddr)callback)((int)handle, argument);
    }
}

/* snd_BankLoadFromIOP (0x001326D8, the symbol table's name): a bank the game sent to the IOP's memory
 * with sceSifSetDma, loaded at once, its handle the result. A hand item's bank (func_00294988's three
 * 0xC800 slots, the Lancer's firing sounds among them) arrives so; the address is the IOP's, which the
 * EE's memory would have read as zeros. */
int func_001326D8(int bank) {
    return openrac_snd_ring_bank_from_iop(bank);
}

/* snd_PlayVAGStreamTwoParts_CB (0x00133350, command 0x2C): (the right channel's location, the left's,
 * 0, 0, volume, pan, group, queue on, 0, flags, callback, argument). Music is two mono files on the disc;
 * speech has no left. Flags as rac1's: 1 paused until snd_ContinueVAGStream, 4 loop. The callback is told
 * the handle, as the IOP's answer would, and the game's wait for a buffered stream (func_002E6D88) ends
 * through snd_IsVAGStreamBuffered_CB. */
int func_00133350(
    int right,
    int left,
    int a2,
    int a3,
    int volume,
    int pan,
    int group,
    int queue_on,
    int a8,
    int flags,
    int callback,
    long long argument
) {
    (void)a2;
    (void)a3;
    /* The command 0x2C the C wrappers queue (func_00133340's): right, left, volume << 16, pan << 16,
     * group, queue on, 0, flags. */
    const uint32_t words[8] =
        {(uint32_t)right,
         (uint32_t)left,
         (uint32_t)volume << 16,
         (uint32_t)pan << 16,
         (uint32_t)group,
         (uint32_t)queue_on,
         (uint32_t)a8,
         (uint32_t)flags};
    openrac_snd_ring_queue_bytes(
        0x2C, (const uint8_t*)words, (int)sizeof(words), (gaddr)callback, argument
    );
    return 0;
}
