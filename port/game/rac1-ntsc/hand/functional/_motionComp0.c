/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/core/00125630.c (games/rac1): func_00125630, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */

typedef int s32_c1254f0;
typedef long s64_c1254f0;
typedef unsigned char u8_c1254f0;
typedef unsigned int u32_c1254f0;
typedef unsigned long u64_c1254f0;
struct MpegDecoder_c1254f0;
extern void func_0012C2F0(void *, char *, int);
extern void func_00125740(struct MpegDecoder_c1254f0 *d, int bx, int by, int mb_type, int motion_type,
                          int pmv[2][2][2], int mvfs[2][2], int *dmvector);
extern void func_00129FB8(void *);
extern char D_001536E8[];
typedef struct {
    char pad0[0x128];
    int unk128;
    int unk12C;
    int unk130;
    int unk134;
    int unk138;
    int unk13C;
} Elem00125630_c1254f0;
typedef struct {
    int unk0;
    int pad4[3];
    int unk10;
} Ref00125630_c1254f0;
typedef struct {
    char pad0[0x11C];
    int unk11C;
    char pad120[0xC];
    int unk12C;
    char pad130[0x44];
    int unk174;
    char pad178[0x48];
    Ref00125630_c1254f0 *unk1C0;
    char pad1C4[0xC];
    Ref00125630_c1254f0 *unk1D0;
    char pad1D4[0xC];
    Ref00125630_c1254f0 *unk1E0;
    char pad1E4[0x3AC];
    Elem00125630_c1254f0 e[2];
    int unk810;
} Dec00125630_c1254f0;
int _motionComp0(void *arg0, int mbAddr, int arg2, int mbType, int motionType,
                  void *pmv, void *mvfs, void *dmv) {
    Dec00125630_c1254f0 *d = (Dec00125630_c1254f0 *)arg0;
    int intra;
    int x;
    int y;
    int bx;
    int by;
    Ref00125630_c1254f0 *ref;
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
            func_0012C2F0(d, D_001536E8, motionType);
            d->unk11C = 1;
            return 0;
        }
        func_00125740((struct MpegDecoder_c1254f0 *)d, bx, by, mbType, motionType,
                      pmv, mvfs, dmv);
        while ((*(volatile unsigned int *)0x1000D400 >> 8) & 1)
            ;
        func_00129FB8(d);
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
struct MpegDecoder_c1254f0;
struct sceMpeg_c1254f0 {
    s32_c1254f0 width;
    s32_c1254f0 height;
    s32_c1254f0 frameCount;
    u8_c1254f0 pad_C[0x4];
    s64_c1254f0 pts;
    s64_c1254f0 dts;
    u64_c1254f0 flags;
    s64_c1254f0 pts2nd;
    s64_c1254f0 dts2nd;
    u64_c1254f0 flags2nd;
    struct MpegDecoder_c1254f0 *sys;
};
struct MpegRefImage_c1254f0 {
    u8_c1254f0 *data;
    s32_c1254f0 width;
    s32_c1254f0 height;
    s32_c1254f0 unkC;
    s32_c1254f0 unk10;
    u8_c1254f0 pad_14[4];
    s64_c1254f0 pts;
    s64_c1254f0 dts;
    s32_c1254f0 status;
    s32_c1254f0 picture_coding_type;
    s32_c1254f0 picture_structure;
    s32_c1254f0 unk13C;
    s32_c1254f0 progressive_frame;
    s32_c1254f0 top_field_first;
    s32_c1254f0 repeat_first_field;
    s32_c1254f0 frame_centre_horizontal_offset[3];
    s32_c1254f0 frame_centre_vertical_offset[3];
    s32_c1254f0 display_horizontal_size;
    s32_c1254f0 display_vertical_size;
};
typedef void (*MpegMcFunc_c1254f0)(void);
struct MpegMcFetch_c1254f0 {
    u8_c1254f0 *addr;
    s32_c1254f0 x;
    s32_c1254f0 n0;
    s32_c1254f0 n1;
    s32_c1254f0 step;
    u8_c1254f0 *p0;
    u8_c1254f0 *p1;
};
struct MpegMbBuffer_c1254f0 {
    u32_c1254f0 spr_base;
    u32_c1254f0 ipu_out;
    u8_c1254f0 *luma_ref[4];
    u8_c1254f0 *chroma_ref[4];
    MpegMcFunc_c1254f0 luma_func[4];
    MpegMcFunc_c1254f0 chroma_func[4];
    struct MpegMcFetch_c1254f0 luma[4];
    struct MpegMcFetch_c1254f0 chroma[4];
    s32_c1254f0 unk128;
    s32_c1254f0 fetch_count;
    s32_c1254f0 unk130;
    u8_c1254f0 pad_134[0x4];
    s32_c1254f0 unk138;
    s32_c1254f0 unk13C;
};
struct MpegDecoder_c1254f0 {
    s32_c1254f0 unk0;
    s32_c1254f0 unk4;
    s32_c1254f0 unk8;
    u8_c1254f0 pad_C[0x40];
    u8_c1254f0 ipu_dma_state;
    u8_c1254f0 pad_4D[0x33];
    s32_c1254f0 unk80;
    u8_c1254f0 pad_84[0x4];
    u64_c1254f0 unk88;
    u8_c1254f0 pad_90[0x4];
    s32_c1254f0 unk94;
    s32_c1254f0 unk98;
    s32_c1254f0 unk9C;
    s32_c1254f0 unkA0;
    s32_c1254f0 unkA4;
    s32_c1254f0 unkA8;
    s32_c1254f0 frame_base;
    s32_c1254f0 unkB0;
    s32_c1254f0 unkB4;
    s32_c1254f0 unkB8;
    s32_c1254f0 unkBC;
    s32_c1254f0 unkC0;
    s32_c1254f0 unkC4;
    s32_c1254f0 unkC8;
    s32_c1254f0 unkCC;
    s32_c1254f0 unkD0;
    s32_c1254f0 unkD4;
    s32_c1254f0 unkD8;
    s32_c1254f0 out_max_width;
    s32_c1254f0 out_max_height;
    s32_c1254f0 out_max_size;
    s32_c1254f0 unkE8;
    u8_c1254f0 pad_EC[0xC];
    s32_c1254f0 unkF8;
    s32_c1254f0 frame_buffers[3];
    u8_c1254f0 pad_108[0x10];
    s32_c1254f0 frame_count;
    s32_c1254f0 unk11C;
    s32_c1254f0 second_field;
    u32_c1254f0 horizontal_size;
    s32_c1254f0 vertical_size;
    s32_c1254f0 mb_width;
    s32_c1254f0 mb_height;
    u32_c1254f0 bit_rate;
    s32_c1254f0 vbv_buffer_size;
    s32_c1254f0 unk13C;
    s32_c1254f0 unk140;
    s32_c1254f0 matrix_coefficients;
    s32_c1254f0 display_horizontal_size;
    s32_c1254f0 display_vertical_size;
    s32_c1254f0 picture_coding_type;
    s32_c1254f0 unk154;
    s32_c1254f0 unk158;
    s32_c1254f0 unk15C;
    s32_c1254f0 unk160;
    s32_c1254f0 f_code_forward_h;
    s32_c1254f0 f_code_forward_v;
    s32_c1254f0 f_code_backward_h;
    s32_c1254f0 f_code_backward_v;
    s32_c1254f0 picture_structure;
    s32_c1254f0 top_field_first;
    s32_c1254f0 frame_pred_frame_dct;
    s32_c1254f0 concealment_motion_vectors;
    s32_c1254f0 repeat_first_field;
    s32_c1254f0 progressive_frame;
    s32_c1254f0 frame_centre_horizontal_offset[3];
    s32_c1254f0 frame_centre_vertical_offset[3];
    s32_c1254f0 closed_gop;
    s32_c1254f0 broken_link;
    s32_c1254f0 temporal_reference_framenum;
    s32_c1254f0 dc_reset;
    s32_c1254f0 quantiser_scale_code;
    struct MpegRefImage_c1254f0 *ref_images[12];
    u8_c1254f0 pad_1E8[0x3A8];
    struct MpegMbBuffer_c1254f0 mb_buf[2];
    s32_c1254f0 mb_buf_index;
    s32_c1254f0 unk814;
    s32_c1254f0 ipu_cmd_status;
    u8_c1254f0 *mc_work;
    s32_c1254f0 unk820;
    s32_c1254f0 unk824;
    s64_c1254f0 pts;
    s64_c1254f0 dts;
    s32_c1254f0 bit_buffer;
    s32_c1254f0 bit_count;
    s32_c1254f0 load_intra_quantiser_matrix;
    s32_c1254f0 load_non_intra_quantiser_matrix;
    s32_c1254f0 unk848;
    s32_c1254f0 temporal_reference_base;
    s32_c1254f0 true_framenum_max;
    s32_c1254f0 temporal_reference_gop_reset;
    struct sceMpeg_c1254f0 *mpeg;
};
extern void func_0012C2F0(void *, char *, int);
extern int _motionComp0(void *, int, int, int, int, void *, void *, void *);
