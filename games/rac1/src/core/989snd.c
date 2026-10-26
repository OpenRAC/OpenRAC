#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * 989snd.c (0x12DB68-0x12F308): 989 Studios' sound library. The
 * "/usr/local/989snd/ee/989snd.c" path string is used from 0x12DBE0 on.
 * Name and extent from the NTSC split, shifted +0x140.
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

/* snd_StartSoundSystem. Ported from Lombyte's project (GPL-3.0); exact for the whole
 * 989snd object with retail ps2eeas and explicit small-data declarations.
 * The compiler output is assembled unchanged. */
struct StartSoundWork {
    volatile s32 read_active;
    u8 pad_4[0xC];
    volatile s32 read_error;
};
extern struct StartSoundWork sound_read_work_r __asm__("D_00137C00");
struct SifClientDataStartSound {
    u8 pad_0[0x24];
    void *volatile server;
};
extern u8 snd_command_buffer1[];
extern u8 snd_command_buffer2[];
extern u8 snd_stream_buffer1[];
extern u8 snd_stream_buffer2[];
extern u8 snd_return_buffer1[];
extern u8 snd_return_buffer2[];
extern u8 snd_sif_bind_rpc_error_string[];
extern u8 snd_989_snd_source_file[];
extern struct SifClientDataStartSound sound_command_client_r __asm__("D_0015ECC0");
extern struct SifClientDataStartSound sound_stream_client_r __asm__("D_0015ECE8");
extern short snd_batch_command_buffers;
extern short snd_batch_free_bytes;
extern s32 snd_batch_free_bytes_1 MACRO_ADDR;
extern short snd_stream_buffers;
extern short snd_batch_return_buffers;
extern short snd_cd_callback_pending;
extern short snd_cd_callback_fn;
extern s64 snd_cd_callback_data MACRO_ADDR;
extern s32 snd_cd_callback_arg_r __asm__("D_0015EE00") MACRO_ADDR;
extern void sceSifInitRpc(u32);
extern s32 sceSifBindRpc(struct SifClientDataStartSound *, u32, s32);
extern s32 func_00116078_2DB68(const char *, ...) __asm__("func_00116078");
extern s32 snd_SendIOPCommandAndWait(s32, s32, void *);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/audio/snd_start_sound_system.c, snd_start_sound_system. */
s32 snd_StartSoundSystem(void) {
    s32 bind_result;
    s32 command_arg;
    u8 *const command_buffer1 = snd_command_buffer2;
    u8 *const command_buffer0 = snd_command_buffer1;

    ((u8 **)&snd_batch_command_buffers)[0] = command_buffer0;
    ((u8 **)&snd_batch_command_buffers)[1] = command_buffer1;
    ((u8 **)&snd_batch_return_buffers)[0] = snd_return_buffer1;
    ((u8 **)&snd_batch_return_buffers)[1] = snd_return_buffer2;
    ((u8 **)&snd_stream_buffers)[0] = snd_stream_buffer1;
    ((u8 **)&snd_stream_buffers)[1] = snd_stream_buffer2;
    sceSifInitRpc(0);

    for (;;) {
        bind_result = sceSifBindRpc(&sound_command_client_r, 0x123456, 0);
        if (bind_result < 0) {
            func_00116078_2DB68((const char *)snd_sif_bind_rpc_error_string, snd_989_snd_source_file, 0x73);
            for (;;) {
            }
        }
        command_arg = 10000;
        for (command_arg--; command_arg != -1; command_arg--) {
        }
        if (sound_command_client_r.server != 0) {
            break;
        }
    }

    (*(s32 *)&snd_cd_callback_pending) = 0;
    (*(s32 *)&snd_cd_callback_fn) = 0;
    snd_cd_callback_data = 0;
    snd_cd_callback_arg_r = 0;

    do {
        bind_result = sceSifBindRpc(&sound_stream_client_r, 0x123457, 0);
        if (bind_result < 0) {
            func_00116078_2DB68((const char *)snd_sif_bind_rpc_error_string, snd_989_snd_source_file, 0x88);
            for (;;) {
            }
        }
        command_arg = 10000;
        for (command_arg--; command_arg != -1; command_arg--) {
        }
    } while (sound_stream_client_r.server == 0);

    *(s32 *)snd_command_buffer1 = 0;
    sound_read_work_r.read_active = 0;
    *(s32 *)snd_command_buffer2 = 0;
    sound_read_work_r.read_error = 0;
    (*(s32 *)&snd_batch_free_bytes) = 0xFFC;
    snd_batch_free_bytes_1 = 0xFFC;
    command_arg = (s32)(u32)&sound_read_work_r.read_active;
    return snd_SendIOPCommandAndWait(0, 4, &command_arg);
}

typedef void (*SndCallback)(int, long);
struct SndCommand {
    SndCallback fn;
    int pad4;
    long arg;
};
/* All SDA ($gp); read through casts like the rest of this file. */
extern short snd_current_buffer;   /* pending RPC record */
extern short snd_cd_status_callback;   /* stop callback */
extern short snd_cd_sync_pending;   /* stop requested */
extern short snd_cd_stream_end_pending;
extern short snd_return_callback_pending;   /* abort */
extern short snd_batch_command_buffers;   /* int *count[2] */
extern short snd_stream_buffers;   /* struct SndCommand *cmd[2] */
extern short snd_batch_return_buffers;   /* int *reply[2] */
extern short snd_batch_index;   /* current buffer */
extern short GlobalTableStatus;
extern short snd_cd_callback_pending;   /* CD read pending */
extern short snd_cd_callback_fn;   /* CD read callback */
extern short snd_return_callback;   /* abort callback */
extern short snd_return_callback_data;   /* its argument (64-bit) */
/* volatile: retail's compiler keeps its 64-bit accesses out of delay
   slots, so they stay lui-based rather than going through $gp. */
extern volatile long snd_cd_callback_data_l __asm__("D_0015EDD8") MACRO_ADDR;
extern unsigned int snd_cd_callback_arg MACRO_ADDR;
extern int snd_iop_return_word[];
extern int snd_GotReturns(void);
extern void snd_SendCurrentBatch(void);
extern int snd_StreamSafeCdSync(int);

#define SDA_I(x) (*(int *)&(x))
#define SND_COUNT ((int **)&D_0015EDA0)
#define SND_CMDS ((struct SndCommand **)&D_0015EDB0)
#define SND_REPLY ((int **)&D_0015EDB8)

/* snd_FlushSoundCommands: once the pending RPC completes, run the
   callbacks queued with the other command buffer (or the abort callback);
   finish a CD read (FlushCache, then its callback with the reply word);
   send the next batch when idle; and poll a stop request. Returns
   whether an RPC or a CD read is still in flight. Adapted from Lombyte's project
   (GPL-3.0) for PAL. */
int snd_FlushSoundCommands(void) {
    int i;
    int idx;
    SndCallback fn;
    long arg;

    if (SDA_I(snd_current_buffer) != 0 && snd_GotReturns() != 0) {
        if (SDA_I(snd_return_callback_pending) != 0) {
            if ((SndCallback)SDA_I(snd_return_callback) != 0) {
                ((SndCallback)SDA_I(snd_return_callback))(snd_iop_return_word[0], *(long *)&snd_return_callback_data);
            }
            SDA_I(snd_return_callback) = 0;
            SDA_I(snd_return_callback_pending) = 0;
        } else {
            idx = SDA_I(snd_batch_index) != 1;
            for (i = 0; i < *SND_COUNT[idx]; i++) {
                if (SND_CMDS[idx][i].fn != 0) {
                    SND_CMDS[idx][i].fn(SND_REPLY[idx][i + 1], SND_CMDS[idx][i].arg);
                }
            }
        }
    }
    if (SDA_I(snd_cd_callback_pending) != 0) {
        flush_cache(0);
        if (snd_cd_callback_arg != 0xFFFFFFFF) {
            if ((SndCallback)SDA_I(snd_cd_callback_fn) != 0) {
                arg = snd_cd_callback_data_l;
                fn = (SndCallback)SDA_I(snd_cd_callback_fn);
                SDA_I(snd_cd_callback_fn) = 0;
                snd_cd_callback_data_l = 0;
                fn(snd_cd_callback_arg, arg);
            }
            snd_cd_callback_arg = 0;
            SDA_I(snd_cd_callback_pending) = 0;
        }
    }
    if (SDA_I(snd_current_buffer) == 0) {
        if (*SND_COUNT[SDA_I(snd_batch_index)] != 0 && SDA_I(GlobalTableStatus) == 0) {
            snd_SendCurrentBatch();
        }
    }
    if (SDA_I(snd_cd_sync_pending) != 0) {
        snd_StreamSafeCdSync(1);
        if (SDA_I(snd_cd_stream_end_pending) != 0) {
            SDA_I(snd_cd_sync_pending) = 0;
            SDA_I(snd_cd_stream_end_pending) = 0;
            if ((void (*)(int))SDA_I(snd_cd_status_callback) != 0) {
                ((void (*)(int))SDA_I(snd_cd_status_callback))(1);
            }
        }
    }
    return SDA_I(snd_current_buffer) != 0 || SDA_I(snd_cd_callback_pending) != 0;
}

#undef SDA_I
#undef SND_COUNT
#undef SND_CMDS
#undef SND_REPLY

LINKER_REMNANT("asm/remnants/core_text", func_0012DFA0);

extern void flush_cache(int arg0);
extern int sound_command_client;
extern int func_00116078(void *arg0);
extern char snd_no_return_error_string[];
extern int sce_sif_check_stat_rpc(void *arg0);
extern short snd_current_buffer;
extern short snd_current_buffer_index;

/* snd_GotReturns: true once the pending IOP reply record's sentinel word
 * and its per-slot echo both read back -1, after taking the returns
 * semaphore (D_0015ECC0). D_0015ED80 is a pointer to that record, not a
 * plain flag; D_0015ED84 is the slot index within it. The slot address
 * is left as one inlined expression (no separate idx/slot locals) --
 * that is what gets retail's addu operand order and epilogue merge. */
int snd_GotReturns(void) {
    int *p;

    flush_cache(0);
    if (*(int *)&snd_current_buffer == 0) {
        return 1;
    }
    if (sce_sif_check_stat_rpc(&sound_command_client) != 0) {
        return 0;
    }
    p = *(int **)&snd_current_buffer;
    if ((unsigned int)p[0] != 0xFFFFFFFFU) {
        func_00116078(snd_no_return_error_string);
        return 0;
    }
    if ((unsigned int)*(int *)((*(int *)&snd_current_buffer_index) * 4 + (char *)p + 4) !=
        (unsigned int)p[0]) {
        func_00116078(snd_no_return_error_string);
        return 0;
    }
    *(int *)&snd_current_buffer = 0;
    return 1;
}


/* snd_PrepareReturnBuffer */
void snd_PrepareReturnBuffer(void *arg0, int arg1) {
    int *p = (int *)arg0;
    *(int *)&snd_current_buffer_index = arg1;
    *(int *)&snd_current_buffer = (int)arg0;
    p[arg1 + 1] = 0;
    p[0] = 0;
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E058);

extern short snd_cd_callback_pending;
extern short snd_cd_load_error;
extern int snd_cd_bank_load_request_a[] __asm__("D_00137C40");
extern int sound_stream_client;
extern char snd_bank_load_by_loc_progress_string[];
extern char snd_bank_load_by_loc_cd_busy_string[];
extern char snd_bank_load_by_loc_rpc_error_string[];
extern char snd_non_idle_error_string[];
extern int snd_FlushSoundCommands(void);

/* IOP command 3 with two words, waiting for the reply word in
   D_0015EE00; 0 (and a message) when a stream is pending, the CD is
   busy or the RPC fails (error 0x106 in D_0015ED88). */
unsigned int snd_BankLoadByLoc(int arg0, int arg1) {
    *(int *)&snd_cd_load_error = 0;
    if (*(int *)&snd_cd_callback_pending != 0) {
        func_00116078(snd_bank_load_by_loc_progress_string);
        return 0;
    }
    if (snd_StreamSafeCdSync(1) == 1) {
        func_00116078(snd_bank_load_by_loc_cd_busy_string);
        return 0;
    }
    snd_cd_bank_load_request_a[1] = arg1;
    snd_cd_callback_arg = 0xFFFFFFFF;
    snd_cd_bank_load_request_a[0] = arg0;
    while (sce_sif_check_stat_rpc(&sound_stream_client) != 0) {
        func_00116078(snd_non_idle_error_string);
        snd_FlushSoundCommands();
        flush_cache(0);
    }
    if (sif_call_rpc(&sound_stream_client, 3, 1, snd_cd_bank_load_request_a, 8, &snd_cd_callback_arg, 4, 0, 0) < 0) {
        func_00116078(snd_bank_load_by_loc_rpc_error_string);
        *(int *)&snd_cd_load_error = 0x106;
        return 0;
    }
    if (snd_cd_callback_arg == 0xFFFFFFFF) {
        do {
            flush_cache(0);
        } while (snd_cd_callback_arg == 0xFFFFFFFF);
    }
    return snd_cd_callback_arg;
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E1B8);

extern short snd_cd_callback_fn;
extern long snd_cd_callback_data MACRO_ADDR;
extern int snd_cd_bank_load_request;
extern char snd_bank_load_from_ee_progress_string[];
extern char snd_bank_load_from_ee_cd_busy_string[];

/* Start an IOP stream (command 0x57) for arg0, unless one is already
   pending (D_0015EDC8) or the CD is busy (func_0012EF48); the reply word
   lands in D_0015EE00. */
void snd_BankLoadFromEE_CB(int arg0, int arg1, long arg2) {
    *(int *)&snd_cd_load_error = 0;
    if (*(int *)&snd_cd_callback_pending != 0) {
        func_00116078(snd_bank_load_from_ee_progress_string);
        return;
    }
    if (snd_StreamSafeCdSync(1) == 1) {
        func_00116078(snd_bank_load_from_ee_cd_busy_string);
        return;
    }
    snd_cd_bank_load_request = arg0;
    snd_cd_callback_arg = 0xFFFFFFFF;
    *(int *)&snd_cd_callback_fn = arg1;
    snd_cd_callback_data = arg2;
    while (sce_sif_check_stat_rpc(&sound_stream_client) != 0) {
        func_00116078(snd_non_idle_error_string);
        snd_FlushSoundCommands();
        flush_cache(0);
    }
    *(int *)&snd_cd_callback_pending = 1;
    sif_call_rpc(&sound_stream_client, 0x57, 1, &snd_cd_bank_load_request, 4, &snd_cd_callback_arg, 4, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E2D8);

extern int snd_SendIOPCommandNoWait(int, int, void *, int, int);

/* Command 8 with no data. */
void snd_ResolveBankXREFS(void) {
    snd_SendIOPCommandNoWait(8, 0, 0, 0, 0);
}

extern int snd_SendIOPCommandNoWait(int, int, void *, int, int);

/* snd_UnloadBank */
void snd_UnloadBank(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(6, 4, &local, 0, 0);
}

/* snd_SetMasterVolume */
void snd_SetMasterVolume(int arg0, int arg1) {
    int local[2];
    local[0] = arg0;
    local[1] = arg1;
    snd_SendIOPCommandNoWait(9, 8, local, 0, 0);
}

/* snd_SetPlaybackMode */
void snd_SetPlaybackMode(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0xB, 4, &local, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E3B0);

/* Command 0xD with two ints. */
void snd_set_mixer_mode(int arg0, int arg1) {
    int local[2];
    local[0] = arg0;
    local[1] = arg1;
    snd_SendIOPCommandNoWait(0xD, 8, local, 0, 0);
}

/* snd_SetGroupVoiceRange */
void snd_SetGroupVoiceRange(int arg0, int arg1, int arg2) {
    int local[3];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    snd_SendIOPCommandNoWait(0x4E, 0xC, local, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E438);

/* Command 0x11 with six ints; the last two arguments are passed through. */
void snd_play_sound_vol_pan_pmpb(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
                   int arg6, int arg7) {
    int local[6];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    local[3] = arg3;
    local[4] = arg4;
    local[5] = arg5;
    snd_SendIOPCommandNoWait(0x11, 0x18, local, arg6, arg7);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E490);

/* Command 0x15 with one int. */
void snd_stop_sound(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x15, 4, &local, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E4D8);

/* Command 0x18 with no data. */
void snd_StopAllSounds(void) {
    snd_SendIOPCommandNoWait(0x18, 0, 0, 0, 0);
}

/* snd_PauseAllSoundsInGroup */
void snd_PauseAllSoundsInGroup(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x16, 4, &local, 0, 0);
}

/* snd_ContinueAllSoundsInGroup */
void snd_ContinueAllSoundsInGroup(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x17, 4, &local, 0, 0);
}

/* snd_SoundIsStillPlaying_CB */
void snd_SoundIsStillPlaying_CB(int arg0, int arg1, int arg2) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x19, 4, &local, arg1, arg2);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E5B8);

/* Command 0x21 with six ints; the last two arguments are passed through. */
void snd_set_sound_params_cb(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
                   int arg6, int arg7) {
    int local[6];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    local[3] = arg3;
    local[4] = arg4;
    local[5] = arg5;
    snd_SendIOPCommandNoWait(0x21, 0x18, local, arg6, arg7);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012E648);

extern unsigned char snd_sync_send_buffer_c[] __asm__("D_00133240");
extern int snd_sync_buffer_w[] __asm__("D_00133200");
extern int snd_PrepareReturnBuffer_s(int *, int) __asm__("func_0012E038");

/* snd_SendIOPCommandAndWait: copy the command data into the send buffer,
   flush pending commands, wait for the RPC server, call command cmd and
   wait for its reply; the reply's second word is the result. Adapted
   from Lombyte's project (GPL-3.0) for PAL. */
int snd_SendIOPCommandAndWait(int cmd, int size, void *arg) {
    unsigned char *data = arg;
    int i;
    int result;

    for (i = 0; i < size; i++) {
        snd_sync_send_buffer_c[i] = data[i];
    }
    goto check;
    do {
        snd_FlushSoundCommands();
        flush_cache(0);
check:;
    } while (*(int *)&snd_current_buffer != 0);
    snd_PrepareReturnBuffer_s(snd_sync_buffer_w, 1);
    while (sce_sif_check_stat_rpc(&sound_command_client) != 0) {
        func_00116078(snd_non_idle_error_string);
        snd_FlushSoundCommands();
        flush_cache(0);
    }
    if (size != 0) {
        sif_call_rpc(&sound_command_client, cmd, 1, snd_sync_send_buffer_c, size, snd_sync_buffer_w, 0xC, 0, 0);
    } else {
        sif_call_rpc(&sound_command_client, cmd, 1, 0, 0, snd_sync_buffer_w, 0xC, 0, 0);
    }
    while (snd_GotReturns() == 0) {
    }
    result = snd_sync_buffer_w[1];
    if (*((int **)&snd_batch_command_buffers)[*(int *)&snd_batch_index] != 0 && *(int *)&GlobalTableStatus == 0) {
        snd_SendCurrentBatch();
    }
    return result;
}

typedef struct {
    int func;
    int pad;
    long data;
} SndCb_2E820;

extern short snd_batch_index;
extern short snd_batch_command_buffers;
extern short snd_batch_free_bytes;
extern short snd_stream_buffers;
extern char snd_send_iop_command_buffer_full_string[];
extern char snd_no_wait_continuing_string[];
extern void snd_PostMessage(void);
extern int func_00116078_p(const char *, ...) __asm__("func_00116078");

/* snd_SendIOPCommandNoWait: sends a bare command at once when nothing is queued or pending; otherwise waits
   for room in the current batch, appends the command and its data, and records the callback for its reply.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/audio/rpc/snd_send_iop_command_no_wait.c, snd_send_iop_command_no_wait. */
void func_0012E820_r(int cmd, int size, unsigned char *data, int cb, long cb_data) __asm__("func_0012E820");
/* snd_SendIOPCommandNoWait */
void func_0012E820_r(int cmd, int size, unsigned char *data, int cb, long cb_data) {
    int need;
    int tries;
    int held;
    int i;
    int slot;
    unsigned char *p;

    held = 0;
    if (*(int *)&GlobalTableStatus == 0 && *(int *)&snd_current_buffer == 0 && size == 0 && cb == 0) {
        snd_PrepareReturnBuffer_s(snd_sync_buffer_w, 1);
        while (sce_sif_check_stat_rpc(&sound_command_client) != 0) {
            func_00116078_p(snd_non_idle_error_string);
            snd_FlushSoundCommands();
            flush_cache(0);
        }
        sif_call_rpc(&sound_command_client, cmd, 1, 0, 0, snd_sync_buffer_w, 0xC, 0, 0);
        return;
    }
    need = size + 4;
    tries = 0;
    if (need % 4 != 0) {
        need += 4 - need % 4;
    }
    while (*((int **)&snd_batch_command_buffers)[*(int *)&snd_batch_index] == 0x100 ||
           ((int *)&snd_batch_free_bytes)[*(int *)&snd_batch_index] < need) {
        if (*(int *)&GlobalTableStatus != 0) {
            *(int *)&GlobalTableStatus = 0;
            held = 1;
        }
        snd_FlushSoundCommands();
        if (tries == 1) {
            func_00116078_p(snd_send_iop_command_buffer_full_string, *(int *)&snd_batch_index, *((int **)&snd_batch_command_buffers)[*(int *)&snd_batch_index]);
        }
        tries++;
    }
    if (tries != 0) {
        func_00116078_p(snd_no_wait_continuing_string, tries);
    }
    if (held != 0) {
        *(int *)&GlobalTableStatus = 1;
    }
    p = (unsigned char *)((int **)&snd_batch_command_buffers)[*(int *)&snd_batch_index] -
        (((int *)&snd_batch_free_bytes)[*(int *)&snd_batch_index] - 0x1000);
    *(short *)p = cmd;
    p += 2;
    *(short *)p = size;
    p += 2;
    for (i = 0; i < size; i++) {
        p[i] = data[i];
    }
    ((int *)&snd_batch_free_bytes)[*(int *)&snd_batch_index] -= need;
    slot = *(int *)&snd_batch_index;
    ((SndCb_2E820 **)&snd_stream_buffers)[slot][*((int **)&snd_batch_command_buffers)[slot]].func = cb;
    ((SndCb_2E820 **)&snd_stream_buffers)[slot][*((int **)&snd_batch_command_buffers)[slot]].data = cb_data;
    snd_PostMessage();
}

/* gp-relative, no retail symbol: gp 0x166D00 - 0x7F40 = 0x15EDC0
   (message-queue index) and gp 0x166D00 - 0x7F60 = 0x15EDA0 (array of
   pointers to the per-queue message-count int). */

/* snd_PostMessage */
void snd_PostMessage(void) {
    int idx = *(int *)&snd_batch_index;
    int **base = (int **)&snd_batch_command_buffers;
    int *msg = base[idx];

    *msg = *msg + 1;
    snd_FlushSoundCommands();
}

extern s32 snd_batch_index_sb SDATA(D_0015EDC0);
extern s32 *snd_batch_command_buffers_sb[2] SDATA(D_0015EDA0);
extern s32 snd_batch_free_bytes_sb[2] SDATA(D_0015EDA8);
extern void *snd_batch_return_buffers_sb[2] SDATA(D_0015EDB8);
extern u8 sound_command_client_sb[] __asm__("D_0015ECC0");
extern char snd_non_idle_error_string_sb[] __asm__("D_00153E20");
extern void StoreObjectIndex_sb(void *, s32, s32 *) __asm__("func_0012E038");
extern s32 SceSifCheckStatRpc_sb(void *) __asm__("func_0011B6B8");
extern s32 printf_sb(const char *, ...) __asm__("func_00116078");
extern void FlushCache_sb(s32) __asm__("func_00118D80");
extern s32 sceSifCallRpc_sb(void *, u32, u32, void *, s32, void *, s32, void (*)(void *), void *) __asm__("func_0011B4C8");

/* snd_SendCurrentBatch: store the reply size into the current batch,
   wait until the sound RPC client is idle (complaining and flushing the
   cache while it is not), send the batch (command 0x4D) with its return
   buffer, then flip to the other batch and reset it.
   Adapted from Lombyte's project (GPL-3.0): src/audio/rpc/snd_send_current_batch.c, snd_send_current_batch. */
void snd_SendCurrentBatch(void) {
    s32 next;

    StoreObjectIndex_sb(snd_batch_return_buffers_sb[snd_batch_index_sb], *snd_batch_command_buffers_sb[snd_batch_index_sb], snd_batch_command_buffers_sb[snd_batch_index_sb]);
    while (SceSifCheckStatRpc_sb(sound_command_client_sb) != 0) {
        printf_sb(snd_non_idle_error_string_sb);
        FlushCache_sb(0);
    }
    sceSifCallRpc_sb(sound_command_client_sb, 0x4D, 1, snd_batch_command_buffers_sb[snd_batch_index_sb], 0x1000 - snd_batch_free_bytes_sb[snd_batch_index_sb],
                  snd_batch_return_buffers_sb[snd_batch_index_sb], (*snd_batch_command_buffers_sb[snd_batch_index_sb] << 2) + 8, 0, 0);
    next = snd_batch_index_sb != 1;
    snd_batch_index_sb = next;
    *snd_batch_command_buffers_sb[next] = 0;
    snd_batch_free_bytes_sb[next] = 0xFFC;
}

/* gp-relative: declared small so -G2 places it in the small-data area,
   accessed as the word it really is. gp 0x166D00 - 0x7F3C = 0x15EDC4. */

/* snd_UnkFunction_0012eaf0 */
void snd_reenable_command_flush(void) {
    *(int *)&GlobalTableStatus = 1;
}


/*
 * Close, not exact (2/32), same size. Retail saves/restores $ra with
 * sq/lq here; v1.36 (correctly for core_text overall) emits sd/ld, so
 * the two spill instructions differ and nothing else does. This is one
 * of the ~14 core_text functions on the sq side of that split -- the
 * still-open half of the sq/lq question, not a source-shape problem.
 */
/* snd_UnkFunction_0012eb00 */
void snd_reset_state_and_flush_commands(void) {
    *(int *)&GlobalTableStatus = 0;
    snd_FlushSoundCommands();
}

/* gp-relative, no retail symbol: gp 0x166D00 - 0x7F74 = 0x15ED8C
   (streaming-enabled flag, same as func_0012F030/func_0012F068's) and
   gp 0x166D00 - 0x7F38 = 0x15EDC8 (drain-commands-first flag). */
extern short snd_cd_stream_active;
extern short snd_batch_return_buffers_1;   /* the word at +0xC, SDA ($gp -0x7F38) */
extern int snd_StreamSafeCdSync(int);              /* snd_StreamSafeCdSync */
extern int snd_SendIOPCommandAndWait(int, int, void *); /* snd_SendIOPCommandAndWait */

/* snd_InitVAGStreamingEx: no-op (returns 0) if VAG streaming is already
 * enabled; otherwise drains any pending sound commands, syncs the CD
 * stream, and sends the "init streaming" IOP command with the four
 * arguments packed into a header, storing (and returning) its result as
 * the streaming-enabled flag. */
int snd_InitVAGStreamingEx(int arg0, int arg1, int arg2, int arg3) {
    int hdr[4];
    int r;

    if (*(int *)&snd_cd_stream_active == 1) {
        return 0;
    }
    if (*(int *)&snd_cd_callback_pending != 0) {
        while (snd_FlushSoundCommands() != 0) {
        }
    }
    snd_StreamSafeCdSync(0);
    hdr[0] = arg0;
    hdr[1] = arg1;
    hdr[2] = arg2;
    hdr[3] = arg3;
    r = snd_SendIOPCommandAndWait(0x2A, 0x10, hdr);
    *(int *)&snd_cd_stream_active = r;
    return r;
}

/* snd_StopAllStreams */
void snd_StopAllStreams(void) {
    snd_SendIOPCommandNoWait(0x34, 0, 0, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012ED40);

extern int snd_SendIOPCommandNoWait_l(int, int, void *, int, long) __asm__("func_0012E820");

/* Command 0x2C: seven words, the middle two each packing two 16-bit
   values; the last two arguments are passed through. */
void snd_PlayVAGStreamByLocEx_CB(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
                   int arg6, int arg7, int arg8, int arg9, long arg10) {
    int local[7];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = (arg4 << 16) | (arg2 & 0xFFFF);
    local[3] = (arg5 << 16) | (arg3 & 0xFFFF);
    local[4] = arg6;
    local[5] = arg7;
    local[6] = arg8;
    snd_SendIOPCommandNoWait_l(0x2C, 0x1C, local, arg9, arg10);
}

/* snd_PauseVAGStream */
void snd_PauseVAGStream(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x2D, 4, &local, 0, 0);
}

/* snd_ContinueVAGStream */
void snd_ContinueVAGStream(int arg0) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x2E, 4, &local, 0, 0);
}

/* snd_GetVAGStreamTimeRemaining_CB */
void snd_GetVAGStreamTimeRemaining_CB(int arg0, int arg1, int arg2) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x32, 4, &local, arg1, arg2);
}

/* snd_IsVAGStreamBuffered_CB */
void snd_IsVAGStreamBuffered_CB(int arg0, int arg1, int arg2) {
    int local = arg0;
    snd_SendIOPCommandNoWait(0x4F, 4, &local, arg1, arg2);
}

extern int snd_SendIOPCommandAndWait(int, int, void *);

/* snd_StreamSafeCheckCDIdle */
void snd_StreamSafeCheckCDIdle(int arg0) {
    int local = arg0;
    snd_SendIOPCommandAndWait(0x36, 4, &local);
}

/* gp-relative, no retail symbol: gp 0x166D00 - 0x7F74 = 0x15ED8C
   (streaming-enabled flag, see func_0012F030/func_0012F068). */
extern short snd_cd_sync_pending;

typedef struct {
    volatile unsigned int count;
    int unk04[3];
    volatile int error;
} CdSafeState;
extern CdSafeState sound_read_work;

extern int sce_cd_read(int, int, int);

/* snd_StreamSafeCdRead */
int snd_StreamSafeCdRead(int arg0, int arg1, int arg2) {
    int local[3];

    if (*(int *)&snd_cd_stream_active == 0) {
        return sce_cd_read(arg0, arg1, arg2);
    }

    if (snd_StreamSafeCdSync(1) == 1) {
        return 0;
    }

    local[0] = arg0;
    sound_read_work.count = 1;
    sound_read_work.error = 0;
    local[1] = arg1;
    local[2] = arg2;
    snd_SendIOPCommandNoWait(0x38, 12, local, 0, 0);
    *(int *)&snd_cd_sync_pending = 1;
    *(int *)&snd_cd_stream_end_pending = 0;
    return 1;
}

extern void flush_cache(int);

/* snd_StreamSafeCdSync */
int snd_StreamSafeCdSync(int arg0) {
    if (*(int *)&snd_cd_stream_active == 0) {
        return sce_cd_sync_s(arg0);
    }

    flush_cache(0);
    *(int *)&snd_cd_stream_end_pending = sound_read_work.count == 0;
    if (*(int *)&snd_cd_stream_end_pending == 1) {
        return 0;
    }

    if (arg0 == 1) {
        return 1;
    }

    while (*(int *)&snd_cd_stream_end_pending == 0) {
        snd_FlushSoundCommands();
        flush_cache(0);
        *(int *)&snd_cd_stream_end_pending = sound_read_work.count == 0;
    }

    return 0;
}

extern int sce_cd_break(void);

/* snd_StreamSafeCdBreak */
int snd_StreamSafeCdBreak(void) {
    if (*(int *)&snd_cd_stream_active == 0) {
        return sce_cd_break();
    }
    snd_SendIOPCommandNoWait(0x37, 0, 0, 0, 0);
    return 1;
}

extern int sce_cd_get_error(void);

/* snd_StreamSafeCdGetError: returns the last CD error while VAG streaming
 * owns the drive (the cached value at D_00137C00.error), otherwise defers
 * to sceCdGetError. The field access (not a byte-offset cast) plus
 * volatile is what makes retail's separate lui+addiu base and its
 * duplicated epilogue restore come out of this compiler. */
int snd_StreamSafeCdGetError(void) {
    if (*(int *)&snd_cd_stream_active == 0) {
        return sce_cd_get_error();
    }
    return sound_read_work.error;
}

/* gp-relative, no retail symbol: gp 0x166D00 - 0x7F74 = 0x15ED8C
   (streaming-enabled flag, same as func_0012F030's) and
   gp 0x166D00 - 0x7F70 = 0x15ED90 (cached callback value). */
extern short snd_cd_status_callback;
extern int sce_cd_callback(void);

/* snd_StreamSafeCdCallback */
int snd_StreamSafeCdCallback(int arg0) {
    int old;

    if (*(int *)&snd_cd_stream_active == 0) {
        return sce_cd_callback();
    }
    old = *(int *)&snd_cd_status_callback;
    *(int *)&snd_cd_status_callback = arg0;
    return old;
}

LINKER_REMNANT("asm/remnants/core_text", func_0012F098);

/* Command 0x50 with five ints. */
void snd_set_reverb_ex(int arg0, int arg1, int arg2, int arg3, int arg4) {
    int local[5];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    local[3] = arg3;
    local[4] = arg4;
    snd_SendIOPCommandNoWait(0x50, 0x14, local, 0, 0);
}

/* snd_PreAllocReverbWorkArea */
void snd_PreAllocReverbWorkArea(int arg0, int arg1) {
    int local[2];
    local[0] = arg0;
    local[1] = arg1;
    snd_SendIOPCommandNoWait(0x51, 8, local, 0, 0);
}

/* snd_AutoReverb */
void snd_AutoReverb(int arg0, int arg1, int arg2, int arg3) {
    int local[4];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    local[3] = arg3;
    snd_SendIOPCommandNoWait(0x10, 0x10, local, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012F160);

/* Command 0x3B with six ints, waiting for the reply. */
int snd_init_movie_sound(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
    int local[6];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    local[3] = arg3;
    local[4] = arg4;
    local[5] = arg5;
    return snd_SendIOPCommandAndWait(0x3B, 0x18, local);
}

/* snd_ResetMovieSound */
void snd_ResetMovieSound(void) {
    snd_SendIOPCommandAndWait(0x3D, 0, 0);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012F210);

/* Command 0x3C with no data, waiting for the reply. */
int snd_close_movie_sound(void) {
    return snd_SendIOPCommandAndWait(0x3C, 0, 0);
}

/* snd_StartMovieSound */
void snd_StartMovieSound(int arg0, int arg1, int arg2, int arg3, int arg4) {
    int local[5];
    local[0] = arg0;
    local[1] = arg1;
    local[2] = arg2;
    local[3] = arg3;
    local[4] = arg4;
    snd_SendIOPCommandAndWait(0x3E, 0x14, local);
}

LINKER_REMNANT("asm/remnants/core_text", func_0012F280);

/* Command 0x5A with two ints, waiting for the reply. */
int snd_UpdateMovieADPCM(int arg0, int arg1) {
    int local[2];
    local[0] = arg0;
    local[1] = arg1;
    return snd_SendIOPCommandAndWait(0x5A, 8, local);
}

/* snd_GetMovieNAX */
void snd_GetMovieNAX(void) {
    snd_SendIOPCommandAndWait(0x5B, 0, 0);
}

/* snd_GetDopplerPitchMod */
int snd_GetDopplerPitchMod(int arg0) {
    return (arg0 * 0x5F4) / 0x2E5;
}
