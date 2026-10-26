#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x119868-0x119D88. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), tty.o: the deci2 TTY queue and
 * protocol handler (sceTtyHandler, sceTtyWrite, sceTtyRead, sceTtyInit).
 * Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt
 * libkernl.a.
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
extern void Deci2Call(int arg0, void *arg1);
extern void _Bfree(void *arg0, void *arg1);
extern int func_001160D8(void);

/*
 * Close but not exact, same open-question category as func_001160D8/
 * func_00115578 (scratch-register/scheduling choice) but manifesting as
 * store reordering instead: retail schedules `self->field8 = ...;
 * self->field4 = 0;` before the branch and puts `self->fieldC = ...` in
 * the delay slot; this compiler schedules fieldC and field8 before the
 * branch and puts field4's store in the delay slot instead. Confirmed
 * source-order independent -- tried every permutation of the 3
 * assignments, all four produced the identical instruction sequence, so
 * this is the scheduler's own choice, not something this source
 * controls. Logic (D_00154A40's first field = arg0, then fields at
 * 0x4/0x8/0xC of the pointed-to struct get 0/self+0x10/self+0x10, return
 * self) is fully understood and correct either way.
 *
 * extern void *D_00154A40;
 *
 * void *func_00119868(void *arg0) {
 *     char *self = (char *)&D_00154A40;
 *     D_00154A40 = arg0;
 *     *(void **)(self + 0x8) = self + 0x10;
 *     *(int *)(self + 0x4) = 0;
 *     *(void **)(self + 0xC) = self + 0x10;
 *     return self;
 * }
 */
extern void *InitBlockData NOT_SDA;

void *tty_queue_init(void *arg0) {
    char *self = (char *)&InitBlockData;
    InitBlockData = arg0;
    *(int *)(self + 0x4) = 0;
    *(void **)(self + 0xC) = self + 0x10;
    *(void **)(self + 0x8) = self + 0x10;
    return self;
}

void tty_queue_write_advance(char *self) {
    char *p;
    *(int *)(self + 0x4) += 1;
    p = *(char **)(self + 0xC) + 1;
    *(char **)(self + 0xC) = p;
    if (p == self + (*(int *)self + 0x10)) {
        *(char **)(self + 0xC) = self + 0x10;
    }
}

void tty_queue_read_advance(char *self) {
    char *p;
    *(int *)(self + 0x4) -= 1;
    p = *(char **)(self + 0x8) + 1;
    *(char **)(self + 0x8) = p;
    if (p == self + (*(int *)self + 0x10)) {
        *(char **)(self + 0x8) = self + 0x10;
    }
}

typedef struct {
    unsigned short len;
} TtyHdr;

struct TtyRing {
    int size;
    int count;
    char *rp;
    char *wp;
};

typedef struct {
    int sock;
    volatile int wlen;
    volatile int rlen;
    volatile int busy;
    char *wbuf;
    char *rbuf;
    struct TtyRing *queue;  /* func_00119868's input ring */
} TtyHandlerState;

extern void kprintf(const char *, ...);
extern int isceDeci2ExRecv_r(int, char *, unsigned short) __asm__("func_001197C0");
extern int isceDeci2ExSend_r(int, char *, unsigned short) __asm__("func_001197F8");
extern char tty_packet_too_large_msg[];
extern char tty_receive_error_msg[];
extern char tty_send_error_msg[];
extern char tty_wlen_error_msg[];

/* sceTtyHandler: the DECI2 event callback (1/2 read, 3 write, 4 write done). */
void sceTtyHandler(int event, int param, TtyHandlerState *ti) {
    int n;

    switch (event) {
    case 1:
    case 2:
        if (param != 0) {
            if ((unsigned int)(ti->rlen + param) > 0x140) {
                kprintf(tty_packet_too_large_msg);
            }
            {
                int off = ti->rlen;

                n = isceDeci2ExRecv_r(ti->sock, ti->rbuf + off, param);
            }
            if (n < 0) {
                kprintf(tty_receive_error_msg);
            }
            ti->rlen += n;
        } else {
            TtyHdr *h = (TtyHdr *)ti->rbuf;

            for (n = 0xC; n < h->len; n++) {
                *ti->queue->wp = ti->rbuf[n];
                tty_queue_write_advance((char *)ti->queue);
            }
            ti->rlen = 0;
        }
        break;
    case 3: {
        int w = isceDeci2ExSend_r(ti->sock, ti->wbuf, ti->wlen);

        if (w < 0) {
            kprintf(tty_send_error_msg, w);
            ti->busy = 0;
        } else {
            ti->wbuf += w;
            ti->wlen -= w;
        }
        break;
    }
    case 4:
        if (ti->wlen != 0) {
            kprintf(tty_wlen_error_msg, ti->wlen);
        }
        ti->busy = 0;
        break;
    }
}

struct TtyState {
    volatile s32 unk0; /* deci2 handle from sceDeci2Open */
    volatile s32 unk4; /* published payload length */
    volatile s32 unk8;
    volatile s32 unkC; /* busy flag, also written by sceTtyHandler */
    s32 unk10; /* MMIO block pointer (with 0x20000000 flag bit) */
    s32 unk14;
    s32 unk18;
};
struct Mmio {
    u16 unk0; /* transmit length */
    u16 unk2;
    u16 unk4;
    u8 unk6;
    s8 unk7; /* CallDebugCharacter mode byte (read with `lb`) */
    u32 unk8;
    u8 data[0xF4]; /* payload window, addressed at block + 0xC */
};
extern struct TtyState tty_info_r __asm__("D_00154B50");
extern u8 tty_send_packet[];
extern s32 sceDeci2ReqSend(s32, s32);
extern s32 disable_interrupts();
extern s32 enable_interrupts();
extern void sceDeci2Poll(s32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/debug/sce_tty_write.c, sceTtyWrite. */
s32 sceTtyWrite(s8 *text, s32 text_len) {
    s32 written_count; /* characters consumed -> return value */
    s32 remaining; /* countdown, hits -1 after len+1 tests */
    s32 window_bytes; /* bytes resident in the payload window */
    s8 *src; /* source cursor */
    struct Mmio *mmio;
    u8 *dst; /* payload cursor */

    written_count = 0;
    remaining = text_len;
    window_bytes = 0;
    src = text;
    if (tty_info_r.unkC == 0) {
        disable_interrupts();
        tty_info_r.unkC = 1;
        mmio = (struct Mmio *)((u32)tty_send_packet | 0x20000000);
        tty_info_r.unk10 = (s32)mmio;
        dst = mmio->data;
        do {
            remaining = remaining - 1;
            if (remaining == -1) {
                break;
            }
            if (*src == 0xA) {
                *dst = 0xD;
                window_bytes = window_bytes + 1;
                dst = dst + 1;
                if (window_bytes >= 0x100) {
                    break;
                }
            }
            *dst = (u8)*src;
            window_bytes = window_bytes + 1;
            src = src + 1;
            dst = dst + 1;
            written_count = written_count + 1;
        } while (window_bytes < 0x100);
        tty_info_r.unk4 = window_bytes + 0xC;
        mmio->unk0 = tty_info_r.unk4;
        if (sceDeci2ReqSend(tty_info_r.unk0, mmio->unk7) < 0) {
            *(s32 *)&tty_info_r.unkC = 0;
            enable_interrupts();
            return -1;
        }
        if (tty_info_r.unkC != 0) {
            do {
                sceDeci2Poll(tty_info_r.unk0);
            } while (tty_info_r.unkC != 0);
        }
        enable_interrupts();
        return written_count;
    }
    return -1;
}

/* The input ring func_00119868 sets up: size, count, read/write heads. */
typedef struct {
    int size;
    int count;
    char *rp;
    char *wp;
} TtyQueue;

typedef struct {
    int sock;
    int unk04;
    int unk08;
    int unk0C;
    char *wbuf;
    char *rbuf;
    TtyQueue *queue;    /* 0x18 */
} TtyState2;

extern TtyState2 tty_info_t __asm__("D_00154B50");
extern TtyQueue *tty_info_queue;

/* sceTtyRead: block for each byte until the handler queues one; stop
   after a newline. */
int sceTtyRead(char *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        while (((volatile TtyQueue *)tty_info_queue)->count == 0) {
        }
        {
            TtyState2 *t = &tty_info_t;

            buf[i] = *t->queue->rp;
            tty_queue_read_advance((char *)t->queue);
        }
        if (buf[i] == '\n' || buf[i] == '\r') {
            return i + 1;
        }
    }
    return i;
}

struct TtyInitState {
    s32 unk0;
    volatile s32 unk4;
    volatile s32 unk8;
    volatile s32 unkC;
    s32 unk10;
    s32 unk14;
    s32 unk18;
};
struct TtyMmio {
    u16 unk0;
    u16 unk2;
    u16 unk4;
    u8 unk6;
    u8 unk7;
    u32 unk8;
};
extern struct TtyInitState tty_info;
extern u8 tty_receive_packet[];
extern s32 flush_cache();
extern s32 tty_queue_init_tty(int) __asm__("func_00119868");
extern s32 sceDeci2Open();
extern void sceTtyHandler();
/* Opens the TTY DECI2 endpoint and initializes its uncached packet headers. */
s32 sceTtyInit(void) {
    struct TtyInitState *state = &tty_info;
    struct TtyMmio *p;
    struct TtyMmio *q;
    u32 mask;
    s32 baud;
    s32 cmd;

    flush_cache(0);
    *(volatile s32 *)&state->unk0 = sceDeci2Open(0x210, state, &sceTtyHandler);
    if (state->unk0 < 0) {
        return 0;
    }
    state->unkC = 0;
    mask = 0x20000000;
    q = (struct TtyMmio *)((u32)tty_receive_packet | mask);
    state->unk4 = 0;
    p = (struct TtyMmio *)((u32)tty_send_packet | mask);
    state->unk8 = 0;
    state->unk14 = (s32)q;
    state->unk10 = (s32)p;
    baud = 0x210;
    cmd = 0x45;
    p->unk2 = 0;
    p->unk4 = baud;
    p->unk6 = cmd;
    p->unk7 = 0x48;
    p->unk8 = 0;
    state->unk18 = tty_queue_init_tty(0x100);
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00119D84);
