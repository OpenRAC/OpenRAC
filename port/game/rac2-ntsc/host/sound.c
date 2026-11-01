/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's 989snd command boundary, without the IOP.
 *
 * The game's sound client sends each request to the IOP's sound driver by
 * RPC: snd_SendCommandSync (0x00132C48: command, size, data) copies the
 * data to its send buffer and waits for the reply, and snd_QueueCommandToRing
 * (0x00132DF0: command, size, data and an optional callback) queues it for
 * the next flush. There is no IOP in the port: commands the native sound
 * engine knows run at once, as Deadlocked's do (rac4-ntsc/host/sound.c), and
 * the others are answered 0 at once instead of waiting for a reply that never
 * comes.
 */
#include <stdint.h>
#include <stdio.h>

#include "openrac/guest.h"
#include "openrac/game_lib.h"
#include "openrac/sound_engine.h"

int func_00132C48(int command_arg, int bytes, int data_arg) {
    const unsigned int command = (unsigned int)command_arg;
    const gaddr data = (gaddr)data_arg;
    if (bytes < 0 || bytes > 0xFFC || (bytes != 0 && data == 0)) {
        return 0;
    }
    if (command == 0) { /* the sound system's start; the host's audio owns the device */
        return openrac_lib_snd_StartSoundSystem();
    }
    if (command == OPENRAC_SND_PLAY_VOL_PAN_PMPB || command == OPENRAC_SND_SET_SOUND_PARAMS) {
        return (int)openrac_sound_command(command, G(data), (uint32_t)bytes);
    }
    static unsigned int last_reported = 0xFFFFFFFFu;
    if (last_reported != command) {
        fprintf(stderr, "[warning] Going Commando synchronous sound command %#x is not supported; answered 0\n",
                command);
        last_reported = command;
    }
    return 0;
}

int func_00132DF0(int command_arg, int bytes, int data_arg, long long callback, int callback_data) {
    const unsigned int command = (unsigned int)command_arg;
    const gaddr data = (gaddr)data_arg;
    if (bytes < 0 || bytes > 0xFFC || (bytes != 0 && data == 0)) {
        return 0;
    }
    if (command == OPENRAC_SND_PLAY_VOL_PAN_PMPB || command == OPENRAC_SND_SET_SOUND_PARAMS) {
        const int handle = (int)openrac_sound_command(command, G(data), (uint32_t)bytes);
        /* The IOP's answer: a voice slot stays pending (func_002E5A50 finds none free, and 42 slots
         * in, the game writes past the table) until its callback is told the handle. Setting a
         * sound's parameters answers with the handle it was given. */
        if (callback != 0) {
            const int answer =
                command == OPENRAC_SND_PLAY_VOL_PAN_PMPB ? handle : (int)GREF(uint32_t, data);
            GFN(void (*)(int, long long),
                (gaddr)callback)(answer, (long long)(unsigned int)callback_data);
        }
        return 1;
    }
    static unsigned int last_reported = 0xFFFFFFFFu;
    if (last_reported != command) {
        fprintf(stderr, "[warning] Going Commando sound command %#x is not supported; dropped\n", command);
        last_reported = command;
    }
    return 0;
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

/* snd_BankLoadFromEE (0x001326D8): a bank in EE memory, loaded at once, its handle the result. A level
 * swaps its dialogue banks so (func_00294988); the console's version waits for the IOP. */
int func_001326D8(int bank) {
    const gaddr at = openrac_guest_ram_address((gaddr)bank);
    return bank != 0 && at < openrac_guest_ram_size
               ? (int)openrac_sound_load_bank(G(at), openrac_guest_ram_size - at)
               : 0;
}

/* snd_PlayVAGStreamByLocEx_CB's form here (0x00133350, command 0x2C): (location, the next entry's
 * location, 0, 0, volume, pan, group, queue on, 0, flags, callback, argument). Flags as rac1's: 1
 * paused until snd_ContinueVAGStream, 4 loop. The location is all the common stream player needs;
 * the callback is told the handle, as the IOP's answer would, and the game's wait for a buffered
 * stream (func_002E6D88) ends through snd_IsVAGStreamBuffered_CB. */
int func_00133350(
    int first,
    int next,
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
    (void)a8;
    (void)next;
    openrac_lib_snd_PlayVAGStreamByLocEx_CB(
        first, 0, 0, 0, volume, pan, group, queue_on, flags, callback, argument
    );
    return 0;
}
