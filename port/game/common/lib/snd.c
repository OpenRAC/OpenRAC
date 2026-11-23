/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The sound library (989snd), replaced at its API: the cleanest cut, since
 * everything below it ran on the IOP (docs/port/RAC1_PAL_SURVEY.md, section
 * 8). Banks and sounds go to the port's sound engine (openrac/sound_engine.h);
 * the sounds a game sends in its command batch are taken by that game's flush.
 * VAG streams (music, speech) are read from the disc and played by the same
 * engine; movie sound is the movie player's. Disc reads made through the library's
 * stream-safe calls go to the disc replacement.
 *
 * Going Commando's and Up Your Arsenal's library hands most answers to the caller's callback rather
 * than returning them, and the callback runs at a later snd_Pump: the requests taken since the last
 * pump are answered together when the pump sends them, and their callbacks run at the pump after.
 * The game counts on that order (a cut scene's stream takes its handle one sound tick after asking,
 * and the random numbers the sound tick draws in between are the ones retail draws), so the
 * requests below keep it: each is answered by the engine when its group is sent and called back at
 * the next pump. Nothing of the console's transport is kept: no command numbers, no byte buffers, no
 * reply words. */
#include <stdio.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "openrac/sound_engine.h"

/* OPENRAC_SOUND_LOG (as the command flush, rac1/host/hand.c): the stops and the still-playing
 * questions, which go to the engine directly rather than through the batch, on stderr too. */
static int sound_log(void) {
    static int on = -1;
    if (on < 0) {
        on = openrac_env_string("OPENRAC_SOUND_LOG") != NULL;
    }
    return on;
}

int openrac_lib_snd_StartSoundSystem(void) {
    return 0;
} /* snd_StartSoundSystem */

/* The disc read of snd_StreamSafeCdRead and the callback the game gave for its end
 * (snd_StreamSafeCdCallback). The read itself is done at once; the callback is
 * called with 1 (done) at the next command flush, as the IOP's answer reaches the
 * EE after the call has returned (the game marks the read as running only once
 * it has). */
static gaddr cd_callback;
static int cd_done_pending;

int openrac_lib_snd_FlushSoundCommands(void) {
    if (cd_done_pending && cd_callback != 0) {
        cd_done_pending = 0;
        GFN(void (*)(int), cd_callback)(1);
    }
    return 0;
} /* snd_FlushSoundCommands */

unsigned int openrac_lib_snd_BankLoadByLoc(int sector, int offset) { /* snd_BankLoadByLoc */
    return openrac_sound_load_bank_from_disc((uint32_t)sector, (uint32_t)offset);
}

/* snd_BankLoadFromEE_CB(bank, callback, argument): a bank in EE memory, loaded at once; the
 * callback is told, as the IOP's answer would: callback(handle, argument). */
void openrac_lib_snd_BankLoadFromEE_CB(int bank, int callback, long long argument) {
    const gaddr at = openrac_guest_ram_address((gaddr)bank);
    const unsigned int handle = bank != 0 && at < openrac_guest_ram_size
                                    ? openrac_sound_load_bank(G(at), openrac_guest_ram_size - at)
                                    : 0;
    if (callback != 0) {
        GFN(void (*)(int, long long), (gaddr)callback)((int)handle, argument);
    }
} /* snd_BankLoadFromEE_CB */

void openrac_lib_snd_ResolveBankXREFS(void) {} /* snd_ResolveBankXREFS */

void openrac_lib_snd_UnloadBank(int bank) {
    openrac_sound_unload_bank((uint32_t)bank);
} /* snd_UnloadBank */

void openrac_lib_snd_SetMasterVolume(int which, int volume) {
    if (sound_log()) {
        fprintf(stderr, "[debug] sound master %d = %#x\n", which, (unsigned)volume);
    }
    openrac_sound_set_master_volume(which, volume);
} /* snd_SetMasterVolume */

void openrac_lib_snd_SetPlaybackMode(int mode) {
    (void)mode;
} /* snd_SetPlaybackMode */

void openrac_lib_snd_StopAllSounds(void) {
    openrac_sound_stop_all();
} /* snd_StopAllSounds */

void openrac_lib_snd_PauseAllSoundsInGroup(int group) {
    openrac_sound_pause_groups((uint32_t)group);
} /* snd_PauseAllSoundsInGroup */

void openrac_lib_snd_ContinueAllSoundsInGroup(int group) {
    openrac_sound_continue_groups((uint32_t)group);
} /* snd_ContinueAllSoundsInGroup */

/* snd_SoundIsStillPlaying_CB(handle, callback, argument): callback(handle) while it plays,
 * callback(0) once it has ended (a stream when stopped). */
void openrac_lib_snd_SoundIsStillPlaying_CB(int a, int b, int c) {
    if (b != 0) {
        const int live = openrac_stream_live((uint32_t)a) || openrac_sound_playing((uint32_t)a);
        if (sound_log()) {
            fprintf(stderr, "[debug] sound still playing %#x -> %d\n", (unsigned)a, live);
        }
        GFN(void (*)(int, long long), (gaddr)b)(live ? a : 0, (long long)c);
    }
} /* snd_SoundIsStillPlaying_CB */

void openrac_lib_snd_reset_state_and_flush_commands(void) {
} /* snd_reset_state_and_flush_commands */

int openrac_lib_snd_InitVAGStreamingEx(int a, int b, int c, int d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    /* Native streams allocate their buffers when played. Report readiness
     * so the game installs its stream-safe CD completion callback. */
    return 1;
} /* snd_InitVAGStreamingEx */

void openrac_lib_snd_StopAllStreams(void) {
    openrac_stream_stop(0);
} /* snd_StopAllStreams */

/* snd_StopSound(handle): a sound or a stream ends. */
void openrac_lib_snd_StopSound(int handle) {
    if (sound_log()) {
        fprintf(stderr, "[debug] sound stop %#x (%s)\n", (unsigned)handle,
                handle == 0 ? "none" : openrac_sound_playing((uint32_t)handle) ? "playing" : "not playing");
    }
    if (handle != 0) {
        if (openrac_stream_live((uint32_t)handle)) {
            openrac_stream_stop((uint32_t)handle);
        } else {
            openrac_sound_stop((uint32_t)handle);
        }
    }
} /* snd_StopSound */

/* snd_PlayVAGStreamByLocEx_CB(sector, 0, 0, 0, volume, pan, group, queue on, flags, callback,
 * argument), as RAC1's music and speech call it: a stream starts (or is queued behind the live
 * stream `queue on`); the callback gets its handle, as the IOP's answer would. Flags: 1 paused,
 * 4 loop. */
void openrac_lib_snd_PlayVAGStreamByLocEx_CB(
    int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, long long k
) {
    (void)b;
    (void)c;
    (void)d;
    (void)f;
    const uint32_t handle = openrac_stream_play((uint32_t)a, e, g, (uint32_t)h, (uint32_t)i);
    if (sound_log()) {
        fprintf(
            stderr,
            "[debug] sound stream %#x: sector %#x volume %#x group %d after %#x flags %#x\n",
            handle,
            (unsigned)a,
            (unsigned)e,
            g,
            (unsigned)h,
            (unsigned)i
        );
    }
    if (j != 0) {
        GFN(void (*)(int, long long), (gaddr)j)((int)handle, k);
    }
} /* snd_PlayVAGStreamByLocEx_CB */

/* A stream of two mono files: the right channel's and the left's (Going Commando's music). */
void openrac_lib_snd_PlayVAGStreamStereo_CB(
    int right,
    int left,
    int volume,
    int group,
    int queue_on,
    int flags,
    int callback,
    long long argument
) {
    const uint32_t handle = openrac_stream_play_stereo(
        (uint32_t)right, (uint32_t)left, volume, group, (uint32_t)queue_on, (uint32_t)flags
    );
    if (sound_log()) {
        fprintf(
            stderr,
            "[debug] sound stream %#x: sectors %#x %#x volume %#x group %d after %#x flags %#x\n",
            handle,
            (unsigned)right,
            (unsigned)left,
            (unsigned)volume,
            group,
            (unsigned)queue_on,
            (unsigned)flags
        );
    }
    if (callback != 0) {
        GFN(void (*)(int, long long), (gaddr)callback)((int)handle, argument);
    }
}

void openrac_lib_snd_PauseVAGStream(int stream) {
    openrac_stream_pause((uint32_t)stream, 1);
} /* snd_PauseVAGStream */

void openrac_lib_snd_ContinueVAGStream(int stream) {
    openrac_stream_pause((uint32_t)stream, 0);
} /* snd_ContinueVAGStream */

/* snd_GetVAGStreamTimeRemaining_CB(handle, callback, argument): milliseconds left in the part
 * playing (the game only compares them with each other). */
void openrac_lib_snd_GetVAGStreamTimeRemaining_CB(int a, int b, int c) {
    if (b != 0) {
        GFN(void (*)(int, long long), (gaddr)b)(openrac_stream_remaining_ms((uint32_t)a), (long long)c);
    }
} /* snd_GetVAGStreamTimeRemaining_CB */

/* snd_IsVAGStreamBuffered_CB(handle, callback, argument): once its first part is decoded. */
void openrac_lib_snd_IsVAGStreamBuffered_CB(int a, int b, int c) {
    if (b != 0) {
        GFN(void (*)(int, long long), (gaddr)b)(openrac_stream_buffered((uint32_t)a), (long long)c);
    }
} /* snd_IsVAGStreamBuffered_CB */

void openrac_lib_snd_StreamSafeCheckCDIdle(int a) {
    (void)a;
} /* snd_StreamSafeCheckCDIdle */

int openrac_lib_snd_StreamSafeCdRead(int sector, int count, int buffer) { /* snd_StreamSafeCdRead */
    const int ok = openrac_lib_sceCdRead((unsigned int)sector, (unsigned int)count, (gaddr)buffer, 0);
    cd_done_pending = ok != 0;
    return ok;
}

int openrac_lib_snd_StreamSafeCdSync(int mode) {
    return openrac_lib_sceCdSync(mode);
} /* snd_StreamSafeCdSync */

int openrac_lib_snd_StreamSafeCdBreak(void) {
    return openrac_lib_sceCdBreak();
} /* snd_StreamSafeCdBreak */

int openrac_lib_snd_StreamSafeCdGetError(void) {
    return openrac_lib_sceCdGetError();
} /* snd_StreamSafeCdGetError */

int openrac_lib_snd_StreamSafeCdCallback(int callback) {
    const gaddr old = cd_callback;
    cd_callback = (gaddr)callback;
    return (int)old;
} /* snd_StreamSafeCdCallback */

void openrac_lib_snd_PreAllocReverbWorkArea(int a, int b) {
    (void)a;
    (void)b;
} /* snd_PreAllocReverbWorkArea */

void openrac_lib_snd_AutoReverb(int a, int b, int c, int d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
} /* snd_AutoReverb */

void openrac_lib_snd_ResetMovieSound(void) {} /* snd_ResetMovieSound */

void openrac_lib_snd_StartMovieSound(int a, int b, int c, int d, int e) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
} /* snd_StartMovieSound */

int openrac_lib_snd_UpdateMovieADPCM(int a, int b) {
    (void)a;
    (void)b;
    return 0;
} /* snd_UpdateMovieADPCM */

int openrac_lib_snd_GetMovieNAX(void) {
    return 0;
} /* snd_GetMovieNAX */

int openrac_lib_snd_GetDopplerPitchMod(int a) {
    (void)a;
    return 0;
} /* snd_GetDopplerPitchMod */

/* ---- Requests answered at a later pump (Going Commando, Up Your Arsenal) ---- */

/* A request taken: the engine call that answers it, its arguments (copied when it was taken, as the
 * library copies them), and the callback the answer goes to. */
typedef struct {
    OpenracSndAnswer answer_of;
    int32_t args[OPENRAC_SND_REQUEST_ARGS];
    gaddr callback;
    long long argument;
    int answer;
} Request;

/* The library takes at most 0x100 requests before it sends them. */
#define GROUP_REQUESTS 0x100

typedef struct {
    Request requests[GROUP_REQUESTS];
    int count;
} RequestGroup;

static RequestGroup groups[2];
static int filling;     /* the group being filled */
static int answers_out; /* the other group has been answered; its callbacks are still to run */

/* The group being filled is answered, in order, and becomes the one whose callbacks the next pump
 * runs. */
static void send_group(void) {
    RequestGroup* group = &groups[filling];
    for (int i = 0; i < group->count; ++i) {
        Request* r = &group->requests[i];
        r->answer = r->answer_of != NULL ? r->answer_of(r->args) : 0;
    }
    filling ^= 1;
    groups[filling].count = 0;
    answers_out = 1;
}

int openrac_lib_snd_Pump(void) {
    if (answers_out) {
        answers_out = 0;
        RequestGroup* answered = &groups[filling ^ 1];
        const int count = answered->count;
        answered->count = 0; /* a callback that makes a request must not see these again */
        for (int i = 0; i < count; ++i) {
            const Request* r = &answered->requests[i];
            if (r->callback != 0) {
                GFN(void (*)(int, long long), r->callback)(r->answer, r->argument);
            }
        }
    }
    openrac_lib_snd_FlushSoundCommands();
    if (!answers_out && groups[filling].count != 0) {
        send_group();
    }
    return answers_out;
} /* snd_Pump */

void openrac_snd_request(
    OpenracSndAnswer answer_of, const int32_t* args, int count, gaddr callback, long long argument
) {
    if (count < 0 || count > OPENRAC_SND_REQUEST_ARGS) {
        return;
    }
    /* A request with nothing to send and no callback, with nothing taken or out, is answered at
     * once. */
    if (count == 0 && callback == 0 && !answers_out && groups[filling].count == 0) {
        if (answer_of != NULL) {
            (void)answer_of(args);
        }
        return;
    }
    /* A full group waits for room: the pumps run until the other group has been called back. */
    while (groups[filling].count == GROUP_REQUESTS) {
        openrac_lib_snd_Pump();
    }
    RequestGroup* group = &groups[filling];
    Request* r = &group->requests[group->count++];
    memset(r, 0, sizeof *r);
    r->answer_of = answer_of;
    if (count > 0) {
        memcpy(r->args, args, (size_t)count * sizeof(int32_t));
    }
    r->callback = callback;
    r->argument = argument;
    openrac_lib_snd_Pump(); /* the library pumps as it takes each request */
}

int openrac_snd_request_sync(OpenracSndAnswer answer_of, const int32_t* args) {
    while (answers_out) {
        openrac_lib_snd_Pump();
    }
    const int answer = answer_of != NULL ? answer_of(args) : 0;
    if (groups[filling].count != 0) {
        send_group();
    }
    return answer;
}

/* LoadExecPS2 ended the program (game_lib.h): nothing of its taken, out or sounding. */
void openrac_snd_client_reset(void) {
    memset(groups, 0, sizeof groups);
    filling = 0;
    answers_out = 0;
    (void)openrac_lib_snd_StreamSafeCdCallback(0); /* the end of its disc read: its code too */
    openrac_lib_snd_StopAllSounds();
}

static int answer_play(const int32_t* a) {
    return (int)openrac_sound_play((uint32_t)a[0], a[1], a[2], a[3], a[4], a[5]);
}

/* snd_PlaySoundVolPanPMPB_CB(bank, sound, volume, pan, pitch modifier, pitch bend, callback,
 * argument): the new sound's handle (0: none) goes to the callback. */
int openrac_lib_snd_PlaySoundVolPanPMPB_CB(
    int bank,
    int sound,
    int volume,
    int pan,
    int pitch_mod,
    int pitch_bend,
    int callback,
    long long argument
) {
    if (sound_log()) {
        fprintf(
            stderr,
            "[debug] sound play: bank %#x sound %d volume %#x pan %d pitch %d bend %d\n",
            (unsigned)bank,
            sound,
            (unsigned)volume,
            pan,
            pitch_mod,
            pitch_bend
        );
    }
    const int32_t args[6] = {bank, sound, volume, pan, pitch_mod, pitch_bend};
    openrac_snd_request(answer_play, args, 6, (gaddr)callback, argument);
    return 1;
}

static int answer_set_params(const int32_t* a) {
    return (int)openrac_sound_set_params((uint32_t)a[0], a[1], a[2], a[3], a[4], a[5]);
}

/* snd_SetSoundParams_CB(handle, mask, volume, pan, pitch modifier, pitch bend, callback, argument):
 * the callback is told the handle while the sound plays and 0 once it has ended, and that 0 is
 * what frees the game's voice slot (Going Commando's func_002E60E8). */
int openrac_lib_snd_SetSoundParams_CB(
    int handle,
    int mask,
    int volume,
    int pan,
    int pitch_mod,
    int pitch_bend,
    int callback,
    long long argument
) {
    const int32_t args[6] = {handle, mask, volume, pan, pitch_mod, pitch_bend};
    openrac_snd_request(answer_set_params, args, 6, (gaddr)callback, argument);
    return 1;
}

static int answer_stream_two_parts(const int32_t* a) {
    const uint32_t handle = openrac_stream_play_stereo(
        (uint32_t)a[0], (uint32_t)a[1], (short)a[2], a[3], (uint32_t)a[4], (uint32_t)a[5]
    );
    if (sound_log()) {
        fprintf(
            stderr,
            "[debug] sound stream %#x: sectors %#x %#x volume %#x group %d after %#x flags %#x\n",
            handle,
            (unsigned)a[0],
            (unsigned)a[1],
            (unsigned)a[2],
            a[3],
            (unsigned)a[4],
            (unsigned)a[5]
        );
    }
    return (int)handle;
}

/* snd_PlayVAGStreamTwoParts_CB(right, left, 0, 0, volume, pan, group, queue on, 0, flags, callback,
 * argument): music is two mono files on the disc, the right channel's and the left's; speech has
 * no left. Flags as rac1's: 1 paused until snd_ContinueVAGStream, 4 loop. The callback is told the
 * stream's handle. */
int openrac_lib_snd_PlayVAGStreamTwoParts_CB(
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
    (void)pan;
    (void)a8;
    const int32_t args[6] = {right, left, (short)volume, group, queue_on, flags};
    openrac_snd_request(answer_stream_two_parts, args, 6, (gaddr)callback, argument);
    return 1;
}

/* snd_BankLoadFromIOP(bank): a bank the game sent to the IOP's memory with sceSifSetDma, loaded at
 * once, its handle. */
int openrac_lib_snd_BankLoadFromIOP(int bank) {
    size_t available = 0;
    const uint8_t* at = openrac_iop_bytes((uint32_t)bank, &available);
    if (sound_log() && at != NULL && available >= 16) {
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
} /* snd_BankLoadFromIOP */
