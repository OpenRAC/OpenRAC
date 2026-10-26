#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x125218-0x1252A0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libvib, libvib.o (sceVibGetProfile; sceVibSetActParam was
 * dead-stripped). Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE),
 * like the prebuilt libvib.a.
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

typedef struct {
    unsigned long a : 14;
    unsigned long b : 2;
    unsigned long c : 8;
    unsigned long d : 8;
} Mode125218;

extern int sceDbcReceiveData(int, Mode125218, int *, int);

/* Builds a mode word from bitfields and runs the func_00124A70 round
   trip. The fields are 64-bit, so each insert is a doubleword and/or,
   and the struct is passed by value. `if (r >= 0) return out;` keeps
   retail's two returns; an `r = out` spelling becomes slt/movz. */
int sce_vib_get_profile(int arg0, int arg1) {
    Mode125218 m;
    int out;
    int r;

    m.a = 2;
    m.b = 2;
    m.c = 3;
    m.d = 1;
    r = sceDbcReceiveData(arg0, m, &out, arg1);
    if (r >= 0) {
        return out;
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_00125298);
