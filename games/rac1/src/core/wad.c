#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * wad.cpp (0x12F348-0x12F580), the last object in core_text. Name from
 * the NTSC split.
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
extern short snd_current_buffer_index;
extern short snd_current_buffer;
extern short GlobalTableStatus;
extern void snd_FlushSoundCommands(void);

extern unsigned char cd_read_spindle_speed[4] MACRO_ADDR;
extern int sce_cd_read(int, int, void *, void *);
extern void sce_gs_sync_v(int);
extern int sce_cd_get_error(void);

/* wad_GetSectors_FiiPv(lba, sectors, buf): sceCdRead with the usual
 * request/poll/retry shape (see func_0012F3F8/func_0012F4A8 below),
 * returning the byte count read (sectors << 11 = sectors * 0x800). */
int wad_GetSectors_FiiPv(int lba, int sectors, void *buf) {
    char hdr[4];

    hdr[0] = 0x20;
    hdr[1] = cd_read_spindle_speed[0];
    hdr[2] = 0;
    hdr[3] = 0;

    do {
        sce_cd_read(lba, sectors, buf, &hdr);
        while (sce_cd_sync_s(1) != 0) {
            sce_gs_sync_v(0);
        }
    } while (sce_cd_get_error() != 0);

    return sectors << 11;
}

typedef struct { char b[4]; } StreamHdr;
extern int disc_table[];

/* Same request/poll/retry shape as func_0012F348 above, reading 6 fixed
   sectors into a stack buffer and copying 0x2960 bytes of it into
   D_00137C80. The nop after `jal func_00120F30` in the poll loop is
   ps2eeas's short-loop padding (tools/ps2eeas_nops.py). */
int load_disc_sectors_into_global_buffer(void) {
    char buf[0x3000];
    StreamHdr hdr;
    unsigned int i;

    hdr.b[0] = 0x20;
    hdr.b[1] = cd_read_spindle_speed[0];
    hdr.b[2] = 0;
    hdr.b[3] = 0;

    do {
        sce_cd_read(0x5DC, 6, buf, &hdr);
        while (sce_cd_sync_s(1) != 0) {
            sce_gs_sync_v(0);
        }
    } while (sce_cd_get_error() != 0);

    for (i = 0; i < 0x2960; i++) {
        ((char *)disc_table)[i] = buf[i];
    }
    return 1;
}

extern int disc_table_level_entries[];
extern int pad_globals_base[];

/* As func_0012F3F8 but the sector number comes from an 8-byte-stride
   table indexed by arg0 and re-read on every retry, and 0x2434 bytes are
   copied into D_0013A5E0. The entry address is formed after the header
   stores, which orders the two constant-address computations as retail
   does. */
int read_file_entry_with_retry(int arg0) {
    int *entry;
    char buf[0x2800];
    StreamHdr hdr;
    unsigned int i;

    hdr.b[0] = 0x20;
    hdr.b[1] = cd_read_spindle_speed[0];
    hdr.b[2] = 0;
    hdr.b[3] = 0;
    entry = &disc_table_level_entries[arg0 * 2];

    do {
        sce_cd_read(*entry, 5, buf, &hdr);
        while (sce_cd_sync_s(1) != 0) {
            sce_gs_sync_v(0);
        }
    } while (sce_cd_get_error() != 0);

    for (i = 0; i < 0x2434; i++) {
        ((char *)pad_globals_base)[i] = buf[i];
    }
    return 1;
}
