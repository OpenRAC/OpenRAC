#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * boot.cpp (0x12DA38-0x12DB68): ParseBin and main. crt0 calls func_0012DB18
 * as main. From 0x12DB18 on, retail's game compiler spills s-registers with
 * sq, as v1.14 does (docs/TOOLCHAIN.md). Name from the NTSC split.
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
extern void sysbit_flush(void *, int);
extern void _Error(void *, void *);
extern char mpeg_work_area_size_too_small_msg[];
extern void mpeg_decode_or_skip_field(void *);
extern void mpeg_decode_or_skip_frame(void *);
extern void mpeg_last_frame(void *);
extern void _dispRefImage(void *, int, int, int);
extern char mpeg_second_field_missing_msg[];
extern char mpeg_error_format[];
extern void sprintf(void *);
extern void mpeg_print_error(void *);
extern char mpeg_sequence_scalable_ext_msg[];
extern char mpeg_unknown_extension_msg[];
extern char mpeg_picture_spatial_scalable_ext_msg[];
extern char mpeg_picture_temporal_scalable_ext_msg[];
extern int sce_ipu_restart_dma(void *);
extern void scf_read_romver(void);
extern signed char RomNameStateData[];
extern int scf_bcd_to_bin(int);
extern int scf_bin_to_bcd(int);
extern void Exit(int);

extern int *level_root MACRO_ADDR;

typedef void (*BinFn)(void);

struct Chunk {
    unsigned char *dst;
    int size;
    int pad8;
    BinFn id;
};

/* ParseBin: copy each boot-binary chunk to its destination while chunk ids agree; return the id. Adapted from Lombyte's project (GPL-3.0) for PAL: gameplay/state/parse_bin.c, parse_bin. */
BinFn ParseBin(void) {
    unsigned char *p;
    struct Chunk *c;
    unsigned char *dst;
    unsigned char *src;
    BinFn id;

    id = 0;
    p = (unsigned char *)(level_root[0] + (int)level_root);
    while (c = (struct Chunk *)p, p = (unsigned char *)(c + 1), dst = c->dst, src = p,
           (id == 0) ? (id = c->id, 1) : (id == c->id)) {
        if (((c->size & 7) == 0) && (((unsigned int)p & 7) == 0) && (((unsigned int)dst & 7) == 0)) {
            unsigned char *d = dst;
            unsigned char *s = p;
            unsigned char *e = dst + c->size;
            while (d != e) {
                *(long *)d = *(long *)s;
                d += 8;
                s += 8;
            }
        } else {
            unsigned char *e = dst + c->size;
            while (dst != e) {
                *(int *)dst = *(int *)src;
                dst += 4;
                src += 4;
            }
        }
        p += c->size;
    }
    return id;
}


extern long func_00116108_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00116108");
extern long func_001188C8_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_001188C8");
extern long func_00114518_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00114518");




extern int fs_call_sema;


extern void sprintf_4(void *, char *, int, int) __asm__("func_00116248");
extern void _Error_a(void *, void *) __asm__("func_0012C468");
extern void _Error(void *, void *);
extern void sprintf(void *);

typedef void (*EntryFn)(void);
extern void func_0011DFC8(void);
extern EntryFn ParseBin(void); /* ParseBin(void) */
extern void startlevel(void);

void func_0012DB18(void) { /* main */
    EntryFn entry;

    func_0011DFC8();
    entry = startlevel;
    do {
        entry();
        entry = ParseBin();
        flush_cache(0);
        flush_cache(2);
    } while (1);
}
