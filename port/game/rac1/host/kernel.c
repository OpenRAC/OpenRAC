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

#include "game_protos.h"
#include "rac1_host.h"

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

void openrac_rac1_run_vsync_handlers(void) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (intc[i].handler != 0 && intc[i].cause == INTC_VBLANK_START) {
            GFN(int (*)(int), intc[i].handler)(intc[i].cause);
        }
    }
    if (vsync_callback != 0) {
        GFN(void (*)(void), vsync_callback)();
    }
}

void openrac_rac1_run_dmac_handlers(int channel) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (dmac[i].handler != 0 && dmac[i].cause == channel) {
            GFN(int (*)(int), dmac[i].handler)(channel);
        }
    }
}

/* AddIntcHandler */
int func_00118A90(int cause, gaddr fn, int next) {
    return add(intc, cause, fn, next);
}

/* RemoveIntcHandler */
int func_00118AA0(int cause, int id) {
    return remove_handler(intc, cause, id);
}

/* AddDmacHandler */
int func_00118AB0(int channel, gaddr fn, int next) {
    return add(dmac, channel, fn, next);
}

/* RemoveDmacHandler */
int func_00118AD0(int channel, int id) {
    return remove_handler(dmac, channel, id);
}

/* sceGsSyncVCallback: the previous callback is returned. */
gaddr func_00123168(gaddr fn) {
    gaddr old = vsync_callback;
    vsync_callback = fn;
    return old;
}

/* Interrupt and DMA controller masks: nothing to mask. */
int func_00119328(int cause) {
    (void)cause;
    return 0;
} /* EnableIntc */

int func_00119390(int cause) {
    (void)cause;
    return 0;
} /* DisableIntc */

int func_001193F8(int channel) {
    (void)channel;
    return 0;
} /* DisableDmac */

int func_00119460(int channel) {
    (void)channel;
    return 0;
} /* EnableDmac */

int func_0011D960(void) {
    return 1;
} /* DIntr */

int func_0011D9A8(void) {
    return 1;
} /* EIntr */

/* Threads: only the movie player makes them, and it is to be replaced as a
 * whole (libraries.tsv, libmpeg). They are given ids and never run. */
static int next_thread = 2;

int func_00118B50(gaddr param) {
    (void)param;
    return next_thread++;
} /* CreateThread */

int func_00118B60(int id) {
    (void)id;
    return 0;
} /* DeleteThread */

int func_00118B70(int id, gaddr arg) { /* StartThread */
    (void)id;
    (void)arg;
    openrac_guest_missing("StartThread (threads do not run in the port)");
    return 0;
}

int func_00118B80(int id) {
    (void)id;
    return 0;
} /* TerminateThread */

int func_00118BA0(int id, int priority) {
    (void)id;
    return priority;
} /* ChangeThreadPriority */

int func_00118BC0(int priority) {
    (void)priority;
    return 0;
} /* RotateThreadReadyQueue */

int func_00118BE0(void) {
    return 1;
} /* GetThreadId */

/* Semaphores: counted, never waited on (one thread). */
#define MAX_SEMAS 32
static int semas[MAX_SEMAS];
static int sema_used[MAX_SEMAS];

int func_00118C70(gaddr params) { /* CreateSema: {count, max_count, init_count, ...} */
    for (int i = 0; i < MAX_SEMAS; i++) {
        if (!sema_used[i]) {
            sema_used[i] = 1;
            semas[i] = GREF(int, params + 8);
            return i + 1;
        }
    }
    return -1;
}

int func_00118C80(int id) { /* DeleteSema */
    if (id >= 1 && id <= MAX_SEMAS) {
        sema_used[id - 1] = 0;
    }
    return id;
}

int func_00118C90(int id) { /* SignalSema */
    if (id >= 1 && id <= MAX_SEMAS) {
        semas[id - 1]++;
    }
    return id;
}

int func_00118CB0(int id) { /* WaitSema */
    if (id >= 1 && id <= MAX_SEMAS && semas[id - 1] > 0) {
        semas[id - 1]--;
    }
    return id;
}

/* Caches: nothing to flush. */
int func_00118D60(int which) {
    (void)which;
    return 0;
} /* EnableCache */

void func_00118D80(int which) {
    (void)which;
} /* FlushCache */

/* The IOP: no second processor. Transfers to it complete at once, its RPC
 * servers are bound and answer, its heap hands out addresses that are never
 * used, modules load. */
int func_00118E10(unsigned int id) {
    (void)id;
    return -1;
} /* sceSifDmaStat: done */

unsigned int func_00118E20(gaddr transfers, int count) { /* sceSifSetDma */
    (void)transfers;
    (void)count;
    return 1;
}

void func_0011AE20(int mode) {
    (void)mode;
} /* sceSifInitRpc */

int func_0011B2F8(gaddr client, unsigned int number, int mode) { /* sceSifBindRpc */
    (void)client;
    (void)number;
    (void)mode;
    return 0;
}

int func_0011B4C8(
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

int func_0011B6B8(gaddr client) {
    (void)client;
    return 0;
} /* sceSifCheckStatRpc: idle */

int func_0011CB40(void) {
    return 0;
} /* sceSifInitIopHeap */

int func_0011CBC8(int size) {
    (void)size;
    return 0x00100000;
} /* sceSifAllocIopHeap */

int func_0011CCB0(int at) {
    (void)at;
    return 0;
} /* sceSifFreeIopHeap */

int func_0011D078(gaddr module, int size, gaddr args) { /* sceSifLoadModuleBuffer */
    (void)module;
    (void)size;
    (void)args;
    return 1;
}

int func_0011D210(void) {
    return 1;
} /* sceSifSyncIop */

int func_0011D248(gaddr image) {
    (void)image;
    return 1;
} /* sceSifRebootIop */

/* fileio: the game reads no asset this way (debug output and rom0: settings
 * only). Writes to the console's terminal go to the log. */
int func_0011BF48(void) {
    return 0;
} /* sceFsReset */

int func_0011BF80(gaddr name, int flags, ...) { /* sceOpen */
    (void)flags;
    fprintf(stderr, "[game] sceOpen(\"%s\"): no files in the port\n", (const char*)G(name));
    return -1;
}

int func_0011C208(int fd) {
    (void)fd;
    return 0;
} /* sceClose */

int func_0011C388(int fd, int offset, int whence) { /* sceLseek */
    (void)fd;
    (void)offset;
    (void)whence;
    return -1;
}

int func_0011C5C0(int fd, gaddr buf, int size) { /* sceRead */
    (void)fd;
    (void)buf;
    (void)size;
    return -1;
}

int func_0011C820(int fd, gaddr buf, int size) { /* sceWrite */
    if (fd == 1 || fd == 2) {
        fprintf(stderr, "[game] %.*s", size, (const char*)G(buf));
        return size;
    }
    return -1;
}

/* The scf library: the console's system settings. */
int func_0012D380(void) { /* sceScfGetLanguage */
    return openrac_rac1_language;
}

void func_0012D818(gaddr clock) { /* sceScfGetLocalTimefromRTC: the clock is local already */
    (void)clock;
}
