/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's 989snd command boundary, without the IOP.
 *
 * The game's sound client sends each request to the IOP's sound driver by
 * RPC: snd_SendCommandSync (0x00132C48: command, size, data) copies the
 * data to its send buffer and waits for the reply, and snd_QueueCommandToRing
 * (0x00132DF0: command, size, data and an optional callback) queues it for
 * the next flush. There is no IOP in the port: synchronous commands are answered at once, and queued
 * commands go through a ring of the same shape as retail's, answered by the native sound engine when
 * a batch is sent and called back at the next pump, as an IOP that answers within a tick would.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "openrac/sound_engine.h"
#include "rac2.h"

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

/* What the IOP's sound driver answers to one command of a batch, run here: the reply word the
 * command's callback gets. `data` is the batch's copy of the command's bytes. Commands the native
 * engine has no use for (the mixer's, mixer_only) and those it does not know answer 0. */
static int command_answer(unsigned int command, const uint8_t* data, int bytes) {
#define WORD(i)                                                                                    \
    (4 * (i) + 4 <= bytes                                                                          \
         ? (int)((uint32_t)data[4 * (i)] | (uint32_t)data[4 * (i) + 1] << 8                        \
                 | (uint32_t)data[4 * (i) + 2] << 16 | (uint32_t)data[4 * (i) + 3] << 24)          \
         : 0)
    switch (command) {
        case OPENRAC_SND_PLAY_VOL_PAN_PMPB:
        case OPENRAC_SND_SET_SOUND_PARAMS:
            /* Playing answers the new handle; setting a sound's parameters answers its handle while
             * it plays and 0 once it has ended, and that 0 is what frees the voice slot
             * (func_002E60E8). */
            return (int)openrac_sound_command(command, data, (uint32_t)bytes);
        case 0x6:
            openrac_lib_snd_UnloadBank(WORD(0));
            return 0;
        case 0x9:
            openrac_lib_snd_SetMasterVolume(WORD(0), WORD(1));
            return 0;
        case 0x15:
            openrac_lib_snd_StopSound(WORD(0));
            return 0;
        case 0x16:
            openrac_lib_snd_PauseAllSoundsInGroup(WORD(0));
            return 0;
        case 0x17:
            openrac_lib_snd_ContinueAllSoundsInGroup(WORD(0));
            return 0;
        case 0x18:
            openrac_lib_snd_StopAllSounds();
            return 0;
        case 0x19: { /* still playing: its handle while it does, else 0 */
            const uint32_t handle = (uint32_t)WORD(0);
            return openrac_stream_live(handle) || openrac_sound_playing(handle) ? (int)handle : 0;
        }
        case 0x2C: /* right, left (rc2_l00_main_br, _bl), volume << 16, pan << 16, group, queue on, 0, flags */
            return (int)openrac_stream_play_stereo(
                (uint32_t)WORD(0),
                (uint32_t)WORD(1),
                (short)(WORD(2) >> 16),
                WORD(4),
                (uint32_t)WORD(5),
                (uint32_t)WORD(7)
            );
        case 0x2D:
            openrac_lib_snd_PauseVAGStream(WORD(0));
            return 0;
        case 0x2E:
            openrac_lib_snd_ContinueVAGStream(WORD(0));
            return 0;
        case 0x32:
            return openrac_stream_remaining_ms((uint32_t)WORD(0));
        case 0x34:
            openrac_lib_snd_StopAllStreams();
            return 0;
        case 0x4F:
            return openrac_stream_buffered((uint32_t)WORD(0));
        default:
            if (!mixer_only(command)) {
                static unsigned int last_reported = 0xFFFFFFFFu;
                if (last_reported != command) {
                    fprintf(
                        stderr,
                        "[warning] Going Commando sound command %#x is not supported; answered 0\n",
                        command
                    );
                    last_reported = command;
                }
            }
            return 0;
    }
#undef WORD
}

/*
 * The command ring of snd_SendIOPCommandNoWait (func_00132DF0) and snd_Pump (func_00132028), with an
 * IOP that answers a batch by the next pump, as the test-only interpreter's does.
 *
 * Retail appends each command to the current batch (at most 0x100 commands and 0x1000 bytes) and
 * commits it, and the commit pumps. A pump first takes the answers of the batch the IOP has (its
 * callbacks run, in order, with their reply words), then, with no batch out, sends the current one.
 * So a command queued with nothing out goes to the IOP at once, and its callback runs at the next
 * pump: the next command queued, the next sound tick (func_002E4AC8 pumps first), or a synchronous
 * command's wait. The game's waits count on that: a cut scene's stream (func_002F53C8) takes its
 * handle one tick after it was asked for, and its loop runs the sound tick once more meanwhile. A
 * port that called back at once ran different ticks there, so the random numbers the sound tick
 * draws, and every fight after it, went elsewhere (R2-AD).
 */
#define RING_COMMANDS 0x100
#define RING_BYTES 0x1000

typedef struct {
    unsigned int command;
    int bytes;
    int at; /* its bytes in the batch's pool */
    gaddr callback;
    long long callback_data;
    int answer;
} RingCommand;

typedef struct {
    RingCommand commands[RING_COMMANDS];
    uint8_t pool[RING_BYTES];
    int count;
    int bytes; /* used, as retail counts it: each command's data plus its 4-byte header, word aligned */
} RingBatch;

static RingBatch batches[2];
static int current;   /* the batch being filled */
static int batch_out; /* the other batch is at the IOP */

/* Sends the current batch: the IOP runs its commands in order, their answers kept for the next pump. */
static void send_batch(void) {
    RingBatch* batch = &batches[current];
    for (int i = 0; i < batch->count; ++i) {
        RingCommand* c = &batch->commands[i];
        c->answer = command_answer(c->command, batch->pool + c->at, c->bytes);
    }
    current ^= 1;
    batches[current].count = 0;
    batches[current].bytes = 0;
    batch_out = 1;
}

/* snd_Pump (0x00132028): the answers of the batch out, the disc read's end (the common library's
 * flush), then the next batch. Returns whether a batch is still out, as retail's (an RPC or a disc
 * read in flight). */
int func_00132028(void) {
    if (batch_out) {
        batch_out = 0;
        RingBatch* answered = &batches[current ^ 1];
        const int count = answered->count;
        answered->count = 0; /* a callback that queues a command must not see these again */
        for (int i = 0; i < count; ++i) {
            const RingCommand* c = &answered->commands[i];
            if (c->callback != 0) {
                GFN(void (*)(int, long long), c->callback)(c->answer, c->callback_data);
            }
        }
    }
    openrac_lib_snd_FlushSoundCommands();
    if (!batch_out && batches[current].count != 0) {
        send_batch();
    }
    return batch_out;
}

/* Queues one command (its bytes copied now, as retail copies them into the ring) and commits it. */
static void queue_command(
    unsigned int command, const uint8_t* data, int bytes, gaddr callback, long long callback_data
) {
    int need = bytes + 4;
    if (need % 4 != 0) {
        need += 4 - need % 4;
    }
    /* A full batch waits for room: retail pumps until the other batch has come back and gone out. */
    while (batches[current].count == RING_COMMANDS || RING_BYTES - batches[current].bytes < need) {
        func_00132028();
    }
    RingBatch* batch = &batches[current];
    RingCommand* c = &batch->commands[batch->count++];
    c->command = command;
    c->bytes = bytes;
    c->at = batch->bytes;
    if (bytes > 0) {
        memcpy(batch->pool + c->at, data, (size_t)bytes);
    }
    c->callback = callback;
    c->callback_data = callback_data;
    c->answer = 0;
    batch->bytes += need;
    func_00132028(); /* snd_CommitRingEntry */
}

int func_00132DF0(int command_arg, int bytes, int data_arg, int callback, long long callback_data) {
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
    /* A bare command (no data, no callback) with nothing queued or out goes straight to the IOP. */
    if (bytes == 0 && callback == 0 && !batch_out && batches[current].count == 0) {
        (void)command_answer(command, NULL, 0);
        return 1;
    }
    queue_command(
        command,
        bytes > 0 ? (const uint8_t*)G(data) : NULL,
        bytes,
        (gaddr)callback,
        (long long)(unsigned int)callback_data
    );
    return 1;
}

/* snd_SendCommandSync (0x00132C48): waits, pumping, until no batch is out (the callbacks of the
 * one out run), has the IOP answer this command, then sends the batch queued meanwhile. */
int func_00132C48(int command_arg, int bytes, int data_arg) {
    const unsigned int command = (unsigned int)command_arg;
    const gaddr data = (gaddr)data_arg;
    if (bytes < 0 || bytes > 0xFFC || (bytes != 0 && data == 0)) {
        return 0;
    }
    while (batch_out) {
        func_00132028();
    }
    int answer = 0;
    if (command == 0) { /* the sound system's start; the host's audio owns the device */
        answer = openrac_lib_snd_StartSoundSystem();
    } else if (command == OPENRAC_SND_PLAY_VOL_PAN_PMPB
               || command == OPENRAC_SND_SET_SOUND_PARAMS) {
        answer = (int)openrac_sound_command(command, G(data), (uint32_t)bytes);
    } else {
        static unsigned int last_reported = 0xFFFFFFFFu;
        if (last_reported != command) {
            fprintf(
                stderr,
                "[warning] Going Commando synchronous sound command %#x is not supported; answered "
                "0\n",
                command
            );
            last_reported = command;
        }
    }
    if (batches[current].count != 0) {
        send_batch();
    }
    return answer;
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
    queue_command(0x2C, (const uint8_t*)words, (int)sizeof(words), (gaddr)callback, argument);
    return 0;
}
