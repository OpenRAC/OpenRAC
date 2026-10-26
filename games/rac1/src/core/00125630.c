#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x125630-0x12A2F0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's MPEG library (libmpeg): the macroblock/slice/picture decoder
 * ("Invalid macroblock_type code", "slice_start_code out of range", ...)
 * over the IPU (0x10002000 IPU_CMD, 0x10002010 IPU_CTRL). Built with
 * Sony's 2.9-ee like the rest of libmpeg (0012A2F0.c, 0012AC80.c): the IPU
 * wait loops that 2.95.3 hoisted into saved registers (func_00128968,
 * func_00128860) are 2.9-ee's rematerialised constants.
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

struct MpegDecoder;
extern void _errMessage(void *, char *, int);
extern void _getAllRefs(struct MpegDecoder *d, int bx, int by, int mb_type, int motion_type,
                          int pmv[2][2][2], int mvfs[2][2], int *dmvector);
extern void dma_ref_image(void *);
extern char str_mpeg_invalid_motion_type[];

typedef struct {
    char pad0[0x128];
    int unk128;
    int unk12C;
    int unk130;
    int unk134;
    int unk138;
    int unk13C;
} Elem00125630;

typedef struct {
    int unk0;
    int pad4[3];
    int unk10;
} Ref00125630;

typedef struct {
    char pad0[0x11C];
    int unk11C;
    char pad120[0xC];
    int unk12C;
    char pad130[0x44];
    int unk174;
    char pad178[0x48];
    Ref00125630 *unk1C0;
    char pad1C4[0xC];
    Ref00125630 *unk1D0;
    char pad1D4[0xC];
    Ref00125630 *unk1E0;
    char pad1E4[0x3AC];
    Elem00125630 e[2];
    int unk810;
} Dec00125630;

/* Motion-compensate one macroblock (or wait for the intra IDCT) and record its output slot. */
int _motionComp0(void *arg0, int mbAddr, int arg2, int mbType, int motionType,
                  void *pmv, void *mvfs, void *dmv) {
    Dec00125630 *d = (Dec00125630 *)arg0;
    int intra;
    int x;
    int y;
    int bx;
    int by;
    Ref00125630 *ref;

    intra = mbType & 1;
    y = mbAddr / d->unk12C;
    x = mbAddr % d->unk12C;
    by = y * 16;
    bx = x * 16;
    if (intra) {
        while ((*(volatile unsigned int *)0x1000D400 >> 8) & 1)
            ;
        d->e[d->unk810].unk138 = 0;
    } else {
        if ((unsigned int)(motionType - 1) >= 3) {
            _errMessage(d, str_mpeg_invalid_motion_type, motionType);
            d->unk11C = 1;
            return 0;
        }
        _getAllRefs((struct MpegDecoder *)d, bx, by, mbType, motionType,
                      pmv, mvfs, dmv);
        while ((*(volatile unsigned int *)0x1000D400 >> 8) & 1)
            ;
        dma_ref_image(d);
        d->e[d->unk810].unk138 = 1;
    }
    if (arg2 == 1 && (mbType & 2)) {
        d->e[d->unk810].unk134 = arg2;
    } else {
        d->e[d->unk810].unk134 = 0;
    }
    d->e[d->unk810].unk130 = intra;
    if (d->unk174 == 3) {
        d->e[d->unk810].unk128 = d->unk1C0->unk0 + (x * d->unk1C0->unk10 + y) * 0x180;
    } else {
        if (d->unk174 == 2) {
            ref = d->unk1E0;
        } else {
            ref = d->unk1D0;
        }
        d->e[d->unk810].unk128 = ref->unk0 + (x * ref->unk10 + y) * 0x180;
    }
    return 1;
}

struct MpegDecoder;
struct sceMpeg {
    s32 width;
    s32 height;
    s32 frameCount;
    u8 pad_C[0x4];
    s64 pts;
    s64 dts;
    u64 flags;
    s64 pts2nd;
    s64 dts2nd;
    u64 flags2nd;
    struct MpegDecoder *sys; /* private decoder state */
};
struct MpegRefImage {
    u8 *data; /* picture base; _getRef0 adds macroblock index * 0x180 */
    s32 width;
    s32 height;
    s32 unkC;
    s32 unk10; /* non-zero: sized check in _isOutSizeOK;
                                           _getRef0 macroblock index x * unk10 + y */
    u8 pad_14[0x14];
    s32 status; /* 1: output by _dispRefImage; ClearMpegReferenceBuffer zeroes it */
    u8 pad_2C[0x18];
    s32 unk44;
    s32 unk48;
    s32 unk4C;
    s32 unk50;
    s32 unk54;
    s32 unk58;
    s32 unk5C;
    s32 unk60;
};
typedef void (*MpegMcFunc)(void);
struct MpegMcFetch {
    u8 *addr;
    s32 x;
    s32 n0;
    s32 n1;
    s32 step;
    u8 *p0;
    u8 *p1;
};
struct MpegMbBuffer {
    u32 spr_base; /* SPR 0x70000000 or 0x70001B00; fetch n at + n * 0x600 */
    u32 ipu_out; /* spr_base + 0x1800; 0x300 bytes received from the IPU (_decMB0) */
    u8 *luma_ref[4]; /* reference picture luma source per fetch */
    u8 *chroma_ref[4]; /* reference picture chroma source per fetch */
    MpegMcFunc luma_func[4]; /* D_00132E30[] copy routine per fetch */
    MpegMcFunc chroma_func[4]; /* D_00132E50[] copy routine per fetch */
    struct MpegMcFetch luma[4];
    struct MpegMcFetch chroma[4];
    s32 unk128; /* read by _doMC */
    s32 fetch_count; /* queued fetches; loop count in _doMC */
    s32 unk130; /* read by _doMC */
    u8 pad_134[0x4];
    s32 unk138; /* _doMC skips the work when 0 */
    s32 unk13C; /* set 1 by _decMB0/_skipMB0, 0 by _slice0 */
};
struct MpegDecoder {
    s32 unk0;
    s32 unk4; /* non-zero: _outputFrame displays a frame */
    s32 unk8;
    u8 pad_C[0x40];
    u8 ipu_dma_state; /* &ipu_dma_state passed to suspend_mpeg_decoder_ipu_dma */
    u8 pad_4D[0x33];
    s32 unk80;
    u8 pad_84[0x4];
    u64 unk88;
    u8 pad_90[0x4];
    s32 unk94;
    s32 unk98;
    s32 unk9C;
    s32 unkA0;
    s32 unkA4;
    s32 unkA8;
    s32 frame_base; /* sceMpeg.frameCount = frame_count - frame_base (_sceMpegFlush) */
    s32 unkB0; /* non-zero: _dispRefImage uses _csc_storeRefImage, else _cpr8 */
    s32 unkB4;
    s32 unkB8;
    s32 unkBC;
    s32 unkC0;
    s32 unkC4;
    s32 unkC8;
    s32 unkCC;
    s32 unkD0;
    s32 unkD4; /* 0 in _sequenceHeader, set by _pictureCodingExtension */
    s32 unkD8; /* output address (_cpr8 masks it to physical) */

    /* Output size limits, checked by _isOutSizeOK against the picture. */
    s32 out_max_width; /* >= picture width when out_max_height != 0 */
    s32 out_max_height; /* >= picture height; 0: check out_max_size */
    s32 out_max_size; /* >= unkC * unk10 of the picture */

    u8 pad_E8[0x10];
    s32 unkF8; /* 1 -> 2 in _outputFrame */
    s32 frame_buffers[3]; /* reserved in _sequenceHeader, passed to _initRefImages */
    u8 pad_108[0x10];
    s32 frame_count; /* +1 per decoded frame (_decodeOrSkipFrame) */
    s32 unk11C;
    s32 second_field; /* toggled per field picture (_decodeOrSkipFrame); set at
                                           _lastFrame: "the second field is missing" */

    /* sequence_header (_sequenceHeader) */
    u32 horizontal_size; /* 12 bits */
    s32 vertical_size; /* 12 bits; > 2800 is an error */
    s32 mb_width; /* (horizontal_size + 15) >> 4 */
    s32 mb_height;
    u32 bit_rate; /* 18 bits */
    s32 vbv_buffer_size; /* 10 bits */
    s32 unk13C; /* 0: mb_height rounded to 32-line pairs (_sequenceHeader) */
    s32 unk140;

    /* sequence_display_extension */
    s32 matrix_coefficients; /* 3rd 8-bit colour_description field */
    s32 display_horizontal_size; /* 14 bits */
    s32 display_vertical_size; /* 14 bits */

    s32 picture_coding_type; /* 1 I, 2 P (_skipMB0 zeroes motion), 3 B (_outputFrame) */
    s32 unk154;
    s32 unk158;
    s32 unk15C;
    s32 unk160;

    /* picture_coding_extension */
    s32 f_code_forward_h;
    s32 f_code_forward_v;
    s32 f_code_backward_h;
    s32 f_code_backward_v;
    s32 picture_structure; /* 3: frame picture */
    s32 top_field_first;
    s32 frame_pred_frame_dct;
    s32 concealment_motion_vectors;
    s32 repeat_first_field;
    s32 progressive_frame;

    u8 pad_18C[0x20];
    s32 unk1AC;
    s32 dc_reset; /* DCR bit of the IPU BDEC command (_decMB0); set by _skipMB0 */
    s32 quantiser_scale_code; /* 5 bits from the slice header (_sliceB); QSC of BDEC */
    struct MpegRefImage *ref_images[12]; /* chosen by picture_structure / picture_coding_type in _outputFrame */
    u8 pad_1E8[0x3A8];
    struct MpegMbBuffer mb_buf[2];
    s32 mb_buf_index; /* toggled per slice in _slice0 */
    u8 pad_814[0x8];
    u8 *mc_work; /* _getRef0: destination of the queued fetches */
    s32 unk820;
    u8 pad_824[0x1C];

    /* quant_matrix_extension / sequence_header */
    s32 load_intra_quantiser_matrix; /* set -> IPU SETIQ intra (0x50000000) */
    s32 load_non_intra_quantiser_matrix;/* set -> IPU SETIQ non-intra (0x58000000) */
    s32 unk848; /* 0: _sequenceHeader loads frame-picture defaults */
    s32 unk84C;
    s32 unk850;
    s32 unk854;
    struct sceMpeg *mpeg; /* owning sceMpeg (_dispRefImage writes its pts/dts/flags) */
};
extern char str_mpeg_invalid_motion_type_a[];
extern char str_mpeg_invalid_motion_type_b[];
extern char str_mpeg_invalid_motion_type_c[];
extern void _errMessage_r(struct MpegDecoder *d, char *fmt, int code) __asm__("func_0012C430");
extern void dualPrimeVector_r(struct MpegDecoder *d, int dmv[2][2], int *dmvector, int mvx, int mvy) __asm__("func_001275C0");
extern void _getRef0(struct MpegDecoder *d, struct MpegRefImage *ref, int sfield, int dfield, int yofs, int h, int bx, int by, int dx, int dy, int fieldpred, int avg);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/library/_getAllRefs.c, _getAllRefs. */
void _getAllRefs(struct MpegDecoder *d, int bx, int by, int mb_type, int motion_type,
                 int pmv[2][2][2], int mvfs[2][2], int *dmvector) {
    int dmv[2][2];
    struct MpegRefImage *refs[2][2];
    int one = 1;
    int avg;
    int currentfield;
    int same = one;

    avg = 0;

    d->mb_buf[d->mb_buf_index].fetch_count = 0;

    if ((mb_type & 8) || d->picture_coding_type == 2) {
        if (d->picture_structure == 3) {
            if (motion_type == 2 || !(mb_type & 8)) {
                _getRef0(d, d->ref_images[0], 0, 0, 0, 16, bx, by, pmv[0][0][0], pmv[0][0][1], 0,
                         0);
            } else if (motion_type == same) {
                _getRef0(d, d->ref_images[0], mvfs[0][0], 0, 0, 8, bx, by, pmv[0][0][0],
                         pmv[0][0][1] >> 1, same, 0);
                _getRef0(d, d->ref_images[0], mvfs[1][0], 1, 0, 8, bx, by, pmv[1][0][0],
                         pmv[1][0][1] >> 1, same, 0);
            } else if (motion_type == 3) {
                dualPrimeVector_r(d, dmv, dmvector, pmv[0][0][0], pmv[0][0][1] >> 1);
                _getRef0(d, d->ref_images[0], 0, 0, 0, 8, bx, by, pmv[0][0][0], pmv[0][0][1] >> 1,
                         same, 0);
                _getRef0(d, d->ref_images[0], 1, 0, 0, 8, bx, by, dmv[0][0], dmv[0][1], same, one);
                _getRef0(d, d->ref_images[0], 1, 1, 0, 8, bx, by, pmv[0][0][0], pmv[0][0][1] >> 1,
                         same, 0);
                _getRef0(d, d->ref_images[0], 0, 1, 0, 8, bx, by, dmv[1][0], dmv[1][1], same, one);
            } else {
                _errMessage_r(d, str_mpeg_invalid_motion_type_a, motion_type);
            }
        } else {
            currentfield = d->picture_structure == 2;
            /* Keep the retail stack order: two field choices per reference set. */
            refs[0][0] = d->ref_images[4];
            refs[0][1] = d->ref_images[8];
            refs[1][0] = d->ref_images[5];
            refs[1][1] = d->ref_images[9];
            one = 2;
            same = 0;
            if (d->picture_coding_type == one && d->second_field) {
                same = currentfield != mvfs[0][0];
            }
            if (motion_type == 1 || !(mb_type & 8)) {
                _getRef0(d, refs[same][mvfs[0][0]], 0, 0, 0, 16, bx, by, pmv[0][0][0], pmv[0][0][1],
                         0, 0);
            } else if (motion_type == 2) {
                _getRef0(d, refs[same][mvfs[0][0]], 0, 0, 0, 8, bx, by, pmv[0][0][0], pmv[0][0][1],
                         0, 0);
                same = d->picture_coding_type == motion_type && d->second_field &&
                       currentfield != mvfs[1][0];
                _getRef0(d, refs[same][mvfs[1][0]], 0, 0, 8, 8, bx, by, pmv[1][0][0], pmv[1][0][1],
                         0, 0);
            } else if (motion_type == 3) {
                same = d->second_field ? motion_type - 2 : 0;
                dualPrimeVector_r(d, dmv, dmvector, pmv[0][0][0], pmv[0][0][1]);
                _getRef0(d, refs[0][currentfield], 0, 0, 0, 16, bx, by, pmv[0][0][0], pmv[0][0][1],
                         0, 0);
                _getRef0(d, refs[same][!currentfield], 0, 0, 0, 16, bx, by, dmv[0][0], dmv[0][1], 0,
                         1);
            } else {
                _errMessage_r(d, str_mpeg_invalid_motion_type_b, motion_type);
            }
        }
        avg = 1;
    }

    if (mb_type & 4) {
        if (d->picture_structure == 3) {
            same = 1;
            if (motion_type == 2) {
                _getRef0(d, d->ref_images[1], 0, 0, 0, 16, bx, by, pmv[0][1][0], pmv[0][1][1], 0,
                         avg);
            } else {
                _getRef0(d, d->ref_images[1], mvfs[0][1], 0, 0, 8, bx, by, pmv[0][1][0],
                         pmv[0][1][1] >> 1, same, avg);
                _getRef0(d, d->ref_images[1], mvfs[1][1], 1, 0, 8, bx, by, pmv[1][1][0],
                         pmv[1][1][1] >> 1, same, avg);
            }
        } else if (motion_type == 1) {
            _getRef0(d, mvfs[0][1] ? d->ref_images[9] : d->ref_images[5], 0, 0, 0, 16, bx, by,
                     pmv[0][1][0], pmv[0][1][1], 0, avg);
        } else if (motion_type == 2) {
            _getRef0(d, mvfs[0][1] ? d->ref_images[9] : d->ref_images[5], 0, 0, 0, 8, bx, by,
                     pmv[0][1][0], pmv[0][1][1], 0, avg);
            _getRef0(d, mvfs[1][1] ? d->ref_images[9] : d->ref_images[5], 0, 0, 8, 8, bx, by,
                     pmv[1][1][0], pmv[1][1][1], 0, avg);
        } else {
            _errMessage_r(d, str_mpeg_invalid_motion_type_c, motion_type);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00125F88);

extern void _Error(void *, void *);
extern void mpeg_clamp_pack_block(int, int);
extern void mpeg_add_block_clamp(int, int, int);
extern char str_mpeg_intra_skip_mb[];

typedef struct {
    int unk0;
    int unk4;
    char pad8[0x20];
    void (*fa[4])(void *);
    void (*fb[4])(void *);
    char a[4][0x1C];
    char b[4][0x1C];
    int unk128;
    int count;
    int unk130;
    int pad134;
    int unk138;
    int unk13C;
} Elem001263A8;

typedef struct {
    char pad0[0x590];
    Elem001263A8 e[2];
    char pad810[0xC];
    int unk81C;
} Dec001263A8;

void func_001263A8_r(void *arg0, int idx) __asm__("func_001263A8");

/* Run a channel's per-block callbacks, then hand its buffer to the output routine. */
void func_001263A8_r(void *arg0, int idx) {
    Dec001263A8 *p = (Dec001263A8 *)arg0;
    int i;

    if (p->e[idx].unk138 != 0) {
        for (i = 0; i < p->e[idx].count; i++) {
            p->e[idx].fa[i](p->e[idx].a[i]);
            p->e[idx].fb[i](p->e[idx].b[i]);
        }
    }
    if (p->e[idx].unk130 != 0 && p->e[idx].unk13C != 0) {
        _Error(p, str_mpeg_intra_skip_mb);
    }
    if (p->e[idx].unk130 != 0) {
        mpeg_clamp_pack_block(p->e[idx].unk128, p->e[idx].unk4);
    } else if (p->e[idx].unk13C != 0) {
        mpeg_clamp_pack_block(p->e[idx].unk128, p->unk81C);
    } else {
        mpeg_add_block_clamp(p->e[idx].unk128, p->unk81C, p->e[idx].unk4);
    }
}

ASM_FUNC("asm/handwritten/core_text", func_001265C8);

ASM_FUNC("asm/handwritten/core_text", func_00126640);

ASM_FUNC("asm/handwritten/core_text", func_001266D8);

ASM_FUNC("asm/handwritten/core_text", func_00126790);

ASM_FUNC("asm/handwritten/core_text", func_00126860);

ASM_FUNC("asm/handwritten/core_text", func_00126910);

ASM_FUNC("asm/handwritten/core_text", func_001269C8);

ASM_FUNC("asm/handwritten/core_text", func_00126AC0);

ASM_FUNC("asm/handwritten/core_text", func_00126BC0);

ASM_FUNC("asm/handwritten/core_text", func_00126C60);

ASM_FUNC("asm/handwritten/core_text", func_00126D10);

ASM_FUNC("asm/handwritten/core_text", func_00126DF0);

ASM_FUNC("asm/handwritten/core_text", func_00126ED8);

ASM_FUNC("asm/handwritten/core_text", func_00126FB0);

ASM_FUNC("asm/handwritten/core_text", func_00127080);

ASM_FUNC("asm/handwritten/core_text", func_001271A0);

ASM_FUNC("asm/handwritten/core_text", func_001272B8);

INCLUDE_ASM("asm/nonmatchings/core_text", func_00127318);

void ipu_set_mpeg1(int arg0) {
    unsigned int *p = (unsigned int *)0x10002010;
    *p = (*p & 0xFF7FFFFF) | (arg0 << 23);
}

typedef struct {
    unsigned char pad0[0x838];
    int unk838;
    int unk83C;
    unsigned char pad840[0x18];
    void *cbData;
} MpegDec;

extern char str_mpeg_bdec_error[];
extern void wait_ipu_idle(void *);
extern void dispatch_mpeg_callback(int, void *);
extern void _Error(void *, void *);

#define IPU_CTRL   ((volatile unsigned int *)0x10002010)
#define IPU_BP     ((volatile unsigned int *)0x10002020)
#define IPU_TOP    ((unsigned long *)0x10002030)
#define D3_CHCR    ((volatile unsigned int *)0x1000B400)
#define D3_QWC     ((volatile unsigned int *)0x1000B420)
#define D4_CHCR    ((volatile unsigned int *)0x1000B000)
#define D4_QWC     ((volatile unsigned int *)0x1000B020)
#define D_ENABLER  ((volatile unsigned int *)0x1000F520)
#define D_ENABLEW  ((volatile unsigned int *)0x1000F590)

/* _waitBdecOut (libmpeg). Adapted from Lombyte's project (GPL-3.0) for PAL. */
int _waitBdecOut(void *arg0) {
    MpegDec *d = arg0;
    int cb1[8];
    int cb2[8];
    int ret;
    int intr;
    unsigned int bp;
    long top;
    unsigned int rest;

    ret = 1;
    wait_ipu_idle(d);
    if (*D4_QWC != 0 && !(*IPU_CTRL & 0x4000)) {
        do {
            if (*D3_QWC == 0 && !(*D3_CHCR & 0x100)) {
                cb1[0] = 1;
                dispatch_mpeg_callback((int)d->cbData, cb1);
            }
        } while (*D4_QWC != 0 && !(*IPU_CTRL & 0x4000));
    }
    bp = *IPU_BP;
    top = *IPU_TOP;
    d->unk838 = top;
    if (top < 0) {
        rest = bp & 0x1F;
        d->unk83C = rest ? 32 - rest : 0;
    } else {
        d->unk83C = 32;
    }
    if (*IPU_CTRL & 0x4000) {
        _Error(d, str_mpeg_bdec_error);
        cb2[0] = 2;
        dispatch_mpeg_callback((int)d->cbData, cb2);
        *(unsigned int *)0x10002010 = 0x40000000;
        cb2[0] = 3;
        dispatch_mpeg_callback((int)d->cbData, cb2);
        intr = disable_interrupts();
        *D_ENABLEW = *D_ENABLER | 0x10000;
        *D4_CHCR = 0;
        *D_ENABLEW = *D_ENABLER & ~0x10000;
        if (intr) {
            enable_interrupts();
        }
        ret = 0;
        *D4_QWC = 0;
    }
    return ret;
}

/* func_001286E8 returns a short: its epilogue sign-extends $v0. */
extern short ipu_vdec(int, int);

/* Returns its callee's value: retail keeps the frame and calls, and
   2.9-ee turns a void one-call function into a bare tail jump (8 bytes
   against 28). */
short mpeg_get_dmvector(int arg0) {
    return ipu_vdec(arg0, 3);
}

/* dualPrimeVector (libmpeg.a:mpc.o): ISO/IEC 13818-2 7.6.3.6
 * Dual_Prime_Arithmetic(), matching mpeg2decode reference source
 * (motion.c) line for line. picture_structure (+0x174) and
 * top_field_first (+0x178) come from the bitstream-state struct instead
 * of globals; FRAME_PICTURE == 3, TOP_FIELD == 1 (ISO encoding).
 */
void dualPrimeVector(void *arg0, int DMV[][2], int *dmvector, int mvx, int mvy) {
    char *s = (char *)arg0;

    if (*(int *)(s + 0x174) == 3) {
        int top_field_first = *(int *)(s + 0x178);
        if (top_field_first) {
            DMV[0][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
            DMV[0][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1] - 1;
            DMV[1][0] = ((3 * mvx + (mvx > 0)) >> 1) + dmvector[0];
            DMV[1][1] = ((3 * mvy + (mvy > 0)) >> 1) + dmvector[1] + 1;
        } else {
            DMV[0][0] = ((3 * mvx + (mvx > 0)) >> 1) + dmvector[0];
            DMV[0][1] = ((3 * mvy + (mvy > 0)) >> 1) + dmvector[1] - 1;
            DMV[1][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
            DMV[1][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1] + 1;
        }
    } else {
        DMV[0][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
        DMV[0][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1];
        if (*(int *)(s + 0x174) == 1) {
            DMV[0][1]--;
        } else {
            DMV[0][1]++;
        }
    }
}


extern char str_mpeg_invalid_mb_addr_inc[];
extern int _peepBit(void *, int);
extern void _flushBuf(void *, int);
extern void _errMessage(void *, char *, int);

int mb_address_increment(void *arg0) {
    char *s = (char *)arg0;
    unsigned int code;
    int val = 0;
    int cont;
    int ret;

    do {
        code = ipu_vdec((int)s, 0);
        switch (code) {
        case 35:
            cont = 1;
            val += 33;
            break;
        case 0:
            ret = _peepBit(s, 11);
            if (*(int *)(s + 0x848) != 0 && ret == 15) {
                _flushBuf(s, 11);
                cont = 1;
            } else {
                _errMessage(s, str_mpeg_invalid_mb_addr_inc, code);
                *(int *)(s + 0x11C) = 1;
                return 1;
            }
            break;
        case 34:
            cont = 1;
            break;
        default:
            val += code;
            cont = 0;
            break;
        }
    } while (cont);

    return val;
}

extern char str_mpeg_skip_to_next_picture[];
extern int _slice0(void *, int);
extern int _waitBdecOut(void *);
extern int _doMC(void *, int);
extern void _Error(void *, void *);

int _pictureData0(void *arg0) {
    char *s = (char *)arg0;
    int count;
    int ret;

    *(int *)(s + 0x810) = 0;
    *(int *)(s + 0x814) = 0;

    count = *(int *)(s + 0x12C) * *(int *)(s + 0x130);
    if (*(int *)(s + 0x174) != 3) {
        count >>= 1;
    }

    do {
        ret = _slice0(s, count);
    } while (ret == 1 || ret == 3);

    wait_ipu_idle(s);
    if (_waitBdecOut(s) == 0) {
        ret = 2;
    }

    while (((*(volatile unsigned int *)0x1000D400 >> 8) & 1) != 0)
        ;

    if (ret == 0) {
        _doMC(s, (unsigned int)*(int *)(s + 0x810) < 1);
    }

    if ((unsigned int)(ret - 1) < 2) {
        _Error(s, str_mpeg_skip_to_next_picture);
    }

    return (unsigned int)ret < 1;
}

extern void next_start_code(void *);
extern int _sliceB(void *);
extern void _Error(void *, void *);
extern char str_mpeg_slice_start_code_range[];
extern char str_mpeg_slice_a0_error[];

/* sliceA0 (libmpeg.a:mpc.o): ISO/IEC 13818-2 6.2.4 slice_header(), the
 * hardware-VLC variant. nextStartCode, then peek 32 bits: if it isn't a
 * slice_start_code (0x101..0x1AF), report it (func_0012C430) and return 2.
 * Otherwise consume the 32 bits, run sliceB (IPU setup) and
 * mbAddressIncrement, stash the raw increment at *arg3 (in case of a
 * fault), and if the fault flag (+0x11C, cleared at entry) got set, report
 * it (func_0012C468) and return 1. On full success, compute the starting
 * macroblock address from the slice's row (low byte of the start code)
 * and the picture's mb width (+0x12C), overwrite *arg3 with 1, latch
 * +0x1B0, and zero eight fields of the *arg4 output struct.
 */
int _sliceA0(void *arg0, int arg1, int *arg2, int *arg3, void *arg4) {
    char *s = (char *)arg0;
    char *p5 = (char *)arg4;
    unsigned int code;
    int sliceB_result;
    int mbInc;
    int row;

    *(int *)(s + 0x11C) = 0;
    next_start_code(s);
    code = _peepBit(s, 0x20);

    if ((unsigned int)(code - 0x101) >= 0xAF) {
        _errMessage(s, str_mpeg_slice_start_code_range, code);
        return 2;
    }

    _flushBuf(s, 0x20);
    sliceB_result = _sliceB(s);
    mbInc = mb_address_increment(s);
    *arg3 = mbInc;

    if (*(int *)(s + 0x11C) != 0) {
        _Error(s, str_mpeg_slice_a0_error);
        return 1;
    }

    row = (sliceB_result << 7) + (int)(code & 0xFF) - 1;
    *arg2 = row * (*(int *)(s + 0x12C)) + mbInc - 1;
    *arg3 = 1;
    *(int *)(s + 0x1B0) = 1;
    *(int *)(p5 + 0x14) = 0;
    *(int *)(p5 + 0x10) = 0;
    *(int *)(p5 + 0x4) = 0;
    *(int *)(p5 + 0x0) = 0;
    *(int *)(p5 + 0x1C) = 0;
    *(int *)(p5 + 0x18) = 0;
    *(int *)(p5 + 0xC) = 0;
    *(int *)(p5 + 0x8) = 0;
    return 0;
}

extern int _sliceA0(void *, int, int *, int *, void *);
extern int mb_address_increment(void *);
extern void _Error(void *, void *);
extern char str_mpeg_too_many_macroblocks[];
extern int _decMB0(void *, int *, int *, int *, void *, void *, void *);
extern int _motionComp0(void *, int, int, int, int, void *, void *, void *);

/* slice0 (libmpeg.a:mpc.o): the per-picture macroblock loop -- sliceA0
 * (func_00127960) parses the first slice header and starting mb address
 * (local "mbAddr"/"mbInc" pair, plus the zeroed 8-word block-info struct
 * at local0); then while mbAddr < mbCount: waitBdecOut (func_001273A0)
 * must succeed; if this macroblock doesn't already have a pending
 * mbAddressIncrement (mbInc==0, i.e. a fresh slice), peek 23 bits looking
 * for the next slice_start_code prefix, and if found read the next
 * mbAddressIncrement (func_00127748) -- both faulting through +0x11C;
 * decode (decMB0, func_00127D40) when mbInc==1, else skip (skipMB0,
 * func_00127C80) the run of skipped macroblocks; run motion compensation
 * (motionComp0, func_00125630) on the result; for every macroblock after
 * the first, run the previous macroblock's IPU motion-comp dispatch
 * (doMC, func_001263A8) using the alternating +0x810 buffer index before
 * flipping it; advance mbAddr and consume one mbInc, looping until the
 * picture's macroblocks are exhausted (return 0) or a fault (1/2/3)
 * happens along the way.
 */
int _slice0(void *arg0, int mbCount) {
    char *s = (char *)arg0;
    int mbAddr = 0;
    int mbInc = 0;
    int local48, local4C, local50;
    int local0[8];
    int scratch20[4];
    int scratch30[4];
    int r;

    r = _sliceA0(s, mbCount, &mbAddr, &mbInc, local0);
    if (r != 0) {
        return r;
    }
    *(int *)(s + 0x11C) = 0;

    for (;;) {
        int mbIdx;
        char *entry;

        if (!(mbAddr < mbCount)) {
            return 0;
        }

        mbIdx = *(int *)(s + 0x810);
        entry = s + mbIdx * 0x140;
        *(int *)(entry + 0x6CC) = 0;
        r = _waitBdecOut(s);
        if (r == 0) {
            return 2;
        }

        if (mbInc == 0) {
            int peeked = _peepBit(s, 0x17);
            if (peeked == 0) {
                *(int *)(s + 0x11C) = 0;
                return 3;
            }
            if (*(int *)(s + 0x11C) != 0) {
                *(int *)(s + 0x11C) = 0;
                return 3;
            }

            mbInc = mb_address_increment(s);
            if (*(int *)(s + 0x11C) != 0) {
                *(int *)(s + 0x11C) = 0;
                return 1;
            }
        }

        if (!(mbAddr < mbCount)) {
            _Error(s, str_mpeg_too_many_macroblocks);
            return 2;
        }

        if (mbInc == 1) {
            r = _decMB0(s, &local48, &local4C, &local50, local0,
                               scratch20, scratch30);
            if (r == 0) {
                *(int *)(s + 0x11C) = 0;
                return 1;
            }
        } else {
            r = _skipMB0(s, local0, &local4C, scratch20, &local48);
            if (r == 0) {
                *(int *)(s + 0x11C) = 0;
                return 2;
            }
        }

        r = _motionComp0(s, mbAddr, mbInc, local48, local4C, local0,
                           scratch20, scratch30);
        if (r == 0) {
            *(int *)(s + 0x11C) = 0;
            return 2;
        }

        if (mbAddr != 0) {
            int fault = *(int *)(s + 0x810);
            _doMC(s, fault ^ 1);
        }

        mbAddr = mbAddr + 1;
        *(int *)(s + 0x810) ^= 1;
        mbInc = mbInc - 1;
    }
}

extern void _Error(void *, void *);
extern char str_mpeg_skipped_mb_in_i_picture[];

/* arg0 is a moby/entity-ish struct: a table index at +0x810 selects a
 * +0x140-strided sub-entry whose +0x6CC field gets set, and several
 * other struct-pointer outputs get filled from +0x150/+0x174 flags.
 * Ends by clearing bit 0 of *a4, reporting through func_0012C468 (an
 * already-matched "log to D_00153968" family member) first when
 * +0x150 == 1. The func_0012C468 call takes arg0 -- the moby struct,
 * still live in $4 -- not a4/s0, which is a common trap here. */
int _skipMB0(void *a0, int *a1, int *a2, int *a3, int *a4) {
    int idx;
    char *entry;
    int ret = 1;
    int c;
    int mask;

    idx = *(int *)((char *)a0 + 0x810);
    entry = (char *)a0 + idx * 0x140;
    *(int *)(entry + 0x6CC) = 1;
    *(int *)((char *)a0 + 0x1B0) = 1;

    if (*(int *)((char *)a0 + 0x150) == 2) {
        *(int *)((char *)a1 + 0x14) = 0;
        *(int *)((char *)a1 + 0x10) = 0;
        *(int *)((char *)a1 + 0x4) = 0;
        *(int *)((char *)a1 + 0x0) = 0;
    }

    if (*(int *)((char *)a0 + 0x174) == 3) {
        *a2 = 2;
    } else {
        *a2 = 1;
        c = (*(int *)((char *)a0 + 0x174) == 2);
        a3[1] = c;
        *a3 = c;
    }

    if (*(int *)((char *)a0 + 0x150) == 1) {
        _Error(a0, str_mpeg_skipped_mb_in_i_picture);
        ret = 0;
    }
    mask = *a4;
    mask &= ~1;
    *a4 = mask;
    return ret;
}

extern char str_mpeg_invalid_mb_type[];
extern int _nextBit(void *, int);
extern void motion_vector(void *, int *, int *, int, int, int, int, int);
extern void motion_vectors_x(void *, int *, int *, int *, int, int, int, int, int, int, int) __asm__("func_00128270");
extern void receive_data_from_ipu(void *, int);
extern void send_ipu_command(char *, unsigned int);

typedef struct {
    char pad000[0x11C];
    int unk11C;
    char pad120[0x30];
    int unk150;
    int unk154;
    int unk158;
    int unk15C;
    int unk160;
    int unk164;
    int unk168;
    int unk16C;
    int unk170;
    int unk174;
    char pad178[4];
    int unk17C;
    int unk180;
    char pad184[0x2C];
    int unk1B0;
    int unk1B4;
    char pad1B8[0x810 - 0x1B8];
    int unk810;
    char pad814[0x848 - 0x814];
    int unk848;
} MbDec;

/* Decodes one macroblock's modes and motion vectors through the IPU VLC engine (libmpeg _decMB0). Adapted from Lombyte's project (GPL-3.0) for PAL: sdk/library/_decMB0.c, _decMB0. */
int _decMB0(void *arg0, int *arg1, int *arg2, int *arg3, void *arg4, void *arg5, void *arg6) {
    MbDec *d = arg0;
    int *mb = arg4;
    int *sp20;
    int *sp24;
    int v;
    int var19;
    int var20;
    int var23;
    int motion_flag;
    int t7;
    int t7b;
    int t4;
    int var103;
    int new_var;
    unsigned int g1;
    unsigned int g2;

    *IPU_CTRL = (*IPU_CTRL & 0xF8FFFFFF) | (d->unk150 << 24);
    sp20 = arg3;
    sp24 = arg5;
    v = ipu_vdec((int)d, 1);
    *arg1 = v;
    if (v == 0) {
        _Error(d, str_mpeg_invalid_mb_type);
        d->unk11C = 1;
        return 0;
    }
    if (v & 0xC) {
        if (d->unk174 == 3 && d->unk17C != 0) {
            *arg2 = 2;
        } else {
            *arg2 = _nextBit(d, 2);
        }
    } else if ((v & 1) && d->unk180 != 0) {
        *arg2 = ((d->unk174 ^ 3) == 0) ? 2 : 1;
    }
    if (d->unk174 == 3) {
        var19 = ((*arg2 ^ 1) == 0) ? 2 : 1;
        var20 = *arg2 == 2;
    } else {
        new_var = 2;
        var20 = 0;
        var19 = ((*arg2 ^ new_var) == 0) ? 2 : 1;
    }
    var23 = 0;
    motion_flag = *arg2 == 3;
    if (var20 == 0) {
        var23 = d->unk174 == 3;
    }
    var103 = 0;
    if (d->unk174 == 3 && d->unk17C == 0 && (*arg1 & 3) != 0) {
        var103 = _nextBit(d, 1);
    } else {
        var103 = 0;
    }
    *sp20 = var103;
    if (*arg1 & 0x10) {
        d->unk1B4 = _nextBit(d, 5);
    }
    if ((*arg1 & 8) || ((*arg1 & 1) && d->unk180 != 0)) {
        if (d->unk848 != 0) {
            motion_vectors_x(d, mb, arg6, sp24, 0, var19, var20, d->unk164 - 1, d->unk168 - 1, motion_flag, var23);
        } else {
            t7 = d->unk158 - 1;
            motion_vector(d, mb, arg6, t7, t7, 0, 0, d->unk154);
        }
    }
    if (d->unk11C != 0) {
        return 0;
    }
    if (*arg1 & 4) {
        if (d->unk848 != 0) {
            motion_vectors_x(d, mb, arg6, sp24, 1, var19, var20, d->unk16C - 1, d->unk170 - 1, 0, var23);
        } else {
            t7b = d->unk160 - 1;
            motion_vector(d, mb + 2, arg6, t7b, t7b, 0, 0, d->unk15C);
        }
    }
    if (d->unk11C != 0) {
        return 0;
    }
    if ((*arg1 & 1) && d->unk180 != 0) {
        _flushBuf(d, 1);
    }
    if ((*arg1 & 3) != 0) {
        receive_data_from_ipu(*(void **)((char *)d + d->unk810 * 0x140 + 0x594), 0x300);
        wait_ipu_idle(d);
        g1 = ((*arg1 & 1) << 27) | (d->unk1B4 << 16);
        g2 = (d->unk1B0 << 26) | 0x20000000;
        send_ipu_command((char *)d, (g1 | g2) | (*sp20 << 25));
    } else {
        *(int *)((char *)d + d->unk810 * 0x140 + 0x6CC) = 1;
    }
    d->unk1B0 = 0;
    if (d->unk11C != 0) {
        return 0;
    }
    if (((*arg1 & 1) || (d->unk1B0 = 1, (*arg1 & 1) != 0)) && d->unk180 == 0) {
        mb[5] = 0;
        mb[4] = 0;
        mb[1] = 0;
        mb[0] = 0;
        mb[7] = 0;
        mb[6] = 0;
        mb[3] = 0;
        mb[2] = 0;
    }
    t4 = d->unk150;
    if (t4 == 2) {
        if (!(*arg1 & 9)) {
            mb[0] = (mb[1] = (mb[4] = (mb[5] = 0)));
            if (d->unk174 == 3) {
                *arg2 = t4;
            } else {
                *arg2 = 1;
                *sp24 = d->unk174 == 2;
            }
        }
    }
    return 1;
}

/* decode_motion_vector (libmpeg.a:mpc.o): ISO/IEC 13818-2 motion_vector()
 * component reconstruction, matching mpeg2decode reference source
 * (motion.c) decode_motion_vector() line for line. full_pel_vector is
 * MPEG-1 (ISO/IEC 11172-1) support; called from func_00128410
 * (motionVector, already matched in this file).
 */
void decode_motion_vector(int *pred, int r_size, int motion_code, int motion_residual, int full_pel_vector) {
    int lim, vec;

    lim = 16 << r_size;
    vec = full_pel_vector ? (*pred >> 1) : (*pred);

    if (motion_code > 0) {
        vec += ((motion_code - 1) << r_size) + motion_residual + 1;
        if (vec >= lim)
            vec -= lim + lim;
    } else if (motion_code < 0) {
        vec -= ((-motion_code - 1) << r_size) + motion_residual + 1;
        if (vec < -lim)
            vec += lim + lim;
    }
    *pred = full_pel_vector ? (vec << 1) : vec;
}


/* motionVectors (libmpeg.a:mpc.o): ISO/IEC 13818-2 6.2.5.2/6.3.17.2/7.6.3
 * motion_vectors(), matching mpeg2decode reference source (motion.c)
 * almost line for line, with the bitstream-state pointer threaded through
 * explicitly as the new first argument (this file's convention) instead
 * of read from a global. func_00128410 is motionVector (already matched
 * in this file); func_00128A58 is Get_Bits.
 */
void motion_vectors(void *arg0, int PMV[][2][2], int *dmvector, int mvfs[][2],
                    int s_idx, int motion_vector_count, int mv_format,
                    int h_r_size, int v_r_size, int dmv, int mvscale) {
    if (motion_vector_count == 1) {
        if (mv_format == 0 && dmv == 0) {
            mvfs[1][s_idx] = mvfs[0][s_idx] = _nextBit(arg0, 1);
        }
        motion_vector(arg0, PMV[0][s_idx], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
        PMV[1][s_idx][0] = PMV[0][s_idx][0];
        PMV[1][s_idx][1] = PMV[0][s_idx][1];
    } else {
        mvfs[0][s_idx] = _nextBit(arg0, 1);
        motion_vector(arg0, PMV[0][s_idx], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
        mvfs[1][s_idx] = _nextBit(arg0, 1);
        motion_vector(arg0, PMV[1][s_idx], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
    }
}

extern void decode_motion_vector(int *, int, int, int, int);

/* motionVector (libmpeg.a:mpc.o): ISO/IEC 13818-2 7.6.3.1 motion_vector(),
 * hardware-VLC variant of mpeg2decode's motion.c. func_001286E8(s, 2) is
 * Get_motion_code (IPU VLC table 2); func_001275A0 is Get_dmvector (table
 * 3, already matched in this file). func_001281E8 is decode_motion_vector.
 */
void motion_vector(void *arg0, int *PMV, int *dmvector, int h_r_size,
                    int v_r_size, int dmv, int mvscale, int full_pel_vector) {
    char *s = (char *)arg0;
    int motion_code;
    int motion_residual;

    motion_code = ipu_vdec((int)s, 2);
    motion_residual = (h_r_size != 0 && motion_code != 0) ? _nextBit(s, h_r_size) : 0;
    decode_motion_vector(PMV, h_r_size, motion_code, motion_residual, full_pel_vector);
    if (dmv) {
        dmvector[0] = mpeg_get_dmvector((int)s);
    }

    motion_code = ipu_vdec((int)s, 2);
    motion_residual = (v_r_size != 0 && motion_code != 0) ? _nextBit(s, v_r_size) : 0;
    if (mvscale) {
        PMV[1] >>= 1;
    }
    decode_motion_vector(PMV + 1, v_r_size, motion_code, motion_residual, full_pel_vector);
    if (mvscale) {
        PMV[1] <<= 1;
    }
    if (dmv) {
        dmvector[1] = mpeg_get_dmvector((int)s);
    }
}

extern int IpuCommandTable[];

void send_ipu_command(char *arg0, unsigned int arg1) {
    volatile int *reg = (volatile int *)0x10002000;
    *reg = arg1;
    *(int *)(arg0 + 0x818) = IpuCommandTable[arg1 >> 28];
}

extern void dispatch_mpeg_cb_nodata(int);

/* Spins until the VIF1 DMA channel (D_CHCR 0x10002010) reports idle,
   nudging the IOP via func_0012BCC8 every 5001 spins so a stalled
   stream cannot deadlock. Same wait loop documented on func_00128860. */
void wait_ipu_idle(void *arg0) {
    int counter = 0;

    while ((*(volatile int *)0x10002010 & 0x80004000) == 0x80000000) {
        if (counter++ >= 0x1389) {
            dispatch_mpeg_cb_nodata(*(int *)((char *)arg0 + 0x858));
            counter = 0;
        }
    }
}

/*
 * IPU wait: the same spin as func_00128590, while IPU_CMD (0x10002000,
 * read as a 64-bit word) is negative (busy) and IPU_CTRL (0x10002010)
 * bit 0x4000 is clear, kicking func_0012BCC8 every 5001 spins. Returns
 * the raw 64-bit IPU_CMD word; func_00128860 and func_00128968 narrow it
 * with dsll32/dsra32, so the return type is `long`.
 *
 * Exact under both compilers once BOTH MMIO reads are spelled inline in
 * the condition: both addresses are then loop invariants that loop.c
 * hoists into $17/$16 as retail has them. The old note's
 * `volatile long *chcr` local made the compiler use the constant as an
 * absolute `ld` address instead (160 against 176 bytes).
 */
long wait_ipu_cmd_result(void *arg0) {
    long v;
    int counter = 0;
    while ((v = *(volatile long *)0x10002000) < 0 && (*(volatile int *)0x10002010 & 0x4000) == 0) {
        if (counter++ >= 0x1389) {
            dispatch_mpeg_cb_nodata(*(int *)((char *)arg0 + 0x858));
            counter = 0;
        }
    }
    return v;
}


typedef struct {
    unsigned char pad0[0x11C];
    int unk11C;
    unsigned char pad120[0x6F8];
    int unk818;
    unsigned char pad81C[0x1C];
    int unk838;
    int unk83C;
    unsigned char pad840[0x18];
    void *cbData;
} MpegVdec;

#define IPU_CMD  ((volatile unsigned long *)0x10002000)
#define IPU_CTRL ((volatile unsigned int *)0x10002010)
#define IPU_BP   ((volatile unsigned int *)0x10002020)
#define IPU_TOP  ((unsigned long *)0x10002030)

/* _ipuVdec (libmpeg). Adapted from Lombyte's project (GPL-3.0) for PAL. */
short ipu_vdec(int arg0, int tbl) {
    MpegVdec *d = (MpegVdec *)arg0;
    int i;
    int j;
    unsigned int cmd;
    long r;
    long top;
    unsigned int bp;

    i = 0;
    j = 0;
    while ((*IPU_CTRL & 0x80004000) == 0x80000000) {
        if (j++ > 5000) {
            dispatch_mpeg_cb_nodata((int)d->cbData);
            j = 0;
        }
    }
    cmd = (tbl << 26) | 0x30000000;
    *(volatile unsigned int *)IPU_CMD = cmd;
    d->unk818 = IpuCommandTable[(int)cmd >> 28];
    r = *IPU_CMD;
    while (r < 0) {
        if (i++ > 5000) {
            dispatch_mpeg_cb_nodata((int)d->cbData);
            i = 0;
        }
        r = *IPU_CMD;
    }
    bp = *IPU_BP;
    top = *IPU_TOP;
    d->unk838 = top;
    if (top < 0) {
        d->unk83C = -(bp & 0x1F) & 0x1F;
    } else {
        d->unk83C = 32;
    }
    d->unk11C = (int)r == 0;
    return (short)r;
}

/*
 * REVERTED -- decode is complete and believed correct; blocked on
 * register allocation, and the overage is a SIZE mismatch (0x114 vs
 * retail's 0x108) that overflows .core_text into .core_data, so it
 * cannot be left in as documented-close.
 *
 * A bitstream refill-and-extract: if the cache is empty or holds fewer
 * than n bits, wait for the DMA channel to go idle, refill from the
 * table, then return the top n bits.
 *
 *   extern long func_00128638_wide(void *, int *) __asm__("func_00128638");
 *
 *   int func_00128860(void *arg0, int n) {
 *       char *s = (char *)arg0;
 *       int bits;
 *       if (*(int *)(s + 0x818) != 0 || *(int *)(s + 0x83C) < n) {
 *           int *tbl = D_00132F70;
 *           int counter = 0;
 *           while ((*(volatile int *)0x10002010 & 0x80004000) == 0x80000000) {
 *               if (counter++ >= 0x1389) {
 *                   func_0012BCC8(*(int *)(s + 0x858));
 *                   counter = 0;
 *               }
 *           }
 *           *(volatile int *)0x10002000 = 0x40000000;
 *           *(int *)(s + 0x818) = tbl[4];
 *           *(int *)(s + 0x838) = (int)func_00128638_wide(s, tbl);
 *           *(int *)(s + 0x83C) = 0x20;
 *       }
 *       bits = *(int *)(s + 0x838);
 *       return (unsigned int)bits >> -n;
 *   }
 *
 * Confirmed right along the way: `counter++ >= 0x1389` is correct, not a
 * violation of the don't-put-++-in-a-comparison lever -- retail carries
 * the `daddu $2,$6,$0` spare move that lever warns about, so here the
 * move is EVIDENCE the ++ belongs inside the comparison. The dsll32/dsra32
 * after the jal is the 64-bit-return narrowing, handled by the _wide alias.
 *
 * Why it failed under 2.95.3: retail keeps only $16/$17/$18 across the
 * call (object, D_00132F70 base, n) in a 0x40 frame and REMATERIALISES
 * the hardware constants 0x10002010 / 0x80004000 / 0x80000000 with
 * lui/ori at each use, where 2.95.3 hoists all three into callee-saved
 * registers (a 0x70 frame, 0x114 against 0x108). 2.9-ee, this object's
 * compiler, rematerialises them like retail (see func_00128968), and the
 * C above then comes out 268 against 264: the frame and the wait loop
 * are right, but n is parked in $a3 and negated early where retail
 * keeps it in $s2 and negates it at the end. Still open.
 */
/* _peepBit: return the next count bits of the IPU bit stream without
   consuming them, refilling the 32-bit window from the IPU (FDEC) when
   it runs short and nudging the callback while the IPU stays busy.
   Adapted from Lombyte's project (GPL-3.0) for PAL. */
int _peepBit(void *arg, int count) {
    char *ctx = arg;
    int counter;

    if (*(int *)(ctx + 0x818) == 0 && *(int *)(ctx + 0x83C) >= count) {
        goto done;
    }
    counter = 0;
    if ((*(volatile unsigned int *)0x10002010 & 0x80004000) == 0x80000000) {
        do {
            if (counter++ >= 0x1389) {
                dispatch_mpeg_cb_nodata(*(void **)(ctx + 0x858));
                counter = 0;
            }
        } while ((*(volatile unsigned int *)0x10002010 & 0x80004000) == 0x80000000);
    }
    *(volatile unsigned int *)0x10002000 = 0x40000000;
    *(int *)(ctx + 0x818) = IpuCommandTable[4];
    *(int *)(ctx + 0x838) = wait_ipu_cmd_result(ctx);
    *(int *)(ctx + 0x83C) = 0x20;
done:
    return (unsigned int)*(int *)(ctx + 0x838) >> (0x20 - count);
}

/*
 * IPU command send: func_00128590's wait (IPU_CTRL & 0x80004000 ==
 * 0x80000000, a func_0012BCC8 kick every 5001 spins) inlined, then
 * cmd = arg1 | 0x40000000 to IPU_CMD, s+0x818 = D_00132F70[cmd >> 28]
 * (func_00128560 inlined), s+0x838 = the narrowed func_00128638 result,
 * s+0x83C = 0x20.
 *
 * Written plainly it is exact under 2.9-ee, which rematerialises the
 * MMIO constants inside the loop and keeps only %hi(D_00132F70) in $18,
 * like retail; 2.95.3 hoists them into saved registers (SIZE 252/240).
 * The stores go in source order 0x838 then 0x83C.
 */
void _flushBuf(void *arg0, int arg1) {
    int counter = 0;
    unsigned int cmd;

    while ((*(volatile int *)0x10002010 & 0x80004000) == 0x80000000) {
        if (counter++ >= 0x1389) {
            dispatch_mpeg_cb_nodata(*(int *)((char *)arg0 + 0x858));
            counter = 0;
        }
    }
    cmd = arg1 | 0x40000000;
    *(volatile int *)0x10002000 = cmd;
    *(int *)((char *)arg0 + 0x818) = IpuCommandTable[cmd >> 28];
    *(int *)((char *)arg0 + 0x838) = wait_ipu_cmd_result(arg0);
    *(int *)((char *)arg0 + 0x83C) = 0x20;
}

/* _nextBit (libmpeg.a:mpc.o), Get_Bits: waits for the IPU, refills the
 * bit cache when it is empty or short, returns the top n bits and tells
 * the IPU to consume them. The D_00132F70 index needs its own local,
 * computed before the command store, or the allocator swaps $v1 and $a0.
 */
int _nextBit(void *arg0, int n) {
    char *s = (char *)arg0;
    int counter = 0;
    unsigned int cmd;
    unsigned int idx;
    unsigned int oldBits;
    unsigned int result;

    while ((*(volatile int *)0x10002010 & 0x80004000) == 0x80000000) {
        if (counter++ >= 0x1389) {
            dispatch_mpeg_cb_nodata(*(int *)(s + 0x858));
            counter = 0;
        }
    }

    if (*(int *)(s + 0x818) != 0 || *(int *)(s + 0x83C) < n) {
        *(volatile int *)0x10002000 = 0x40000000;
        *(int *)(s + 0x818) = IpuCommandTable[4];
        *(int *)(s + 0x838) = wait_ipu_cmd_result(s);
    }

    *(int *)(s + 0x83C) = 0x20;
    oldBits = *(unsigned int *)(s + 0x838);
    cmd = n | 0x40000000;
    idx = cmd >> 28;
    *(volatile unsigned int *)0x10002000 = cmd;
    result = oldBits >> (0x20 - n);
    *(int *)(s + 0x818) = IpuCommandTable[idx];
    *(int *)(s + 0x838) = wait_ipu_cmd_result(s);
    return result;
}


void next_start_code(void *arg0) {
    int pad;

    wait_ipu_idle(arg0);
    pad = -(*(volatile int *)0x10002020 & 7) & 7;
    if (pad != 0) {
        _flushBuf(arg0, pad);
    }
    while (_peepBit(arg0, 0x18) != 1) {
        _flushBuf(arg0, 8);
    }
}

extern void extra_bit_information(void *);

int _sliceB(void *arg0) {
    *(int *)((char *)arg0 + 0x1B4) = _nextBit(arg0, 5);
    if (_nextBit(arg0, 1) != 0) {
        _nextBit(arg0, 1);
        _flushBuf(arg0, 7);
        extra_bit_information(arg0);
    }
    return 0;
}

extern void _sequenceHeader(void *);
extern void _groupOfPicturesHeader(void *);
extern void picture_header(void *);

typedef struct {
    int n;
    long long v[2];
} Buf18;

/* nextHeader (libmpeg.a:mpc.o): ISO/IEC 13818-2 6.2.2.1 Get_Hdr() loop --
 * nextStartCode, Get_Bits32, dispatch on the start code: sequence_header
 * (0x1B3, func_0012C4E0), group_start (0x1B8, func_00129240 groupOf...),
 * picture_start (0x100, func_00128DA0 pictureHeader then a PTS/DTS-style
 * query through func_0012BC78, whose two 64-bit outputs land at +0x828/
 * +0x830), sequence_end (0x1B7, returns 0), anything else loops again.
 * On picture_start it returns picture_coding_type (+0x150).
 */
int mpeg_next_header(void *arg0) {
    char *s = (char *)arg0;
    unsigned int code;
    Buf18 buf;

    for (;;) {
        next_start_code(s);
        code = _nextBit(s, 0x20);
        switch (code) {
        case 0x1B3:
            _sequenceHeader(s);
            break;
        case 0x1B8:
            _groupOfPicturesHeader(s);
            break;
        case 0x100:
            picture_header(s);
            buf.n = 5;
            buf.v[0] = -1;
            buf.v[1] = -1;
            dispatch_mpeg_callback(*(int *)(s + 0x858), &buf);
            *(long long *)(s + 0x830) = buf.v[1];
            *(long long *)(s + 0x828) = buf.v[0];
            return *(int *)(s + 0x150);
        case 0x1B7:
            return 0;
        }
    }
}

extern void _extensionAndUserData(void *);
extern void update_temp_track_data(void *, int);

/* pictureHeader (libmpeg.a:mpc.o): ISO/IEC 13818-2 6.2.3 picture_header().
 * temporal_reference (+bits 10) is kept live across the whole function to
 * feed func_001291C8 (updateTempTackData) at the end. picture_coding_type
 * (+0x150) is re-read from the struct at each use rather than kept in a
 * local -- it must survive calls that could (as far as the compiler
 * knows) touch the struct. P_TYPE/B_TYPE (2/3) is the classic
 * (x-2)<2u range check; B_TYPE alone is a plain ==3.
 */
void picture_header(void *arg0) {
    char *s = (char *)arg0;
    int temporal_reference = _nextBit(s, 10);

    *(int *)(s + 0x150) = _nextBit(s, 3);   /* picture_coding_type */
    _nextBit(s, 16);                         /* vbv_delay, discarded */

    if ((unsigned int)(*(int *)(s + 0x150) - 2) < 2) {
        *(int *)(s + 0x154) = _nextBit(s, 1);  /* full_pel_forward_vector */
        *(int *)(s + 0x158) = _nextBit(s, 3);  /* forward_f_code */
    }
    if (*(int *)(s + 0x150) == 3) {
        *(int *)(s + 0x15C) = _nextBit(s, 1);  /* full_pel_backward_vector */
        *(int *)(s + 0x160) = _nextBit(s, 3);  /* backward_f_code */
    }

    extra_bit_information(s);   /* extra_bit_information */
    _extensionAndUserData(s);   /* extension_and_user_data */
    update_temp_track_data(s, temporal_reference);  /* Update_Temporal_Reference_Tacking_Data */
}

extern void report_unknown_mpeg_extension(void *);
extern void _sequenceExtension(void *);
extern void _sequenceDisplayExtension(void *);
extern void _quantMatrixExtension(void *);
extern void _copyrightExtension(void *);
extern void _sequenceScalableExtension(void *);
extern void picture_display_extension(void *);
extern void _pictureCodingExtension(void *);
extern void _pictureSpatialScalableExtension(void *);
extern void _pictureTemporalScalableExtension(void *);


/* _extensionAndUserData (libmpeg.a:mpc.o): extension_and_user_data */
void _extensionAndUserData(void *arg0) {
    char *s = (char *)arg0;
    void (*funcs[])(void *) = {
        report_unknown_mpeg_extension,
        _sequenceExtension,
        _sequenceDisplayExtension,
        _quantMatrixExtension,
        _copyrightExtension,
        _sequenceScalableExtension,
        report_unknown_mpeg_extension,
        picture_display_extension,
        _pictureCodingExtension,
        _pictureSpatialScalableExtension,
        _pictureTemporalScalableExtension,
    };
    int code;
    unsigned int ext_id;

    next_start_code(s);
    while ((code = _peepBit(s, 32)) == 0x1B5 || code == 0x1B2) {
        if (code == 0x1B5) {
            _flushBuf(s, 32);
            ext_id = _nextBit(s, 4);
            ext_id = ext_id <= 10 ? ext_id : 0;
            funcs[ext_id](s);
            next_start_code(s);
        } else {
            _flushBuf(s, 32);
            next_start_code(s);
        }
    }
}

typedef struct {
    unsigned char pad_0[0xD4];
    int pict_struct0;
    unsigned char pad_D8[0x8C];
    int f_code00;
    int f_code01;
    int f_code10;
    int f_code11;
    int picture_structure;
    int top_field_first;
    int frame_pred_frame_dct;
    int concealment_mvs;
    int repeat_first_field;
    int progressive_frame;
} PicExt;

/* _pictureCodingExtension: parse an MPEG-2 picture coding extension:
   the four f_codes, intra DC precision, q_scale_type, intra VLC format
   and alternate scan straight into IPU_CTRL (0x10002010), the rest into
   the decoder state; the composite display fields are skipped. Adapted
   from Lombyte's project (GPL-3.0) for PAL. */
void _pictureCodingExtension(void *arg) {
    PicExt *arg0 = arg;
    int temp_2_35;
    int temp_6_65;
    int temp_6_77;
    int temp_6_89;

    arg0->f_code00 = _nextBit(arg0, 4);
    arg0->f_code01 = _nextBit(arg0, 4);
    arg0->f_code10 = _nextBit(arg0, 4);
    arg0->f_code11 = _nextBit(arg0, 4);
    *(int *)0x10002010 = (*(int *)0x10002010 & 0xFFFCFFFF) | (_nextBit(arg0, 2) << 0x10);
    temp_2_35 = _nextBit(arg0, 2);
    arg0->picture_structure = temp_2_35;
    if (arg0->pict_struct0 == 0) {
        arg0->pict_struct0 = temp_2_35;
    }
    arg0->top_field_first = _nextBit(arg0, 1);
    arg0->frame_pred_frame_dct = _nextBit(arg0, 1);
    arg0->concealment_mvs = _nextBit(arg0, 1);
    temp_6_65 = (*(volatile unsigned int *)0x10002010 & 0xFFBFFFFF) | (_nextBit(arg0, 1) << 0x16);
    *(volatile unsigned int *)0x10002010 = temp_6_65;
    temp_6_77 = (*(volatile unsigned int *)0x10002010 & 0xFFDFFFFF) | (_nextBit(arg0, 1) << 0x15);
    *(volatile unsigned int *)0x10002010 = temp_6_77;
    temp_6_89 = (*(volatile unsigned int *)0x10002010 & 0xFFEFFFFF) | (_nextBit(arg0, 1) << 0x14);
    *(volatile unsigned int *)0x10002010 = temp_6_89;
    arg0->repeat_first_field = _nextBit(arg0, 1);
    _nextBit(arg0, 1);
    arg0->progressive_frame = _nextBit(arg0, 1);
    if (_nextBit(arg0, 1) != 0) {
        _nextBit(arg0, 1);
        _nextBit(arg0, 3);
        _nextBit(arg0, 1);
        _nextBit(arg0, 7);
        _nextBit(arg0, 8);
    }
}


void extra_bit_information(void *arg0) {
    while (_nextBit(arg0, 1) != 0) {
        _flushBuf(arg0, 8);
    }
}

/* updateTempTackData (libmpeg.a:mpc.o): Update_Temporal_Reference_Tracking_Data
 * Updates bitstream frame numbers based on picture_coding_type (+0x150),
 * Temporal_Reference_Base (+0x84C), and GOP reset state (+0x854).
 */
void update_temp_track_data(void *arg0, int arg1) {
    char *s = (char *)arg0;
    int flag = 0;
    int x = 0;
    int max;

    if (*(int *)(s + 0x150) != 3) {
        if (arg1 != 0) {
            if (arg1 < 0) {
                flag = *(int *)(s + 0x854) == 0;
            }
            *(int *)(s + 0x854) = 0;
            x = arg1;
        }
    }

    *(int *)(s + 0x1AC) = *(int *)(s + 0x84C) + arg1;

    if (flag) {
        if (x >= arg1) {
            *(int *)(s + 0x1AC) += 0x400;
        }
    }

    max = *(int *)(s + 0x850);
    if (max < *(int *)(s + 0x1AC)) {
        max = *(int *)(s + 0x1AC);
    }
    *(int *)(s + 0x850) = max;
}


/* groupOfPicturesHeader (libmpeg.a:mpc.o): ISO/IEC 13818-2 6.2.2.6
 * group_of_pictures_header(). The reference decoder's "if (ld == &base)"
 * scalable-layer bookkeeping collapses to unconditional field updates here
 * (this port keeps one bitstream state, not a base/enhancement pair):
 * Temporal_Reference_Base (+0x84C) = True_Framenum_max (+0x850) + 1,
 * Temporal_Reference_GOP_Reset (+0x854) = 1, plus a +0xE8 reset.
 * Then drop_flag(1), hour(5), minute(6), marker_bit(1), sec(6), frame(6)
 * are read and discarded (dead outside VERBOSE), while closed_gop(1) and
 * broken_link(1) are kept at +0x1A4/+0x1A8; ends with a tail call into
 * extensionAndUserData.
 */
void _groupOfPicturesHeader(void *arg0) {
    char *s = (char *)arg0;

    *(int *)(s + 0xE8) = 0;
    *(int *)(s + 0x84C) = *(int *)(s + 0x850) + 1;
    *(int *)(s + 0x854) = 1;

    _nextBit(s, 1);  /* drop_flag */
    _nextBit(s, 5);  /* hour */
    _nextBit(s, 6);  /* minute */
    _nextBit(s, 1);  /* marker_bit */
    _nextBit(s, 6);  /* sec */
    _nextBit(s, 6);  /* frame */
    *(int *)(s + 0x1A4) = _nextBit(s, 1);  /* closed_gop */
    *(int *)(s + 0x1A8) = _nextBit(s, 1);  /* broken_link */

    _extensionAndUserData(s);
}

extern void _Error(void *, void *);
extern char str_mpeg_load_chroma_intra_qm[];
extern char str_mpeg_load_chroma_non_intra_qm[];

/* quantMatrixExtension (libmpeg.a:mpc.o): ISO/IEC 13818-2 6.2.3.2
 * quant_matrix_extension(). Unlike the reference decoder's 64-coefficient
 * software copy loops, this hardware-assisted port just flags the IPU to
 * pull the following intra/non-intra matrix straight off the bitstream
 * (waitIpuIdle, sendIpuCommand(0x50000000 / 0x58000000), waitIpuIdle) when
 * load_intra/non_intra_quantizer_matrix is set, storing the flag at
 * +0x840/+0x844 either way. The chroma variants aren't hardware-supported:
 * setting either just logs a warning through func_0012C468.
 */
void _quantMatrixExtension(void *arg0) {
    char *s = (char *)arg0;
    int flag;

    flag = _nextBit(s, 1);        /* load_intra_quantizer_matrix */
    *(int *)(s + 0x840) = flag;
    if (flag != 0) {
        wait_ipu_idle(s);
        send_ipu_command(s, 0x50000000);
        wait_ipu_idle(s);
    }

    flag = _nextBit(s, 1);        /* load_non_intra_quantizer_matrix */
    *(int *)(s + 0x844) = flag;
    if (flag != 0) {
        wait_ipu_idle(s);
        send_ipu_command(s, 0x58000000);
        wait_ipu_idle(s);
    }

    if (_nextBit(s, 1) != 0) {    /* load_chroma_intra_quantizer_matrix */
        _Error(s, str_mpeg_load_chroma_intra_qm);
    }

    if (_nextBit(s, 1) != 0) {    /* load_chroma_non_intra_quantizer_matrix */
        _Error(s, str_mpeg_load_chroma_non_intra_qm);
    }
}
typedef struct {
    char pad_00[0x13C];
    int  unk_13C;
    char pad_140[0x150 - 0x140];
    int  unk_150;
    char pad_154[0x174 - 0x154];
    int  unk_174;
    int  unk_178;
    char pad_17C[0x184 - 0x17C];
    int  unk_184;
    char pad_188[0x18C - 0x188];
    int  f1[3];
    int  f2[3];
} State;

void picture_display_extension(void *arg0) {
    State *s = (State *)arg0;
    int count;
    int i;

    if (s->unk_13C != 0) {
        if (s->unk_184 == 0) {
            count = 1;
        } else {
            count = s->unk_178 != 0 ? 3 : 2;
        }
    } else {
        if (s->unk_174 != 3) {
            count = 1;
        } else {
            count = s->unk_184 != 0 ? 3 : 2;
        }
    }

    for (i = 0; i < count; i++) {
        s->f1[i] = _nextBit(s, 0x10);
        _nextBit(s, 1);
        s->f2[i] = _nextBit(s, 0x10);
        _nextBit(s, 1);
    }
}


/* copyrightExtension (libmpeg.a:mpc.o) -- ISO/IEC 13818-2 6.2.3.6: skip the
 * copyright_extension() fields with func_00128A58 (nextBit, "get n bits").
 * Reference decoder (mpeg2decode gethdr.c copyright_extension()) assigns
 * each field to a local only used by its VERBOSE printf, which this port
 * drops entirely, so every nextBit's result is a dead store the compiler
 * elides -- the sequence is bare `nextBit(s, width)` calls, and the final
 * one, its value discarded, becomes 2.9-ee's bare tail jump.
 */
void _copyrightExtension(void *arg0) {
    _nextBit(arg0, 1);   /* copyright_flag */
    _nextBit(arg0, 8);   /* copyright_identifier */
    _nextBit(arg0, 1);   /* original_or_copy */
    _nextBit(arg0, 7);   /* reserved */
    _nextBit(arg0, 1);   /* marker_bit */
    _nextBit(arg0, 20);  /* copyright_number_1 */
    _nextBit(arg0, 1);   /* marker_bit */
    _nextBit(arg0, 22);  /* copyright_number_2 */
    _nextBit(arg0, 1);   /* marker_bit */
    _nextBit(arg0, 22);  /* copyright_number_3 */
}

extern int _pictureData0(void *);
extern char str_mpeg_odd_field_pictures[];
extern char str_mpeg_unknown_picture_structure[];

/*
 * Decoder state step: if the picture state (s+0x174) is 3 and s+0x120 is
 * set, report D_00153A40 through func_0012C468, clear s+0x120 and re-read
 * the state; pick the target pointer by state (3 -> +0x1C0, 1 -> +0x1D0,
 * 2 -> +0x1E0, anything else -> +0x1C0 plus a D_00153A60 report; the
 * field is a POINTER, the target of the final +0x28 write); then
 * r = func_00127858(s), and if r, target->+0x28 = 1. Exact under both
 * compilers.
 *
 * It is a `switch`, with the case bodies in the order 3, 1, 2, default.
 * GCC's decision tree is fixed (==2, <3, ==1 / ==3), but the body that
 * follows the dispatch is the first case in source order, and jump.c
 * then turns `beq 3,case3; b default` into retail's `bne 3,default` plus
 * fall-through (the nested-if decode was 200 against 204). The state is
 * re-read into the local after the report, as 2.95.3 needs; switching on
 * the memory re-read is exact under 2.9-ee too.
 */
int mpeg_dec_picture(void *arg0) {
    char *s = arg0;
    int state = *(int *)(s + 0x174);
    char *target;
    int r;

    if (state == 3 && *(int *)(s + 0x120) != 0) {
        _Error(s, str_mpeg_odd_field_pictures);
        *(int *)(s + 0x120) = 0;
        state = *(int *)(s + 0x174);
    }
    switch (state) {
    case 3:
        target = *(char **)(s + 0x1C0);
        break;
    case 1:
        target = *(char **)(s + 0x1D0);
        break;
    case 2:
        target = *(char **)(s + 0x1E0);
        break;
    default:
        target = *(char **)(s + 0x1C0);
        _Error(s, str_mpeg_unknown_picture_structure);
        break;
    }
    r = _pictureData0(s);
    if (r != 0) {
        *(int *)(target + 0x28) = 1;
    }
    return r;
}

extern void _dispRefImage();

void mpeg_output_frame(void *arg0, int arg1, int arg2) {
    char *s = (char *)arg0;
    int v, w;

    if (arg2 != 0) {
        if (*(int *)(s + 0x174) == 3) {
            if (*(int *)(s + 0x150) == 3) {
                v = *(int *)(s + 0x1C4);
            } else {
                v = *(int *)(s + 0x1B8);
            }
            _dispRefImage(s, v, arg1 - 1);
        } else {
            if (*(int *)(s + 0x150) == 3) {
                v = *(int *)(s + 0x1D4);
                w = *(int *)(s + 0x1E4);
            } else {
                v = *(int *)(s + 0x1C8);
                w = *(int *)(s + 0x1D8);
            }
            _dispRefImageField(s, v, w, arg1 - 1);
        }
    }
    if (*(int *)(s + 0xF8) == 1) {
        *(int *)(s + 0xF8) = 2;
    }
}

/* libmpeg picture-buffer update: for a B picture refresh the current
   frame pointers and decide from the ready flags (+0x28) of the
   reference frames whether the picture can be output; otherwise rotate
   forward/backward pairs (when arg1 is 0). Then stamp the selected
   frame record with the picture's parameters. */
#define PI_(o) (*(int *)((char *)p + (o)))
#define PP_(o) (*(char **)((char *)p + (o)))
#define RDY_(o) (*(int *)(PP_(o) + 0x28))
int mpeg_update_ref_image(void *p, int arg1) {
    char *rec;
    int ret;
    int n;
    char *tmp;
    char *cur;

    rec = 0;
    ret = 0;
    n = (PI_(0x174) == 3) ? 2 : 4;
    if (PI_(0x150) == 3) {
        PP_(0x1C0) = PP_(0x1C4);
        PP_(0x1D0) = PP_(0x1D4);
        PP_(0x1E0) = PP_(0x1E4);
        if (PI_(0xA0) + PI_(0xA4) >= n) {
            PI_(0xE8) = 0;
            PI_(0x1A8) = 0;
            PI_(0x1A4) = 0;
        }
        if ((PI_(0xE8) != 0 || PI_(0x1A8) != 0) && PI_(0x1A4) == 0) {
            RDY_(0x1B8) = 0;
            RDY_(0x1C8) = 0;
            RDY_(0x1D8) = 0;
        }
        PI_(0xE8) = 0;
        PI_(0x1A8) = 0;
        if (PI_(0x174) == 3) {
            if (RDY_(0x1B8) == 1 || PI_(0x1A4) != 0) {
                if (RDY_(0x1BC) == 1) {
                    ret = 1;
                }
            }
        } else {
            if ((RDY_(0x1C8) == 1 && RDY_(0x1D8) == 1) || PI_(0x1A4) != 0) {
                if (RDY_(0x1CC) == 1) {
                    if (RDY_(0x1DC) == 1) {
                        ret = 1;
                    }
                }
            }
        }
    } else {
        if (arg1 == 0) {
            tmp = PP_(0x1B8);
            PP_(0x1B8) = PP_(0x1BC);
            PP_(0x1BC) = tmp;
            tmp = PP_(0x1C8);
            PP_(0x1C8) = PP_(0x1CC);
            PP_(0x1CC) = tmp;
            tmp = PP_(0x1D8);
            PP_(0x1D8) = PP_(0x1DC);
            PP_(0x1DC) = tmp;
        }
        PP_(0x1C0) = PP_(0x1BC);
        PP_(0x1D0) = PP_(0x1CC);
        PP_(0x1E0) = PP_(0x1DC);
        if (PI_(0x174) == 3) {
            if (PI_(0x150) != 2 || RDY_(0x1B8) == 1) {
                ret = 1;
            }
        } else {
            cur = (PI_(0x174) == 1) ? PP_(0x1E0) : PP_(0x1D0);
            if (PI_(0x150) != 2 || (arg1 != 0 && *(int *)(cur + 0x28) == 1) ||
                (RDY_(0x1C8) == 1 && RDY_(0x1D8) == 1)) {
                ret = 1;
            }
        }
    }
    switch (PI_(0x174)) {
    case 3:
        rec = PP_(0x1C0);
        break;
    case 1:
        rec = PP_(0x1D0);
        break;
    case 2:
        rec = PP_(0x1E0);
        break;
    }
    *(int *)(rec + 0x28) = 0;
    *(long *)(rec + 0x18) = *(long *)((char *)p + 0x828);
    *(int *)(rec + 0x2C) = PI_(0x150);
    *(long *)(rec + 0x20) = *(long *)((char *)p + 0x830);
    *(int *)(rec + 0x30) = PI_(0x174);
    *(int *)(rec + 0x34) = PI_(0x13C);
    *(int *)(rec + 0x38) = PI_(0x188);
    *(int *)(rec + 0x3C) = PI_(0x178);
    *(int *)(rec + 0x40) = PI_(0x184);
    *(int *)(rec + 0x44) = PI_(0x18C);
    *(int *)(rec + 0x48) = PI_(0x190);
    *(int *)(rec + 0x4C) = PI_(0x194);
    *(int *)(rec + 0x50) = PI_(0x198);
    *(int *)(rec + 0x54) = PI_(0x19C);
    *(int *)(rec + 0x58) = PI_(0x1A0);
    *(int *)(rec + 0x5C) = PI_(0x148);
    *(int *)(rec + 0x60) = PI_(0x14C);
    return ret;
}
#undef PI_
#undef PP_
#undef RDY_

extern char str_mpeg_buffer_too_small_fmt[];
extern void sprintf_4(void *, char *, int, int) __asm__("func_00116248");
extern void _Error_a(void *, void *) __asm__("func_0012C468");

/* Does the request at arg1 fit the heap described by arg0? A sized
   request (+0xE0 non-zero) has to fit both the byte budget at +0xDC and
   the entry budget at +0xE0; an unsized one has to fit width * height
   against +0xE4. On a refusal, format the two figures into the message
   at D_00153A80 and report it. Returns whether it fits. */
int _isOutSizeOK(void *arg0, void *arg1) {
    char buf[0x100];
    char *h = (char *)arg0;
    char *q = (char *)arg1;
    int e = *(int *)(h + 0xE0);
    int ok;

    if (e != 0) {
        ok = *(int *)(h + 0xDC) >= *(int *)(q + 0x4) &&
             e >= *(int *)(q + 0x8);
    } else {
        ok = *(int *)(h + 0xE4) >= *(int *)(q + 0xC) * *(int *)(q + 0x10);
    }
    if (ok == 0) {
        sprintf_4(buf, str_mpeg_buffer_too_small_fmt, *(int *)(q + 0x4),
                        *(int *)(q + 0x8));
        _Error_a(arg0, buf);
    }
    return ok;
}

struct Cpr8Mpeg {
    unsigned char pad_0[0xD8];
    int unkD8;
    unsigned char pad_DC[0x4];
    int unkE0;
    int unkE4;
    unsigned char pad_E8[0x8C];
    int unk174;
};

struct Cpr8Pic {
    int unk0;
    unsigned char pad_4[0x8];
    int unkC;
    int unk10;
};

/* _cpr8 (libmpeg): copy a decoded picture to its destination with two
   chained DMA transfers per row, per field (two fields when +0xE0 gives
   an interlaced width). Adapted from Lombyte's project (GPL-3.0) for PAL; its
   goto-shaped control flow is what matches. */
void _cpr8(struct Cpr8Mpeg *arg0, struct Cpr8Pic *arg1) {
    struct Cpr8Mpeg *m;
    int fields;
    int base;
    int intr2;
    int intr1;
    int width;
    int i;
    int dst;
    int src;
    int qwc;
    int row;
    int stride;
    int w;
    int f;

    m = (struct Cpr8Mpeg *)arg0;
    src = arg1->unk0 & 0x0FFFFFFF;
    base = arg0->unkD8 & 0x0FFFFFFF;
    if (m->unk174 == 3) {
        goto block_3;
    }
    width = m->unkE0;
    w = width;
    if (width != 0) {
        goto block_8;
    }
    goto block_4;
block_3:
    w = m->unkE0;
block_4:
    row = arg1->unk10 * 0x180;
    qwc = row >> 4;
    if (w == 0) {
        goto block_6;
    }
    stride = (w >> 4) * 0x180;
    goto block_7;
block_6:
    stride = row;
block_7:
    fields = 1;
    goto block_9;
block_8:
    stride = (width >> 4) * 0xC0;
    row = ((int) arg1->unk10 >> 1) * 0x180;
    fields = 2;
    qwc = row >> 4;
block_9:
    f = 0;
    while (f < fields) {
        dst = base;
        for (i = 0; i < arg1->unkC; i++) {
            intr1 = disable_interrupts();
            *(volatile int *)0x1000D480 = 0;
            *(volatile int *)0x1000D410 = src;
            *(volatile int *)0x1000D420 = qwc;
            *(volatile int *)0x1000D400 = 0x101;
            if (intr1 == 0) {
                goto block_15;
            }
            enable_interrupts();
        block_15:
            while (*(volatile unsigned int *)0x1000D400 & 0x100) {
            }
            intr2 = disable_interrupts();
            *(volatile int *)0x1000D080 = 0;
            *(volatile int *)0x1000D010 = dst;
            *(volatile int *)0x1000D020 = qwc;
            *(volatile int *)0x1000D000 = 0x100;
            if (intr2 == 0) {
                goto block_19;
            }
            enable_interrupts();
        block_19:
            while (*(volatile unsigned int *)0x1000D000 & 0x100) {
            }
            while (*(volatile unsigned int *)0x1000D020 != 0) {
            }
            dst += stride;
            src += row;
        }
        f++;
        base += m->unkE4 * 0xC0;
    }
    return;
}

int mpeg_mark_picture_output(void *arg0) {
    Obj40 *s = (Obj40 *)arg0;
    int r = 1;
    if (s->unk008 != 2) {
        int v = s->unk118;
        s->unk008 = 2;
        s->unk0AC = v;
    }
    s->unk820 = r;
    return r;
}

long _getPtsDtsFlags(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4) {
    char *s = arg0;
    char *t = arg1;
    long *o1 = arg2;
    long v;
    long packed;

    if (*(int *)(s + 0x70) != 0) {
        int base;
        extern long func_0011EEC8(long, long);
        v = *(long *)(t + 0x18);
        if (v >= 0 || (base = *(int *)(s + 0x80)) < 0) {
            *o1 = v;
        } else {
            int n = *(long *)(s + 0x88);
            long r = func_0011EEC8(n & 1, *(long *)(s + 0x78) & 1);
            int cnt = *(int *)(s + 0x90);
            int a = func_0011EEC8(r, cnt & 1);
            *o1 = base + ((int)(func_0011EEC8(*(long *)(s + 0x78), n) >> 1) + a);
            if (func_0011EEC8(n & 1, *(long *)(s + 0x78) & 1)) {
                *(int *)(s + 0x90) = cnt + 1;
            }
        }
    } else {
        *o1 = *(long *)(t + 0x18);
    }
    if (*(int *)(s + 0xF8) == 2 && (v = *(long *)(s + 0xF0)) >= 0) {
        *o1 = v;
        *(int *)(s + 0xF8) = 0;
        *(long *)(s + 0xF0) = -1;
    }
    *(long *)arg3 = *(long *)(t + 0x20);
    packed = ((long)*(int *)(t + 0x34) << 8) | ((long)*(int *)(t + 0x38) << 7)
           | ((long)*(int *)(t + 0x3C) << 6) | ((long)*(int *)(t + 0x40) << 5)
           | ((long)*(int *)(t + 0x30) << 3) | *(int *)(t + 0x2C);
    *(long *)arg4 = packed;
    return packed;
}

extern long _getPtsDtsFlags(void *s, void *t, void *o1, void *o3, void *o4);
extern unsigned int mpeg_display_field_count_table[];
extern void _csc_storeRefImage(void *arg0, void *arg1);
void _dispRefImage(char *s, char *t) {
    char *base;
    long x;

    base = *(char **)(s + 0x858);
    _getPtsDtsFlags(s, t, base + 0x10, base + 0x18, base + 0x20);
    base = *(char **)(s + 0x858);
    x = *(long *)(base + 0x20);
    *(int *)(s + 0x80) = *(int *)(base + 0x10);
    *(unsigned long *)(s + 0x88) = mpeg_display_field_count_table[(int)(x >> 5) & 0xF];
    *(int *)(s + 0xCC) = *(int *)(t + 0x5C);
    *(int *)(s + 0xD0) = *(int *)(t + 0x60);
    *(int *)(s + 0xB4) = *(int *)(t + 0x44);
    *(int *)(s + 0xB8) = *(int *)(t + 0x48);
    *(int *)(s + 0xBC) = *(int *)(t + 0x4C);
    *(int *)(s + 0xC0) = *(int *)(t + 0x50);
    *(int *)(s + 0xC4) = *(int *)(t + 0x54);
    *(int *)(s + 0xC8) = *(int *)(t + 0x58);
    if (_isOutSizeOK(s, t) == 0) return;
    if (*(int *)(t + 0x28) != 1) return;
    if (*(int *)(s + 0xB0) != 0) {
        _csc_storeRefImage(s, t);
    } else {
        _cpr8((void *)s, (void *)t);
    }
    mpeg_mark_picture_output(s);
}

extern long _getPtsDtsFlags(void *s, void *t, void *o1, void *o3, void *o4);
extern void _csc_storeRefImage(void *arg0, void *arg1);

/* Transcribed from the assembly. Two calls to func_00129CA0 (one for each record,
   with the flag at +0x88 set to 1 for the second), a third that copies fields of
   both records into the object and calls func_00129948, then either a halving of
   p1+0x10 with a tail call to func_00129C78, or an early return. Retail's $2 return
   value is not read by the caller (func_00129600 declares this function void), so
   the early returns just return. */
void _dispRefImageField(char *s, char *a1, char *a2) {
    char *p1;
    char *p2;
    char *t1;
    char *t2;
    char *base;
    char *base2;
    char *base3;
    long v2;
    long v3;
    int f22;
    int w;
    int w2;
    int r;

    p1 = a1;
    p2 = a2;
    f22 = 0;
    if (*(int *)(s + 0x174) == 2) {
        t1 = p1;
        t2 = p2;
        f22 = 0x40;
    } else {
        t1 = p2;
        t2 = p1;
    }
    base = *(char **)(s + 0x858);
    _getPtsDtsFlags(s, t1, base + 0x10, base + 0x18, base + 0x20);
    base2 = *(char **)(s + 0x858);
    w = *(int *)(base2 + 0x10);
    ((union { long l; int i; } *)(s + 0x88))->l = 1;
    *(int *)(s + 0x80) = w;
    _getPtsDtsFlags(s, t2, base2 + 0x28, base2 + 0x30, base2 + 0x38);
    base3 = *(char **)(s + 0x858);
    w2 = *(int *)(base3 + 0x28);
    ((union { long l; int i; } *)(s + 0x88))->l = 1;
    *(int *)(s + 0x80) = w2;
    v2 = *(long *)(base3 + 0x20) | f22;
    *(int *)(s + 0xCC) = *(int *)(t1 + 0x5C);
    *(long *)(base3 + 0x20) = v2;
    v3 = *(long *)(base3 + 0x38) | f22;
    *(int *)(s + 0xD0) = *(int *)(t1 + 0x60);
    *(long *)(base3 + 0x38) = v3;
    *(int *)(s + 0xB4) = *(int *)(t1 + 0x44);
    *(int *)(s + 0xB8) = *(int *)(t2 + 0x48);
    *(int *)(s + 0xC0) = *(int *)(t1 + 0x50);
    *(int *)(s + 0xC4) = *(int *)(t2 + 0x54);
    r = _isOutSizeOK(s, p1);
    if (r == 0) return;
    if (*(int *)(p1 + 0x28) != 1) return;
    if (*(int *)(p2 + 0x28) != 1) return;
    *(int *)(p1 + 0x10) = (int)((unsigned int)*(int *)(p1 + 0x10) << 1);
    if (*(int *)(s + 0xB0) != 0) {
        _csc_storeRefImage(s, p1);
    } else {
        _cpr8((void *)s, (void *)p1);
    }
    *(int *)(p1 + 0x10) = *(int *)(p1 + 0x10) >> 1;
    mpeg_mark_picture_output(s);
}

extern char mpeg_ref_image_chain[];

struct DmaEnt {
    int unk0;
    int unk4;
    int a[4];
    int b[69];
    int n;
    int pad[4];
};

struct DmaObj {
    unsigned char pad_0[0x590];
    struct DmaEnt e[2];
    int unk810;
};

/* Builds a DMA tag chain (two 64-bit tags per entry) at the uncached alias of D_0015E940, then starts DMA channel 0x1000D400 on it. */
void dma_ref_image(void *arg0) {
    struct DmaObj *s = arg0;
    int m;
    int n;
    int i;
    long *p;
    long tag;
    int lo;
    int hi;
    int r;

    m = s->unk810;
    p = (long *)(((unsigned int)mpeg_ref_image_chain & 0x0FFFFFFF) | 0x20000000);
    n = s->e[m].n;
    i = 0;
    if (n > 0) do {
        lo = s->e[m].a[i] & 0x0FFFFFFF;
        hi = s->e[m].b[i] & 0x0FFFFFFF;
        tag = 3;
        if (i == n - 1)
            tag = 0;
        p[0] = ((long)lo << 32) | 0x30000030;
        p[2] = ((long)hi << 32) | (tag << 28) | 0x30;
        p += 4;
    } while (++i < n);
    r = disable_interrupts();
    __asm__ __volatile__("sync");
    *(volatile int *)0x1000D480 = s->e[s->unk810].unk0;
    *(volatile int *)0x1000D430 = (int)mpeg_ref_image_chain;
    *(volatile int *)0x1000D420 = 0;
    *(volatile int *)0x1000D400 = 0x105;
    if (r != 0) {
        enable_interrupts();
    }
}


/* receiveDataFromIPU (libmpeg.a:mpc.o): kicks off an IPU->main-memory DMA
 * receive. func_0011D960 presumably starts/queries the DMA channel; its
 * result gates whether func_0011D9A8 (presumably a completion/kick) runs.
 * The three MMIO stores (0x1000B000/10/20 -- D2_CHCR/D2_MADR/D2_QWC, the
 * IPU_to_RAM DMA channel) queue the transfer: MADR = (arg0 physical addr)
 * | 0x80000000 (spr flag), QWC = arg1 >> 4 (bytes -> qwords), CHCR = 0x100
 * (start, from-memory... actually to-memory chain/normal mode).
 */
void receive_data_from_ipu(void *arg0, int arg1) {
    int r = disable_interrupts();
    *(volatile unsigned int *)0x1000B010 = ((unsigned int)arg0 & 0x0FFFFFFF) | 0x80000000;
    *(volatile unsigned int *)0x1000B020 = arg1 >> 4;
    *(volatile unsigned int *)0x1000B000 = 0x100;
    if (r != 0) {
        enable_interrupts();
    }
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012A2EC);
