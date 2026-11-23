/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's 989snd, replaced at its API.
 *
 * The game's sound client turns each call into a request for the IOP's sound driver, sent by RPC:
 * at once and waited for (snd_SendCommandSync, 0x00132C48) or queued for the next pump
 * (snd_QueueCommandToRing, 0x00132DF0). The port replaces the client's entry points instead
 * (libraries.tsv): those every game shares go to lib/snd.c, which answers them with the native
 * sound engine and hands the answers to their callbacks at the next pump, as the library does; the
 * ones below configure the IOP's own mixer, which the native engine has no equivalent of, and are
 * taken as requests with nothing to do, so the callbacks still run at the same pumps. Nothing is
 * left that sends a request to an IOP.
 */
#include <stdint.h>
#include <stdio.h>

#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "rac2.h"

/* snd_SendIOPCommandNoWait (0x00132DF0) and snd_SendCommandSync (0x00132C48): every entry point
 * that reached them is replaced, so nothing should call them. A call is said once and answered 0. */
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

int func_00132DF0(int command_arg, int bytes, int data_arg, int callback, long long callback_data) {
    (void)bytes;
    (void)data_arg;
    (void)callback;
    (void)callback_data;
    unreachable("snd_SendIOPCommandNoWait", command_arg);
    return 0;
}

int func_00132C48(int command_arg, int bytes, int data_arg) {
    (void)bytes;
    (void)data_arg;
    unreachable("snd_SendCommandSync", command_arg);
    return 0;
}

/* The IOP mixer's settings the game sends (one, two, three, five or seven words): the mixer mode
 * (0x00132978), the reverb (0x00133710), the effect settings block (0x001328C0) and the voice
 * ranges (0x001329B0). */
void func_00132978(int a, int b) {
    const int32_t args[2] = {a, b};
    openrac_snd_request(NULL, args, 2, 0, 0);
}

void func_00133710(int a, int b, int c, int d, int e) {
    const int32_t args[5] = {a, b, c, d, e};
    openrac_snd_request(NULL, args, 5, 0, 0);
}

void func_001328C0(int value, gaddr settings) {
    const int32_t args[2] = {value, (int32_t)settings};
    openrac_snd_request(NULL, args, 2, 0, 0);
}

void func_001329B0(int a, int b, int c) {
    const int32_t args[3] = {a, b, c};
    openrac_snd_request(NULL, args, 3, 0, 0);
}

/* The synchronous requests of the movie's sound (0x001337F0, 0x00133820, 0x00133850, 0x001338C8:
 * the movie player's own, which plays its sound itself): nothing to do once the callbacks out
 * have run. */
void func_001337F0(void) {
    (void)openrac_snd_request_sync(NULL, NULL);
}

void func_00133820(void) {
    (void)openrac_snd_request_sync(NULL, NULL);
}

void func_00133850(int a, int b, int c, int d, int e, int f) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;
    (void)openrac_snd_request_sync(NULL, NULL);
}

void func_001338C8(void) {
    (void)openrac_snd_request_sync(NULL, NULL);
}

static int answer_start(const int32_t* args) {
    (void)args;
    return openrac_lib_snd_StartSoundSystem();
}

/* snd_StartSoundSystem (0x00131DE8): binds the two sound servers and sends the first request; the
 * host's audio owns the device, so only the request's wait is left. */
int func_00131DE8(int options) {
    (void)options;
    return openrac_snd_request_sync(answer_start, NULL);
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
