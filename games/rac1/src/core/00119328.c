#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x119328-0x119868. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), three members of libkernl.a back to
 * back: intr.o (DisableIntc/EnableIntc/DisableDmac/EnableDmac), thread.o
 * (topThread, InitThread = func_001195A0, iWakeupThread) and deci2.o
 * (sceDeci2Open ... kputs). Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt archive it matches.
 */

/* Declarations in scope here before the split. */
extern long strtol(int arg0, int arg1, int arg2);
extern int SbrkErrno;
extern void *_impure_ptr NOT_SDA;
extern int __sread(void *arg0, void *arg1, void *arg2);
extern int __swrite(void *arg0, void *arg1, void *arg2);
extern long __sseek(void *arg0, void *arg1, void *arg2);
extern void __sclose(void *arg0);
extern void libc_fflush(void);
extern void _fwalk(void *, void *);
extern char locale_lconv[];
extern int libc_glue_read();
extern long func_00116108_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00116108");
extern long func_001188C8_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_001188C8");
extern long func_00114518_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00114518");
extern int _close_r(int *errOut, int arg1);
extern int __sfvwrite(int, void *);
extern void __sinit(void *);
extern void _vfprintf_r(void *, void *, int, int);
extern int libc_glue_write();
extern int CoreGlobalWord NOT_SDA;

ASM_FUNC("asm/handwritten/core_text", func_00119328);

ASM_FUNC("asm/handwritten/core_text", func_00119390);

ASM_FUNC("asm/handwritten/core_text", func_001193F8);

ASM_FUNC("asm/handwritten/core_text", func_00119460);

typedef struct {
    unsigned char type;
    unsigned char id;
} TopThreadEntry;

typedef struct {
    int counter;
    unsigned int pad;
    TopThreadEntry entries[0x200];
} TopThreadContext;

extern int top_thread_semaphore;
extern char top_thread_bad_request_message[];
extern int wait_semaphore(int);
extern int WakeupThread(int);
extern int rotate_thread_ready_queue(int);
extern int SuspendThread(int);
extern int kprintf(const char *, ...);

/* topThread: kernel service dispatcher. Adapted from Lombyte's project (GPL-3.0) for PAL. */
void topThread(void *arg0) {
    TopThreadContext *ctx = (TopThreadContext *)arg0;
    int i;
    unsigned char type;

    for (;;) {
        wait_semaphore(top_thread_semaphore);
        i = ctx->counter & 0x1FF;
        ctx->counter = i + 1;
        type = ctx->entries[i].type;
        switch (type) {
        case 0:
            WakeupThread(ctx->entries[i].id);
            break;
        case 1:
            rotate_thread_ready_queue(ctx->entries[i].id);
            break;
        case 2:
            SuspendThread(ctx->entries[i].id);
            break;
        default:
            kprintf(top_thread_bad_request_message);
            break;
        }
    }
}

/*
 * InitThread: start the kernel-side service thread once. Bail with -1 if
 * the thread id D_0012FCF8 is already > 0; CreateSema (func_00118C70,
 * maxCount 0xFF, initCount 0) into D_00154600; CreateThread
 * (func_00118B50) with entry topThread (func_001194C8), stack D_00154200,
 * stackSize 0x400, gpReg &_gp (D_00166D00), initPriority 0, its id into
 * D_0012FCF8 (DeleteSema and -1 on failure); clear D_00154608[0..1],
 * StartThread(tid, D_00154608), ChangeThreadPriority(GetThreadId(), 1);
 * return D_0012FCF8 (re-read). Exact under 2.9-ee.
 *
 * Levers: Sony's real kernel.h structs -- ThreadParam is 12 ints with
 * `status` first (entry at +4, stack +8, stackSize +0xC, gpReg +0x10,
 * initPriority +0x14), SemaParam 6 ints -- which put the thread at sp+0
 * and the semaphore at sp+0x30 in a 0x80 frame (the old decode had a
 * 9-int struct with entry first); and the CreateSema result stored to
 * the global and the global tested, which gives retail's test of $v0
 * with the store in the bltz slot. Under 2.95.3 the CreateThread failure
 * block is laid out after the success path (48/212, same size).
 */
extern int create_semaphore(void *);
extern int create_thread(void *);
extern int start_thread(int, void *);
extern int change_thread_priority(int, int);
extern void topThread(void *);
extern int top_thread_id;
extern char top_thread_stack[];
extern char gp_base[];
extern int top_thread_queue[];

typedef struct {
    int status;
    void (*entry)(void *);
    void *stack;
    int stackSize;
    void *gpReg;
    int initPriority;
    int currentPriority;
    unsigned int attr;
    unsigned int option;
    int waitType;
    int waitId;
    int wakeupCount;
} ThreadParam_1195A0;

typedef struct {
    int currentCount;
    int maxCount;
    int initCount;
    int numWaitThreads;
    unsigned int attr;
    unsigned int option;
} SemaParam_1195A0;

int InitThread(void) {
    ThreadParam_1195A0 tp;
    SemaParam_1195A0 sp;
    int tid;

    if (top_thread_id > 0) {
        return -1;
    }
    sp.maxCount = 0xFF;
    sp.initCount = 0;
    top_thread_semaphore = create_semaphore(&sp);
    if (top_thread_semaphore < 0) {
        return -1;
    }
    tp.entry = topThread;
    tp.stack = top_thread_stack;
    tp.stackSize = 0x400;
    tp.gpReg = gp_base;
    tp.initPriority = 0;
    tid = create_thread(&tp);
    top_thread_id = tid;
    if (tid < 0) {
        delete_semaphore(top_thread_semaphore);
        return -1;
    }
    top_thread_queue[0] = 0;
    top_thread_queue[1] = 0;
    start_thread(tid, top_thread_queue);
    change_thread_priority(get_thread_id(), 1);
    return top_thread_id;
}

ASM_FUNC("asm/handwritten/core_text", func_00119678);

LINKER_REMNANT("asm/remnants/core_text", func_00119710);

extern int deci2_open_work_area;
/* Deci2Call (syscall 0x7C) returns the result; the file's later
   `extern void func_00118E90(int, void *)` drops it, hence the alias. */
extern int Deci2Call_ret(int, void *) __asm__("func_00118E90");

/* libkernel sceDeci2Open(protocol, opt, handler): Deci2Call(1, {protocol,
   opt, handler, uncached &D_00154A10}). Returning the call's value is
   what gives retail's schedule (tag complete before the stores, the &buf
   move ahead of the $ra save); the void spelling is 30/68. */
/* sceDeci2Open: returns Deci2Call's int result (func_00118E90 is
   Deci2Call, declared void in this file, so it is reached through an
   alias). */
int sceDeci2Open(unsigned short arg0, void *arg1, void *arg2) {
    unsigned int buf[4];

    buf[0] = arg0;
    buf[1] = (unsigned int)arg1;
    buf[2] = (unsigned int)arg2;
    buf[3] = (unsigned int)&deci2_open_work_area | 0x20000000;
    return Deci2Call_ret(1, buf);
}

LINKER_REMNANT("asm/remnants/core_text", func_00119760);

extern void Deci2Call(int arg0, void *arg1);

void sceDeci2ReqSend(int arg0, signed char arg1) {
    int buf[4];
    buf[0] = arg0;
    buf[1] = arg1;
    Deci2Call(0x3, buf);
}


void sceDeci2Poll(int arg0) {
    int local = arg0;
    Deci2Call(0x4, &local);
}

void isceDeci2ExRecv(int arg0, int arg1, unsigned short arg2) {
    int buf[4];
    buf[0] = arg0;
    buf[1] = arg1;
    buf[2] = arg2;
    Deci2Call(-0x5, buf);
}

void isceDeci2ExSend(int arg0, int arg1, unsigned short arg2) {
    int buf[4];
    buf[0] = arg0;
    buf[1] = arg1;
    buf[2] = arg2;
    Deci2Call(-0x6, buf);
}

LINKER_REMNANT("asm/remnants/core_text", func_00119830);

void kputs(int arg0) {
    int local = arg0;
    Deci2Call(0x10, &local);
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00119864);
