/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * 989snd's command boundary as Going Commando and Up Your Arsenal build it, without the IOP
 * (openrac/game_lib.h, "989snd's command ring").
 *
 * The game's sound client sends each request to the IOP's sound driver by RPC: snd_SendCommandSync
 * (command, size, data) copies the data to its send buffer and waits for the reply, and
 * snd_QueueCommandToRing (command, size, data and an optional callback) queues it for the next
 * flush. There is no IOP in the port: synchronous commands are answered at once, and queued
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

/* Commands that only configure the IOP's mixer, which the native engine has no equivalent of: the
 * mixer mode (0xD), reverb (0x10, 0x50, 0x51, 0x60), voice ranges (0x4E), the playback mode (0xB),
 * the banks' cross references (8) and Up Your Arsenal's voice allocation mode (0x64). */
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
        case 0x64:
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
             * (Going Commando's func_002E60E8). */
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
                        stderr, "[warning] sound command %#x is not supported; answered 0\n", command
                    );
                    last_reported = command;
                }
            }
            return 0;
    }
#undef WORD
}

/*
 * The command ring of snd_QueueCommandToRing and snd_Pump, with an IOP that answers a batch by the
 * next pump, as the test-only interpreter's does.
 *
 * Retail appends each command to the current batch (at most 0x100 commands and 0x1000 bytes) and
 * commits it, and the commit pumps. A pump first takes the answers of the batch the IOP has (its
 * callbacks run, in order, with their reply words), then, with no batch out, sends the current one.
 * So a command queued with nothing out goes to the IOP at once, and its callback runs at the next
 * pump: the next command queued, the next sound tick (Going Commando's func_002E4AC8 pumps first),
 * or a synchronous command's wait. The game's waits count on that: a cut scene's stream
 * (func_002F53C8) takes its handle one tick after it was asked for, and its loop runs the sound tick
 * once more meanwhile. A port that called back at once ran different ticks there, so the random
 * numbers the sound tick draws, and every fight after it, went elsewhere (R2-AD).
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

int openrac_snd_ring_pump(void) {
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

void openrac_snd_ring_queue_bytes(
    unsigned int command, const uint8_t* data, int bytes, gaddr callback, long long callback_data
) {
    int need = bytes + 4;
    if (need % 4 != 0) {
        need += 4 - need % 4;
    }
    /* A full batch waits for room: retail pumps until the other batch has come back and gone out. */
    while (batches[current].count == RING_COMMANDS || RING_BYTES - batches[current].bytes < need) {
        openrac_snd_ring_pump();
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
    openrac_snd_ring_pump(); /* snd_CommitRingEntry */
}

int openrac_snd_ring_queue(
    unsigned int command, gaddr data, int bytes, gaddr callback, long long callback_data
) {
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
    openrac_snd_ring_queue_bytes(
        command, bytes > 0 ? (const uint8_t*)G(data) : NULL, bytes, callback, callback_data
    );
    return 1;
}

/* Synchronous commands the port has nothing to do for, answered 0 without a warning: the stream
 * CD's idle check (0x36) and the movie's sound (0x3B-0x40, 0x5A, 0x5B: the movie player's own). */
static int sync_quiet(unsigned int command) {
    switch (command) {
        case 0x36:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x40:
        case 0x5A:
        case 0x5B:
            return 1;
        default:
            return 0;
    }
}

int openrac_snd_ring_send_sync(unsigned int command, gaddr data, int bytes) {
    if (bytes < 0 || bytes > 0xFFC || (bytes != 0 && data == 0)) {
        return 0;
    }
    if (openrac_env_string("OPENRAC_SOUND_LOG") != NULL) {
        fprintf(
            stderr,
            "[debug] synchronous sound command %#x: %#x %#x %#x %#x\n",
            command,
            bytes > 0 ? (unsigned)GREF(uint32_t, data) : 0,
            bytes > 4 ? (unsigned)GREF(uint32_t, data + 4u) : 0,
            bytes > 8 ? (unsigned)GREF(uint32_t, data + 8u) : 0,
            bytes > 12 ? (unsigned)GREF(uint32_t, data + 12u) : 0
        );
    }
    while (batch_out) {
        openrac_snd_ring_pump();
    }
    int answer = 0;
    if (command == 0) { /* the sound system's start; the host's audio owns the device */
        answer = openrac_lib_snd_StartSoundSystem();
    } else if (command == 0x2A) { /* snd_InitVAGStreamingEx: native streams are always ready */
        answer = 1;
    } else if (command == OPENRAC_SND_PLAY_VOL_PAN_PMPB
               || command == OPENRAC_SND_SET_SOUND_PARAMS) {
        answer = (int)openrac_sound_command(command, G(data), (uint32_t)bytes);
    } else if (!sync_quiet(command)) {
        static unsigned int last_reported = 0xFFFFFFFFu;
        if (last_reported != command) {
            fprintf(
                stderr,
                "[warning] synchronous sound command %#x is not supported; answered 0\n",
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

int openrac_snd_ring_bank_from_iop(int bank) {
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
