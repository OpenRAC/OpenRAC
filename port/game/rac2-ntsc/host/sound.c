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
#include <string.h>

#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
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

/* Commands that only configure the IOP's mixer, which the native engine has no equivalent of: the
 * mixer mode (0xD), reverb (0x10, 0x50, 0x51, 0x60), voice ranges (0x4E), the playback mode (0xB) and
 * the banks' cross references (8). */
static int mixer_only(unsigned int command) {
    switch (command) {
        case 0x8:
        case 0xB:
        case 0xD:
        case 0x10:
        case 0x4E:
        case 0x50:
        case 0x51:
        case 0x60:
            return 1;
        default:
            return 0;
    }
}

/* Most commands reach the IOP from a wrapper with a libraries.tsv row, but the catalogue's name of
 * several wrappers is not their address (func_00133340 is at 0x00133350), and their C is what the
 * callers get. So the commands are taken here, where every wrapper ends. Replies go to the callback,
 * as the IOP's answer would. Returns 0 for a command it does not know. */
static int stream_or_sound_command(
    unsigned int command, gaddr data, long long callback, int callback_data
) {
#define WORD(i) ((int)GREF(uint32_t, data + (i) * 4u))
    const int cb = (int)callback;
    switch (command) {
        case 0x6:
            openrac_lib_snd_UnloadBank(WORD(0));
            return 1;
        case 0x9:
            openrac_lib_snd_SetMasterVolume(WORD(0), WORD(1));
            return 1;
        case 0x15:
            openrac_lib_snd_StopSound(WORD(0));
            return 1;
        case 0x16:
            openrac_lib_snd_PauseAllSoundsInGroup(WORD(0));
            return 1;
        case 0x17:
            openrac_lib_snd_ContinueAllSoundsInGroup(WORD(0));
            return 1;
        case 0x18:
            openrac_lib_snd_StopAllSounds();
            return 1;
        case 0x19:
            openrac_lib_snd_SoundIsStillPlaying_CB(WORD(0), cb, callback_data);
            return 1;
        case 0x2C: /* right, left (rc2_l00_main_br, _bl), volume << 16, pan << 16, group, queue on, 0, flags */
            openrac_lib_snd_PlayVAGStreamStereo_CB(
                WORD(0),
                WORD(1),
                (short)(WORD(2) >> 16),
                WORD(4),
                WORD(5),
                WORD(7),
                cb,
                (long long)(unsigned int)callback_data
            );
            return 1;
        case 0x2D:
            openrac_lib_snd_PauseVAGStream(WORD(0));
            return 1;
        case 0x2E:
            openrac_lib_snd_ContinueVAGStream(WORD(0));
            return 1;
        case 0x32:
            openrac_lib_snd_GetVAGStreamTimeRemaining_CB(WORD(0), cb, callback_data);
            return 1;
        case 0x34:
            openrac_lib_snd_StopAllStreams();
            return 1;
        case 0x4F:
            openrac_lib_snd_IsVAGStreamBuffered_CB(WORD(0), cb, callback_data);
            return 1;
        default:
            return 0;
    }
#undef WORD
}

int func_00132DF0(int command_arg, int bytes, int data_arg, long long callback, int callback_data) {
    const unsigned int command = (unsigned int)command_arg;
    const gaddr data = (gaddr)data_arg;
    if (bytes < 0 || bytes > 0xFFC || (bytes != 0 && data == 0)) {
        return 0;
    }
    if (openrac_env_string("OPENRAC_SOUND_LOG") != NULL) {
        fprintf(
            stderr,
            "[debug] sound command %#x: %#x %#x %#x %#x %#x %#x %#x %#x\n",
            command,
            bytes > 0 ? (unsigned)GREF(uint32_t, data) : 0,
            bytes > 4 ? (unsigned)GREF(uint32_t, data + 4u) : 0,
            bytes > 8 ? (unsigned)GREF(uint32_t, data + 8u) : 0,
            bytes > 12 ? (unsigned)GREF(uint32_t, data + 12u) : 0,
            bytes > 16 ? (unsigned)GREF(uint32_t, data + 16u) : 0,
            bytes > 20 ? (unsigned)GREF(uint32_t, data + 20u) : 0,
            bytes > 24 ? (unsigned)GREF(uint32_t, data + 24u) : 0,
            bytes > 28 ? (unsigned)GREF(uint32_t, data + 28u) : 0
        );
    }
    if (command == OPENRAC_SND_PLAY_VOL_PAN_PMPB || command == OPENRAC_SND_SET_SOUND_PARAMS) {
        const int answer = (int)openrac_sound_command(command, G(data), (uint32_t)bytes);
        /* The IOP's answer, which holds a voice slot until it comes (func_002E5A50 finds none free, and
         * 42 slots in, the game writes past the table). Playing answers the new handle; setting a
         * sound's parameters answers its handle while it plays and 0 once it has ended, and that 0
         * is what frees the slot (func_002E60E8). */
        if (callback != 0) {
            GFN(void (*)(int, long long),
                (gaddr)callback)(answer, (long long)(unsigned int)callback_data);
        }
        return 1;
    }
    if (mixer_only(command) || stream_or_sound_command(command, data, callback, callback_data)) {
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

/* snd_BankLoadFromIOP (0x001326D8, the symbol table's name): a bank the game sent to the IOP's memory
 * with sceSifSetDma, loaded at once, its handle the result. A hand item's bank (func_00294988's three
 * 0xC800 slots, the Lancer's firing sounds among them) arrives so; the address is the IOP's, which the
 * EE's memory would have read as zeros. */
int func_001326D8(int bank) {
    size_t available = 0;
    const uint8_t* at = openrac_iop_bytes((uint32_t)bank, &available);
    if (openrac_env_string("OPENRAC_SOUND_LOG") != NULL && at != NULL && available >= 16) {
        uint32_t w[4];
        memcpy(w, at, sizeof(w));
        fprintf(
            stderr,
            "[debug] sound bank from the IOP at %#x: %#x %#x %#x %#x\n",
            (unsigned)bank,
            w[0],
            w[1],
            w[2],
            w[3]
        );
    }
    return bank != 0 && at != NULL ? (int)openrac_sound_load_bank(at, available) : 0;
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
    (void)a8;
    (void)pan;
    openrac_lib_snd_PlayVAGStreamStereo_CB(
        right, left, volume, group, queue_on, flags, callback, argument
    );
    return 0;
}
