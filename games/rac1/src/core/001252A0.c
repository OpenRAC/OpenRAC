#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x1252A0-0x125630. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libvu0, libvu0.o: the sceVu0 matrix and vector routines, VU0
 * macro-mode inline assembly (_sceVu0ecossin and sceVu0RotMatrixZ/X/Y
 * among them; the linker dead-stripped what the game does not call).
 * Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt
 * libvu0.a.
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

LINKER_REMNANT("asm/remnants/core_text", func_001252A0);
void sceVu0Normalize(void *dst, void *src) {
    __asm__ __volatile__(
        "lqc2 $vf4,0x0(%1)\n\t"
        "vmul.xyz $vf5,$vf4,$vf4\n\t"
        "vaddy.x $vf5,$vf5,$vf5y\n\t"
        "vaddz.x $vf5,$vf5,$vf5z\n\t"
        ".word 0x4A0503BD\n\t"  /* vsqrt Q,$vf5x */
        ".word 0x4A0003BF\n\t"  /* vwaitq */
        ".word 0x4B000160\n\t"  /* vaddq.x $vf5,$vf0,Q */
        "vnop\n\tvnop\n\t"
        ".word 0x4A6503BC\n\t"  /* vdiv Q,$vf0w,$vf5x */
        "vsub.xyzw $vf6,$vf0,$vf0\n\t"
        ".word 0x4A0003BF\n\t"  /* vwaitq */
        ".word 0x4BC0219C\n\t"  /* vmulq.xyz $vf6,$vf4,Q */
        "sqc2 $vf6,0x0(%0)"
        : : "r"(dst), "r"(src) : "memory");
}

LINKER_REMNANT("asm/remnants/core_text", func_00125300);
void sceVu0ScaleVector(float *dst, float *src, float s) {
    __asm__ __volatile__(
        "lqc2 $vf4,0x0(%1)\n\t"
        "mfc1 $8,%2\n\t"
        "qmtc2 $8,$vf5\n\t"
        "vmulx.xyzw $vf6,$vf4,$vf5x\n\t"
        "sqc2 $vf6,0x0(%0)"
        : : "r"(dst), "r"(src), "f"(s) : "$8", "memory");
}

LINKER_REMNANT("asm/remnants/core_text", func_00125340);

void sceVu0UnitMatrix(float *m) {
    __asm__ __volatile__(
        "vsub.xyzw $vf4,$vf0,$vf0\n\t"
        "vadd.w $vf4,$vf4,$vf0\n\t"
        "vmr32.xyzw $vf5,$vf4\n\t"
        "vmr32.xyzw $vf6,$vf5\n\t"
        "vmr32.xyzw $vf7,$vf6\n\t"
        "sqc2 $vf4,0x30(%0)\n\t"
        "sqc2 $vf5,0x20(%0)\n\t"
        "sqc2 $vf6,0x10(%0)\n\t"
        "sqc2 $vf7,0x0(%0)"
        : : "r"(m) : "memory");
}

ASM_FUNC("asm/handwritten/core_text", func_00125380);

ASM_FUNC("asm/handwritten/core_text", func_001253F8);

ASM_FUNC("asm/handwritten/core_text", func_001254A0);

ASM_FUNC("asm/handwritten/core_text", func_00125548);

LINKER_REMNANT("asm/remnants/core_text", func_001255F0);
