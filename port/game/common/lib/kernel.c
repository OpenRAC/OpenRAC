/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The EE kernel and the services of the I/O processor that the game reaches
 * through it (libraries.tsv). The port has no second processor and no
 * kernel: the game is single-threaded and polls, so these keep its handlers
 * (called where the console would raise the interrupt), answer at once, and
 * leave the IOP's services to the replacements of the libraries above them
 * (disc, memory card, pads, sound). */
#include <stdio.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"

#define MAX_HANDLERS 8

typedef struct {
    int cause;
    gaddr handler;
    int arg;
} handler;

static handler intc[MAX_HANDLERS], dmac[MAX_HANDLERS];
static gaddr vsync_callback;

static int add(handler* table, int cause, gaddr fn, int arg) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (table[i].handler == 0) {
            table[i] = (handler){cause, fn, arg};
            return i + 1;
        }
    }
    return -1;
}

static int remove_handler(handler* table, int cause, int id) {
    if (id >= 1 && id <= MAX_HANDLERS && table[id - 1].cause == cause) {
        table[id - 1].handler = 0;
        return 0;
    }
    return -1;
}

/* The interrupt causes of the console's INTC that the game uses. */
#define INTC_VBLANK_START 2

void openrac_game_run_vsync_handlers(void) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (intc[i].handler != 0 && intc[i].cause == INTC_VBLANK_START) {
            GFN(int (*)(int), intc[i].handler)(intc[i].cause);
        }
    }
    if (vsync_callback != 0) {
        GFN(void (*)(void), vsync_callback)();
    }
}

void openrac_game_run_dmac_handlers(int channel) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (dmac[i].handler != 0 && dmac[i].cause == channel) {
            GFN(int (*)(int), dmac[i].handler)(channel);
        }
    }
}

/* AddIntcHandler */
int openrac_lib_AddIntcHandler(int cause, gaddr fn, int next) {
    return add(intc, cause, fn, next);
}

/* RemoveIntcHandler */
int openrac_lib_RemoveIntcHandler(int cause, int id) {
    return remove_handler(intc, cause, id);
}

/* AddDmacHandler */
int openrac_lib_AddDmacHandler(int channel, gaddr fn, int next) {
    return add(dmac, channel, fn, next);
}

/* RemoveDmacHandler */
int openrac_lib_RemoveDmacHandler(int channel, int id) {
    return remove_handler(dmac, channel, id);
}

/* sceGsSyncVCallback: the previous callback is returned. */
gaddr openrac_lib_sceGsSyncVCallback(gaddr fn) {
    gaddr old = vsync_callback;
    vsync_callback = fn;
    return old;
}

/* Interrupt and DMA controller masks: nothing to mask. */
int openrac_lib_EnableIntc(int cause) {
    (void)cause;
    return 0;
} /* EnableIntc */

int openrac_lib_DisableIntc(int cause) {
    (void)cause;
    return 0;
} /* DisableIntc */

int openrac_lib_DisableDmac(int channel) {
    (void)channel;
    return 0;
} /* DisableDmac */

int openrac_lib_EnableDmac(int channel) {
    (void)channel;
    return 0;
} /* EnableDmac */

int openrac_lib_DIntr(void) {
    return 1;
} /* DIntr */

int openrac_lib_EIntr(void) {
    return 1;
} /* EIntr */

/* Threads: only the movie player makes them, and it is to be replaced as a
 * whole (libraries.tsv, libmpeg). They are given ids and never run. */
static int next_thread = 2;

int openrac_lib_CreateThread(gaddr param) {
    (void)param;
    return next_thread++;
} /* CreateThread */

int openrac_lib_DeleteThread(int id) {
    (void)id;
    return 0;
} /* DeleteThread */

int openrac_lib_StartThread(int id, gaddr arg) { /* StartThread */
    (void)id;
    (void)arg;
    openrac_guest_missing("StartThread (threads do not run in the port)");
    return 0;
}

int openrac_lib_TerminateThread(int id) {
    (void)id;
    return 0;
} /* TerminateThread */

int openrac_lib_ChangeThreadPriority(int id, int priority) {
    (void)id;
    return priority;
} /* ChangeThreadPriority */

int openrac_lib_RotateThreadReadyQueue(int priority) {
    (void)priority;
    return 0;
} /* RotateThreadReadyQueue */

int openrac_lib_GetThreadId(void) {
    return 1;
} /* GetThreadId */

/* Semaphores: counted, never waited on (one thread). */
#define MAX_SEMAS 32
static int semas[MAX_SEMAS];
static int sema_used[MAX_SEMAS];

int openrac_lib_CreateSema(gaddr params) { /* CreateSema: {count, max_count, init_count, ...} */
    for (int i = 0; i < MAX_SEMAS; i++) {
        if (!sema_used[i]) {
            sema_used[i] = 1;
            semas[i] = GREF(int, params + 8);
            return i + 1;
        }
    }
    return -1;
}

int openrac_lib_DeleteSema(int id) { /* DeleteSema */
    if (id >= 1 && id <= MAX_SEMAS) {
        sema_used[id - 1] = 0;
    }
    return id;
}

int openrac_lib_SignalSema(int id) { /* SignalSema */
    if (id >= 1 && id <= MAX_SEMAS) {
        semas[id - 1]++;
    }
    return id;
}

int openrac_lib_WaitSema(int id) { /* WaitSema */
    if (id >= 1 && id <= MAX_SEMAS && semas[id - 1] > 0) {
        semas[id - 1]--;
    }
    return id;
}

/* Caches: nothing to flush. */
int openrac_lib_EnableCache(int which) {
    (void)which;
    return 0;
} /* EnableCache */

void openrac_lib_FlushCache(int which) {
    (void)which;
} /* FlushCache */

/* The IOP: no second processor. Transfers to it complete at once, its RPC
 * servers are bound and answer, its heap hands out addresses that are never
 * used, modules load. */
int openrac_lib_sceSifDmaStat(unsigned int id) {
    (void)id;
    return -1;
} /* sceSifDmaStat: done */

unsigned int openrac_lib_sceSifSetDma(gaddr transfers, int count) { /* sceSifSetDma */
    (void)transfers;
    (void)count;
    return 1;
}

void openrac_lib_sceSifInitRpc(int mode) {
    (void)mode;
} /* sceSifInitRpc */

int openrac_lib_sceSifBindRpc(gaddr client, unsigned int number, int mode) { /* sceSifBindRpc */
    (void)client;
    (void)number;
    (void)mode;
    return 0;
}

int openrac_lib_sceSifCallRpc(
    gaddr client,
    int number,
    int mode,
    gaddr send,
    int ssize,
    gaddr receive,
    int rsize,
    gaddr end,
    gaddr end_param
) { /* sceSifCallRpc */
    (void)client;
    (void)number;
    (void)mode;
    (void)send;
    (void)ssize;
    (void)receive;
    (void)rsize;
    (void)end;
    (void)end_param;
    return 0;
}

int openrac_lib_sceSifCheckStatRpc(gaddr client) {
    (void)client;
    return 0;
} /* sceSifCheckStatRpc: idle */

int openrac_lib_sceSifInitIopHeap(void) {
    return 0;
} /* sceSifInitIopHeap */

int openrac_lib_sceSifAllocIopHeap(int size) {
    (void)size;
    return 0x00100000;
} /* sceSifAllocIopHeap */

int openrac_lib_sceSifFreeIopHeap(int at) {
    (void)at;
    return 0;
} /* sceSifFreeIopHeap */

int openrac_lib_sceSifLoadModuleBuffer(
    gaddr module, int size, gaddr args
) { /* sceSifLoadModuleBuffer */
    (void)module;
    (void)size;
    (void)args;
    return 1;
}

int openrac_lib_sceSifSyncIop(void) {
    return 1;
} /* sceSifSyncIop */

int openrac_lib_sceSifRebootIop(gaddr image) {
    (void)image;
    return 1;
} /* sceSifRebootIop */

/* fileio: the game reads no asset this way (debug output and rom0: settings
 * only). Writes to the console's terminal go to the log. */
int openrac_lib_sceFsReset(void) {
    return 0;
} /* sceFsReset */

int openrac_lib_sceOpen(gaddr name, int flags, ...) { /* sceOpen */
    (void)flags;
    fprintf(stderr, "[game] sceOpen(\"%s\"): no files in the port\n", (const char*)G(name));
    return -1;
}

int openrac_lib_sceClose(int fd) {
    (void)fd;
    return 0;
} /* sceClose */

int openrac_lib_sceLseek(int fd, int offset, int whence) { /* sceLseek */
    (void)fd;
    (void)offset;
    (void)whence;
    return -1;
}

int openrac_lib_sceRead(int fd, gaddr buf, int size) { /* sceRead */
    (void)fd;
    (void)buf;
    (void)size;
    return -1;
}

int openrac_lib_sceWrite(int fd, gaddr buf, int size) { /* sceWrite */
    if (fd == 1 || fd == 2) {
        fprintf(stderr, "[game] %.*s", size, (const char*)G(buf));
        return size;
    }
    return -1;
}

/* The scf library: the console's system settings. */
int openrac_lib_sceScfGetLanguage(void) { /* sceScfGetLanguage */
    return openrac_game_language;
}

void openrac_lib_sceScfGetLocalTimefromRTC(gaddr clock
) { /* sceScfGetLocalTimefromRTC: the clock is local already */
    (void)clock;
}
