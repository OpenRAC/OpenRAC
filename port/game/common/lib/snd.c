/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The sound library (989snd), replaced at its API: the cleanest cut, since
 * everything below it ran on the IOP (docs/port/RAC1_PAL_SURVEY.md, section
 * 8). Until the port's sound engine exists (banks, voices, VAG streams,
 * music, movie sound: docs/port/ROADMAP.md), the game is silent: banks
 * "load", sounds finish at once, streams are never buffered. Disc reads made
 * through the library's stream-safe calls go to the disc replacement. */
#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"

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

static unsigned int next_bank = 1;

unsigned int openrac_lib_snd_BankLoadByLoc(int sector, int offset) { /* snd_BankLoadByLoc */
    (void)sector;
    (void)offset;
    return next_bank++;
}

void openrac_lib_snd_BankLoadFromEE_CB(int a, int b, long long callback) {
    (void)a;
    (void)b;
    (void)callback;
} /* snd_BankLoadFromEE_CB */

void openrac_lib_snd_ResolveBankXREFS(void) {} /* snd_ResolveBankXREFS */

void openrac_lib_snd_UnloadBank(int bank) {
    (void)bank;
} /* snd_UnloadBank */

void openrac_lib_snd_SetMasterVolume(int which, int volume) {
    (void)which;
    (void)volume;
} /* snd_SetMasterVolume */

void openrac_lib_snd_SetPlaybackMode(int mode) {
    (void)mode;
} /* snd_SetPlaybackMode */

void openrac_lib_snd_StopAllSounds(void) {} /* snd_StopAllSounds */

void openrac_lib_snd_PauseAllSoundsInGroup(int group) {
    (void)group;
} /* snd_PauseAllSoundsInGroup */

void openrac_lib_snd_ContinueAllSoundsInGroup(int group) {
    (void)group;
} /* snd_ContinueAllSoundsInGroup */

void openrac_lib_snd_SoundIsStillPlaying_CB(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
} /* snd_SoundIsStillPlaying_CB */

void openrac_lib_snd_reset_state_and_flush_commands(void) {
} /* snd_reset_state_and_flush_commands */

int openrac_lib_snd_InitVAGStreamingEx(int a, int b, int c, int d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    return 0;
} /* snd_InitVAGStreamingEx */

void openrac_lib_snd_StopAllStreams(void) {} /* snd_StopAllStreams */

void openrac_lib_snd_PlayVAGStreamByLocEx_CB(
    int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, long long k
) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;
    (void)g;
    (void)h;
    (void)i;
    (void)j;
    (void)k;
} /* snd_PlayVAGStreamByLocEx_CB */

void openrac_lib_snd_PauseVAGStream(int stream) {
    (void)stream;
} /* snd_PauseVAGStream */

void openrac_lib_snd_ContinueVAGStream(int stream) {
    (void)stream;
} /* snd_ContinueVAGStream */

void openrac_lib_snd_GetVAGStreamTimeRemaining_CB(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
} /* snd_GetVAGStreamTimeRemaining_CB */

void openrac_lib_snd_IsVAGStreamBuffered_CB(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
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
