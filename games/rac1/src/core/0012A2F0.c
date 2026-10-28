#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x12A2F0-0x12AC80. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
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
extern void *InitBlockData NOT_SDA;
extern int sif_sregs[];
extern void disable_dmac_atomic(int);
extern void remove_dmac_handler(int, int);
extern int sif_cmd_dmac_handler_id;
extern int sif_cmd_init_done;
extern int sif_cmd_data_sys_handlers NOT_SDA;
extern int sif_cmd_data_usr_handlers NOT_SDA;
extern void sif_send_cmd_common(int, int, int, int, int, int, int);
extern void sceSifExitCmd(void);
extern int RpcCommandState NOT_SDA;
extern int disable_interrupts(void);
extern void enable_interrupts(void);
extern int create_semaphore(void *);
extern int fs_iob_sema;
extern int fs_iob_sema_2;
extern char fs_iob_table[];
extern int fs_call_sema;
extern void fs_call_sema_init(void);
extern int fs_call_sema NOT_SDA;
extern char sif_lib_version_string[];
extern char FsResetBuffer[];
extern int fs_expected_version;
extern void libc_memset(void *, int, int);
extern int FsResetState;
extern int iop_heap_bind_state;
extern char iop_heap_rpc_client[];
extern int iop_heap_rpc_reply;
extern int iop_heap_rpc_arg;
extern char lf_iop_version[];
extern int lf_expected_version;
extern int _sceSifLoadModuleBuffer(int arg0, int arg1, int arg2, void *arg3);
extern void tty_reset_init_flag(void);
extern void init_tlb_32mb(void);
extern void kernel_init_tlb(void);
extern int FirstSemaphore;
extern int SecondSemaphore;
extern int osd_syscall_patch_table[];
extern char osd_kernel_patch_code[];
extern int PatchIsNeeded(void);
extern void flush_cache(int);
extern int sce_cd_sync_s(int);
extern void *cd_user_callback;
extern int cd_callback_sema;
extern int cd_ncmd_sema;
extern int cd_scmd_sema;
extern int cd_ncmd_busy;
extern int cd_poweroff_installing;
extern int cd_poweroff_installed;
extern int sif_call_rpc();
extern int cd_ncmd_prechk(int);
extern void signal_semaphore(int);
extern char cd_ncmd_rpc_client[];
extern int cd_ncmd_recv_buffer;
extern void scePrintf();
extern int sce_sif_check_stat_rpc(void *);
extern char cd_scmd_wait_msg[];
extern char cd_scmd_rpc_client[];
extern int cd_debug_level;
extern int cd_scmd_prechk(int);
extern int cd_scmd_recv_buffer;
extern char CoreDataTable[];
extern void sceDmaPause(void *);
extern char dma_sync_timeout_msg[];
extern int DmaChannels[];
extern int sce_mc_open(int, int, int, int);
extern int mc_current_command;
extern int *mc_get_info_type_out;
extern int *mc_get_info_free_out;
extern int *mc_get_info_format_out;
extern char RpcQueueData[];
extern int RpcReceiveData;
extern void debug_print_stub(void *);
extern char str_dbc_set_work_addr_error[];
extern int sceDbcGetDepNumber(int);
typedef struct {
    char unk_00[4];
    int  unk_04;
    int  unk_08;
    char *unk_0C;
    char unk_10[0x320];
} Ent330;
extern Ent330 pad2_sockets[];
extern void sync_dcache_range(void *a, void *b);
extern void ipu_vdec(int, int);
extern int IpuCommandTable[];
extern void dispatch_mpeg_cb_nodata(int);
extern void wait_ipu_idle(void *);
extern void _flushBuf(void *, int);
extern int _peepBit(void *, int);
extern int _nextBit(void *, int);
extern void extra_bit_information(void *);
extern char str_mpeg_buffer_too_small_fmt[];
extern void sprintf_4(void *, char *, int, int) __asm__("func_00116248");
extern void _Error_a(void *, void *) __asm__("func_0012C468");

extern void send_ipu_command();
extern void dispatch_mpeg_callback();

/* _doCSC (libmpeg). Stack-size probe #2: a 32-byte local (two 16-byte
   compiler stack slots) instead of one, since a single int/4-int array
   both land in one slot here (frame still 0x10 short of retail's 0x70,
   see c1-c4). */
void do_csc(void *arg0, unsigned int arg1, int arg2) {
    char *p = (char *)arg0;
    int r;
    int event[8];

    while (*(volatile int *)0x10002010 < 0) {
    }
    r = disable_interrupts();
    *(volatile unsigned int *)0x1000B010 = arg1 & 0x0FFFFFFF;
    *(volatile unsigned int *)0x1000B020 = (unsigned int)arg2 << 6;
    *(volatile unsigned int *)0x1000B000 = 0x100;
    if (r != 0) {
        enable_interrupts();
    }
    send_ipu_command(arg0, arg2 | 0x70000000);
    event[0] = 4;
    dispatch_mpeg_callback(*(int *)(p + 0x858), event);
    while (((*(volatile unsigned int *)0x1000B000 >> 8) & 1) != 0) {
    }
    while (*(volatile int *)0x10002010 < 0) {
    }
}

ASM_FUNC("asm/handwritten/core_text", func_0012A418);

extern int add_dmac_handler2_r(int, void *, int, void *) __asm__("func_00118AC0");
extern void enable_dmac_atomic_r(int) __asm__("func_00119460");
extern void do_csc2_dma_handler_r(void) __asm__("func_0012A418");
extern char csc_handler_error_msg[];

typedef struct {
    volatile int done; /* 0x00: chunks finished (callback counts) */
    int err;           /* 0x04 */
    int size;          /* 0x08 */
    int next;          /* 0x0C */
    int total;         /* 0x10 */
} CscRec_2A558;

/* libmpeg chunked DMA copy: arm the channel-3 callback with a progress record, start the transfer, wait for all chunks. */
void do_csc2(void *arg0, unsigned int arg1, int arg2) {
    char *p = (char *)arg0;
    int event[8];
    CscRec_2A558 rec;
    CscRec_2A558 *rp;
    int h;
    int r;

    rec.total = arg2 / 0x3FF + 1;
    rec.size = arg2;
    rec.next = (arg1 + 0xFFC00) & 0x0FFFFFFF;
    rec.err = 0;
    rec.done = 0;
    rp = &rec;
    while (*(volatile int *)0x10002010 < 0) {
    }
    h = add_dmac_handler2_r(3, (void *)do_csc2_dma_handler_r, 0, rp);
    *(volatile unsigned int *)0x1000E010 = 8;
    enable_dmac_atomic_r(3);
    r = disable_interrupts();
    {
        volatile unsigned int *madr = (volatile unsigned int *)0x1000B010;
        volatile unsigned int *qwc = (volatile unsigned int *)0x1000B020;
        volatile unsigned int *chcr = (volatile unsigned int *)0x1000B000;
        *madr = arg1 & 0x0FFFFFFF;
        *qwc = 0xFFC0;
        *chcr = 0x100;
    }
    if (r != 0) {
        enable_interrupts();
    }
    *(volatile unsigned int *)0x10002000 = 0x700003FF;
    event[0] = 4;
    dispatch_mpeg_callback(*(void **)(p + 0x858), event);
    while (rec.done < rec.total) {
    }
    if (rec.err != 0) {
        _Error_a(p, csc_handler_error_msg);
    }
    while (*(volatile int *)0x10002010 < 0) {
    }
    disable_dmac_atomic(3);
    remove_dmac_handler(3, h);
}

ASM_FUNC("asm/handwritten/core_text", func_0012A718);

extern void *dispatch_mpeg_callback_result(void *, int *) __asm__("func_0012BC78");
/* Transfer the MPEG input through the IPU/SPR DMA channel, splitting large blocks. */
extern void *add_dmac_handler2(int, void *, int, void *);
extern void enable_dmac_atomic(int);
extern void do_csc2(void *, unsigned int, int);
extern void csc_store_ref_image_dma_handler();

void _csc_storeRefImage(void *arg0, int *arg1) {
    char *p = (char *)arg0;
    int ev[8];
    int loc[4];
    int n, small, r, addr;
    unsigned int mask;
    void *h;

    ev[0] = 2;
    n = arg1[3] * arg1[4];
    dispatch_mpeg_callback_result(*(void **)(p + 0x858), ev);
    if ((*(volatile unsigned int *)0x10002010 & 0x4000) != 0) {
        *(volatile unsigned int *)0x10002010 = 0x40000000;
    }
    small = n < 0x400;
    while (*(volatile int *)0x10002010 < 0) {
    }
    send_ipu_command(arg0, 0);
    addr = arg1[0];
    while (*(volatile int *)0x10002010 < 0) {
    }
    mask = 0x0FFFFFFF;
    loc[1] = addr & mask;
    loc[0] = n * 0x18;
    if ((unsigned int)loc[0] > 0xFFFF) {
        h = add_dmac_handler2(4, (void *)csc_store_ref_image_dma_handler, 0, &loc[0]);
        *(volatile unsigned int *)0x1000E010 = 0x10;
        enable_dmac_atomic(4);
        r = disable_interrupts();
        *(volatile unsigned int *)0x1000B410 = loc[1];
        *(volatile unsigned int *)0x1000B420 = 0xFFFF;
        *(volatile unsigned int *)0x1000B400 = 0x101;
        if (r != 0) {
            enable_interrupts();
        }
        loc[1] = (loc[1] + 0xFFFF0) & mask;
        loc[0] = loc[0] - 0xFFFF;
        if (small) {
            do_csc(arg0, *(int *)(p + 0xD8), n);
        } else {
            do_csc2(arg0, *(int *)(p + 0xD8), n);
        }
        disable_dmac_atomic(4);
        remove_dmac_handler(4, (int)h);
    } else {
        r = disable_interrupts();
        *(volatile unsigned int *)0x1000B410 = arg1[0] & mask;
        *(volatile unsigned int *)0x1000B420 = loc[0];
        *(volatile unsigned int *)0x1000B400 = 0x101;
        if (r != 0) {
            enable_interrupts();
        }
        loc[0] = 0;
        if (small) {
            do_csc(arg0, *(int *)(p + 0xD8), n);
        } else {
            do_csc2(arg0, *(int *)(p + 0xD8), n);
        }
    }
    ev[0] = 3;
    dispatch_mpeg_callback_result(*(void **)(p + 0x858), ev);
}

extern long sysbit_flush(void *, int);

/* _sysbitInit: points the bit stream at arg0 at the arg3-byte ring buffer
   at arg2, reading from arg1, and resets it. The store order is
   load-bearing (LEVERS.md lever 3): arg1's +0x8 store has to come before
   its +0xC store, after the other fields in retail's order. */
void sysbit_init(void *arg0, int arg1, int arg2, int arg3) {
    char *p = (char *)arg0;
    *(int *)(p + 0x28) = arg3;
    *(int *)(p + 0x8) = arg1;
    *(long *)(p + 0x0) = 0;
    *(int *)(p + 0x10) = 0;
    *(long *)(p + 0x18) = 0;
    *(int *)(p + 0x20) = arg2;
    *(int *)(p + 0x24) = arg2 + arg3;
    *(int *)(p + 0xC) = arg1;
    sysbit_flush(arg0, 0);
}

/* Peek n high bits of the 64-bit accumulator, without consuming them.
 * As a C shift, this requires 1 <= n <= 64. GNU 2.10-ee assembles the
 * final int conversion directly into the return delay slot. */
int sysbit_show(void *arg0, int n) {
    unsigned long acc = *(unsigned long *)arg0;
    return (int)(acc >> (64 - n));
}

/* The bitstream reader's state (func_0012AA70 sets it up). */
typedef struct {
    unsigned long acc;     /* 0x00: the next bits, top-aligned */
    unsigned char *start0; /* 0x08 */
    unsigned char *cur;    /* 0x0C: next byte to load */
    unsigned int bits;     /* 0x10: valid bits in acc */
    long total;            /* 0x18: bits consumed so far */
    unsigned char *start;  /* 0x20: the ring buffer */
    unsigned char *end;    /* 0x24 */
    int len;               /* 0x28 */
} BitStream;

/* Consume n bits, then refill the accumulator a byte at a time from the
   ring buffer until more than 56 bits are buffered; returns the new
   total (mpeg2decode's Flush_Buffer shape). The `c` local and the
   returned total are load-bearing: they bring the function to the 40
   insns that put GCSE's hash buckets, and so PRE's registers, in
   retail's order. */
long sysbit_flush(void *arg0, int n) {
    BitStream *p = arg0;
    p->acc <<= n;
    p->bits -= n;
    if (p->bits <= 56) {
        do {
            unsigned int c = *p->cur++;
            p->acc |= (unsigned long)c << (56 - p->bits);
            if (p->cur >= p->end) p->cur = p->start;
            p->bits += 8;
        } while (p->bits <= 56);
    }
    return p->total += n;
}

/* Get n bits: peek them (func_0012AAA8), then consume them
   (func_0012AAC8). */
int sysbit_get(void *arg0, int arg1) {
    int r = sysbit_show(arg0, arg1);
    sysbit_flush(arg0, arg1);
    return r;
}


/* Get one bit, as func_0012AB60. */
int sysbit_marker(void *arg0) {
    int r;
    r = sysbit_show(arg0, 1);
    sysbit_flush(arg0, 1);
    return r;
}

/* _sysbitJump (libmpeg bit.c): skip n bytes, then refill through
   _sysbitFlush. Clearing acc and bits before the total update is the
   one statement order that matches. */
void sysbit_jump(void *arg0, int n) {
    BitStream *p = arg0;
    p->acc = 0;
    p->bits = 0;
    p->total += n * 8;
    p->cur = p->start0 + (p->total >> 3);
    if (p->cur >= p->end) p->cur -= p->len;
    sysbit_flush(p, 0);
}

unsigned int sysbit_ptr(char *arg0, int arg1) {
    unsigned int v = *(int *)(arg0 + 0x8) + (arg1 >> 3);
    if (v >= *(unsigned int *)(arg0 + 0x24)) {
        v -= *(int *)(arg0 + 0x28);
    }
    return v;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012AC7C);
