/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The sound library (989snd), replaced at its API: the cleanest cut, since
 * everything below it ran on the IOP (docs/port/RAC1_PAL_SURVEY.md, section
 * 8). Until the port's sound engine exists (banks, voices, VAG streams,
 * music, movie sound: docs/port/ROADMAP.md), the game is silent: banks
 * "load", sounds finish at once, streams are never buffered. Disc reads made
 * through the library's stream-safe calls go to the disc replacement. */
#include "game_protos.h"
#include "rac1_host.h"

int func_0012DB68(void) {
    return 0;
} /* snd_StartSoundSystem */

int func_0012DDC0(void) {
    return 0;
} /* snd_FlushSoundCommands */

static unsigned int next_bank = 1;

unsigned int func_0012E060(int sector, int offset) { /* snd_BankLoadByLoc */
    (void)sector;
    (void)offset;
    return next_bank++;
}

void func_0012E1C8(int a, int b, long long callback) {
    (void)a;
    (void)b;
    (void)callback;
} /* snd_BankLoadFromEE_CB */

void func_0012E2E8(void) {} /* snd_ResolveBankXREFS */

void func_0012E318(int bank) {
    (void)bank;
} /* snd_UnloadBank */

void func_0012E348(int which, int volume) {
    (void)which;
    (void)volume;
} /* snd_SetMasterVolume */

void func_0012E380(int mode) {
    (void)mode;
} /* snd_SetPlaybackMode */

void func_0012E4F8(void) {} /* snd_StopAllSounds */

void func_0012E528(int group) {
    (void)group;
} /* snd_PauseAllSoundsInGroup */

void func_0012E558(int group) {
    (void)group;
} /* snd_ContinueAllSoundsInGroup */

void func_0012E588(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
} /* snd_SoundIsStillPlaying_CB */

void func_0012EC40(void) {} /* snd_reset_state_and_flush_commands */

int func_0012EC60(int a, int b, int c, int d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    return 0;
} /* snd_InitVAGStreamingEx */

void func_0012ED10(void) {} /* snd_StopAllStreams */

void func_0012ED48(
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

void func_0012EDB0(int stream) {
    (void)stream;
} /* snd_PauseVAGStream */

void func_0012EDE0(int stream) {
    (void)stream;
} /* snd_ContinueVAGStream */

void func_0012EE10(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
} /* snd_GetVAGStreamTimeRemaining_CB */

void func_0012EE40(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
} /* snd_IsVAGStreamBuffered_CB */

void func_0012EE70(int a) {
    (void)a;
} /* snd_StreamSafeCheckCDIdle */

int func_0012EE98(int sector, int count, int buffer) { /* snd_StreamSafeCdRead */
    return func_00121750((unsigned int)sector, (unsigned int)count, (gaddr)buffer, 0);
}

int func_0012EF48(int mode) {
    return func_00120F30(mode);
} /* snd_StreamSafeCdSync */

int func_0012EFE8(void) {
    return func_001219C8();
} /* snd_StreamSafeCdBreak */

int func_0012F030(void) {
    return func_00121930();
} /* snd_StreamSafeCdGetError */

int func_0012F068(int callback) {
    (void)callback;
    return 0;
} /* snd_StreamSafeCdCallback */

void func_0012F0E8(int a, int b) {
    (void)a;
    (void)b;
} /* snd_PreAllocReverbWorkArea */

void func_0012F120(int a, int b, int c, int d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
} /* snd_AutoReverb */

void func_0012F1E8(void) {} /* snd_ResetMovieSound */

void func_0012F248(int a, int b, int c, int d, int e) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
} /* snd_StartMovieSound */

int func_0012F288(int a, int b) {
    (void)a;
    (void)b;
    return 0;
} /* snd_UpdateMovieADPCM */

int func_0012F2B8(void) {
    return 0;
} /* snd_GetMovieNAX */

int func_0012F2E0(int a) {
    (void)a;
    return 0;
} /* snd_GetDopplerPitchMod */
