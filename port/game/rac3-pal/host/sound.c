/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's 989snd, replaced at its API (rac3-decomp's core 0x0013B330-0x0013D420,
 * Going Commando's client at new addresses). The game's client turns each call into a request for
 * the IOP's sound server, sent by RPC: at once and waited for (snd_SendCommandSync, 0x0013C348) or
 * queued for the next pump (snd_QueueCommandToRing, 0x0013C578). The port replaces the client's
 * entry points instead (libraries.tsv): those every game shares go to lib/snd.c, which answers them
 * with the native sound engine and hands the answers to their callbacks at the next pump, as the
 * library does; the ones below are this game's: questions whose answers the library hands on at
 * the next pump, and settings of the IOP's own mixer, which the native engine has no equivalent of,
 * taken as requests with nothing to do so that the callbacks still run at the same pumps. Nothing
 * is left that sends a request to an IOP, and no reply word is kept. The multiplayer program's
 * copies of these (host/mp.c) come here. */
#include <stdint.h>
#include <stdio.h>

#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "openrac/sound_engine.h"
#include "rac3_version.h"

/* The reply wait (0x0013B818): only the client's sends, all replaced, called it; nothing waits for
 * a reply. */
int func_0013B818(void) {
    return 1;
}

/* snd_SendCommandSync (0x0013C348) and snd_QueueCommandToRing (0x0013C578): every entry point that
 * reached them is replaced, so nothing should call them. A call is said once and answered 0. */
static void unreachable(const char* what, int command) {
    static int said;
    if (!said) {
        fprintf(
            stderr,
            "[warning] sound: %s of command %#x reached; the port replaces its callers\n",
            what,
            (unsigned)command
        );
        said = 1;
    }
}

int func_0013C348(int command, int bytes, int data) {
    (void)bytes;
    (void)data;
    unreachable("snd_SendCommandSync", command);
    return 0;
}

int func_0013C578(int command, int bytes, int data, int callback, long long argument) {
    (void)bytes;
    (void)data;
    (void)callback;
    (void)argument;
    unreachable("snd_QueueCommandToRing", command);
    return 0;
}

static int answer_start(const int32_t* args) {
    (void)args;
    return openrac_lib_snd_StartSoundSystem();
}

/* snd_StartSoundSystem (0x0013B3F8): binds the sound servers and sends the first request; the
 * host's audio owns the device, so only the request's wait is left. */
int func_0013B3F8(int options) {
    (void)options;
    return openrac_snd_request_sync(answer_start, NULL);
}

/* snd_InitVAGStreamingEx (0x0013CA30): native streams are always ready. The library asks once:
 * a second call answers 0, as its flag of the first answer (0x001D4DAC) has it. */
static int streams_ready;

static int answer_streams(const int32_t* args) {
    (void)args;
    return 1;
}

int func_0013CA30(int a, int b, int c, int d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    if (streams_ready) {
        return 0;
    }
    (void)openrac_lib_snd_StreamSafeCdSync(0);
    streams_ready = openrac_snd_request_sync(answer_streams, NULL);
    return streams_ready;
}

/* A program LoadExecPS2 ended (entry.c, mp.c): its requests dropped, its sounds stopped, and the
 * next program asks for its streams again. */
void rac3_sound_reset(void) {
    openrac_snd_client_reset();
    streams_ready = 0;
}

static int answer_still_playing(const int32_t* args) {
    const uint32_t handle = (uint32_t)args[0];
    const int live = openrac_stream_live(handle) || openrac_sound_playing(handle);
    if (openrac_env_string("OPENRAC_SOUND_LOG") != NULL) {
        fprintf(stderr, "[debug] sound still playing %#x -> %d\n", handle, live);
    }
    return live ? (int)handle : 0;
}

/* snd_SoundIsStillPlaying_CB (0x0013C200): (handle, callback, argument); the callback is told the
 * handle while it plays and 0 once it has ended, at the next pump. */
int func_0013C200(int handle, int callback, long long argument) {
    const int32_t args[1] = {handle};
    openrac_snd_request(answer_still_playing, args, 1, (gaddr)callback, argument);
    return 1;
}

static int answer_time_left(const int32_t* args) {
    return openrac_stream_remaining_ms((uint32_t)args[0]);
}

/* The music tick's question of each stream (0x0013CC70, func_003A2120): (handle, callback, argument),
 * the callback told the milliseconds left at the next pump, kept at the stream's +0x14. The fade
 * between tracks (func_003A2580) compares it with a quarter of snd_GetVAGStreamTimeRemaining_CB's
 * first answer, so it is milliseconds left as well; answered 0, the fade took the old track as
 * ended at once and cut it off. */
int func_0013CC70(int handle, int callback, long long argument) {
    const int32_t args[1] = {handle};
    openrac_snd_request(answer_time_left, args, 1, (gaddr)callback, argument);
    return 1;
}

static int answer_stop_streams(const int32_t* args) {
    (void)args;
    openrac_lib_snd_StopAllStreams();
    return 0;
}

/* snd_StopAllStreams (0x0013CAE8): a request of no words, so at once when nothing is waiting. */
int func_0013CAE8(void) {
    openrac_snd_request(answer_stop_streams, NULL, 0, 0, 0);
    return 1;
}

/* The IOP mixer's settings the game sends, which the native mixer has no equivalent of: a block of
 * effect settings (0x0013BF68), the voice ranges (0x0013C028), the voice allocation mode
 * (0x0013C068) and a three-word setting of the mixer's (0x0013C230). The reverb (0x0013CF80) goes
 * to the native reverb. */
int func_0013BF68(int value, gaddr settings) {
    const int32_t args[2] = {value, (int32_t)settings};
    openrac_snd_request(NULL, args, 2, 0, 0);
    return 1;
}

int func_0013C028(int a, int b, int c) {
    const int32_t args[3] = {a, b, c};
    openrac_snd_request(NULL, args, 3, 0, 0);
    return 1;
}

int func_0013C068(int mode) {
    const int32_t args[1] = {mode};
    openrac_snd_request(NULL, args, 1, 0, 0);
    return 1;
}

/* snd_SetReverbEx(core, type, depth, delay, feedback): the native reverb (audio/reverb.h) takes the
 * effect when the request is sent. */
static int answer_reverb(const int32_t* a) {
    openrac_sound_set_reverb(a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

int func_0013CF80(int core, int type, int depth, int delay, int feedback) {
    const int32_t args[5] = {core, type, depth, delay, feedback};
    openrac_snd_request(answer_reverb, args, 5, 0, 0);
    return 1;
}

int func_0013C230(int a, int b, int c) {
    const int32_t args[3] = {a, b, c};
    openrac_snd_request(NULL, args, 3, 0, 0);
    return 1;
}

/* The movie's sound (0x0013D080, 0x0013D0F8, 0x0013D120, 0x0013D158): synchronous requests the movie
 * player has no use for, since it plays its sound itself; answered 0 once the callbacks out ran. */
int func_0013D080(int a, int b, int c, int d, int e, int f) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;
    return openrac_snd_request_sync(NULL, NULL);
}

int func_0013D0F8(void) {
    return openrac_snd_request_sync(NULL, NULL);
}

int func_0013D120(int a, int b, int c, int d, int e) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    return openrac_snd_request_sync(NULL, NULL);
}

int func_0013D158(void) {
    return openrac_snd_request_sync(NULL, NULL);
}

static int answer_stop(const int32_t* args) {
    openrac_lib_snd_StopSound(args[0]);
    return 0;
}

/* snd_StopSound as a request (the multiplayer program's 0x00193D98, which the port leaves to its
 * library as retail does): the sound ends when its request is sent. */
int rac3_snd_stop_request(int handle) {
    const int32_t args[1] = {handle};
    openrac_snd_request(answer_stop, args, 1, 0, 0);
    return 1;
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

/* snd_BankLoadFromEE_CB (0x0013BC30): the shared load (the bank read at once, the callback told its
 * handle). Retail ends in sceSifCallRpc and hands its result on, 0 when the call was sent; the
 * multiplayer menu (func_L39_005A61D8) reads it. */
int func_0013BC30(int bank, int callback, long long argument) {
    openrac_lib_snd_BankLoadFromEE_CB(bank, callback, argument);
    return 0;
}
