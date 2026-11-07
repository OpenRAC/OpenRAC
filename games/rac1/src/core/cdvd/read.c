#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/cdvd/callbacks.h"
#include "data/executable.h"
#include "asm_functions/executable.h"

/*
 * core_text object 0x121750-0x121D18. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Six Sony SDK archive members back to back (each ends 8-byte aligned, so
 * no fill separates them): libcdvd's cdvd005.o (sceCdRead), cdvd015.o
 * (sceCdGetError, sce_cd_get_error), cdvd018.o (sceCdBreak, sce_cd_break)
 * and cdvd039.o (sceCdReadClock), then libgraph's graph001.o
 * (sceGsResetGraph, sceGsGetGParam = sceGsGetGParam). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt archives they match.
 */

extern void scePrintf();

/* sceCdRead: queue one CD/DVD read request (lba, sectors, buffer, mode)
 * in the request block at native_cd_read_request and start it through the RPC
 * sif_call_rpc. Returns 1 when the RPC was accepted, 0 otherwise.
 * The volatile state clears precede the semaphore read; the verified
 * GNU 2.10-ee profile fills SignalSema's call slot with that word load.
 * Sector size by mode->datapattern: 1 -> 0x918, 2 -> 0x924, else 0x800. */

extern int cd_read_mode_flags NOT_SDA;
extern char native_cd_read_request[];
extern char native_cd_read_completion_buffer[];
extern int native_cd_read_response NOT_SDA;
extern char native_cd_read_submit_debug_message[];
extern char native_cd_read_queued_debug_message[];
extern volatile int cd_callback_num;
extern void sceSifWriteBackDCache(void *, int);

int sce_cd_read(int lba, int sectors, void *buf, unsigned char *mode) {
    char *req = native_cd_read_request;
    int size;
    int r;

    if ((cd_read_mode_flags & 1) == 0) {
        if (cd_ncmd_disk_ready() == 6) {
            return 0;
        }
    }
    if (cd_ncmd_prechk(4) == 0) {
        return 0;
    }

    *(int *)(req + 0x0) = lba;
    *(int *)(req + 0x4) = sectors;
    *(void **)(req + 0x8) = buf;
    req[0xC] = mode[0];
    req[0xD] = mode[1];
    req[0xE] = mode[2];
    *(void **)(req + 0x10) = native_cd_read_completion_buffer;
    *(void **)(req + 0x14) = &native_cd_read_response;

    switch (mode[2]) {
        case 1:
            size = sectors * 0x918;
            break;
        case 2:
            size = sectors * 0x924;
            break;
        case 0:
        default:
            size = sectors << 11;
            break;
    }

    native_cd_read_response = 0;
    if ((*(volatile int *)&cd_read_mode_flags & 2) == 0) {
        sceSifWriteBackDCache(buf, size);
    }
    sceSifWriteBackDCache(native_cd_read_completion_buffer, 0x90);
    sceSifWriteBackDCache(req, 0x18);
    sceSifWriteBackDCache(&native_cd_read_response, 4);
    if (cd_debug > 0) {
        scePrintf(native_cd_read_submit_debug_message);
    }

    cd_callback_num = 1;
    *(volatile int *)&cd_read_pending = 1;
    r = sif_call_rpc(cd_ncmd_client, 1, 1, req, 0x18, 0, 0, (void *)cd_read_end_callback,
                     native_cd_read_completion_buffer);
    if (r < 0) {
        cd_callback_num = 0;
        *(volatile int *)&cd_read_pending = 0;
        signal_semaphore(*(volatile int *)&cd_ncmd_sema);
        return 0;
    }
    if (cd_debug > 0) {
        scePrintf(native_cd_read_queued_debug_message);
    }
    return 1;
}

/* sceCdGetError, cd_ncmd_disk_ready's sibling: S-command 3, RPC 4, and -1
   rather than 0 as the failure result.

   The RPC failure is handled first (SignalSema, return -1), then the
   success path. That is the layout retail has, and it keeps 2.9-ee from
   cross-jumping the two `return -1` tails into one (SIZE 144/152 with
   the success path first, which 2.95.3 needed). */
int sce_cd_get_error(void) {
    int r;

    if (cd_scmd_prechk(3) == 0) {
        return -1;
    }
    if (sif_call_rpc(cd_scmd_client, 4, 0, 0, 0, &cd_scmd_result, 4, 0, 0) < 0) {
        signal_semaphore(cd_scmd_sema);
        return -1;
    }
    r = *(int *)((unsigned int)&cd_scmd_result | 0x20000000);
    signal_semaphore(cd_scmd_sema);
    return r;
}

extern volatile int cd_callback_num;

/* sceCdBreak, sibling of sce_cd_get_error: S-command 0x1E, RPC 0x16 on
   cd_scmd_client under the cd_scmd_sema semaphore, with the cd_callback_num busy flag
   set around it; returns the reply word read uncached. The volatile flag
   and semaphore reads stay out of the delay slots, where reorg never puts
   a volatile access (this module sees its globals as volatile, as
   cdvd000.o does).

   As in sce_cd_get_error, the RPC failure path comes first: with the success
   path first 2.9-ee merges the two `return 0` tails (SIZE 176/184). */
int sce_cd_break(void) {
    int r;

    if (cd_scmd_prechk(0x1E) == 0) {
        return 0;
    }
    cd_callback_num = 8;
    if (sif_call_rpc(cd_scmd_client, 0x16, 0, 0, 0, &cd_scmd_result, 4, 0, 0) < 0) {
        signal_semaphore(*(volatile int *)&cd_scmd_sema);
        cd_callback_num = 0;
        return 0;
    }
    cd_callback_num = 0;
    r = *(int *)((unsigned int)&cd_scmd_result | 0x20000000);
    signal_semaphore(*(volatile int *)&cd_scmd_sema);
    return r;
}

extern char cd_read_clock_before_rpc_debug_message[];
extern char cd_read_clock_after_copy_debug_message[];
typedef struct {
    char b[8];
} CdClock;

/* sceCdReadClock: S-command 0xF, RPC 1 with a 0x10-byte reply; the
   8-byte clock after the status word is copied out uncached. Failure
   path first, as in sce_cd_get_error. */
int sce_cd_read_clock(CdClock *out) {
    int r;

    if (cd_scmd_prechk(0xF) == 0) {
        return 0;
    }
    if (cd_debug > 0) {
        scePrintf(cd_read_clock_before_rpc_debug_message);
    }
    if (sif_call_rpc(cd_scmd_client, 1, 0, 0, 0, &cd_scmd_result, 0x10, 0, 0) < 0) {
        signal_semaphore(cd_scmd_sema);
        return 0;
    }
    *out = *(CdClock *)(((unsigned int)&cd_scmd_result + 4) | 0x20000000);
    if (cd_debug > 0) {
        scePrintf(cd_read_clock_after_copy_debug_message);
    }
    r = *(int *)((unsigned int)&cd_scmd_result | 0x20000000);
    signal_semaphore(cd_scmd_sema);
    return r;
}

typedef struct {
    short sceGsInterMode;
    short sceGsOutMode;
    short sceGsFFMode;
    short sceGsVersion;
    volatile int (*sceGsVSCfunc)(int);
    int sceGsVSCid;
} sceGsGParam;

extern void *sceGsGetGParam(void);          /* sceGsGetGParam */

/* sceGsResetGraph. Adapted from Lombyte's project (GPL-3.0) for PAL. */
void sceGsResetGraph(short mode, short inter, short out, short ff) {
    sceGsGParam *gp;

    switch (mode) {
        case 0:
            gp = (sceGsGParam *)sceGsGetGParam();
            *(volatile unsigned long *)0x12001000 = 0x200;
            gp->sceGsInterMode = inter;
            gp->sceGsOutMode = out;
            gp->sceGsVersion = (short)((*(volatile unsigned long *)0x12001000 >> 16) & 0xFF);
            gs_put_imr(0xFF00);
            gp->sceGsFFMode = (ff != 0);
            if (gp->sceGsVSCfunc != 0) {
                disable_intc_atomic(2);
                remove_intc_handler(2, gp->sceGsVSCid);
                gp->sceGsVSCfunc = 0;
                gp->sceGsVSCid = 0;
            }
            set_gs_crt(inter & 1, out & 0xFF, ff & 1);
            break;
        case 1:
            *(volatile unsigned long *)0x12001000 = 0x100;
            break;
        case 5:
            gp = (sceGsGParam *)sceGsGetGParam();
            gp->sceGsFFMode = (ff != 0);
            gp->sceGsInterMode = inter;
            gp->sceGsOutMode = out;
            gp->sceGsVersion = (short)((*(volatile unsigned long *)0x12001000 >> 16) & 0xFF);
            set_gs_crt(inter & 1, out & 0xFF, ff & 1);
            break;
    }
}

extern char CoreDataTable[];

void *sceGsGetGParam(void) {
    return CoreDataTable;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00121D14);
