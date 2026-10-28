#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x122AD8-0x123168. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph, graph018.o (sceGsExecStoreImage). Built with Sony's
 * 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt libgraph.a.
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

typedef int u128_si __attribute__((mode(TI)));
typedef struct {
    unsigned long SBP:14; unsigned long p14:2; unsigned long SBW:6; unsigned long p22:2;
    unsigned long SPSM:6; unsigned long p30:2; unsigned long DBP:14; unsigned long p46:2;
    unsigned long DBW:6; unsigned long p54:2; unsigned long DPSM:6; unsigned long p62:2;
} SiBitbltbuf;
typedef struct {
    unsigned long RRW:12; unsigned long p12:20; unsigned long RRH:12; unsigned long p44:20;
} SiTrxreg;
typedef struct {
    unsigned int vifcode[4];
    unsigned long giftag[2];
    SiBitbltbuf bitbltbuf; long bitbltbufaddr;
    unsigned long trxpos; long trxposaddr;
    SiTrxreg trxreg; long trxregaddr;
    unsigned long finish; long finishaddr;
    unsigned long trxdir; long trxdiraddr;
} SiStoreImage __attribute__((aligned(16)));

extern char gs_store_image_dma1_timeout_msg[];
extern char gs_store_image_gs_timeout_msg[];
extern char gs_store_image_readback_timeout_msg[];
extern char gs_store_image_vif1_short_msg[];
extern u128_si gs_store_image_vif1_restore_qword;
extern unsigned long gs_get_imr(void);
extern int gs_put_imr(unsigned long);

int sceGsExecStoreImage(SiStoreImage *sp, u128_si *dstaddr)
{
    unsigned int cnt = 0;
    int nfull = 0, nrem = 0, nbyte = 0, nrest = 0;
    int w, h, psm, bytes, i, old, n;
    int h2;
    union { u128_si q; unsigned char c[16]; } buf;

    w = sp->trxreg.RRW;
    h = sp->trxreg.RRH;
    psm = sp->bitbltbuf.SPSM;
    h2 = 0;
    switch (psm) {
    case 0x00:
    case 0x30:
        bytes = w * h * 4;
        nbyte = bytes & 0xF;
        nfull = (bytes >> 4) & ~7u;
        nrem = (bytes >> 4) & 7;
        if (!nbyte) {
            h2 = h;
            nrest = 0;
        } else {
            h2 = (h + 3) & ~3;
            n = w * h2;
            nrest = (n >> 2) - nfull - nrem - 1;
        }
        break;
    case 0x01:
    case 0x31:
        bytes = w * h * 3;
        nbyte = bytes & 0xF;
        nfull = (bytes >> 4) & ~7u;
        nrem = (bytes >> 4) & 7;
        if (!nbyte) {
            h2 = h;
            nrest = 0;
        } else {
            h2 = (h + 15) & ~15;
            n = w * h2;
            n = ((n * 3) >> 4) - nfull;
            nrest = n - nrem - 1;
        }
        break;
    case 0x02:
    case 0x0A:
    case 0x32:
    case 0x3A:
        bytes = w * h * 2;
        nbyte = bytes & 0xF;
        nrem = (bytes >> 4) & 7;
        nfull = (bytes >> 4) & ~7u;
        if (!nbyte) {
            h2 = h;
            nrest = 0;
        } else {
            h2 = (h + 7) & ~7u;
            n = w * h2;
            nrest = (n >> 3) - nfull - nrem - 1;
        }
        break;
    case 0x13:
    case 0x1B:
        bytes = w * h;
        nbyte = bytes & 0xF;
        nrem = (bytes >> 4) & 7;
        nfull = (bytes >> 4) & ~7u;
        if (!nbyte) {
            h2 = h;
            nrest = 0;
        } else {
            h2 = (h + 7) & ~7u;
            n = w * h2;
            nrest = (n >> 4) - nfull - nrem - 1;
        }
        break;
    case 0x14:
    case 0x24:
    case 0x2C:
        bytes = (w * h) >> 1;
        nbyte = bytes & 0xF;
        nrem = (bytes >> 4) & 7;
        nfull = (bytes >> 4) & ~7u;
        if (!nbyte) {
            h2 = h;
            nrest = 0;
        } else {
            h2 = (h + 7) & ~7u;
            n = w * h2;
            nrest = (n >> 5) - nfull - nrem - 1;
        }
        break;
    }

    if (nbyte) {
        *(long *)((unsigned int)&sp->trxreg | 0x20000000) = (long)(int)sp->trxreg.RRW | ((long)h2 << 32);
    }

    while (*(volatile int *)0x10009000 & 0x100) {
        if (cnt++ > 0x1000000) {
            scePrintf(gs_store_image_dma1_timeout_msg);
            return -1;
        }
    }

    old = gs_put_imr(gs_get_imr() | 0x200);
    *(volatile long *)0x12001000 = 2;
    *(volatile int *)0x10009020 = 7;
    if (((unsigned int)sp & 0x70000000) == 0x70000000) {
        *(volatile int *)0x10009010 = ((unsigned int)sp & 0x0FFFFFFF) | 0x80000000;
    } else {
        *(volatile int *)0x10009010 = (unsigned int)sp & 0x0FFFFFFF;
    }
    *(volatile int *)0x10009000 = 0x101;
    while (*(volatile int *)0x10009000 & 0x100) {
        if (cnt++ > 0x1000000) {
            scePrintf(gs_store_image_dma1_timeout_msg);
            return -1;
        }
    }

    while (!(*(volatile long *)0x12001000 & 2)) {
        if (cnt++ > 0x1000000) {
            scePrintf(gs_store_image_gs_timeout_msg);
            *(volatile u128_si *)0x10005000 = gs_store_image_vif1_restore_qword;
            return -1;
        }
    }

    *(volatile int *)0x10003C00 = 0x800000;
    *(volatile long *)0x12001040 = 1;

    if (nfull) {
        *(volatile int *)0x10009020 = nfull;
        if (((unsigned int)dstaddr & 0x70000000) == 0x70000000) {
            *(volatile int *)0x10009010 = ((unsigned int)dstaddr & 0x0FFFFFFF) | 0x80000000;
        } else {
            *(volatile int *)0x10009010 = (unsigned int)dstaddr & 0x0FFFFFFF;
        }
        *(volatile int *)0x10009000 = 0x100;
        while (*(volatile int *)0x10009000 & 0x100) {
            if (cnt++ > 0x1000000) {
                scePrintf(gs_store_image_readback_timeout_msg);
                *(volatile long *)0x12001000 = 0x100;
                *(volatile long *)0x12001040 = 0;
                *(volatile int *)0x10003000 = 1;
                *(volatile int *)0x10003C10 = 1;
                return -1;
            }
        }
    }

    for (i = 0; i < nrem; i++) {
        while (!(*(volatile int *)0x10003C00 & 0x1F000000)) {
            if (cnt++ > 0x1000000) {
                scePrintf(gs_store_image_vif1_short_msg);
                *(volatile long *)0x12001000 = 0x100;
                *(volatile long *)0x12001040 = 0;
                *(volatile int *)0x10003000 = 1;
                *(volatile int *)0x10003C10 = 1;
                return -1;
            }
        }
        dstaddr[nfull + i] = *(volatile u128_si *)0x10005000;
    }

    if (nbyte) {
        while (!(*(volatile int *)0x10003C00 & 0x1F000000)) {
            if (cnt++ > 0x1000000) {
                scePrintf(gs_store_image_vif1_short_msg);
                *(volatile long *)0x12001000 = 0x100;
                *(volatile long *)0x12001040 = 0;
                *(volatile int *)0x10003000 = 1;
                *(volatile int *)0x10003C10 = 1;
                return -1;
            }
        }
        buf.q = *(volatile u128_si *)0x10005000;
        for (i = 0; i < nbyte; i++) {
            ((unsigned char *)&dstaddr[nfull + nrem])[i] = buf.c[i];
        }
        for (i = 0; i < nrest; i++) {
            while (!(*(volatile int *)0x10003C00 & 0x1F000000)) {
                if (cnt++ > 0x1000000) {
                    scePrintf(gs_store_image_vif1_short_msg);
                    *(volatile long *)0x12001000 = 0x100;
                    *(volatile long *)0x12001040 = 0;
                    *(volatile int *)0x10003000 = 1;
                    *(volatile int *)0x10003C10 = 1;
                    return -1;
                }
            }
            buf.q = *(volatile u128_si *)0x10005000;
        }
    }

    *(volatile int *)0x10003C00 = 0;
    *(volatile long *)0x12001040 = 0;
    gs_put_imr(old);
    {
        u128_si t = gs_store_image_vif1_restore_qword;
        *(volatile long *)0x12001000 = 2;
        *(volatile u128_si *)0x10005000 = t;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00123164);
