#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x121D18-0x122140. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph, graph003.o (checkModelVersion, _GetGsDxDyOffset,
 * sceGsSetDefDispEnv). Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libgraph.a.
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

extern int sceOpen(void *path, int mode);
extern int sceRead(int fd, void *buf, int len);
extern void sceClose(int fd);
extern int libc_atoi(void *arg0);
extern char romver_path[];

/* Read a small text file a byte at a time into a 0x100-byte stack buffer,
 * stopping at a NUL or the buffer limit, then strtol() the tail starting
 * 9 bytes before the end and compare it against a build number. Same
 * open/read/close triplet as func_0012D2A0 above. */
int checkModelVersion(void) {
    int fd;
    char buf[0x100];
    char *p;
    unsigned int n;

    fd = sceOpen(romver_path, 1);
    if (fd < 0) {
        return -1;
    }
    n = 0;
    p = buf;
    while (n < 0x100) {
        sceRead(fd, p, 1);
        if (*p++ == 0) {
            break;
        }
        n++;
    }
    sceClose(fd);
    return 0x1315670 < libc_atoi(&buf[n - 9]);
}

ASM_FUNC("asm/handwritten/core_text", func_00121DB8);

typedef struct {
    short interMode;
    short outMode;
    short ffMode;
    short version;
} GsGParam;

typedef struct {
    unsigned long pmode;
    unsigned long smode2;
    unsigned long dispfb;
    unsigned long display;
    unsigned long bgcolor;
} GsDispEnv;

extern GsGParam *sceGsGetGParam(void);
extern void _GetGsDxDyOffset(int, int *, int *, int *, int *);
extern char gs_def_disp_env_mode_error_msg[];

void sceGsDefDispEnv(GsDispEnv *disp, short psm, short w, short h, short dx, short dy) {
    GsGParam *gp;
    int dx1, dy1, dx2, dy2;
    int mode;
    short omode;
    unsigned short imode;

    gp = sceGsGetGParam();
    if (gp->outMode != 2 && gp->outMode != 3 && checkModelVersion() != 0) {
        _GetGsDxDyOffset(gp->outMode, &dx1, &dy1, &dx2, &dy2);
    } else {
        dy2 = 0;
        dx2 = 0;
        dy1 = 0;
        dx1 = 0;
    }
    imode = gp->interMode;
    disp->pmode = 0x66;
    if (gp->interMode != 0) {
        if (gp->ffMode != 0) {
            disp->smode2 = 3;
        } else {
            disp->smode2 = 1;
        }
    } else {
        disp->smode2 = 2;
    }
    disp->dispfb = ((unsigned long)(psm & 0xF) << 15) |
                   ((unsigned long)(((w + 63) >> 6) & 0x3F) << 9);
    omode = gp->outMode;
    if (omode == 2) {
        if ((short)imode == 1) {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 636;
            disp->display = ((((unsigned long)(magh - 1) << 23) |
                            (((dx * magh + dxo) & 0xFFF) |
                             ((unsigned long)(gp->ffMode ? (h << 1) - 1 : h - 1) << 44))) |
                            (((unsigned long)(dy + dy1 + 50) & 0xFFF) << 12)) |
                            ((unsigned long)(magh * w - 1) << 32);
        } else {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 636;
            magh = (w + 2559) / w;
            disp->display = ((((unsigned long)((w + 2559) / w - 1) << 23) |
                            (((dx * ((w + 2559) / w) + dxo) & 0xFFF) |
                             ((unsigned long)(h - 1) << 44))) |
                            (((unsigned long)(dy + dy1 + 25) & 0xFFF) << 12)) |
                            ((unsigned long)((w + 2559) / w * w - 1) << 32);
        }
    } else if (omode == 3) {
        if ((short)imode == 1) {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 656;
            disp->display = ((((unsigned long)(magh - 1) << 23) |
                            (((dx * magh + dxo) & 0xFFF) |
                             ((unsigned long)(gp->ffMode ? (h << 1) - 1 : h - 1) << 44))) |
                            (((unsigned long)(dy + dy1 + 72) & 0xFFF) << 12)) |
                            ((unsigned long)(magh * w - 1) << 32);
        } else {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 656;
            magh = (w + 2559) / w;
            disp->display = ((((unsigned long)((w + 2559) / w - 1) << 23) |
                            (((dx * ((w + 2559) / w) + dxo) & 0xFFF) |
                             ((unsigned long)(h - 1) << 44))) |
                            (((unsigned long)(dy + dy1 + 36) & 0xFFF) << 12)) |
                            ((unsigned long)((w + 2559) / w * w - 1) << 32);
        }
    } else if (omode == 0x50) {
        unsigned long dxo = (unsigned long)(dx * 2) + 232;
        disp->display = ((((unsigned long)(h - 1) << 44) |
                          ((((720 - w) / 2 * 2 + (unsigned long)dx1 + dxo) & 0xFFF) | 0x800000)) |
                         (((unsigned long)(dy + dy1 + 35) & 0xFFF) << 12)) |
                        ((unsigned long)(w * 2 - 1) << 32);
    } else {
        scePrintf(gs_def_disp_env_mode_error_msg);
    }
    disp->bgcolor = 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012213C);
