/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/core/00125630.c (games/rac1): func_00125F88, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */

typedef int s32_c125e48;
typedef long s64_c125e48;
typedef unsigned char u8_c125e48;
typedef unsigned int u32_c125e48;
typedef unsigned long u64_c125e48;
struct MpegDecoder_c125e48;
struct MpegDecoder_c125e48;
struct sceMpeg_c125e48 {
    s32_c125e48 width;
    s32_c125e48 height;
    s32_c125e48 frameCount;
    u8_c125e48 pad_C[0x4];
    s64_c125e48 pts;
    s64_c125e48 dts;
    u64_c125e48 flags;
    s64_c125e48 pts2nd;
    s64_c125e48 dts2nd;
    u64_c125e48 flags2nd;
    struct MpegDecoder_c125e48 *sys;
};
struct MpegRefImage_c125e48 {
    u8_c125e48 *data;
    s32_c125e48 width;
    s32_c125e48 height;
    s32_c125e48 unkC;
    s32_c125e48 unk10;
    u8_c125e48 pad_14[4];
    s64_c125e48 pts;
    s64_c125e48 dts;
    s32_c125e48 status;
    s32_c125e48 picture_coding_type;
    s32_c125e48 picture_structure;
    s32_c125e48 unk13C;
    s32_c125e48 progressive_frame;
    s32_c125e48 top_field_first;
    s32_c125e48 repeat_first_field;
    s32_c125e48 frame_centre_horizontal_offset[3];
    s32_c125e48 frame_centre_vertical_offset[3];
    s32_c125e48 display_horizontal_size;
    s32_c125e48 display_vertical_size;
};
typedef void (*MpegMcFunc_c125e48)(void);
struct MpegMcFetch_c125e48 {
    u8_c125e48 *addr;
    s32_c125e48 x;
    s32_c125e48 n0;
    s32_c125e48 n1;
    s32_c125e48 step;
    u8_c125e48 *p0;
    u8_c125e48 *p1;
};
struct MpegMbBuffer_c125e48 {
    u32_c125e48 spr_base;
    u32_c125e48 ipu_out;
    u8_c125e48 *luma_ref[4];
    u8_c125e48 *chroma_ref[4];
    MpegMcFunc_c125e48 luma_func[4];
    MpegMcFunc_c125e48 chroma_func[4];
    struct MpegMcFetch_c125e48 luma[4];
    struct MpegMcFetch_c125e48 chroma[4];
    s32_c125e48 unk128;
    s32_c125e48 fetch_count;
    s32_c125e48 unk130;
    u8_c125e48 pad_134[0x4];
    s32_c125e48 unk138;
    s32_c125e48 unk13C;
};
struct MpegDecoder_c125e48 {
    s32_c125e48 unk0;
    s32_c125e48 unk4;
    s32_c125e48 unk8;
    u8_c125e48 pad_C[0x40];
    u8_c125e48 ipu_dma_state;
    u8_c125e48 pad_4D[0x33];
    s32_c125e48 unk80;
    u8_c125e48 pad_84[0x4];
    u64_c125e48 unk88;
    u8_c125e48 pad_90[0x4];
    s32_c125e48 unk94;
    s32_c125e48 unk98;
    s32_c125e48 unk9C;
    s32_c125e48 unkA0;
    s32_c125e48 unkA4;
    s32_c125e48 unkA8;
    s32_c125e48 frame_base;
    s32_c125e48 unkB0;
    s32_c125e48 unkB4;
    s32_c125e48 unkB8;
    s32_c125e48 unkBC;
    s32_c125e48 unkC0;
    s32_c125e48 unkC4;
    s32_c125e48 unkC8;
    s32_c125e48 unkCC;
    s32_c125e48 unkD0;
    s32_c125e48 unkD4;
    s32_c125e48 unkD8;
    s32_c125e48 out_max_width;
    s32_c125e48 out_max_height;
    s32_c125e48 out_max_size;
    s32_c125e48 unkE8;
    u8_c125e48 pad_EC[0xC];
    s32_c125e48 unkF8;
    s32_c125e48 frame_buffers[3];
    u8_c125e48 pad_108[0x10];
    s32_c125e48 frame_count;
    s32_c125e48 unk11C;
    s32_c125e48 second_field;
    u32_c125e48 horizontal_size;
    s32_c125e48 vertical_size;
    s32_c125e48 mb_width;
    s32_c125e48 mb_height;
    u32_c125e48 bit_rate;
    s32_c125e48 vbv_buffer_size;
    s32_c125e48 unk13C;
    s32_c125e48 unk140;
    s32_c125e48 matrix_coefficients;
    s32_c125e48 display_horizontal_size;
    s32_c125e48 display_vertical_size;
    s32_c125e48 picture_coding_type;
    s32_c125e48 unk154;
    s32_c125e48 unk158;
    s32_c125e48 unk15C;
    s32_c125e48 unk160;
    s32_c125e48 f_code_forward_h;
    s32_c125e48 f_code_forward_v;
    s32_c125e48 f_code_backward_h;
    s32_c125e48 f_code_backward_v;
    s32_c125e48 picture_structure;
    s32_c125e48 top_field_first;
    s32_c125e48 frame_pred_frame_dct;
    s32_c125e48 concealment_motion_vectors;
    s32_c125e48 repeat_first_field;
    s32_c125e48 progressive_frame;
    s32_c125e48 frame_centre_horizontal_offset[3];
    s32_c125e48 frame_centre_vertical_offset[3];
    s32_c125e48 closed_gop;
    s32_c125e48 broken_link;
    s32_c125e48 temporal_reference_framenum;
    s32_c125e48 dc_reset;
    s32_c125e48 quantiser_scale_code;
    struct MpegRefImage_c125e48 *ref_images[12];
    u8_c125e48 pad_1E8[0x3A8];
    struct MpegMbBuffer_c125e48 mb_buf[2];
    s32_c125e48 mb_buf_index;
    s32_c125e48 unk814;
    s32_c125e48 ipu_cmd_status;
    u8_c125e48 *mc_work;
    s32_c125e48 unk820;
    s32_c125e48 unk824;
    s64_c125e48 pts;
    s64_c125e48 dts;
    s32_c125e48 bit_buffer;
    s32_c125e48 bit_count;
    s32_c125e48 load_intra_quantiser_matrix;
    s32_c125e48 load_non_intra_quantiser_matrix;
    s32_c125e48 unk848;
    s32_c125e48 temporal_reference_base;
    s32_c125e48 true_framenum_max;
    s32_c125e48 temporal_reference_gop_reset;
    struct sceMpeg_c125e48 *mpeg;
};
extern void _getRef0(struct MpegDecoder_c125e48 *d, struct MpegRefImage_c125e48 *ref, int sfield, int dfield, int yofs, int h, int bx, int by, int dx, int dy, int fieldpred, int avg);
extern MpegMcFunc_c125e48 D_00132E30[];
extern MpegMcFunc_c125e48 D_00132E50[];
void _getRef0(struct MpegDecoder_c125e48 *d, struct MpegRefImage_c125e48 *ref, int sfield, int dfield, int yofs, int h,
                   int bx, int by, int dx, int dy, int fieldpred, int avg) {
    struct MpegMcFetch_c125e48 *fl;
    struct MpegMcFetch_c125e48 *fc;
    u8_c125e48 *mc;
    int n, x, y, mbx, mby, ly, lx, v, base, code, mb;
    int cmvx, cmvy, ch, cmbx, cmby;
    int xh, yh, cyo, o0, o1;
    u8_c125e48 *data;
    n = d->mb_buf[d->mb_buf_index].fetch_count;
    mc = d->mc_work;
    fl = &d->mb_buf[d->mb_buf_index].luma[n];
    fc = &d->mb_buf[d->mb_buf_index].chroma[n];
    x = (dx >> 1) + bx;
    if (fieldpred) {
        y = ((dy >> 1) << 1) + yofs + (by + sfield);
    } else {
        y = (dy >> 1) + yofs + (by + sfield);
    }
    mbx = x >> 4;
    mby = y >> 4;
    mb = mbx * ref->unk10 + mby;
    fl->x = x - (mbx << 4);
    fl->addr = mc + ((dfield + yofs) << 5);
    ly = y - (mby << 4);
    yh = dy & 1;
    xh = dx & 1;
    if (yh) {
        if (ly + (h << fieldpred) >= 16) {
            v = (16 >> fieldpred) - (ly >> fieldpred) - 1;
            fl->n0 = v;
            fl->n1 = h - v;
        } else {
            fl->n0 = h;
            fl->n1 = 0;
        }
    } else {
        if (ly + (h << fieldpred) >= 17) {
            v = (16 >> fieldpred) - (ly >> fieldpred);
            fl->n0 = v;
            fl->n1 = h - v;
        } else {
            fl->n0 = h;
            fl->n1 = 0;
        }
    }
    base = d->mb_buf[d->mb_buf_index].spr_base + n * 0x600;
    cmvx = dx / 2;
    o0 = ly << 4;
    fl->p0 = (u8_c125e48 *)(base + o0);
    o1 = o0 + 0x300;
    fl->p1 = (u8_c125e48 *)(base + o1);
    fl->step = 16 << fieldpred;
    cmvy = dy / 2;
    code = (avg << 2) | (xh << 1) | yh;
    ch = h >> 1;
    x = (cmvx >> 1) + (bx >> 1);
    cyo = yofs >> 1;
    if (fieldpred) {
        y = ((cmvy >> 1) << 1) + cyo + ((by >> 1) + sfield);
    } else {
        y = (cmvy >> 1) + cyo + ((by >> 1) + sfield);
    }
    cmby = y >> 3;
    cmbx = x >> 3;
    xh = cmvx & 1;
    yh = cmvy & 1;
    fc->x = x - (cmbx << 3);
    fc->addr = mc + (((dfield + cyo) << 4) + 0x200);
    ly = y - (cmby << 3);
    if (yh) {
        if (ly + (ch << fieldpred) >= 8) {
            v = (8 >> fieldpred) - (ly >> fieldpred) - 1;
            fc->n0 = v;
            fc->n1 = ch - v;
        } else {
            fc->n0 = ch;
            fc->n1 = 0;
        }
    } else {
        if (ly + (ch << fieldpred) >= 9) {
            v = (8 >> fieldpred) - (ly >> fieldpred);
            fc->n0 = v;
            fc->n1 = ch - v;
        } else {
            fc->n0 = ch;
            fc->n1 = 0;
        }
    }
    fc->step = 8 << fieldpred;
    d->mb_buf[d->mb_buf_index].luma_func[n] = D_00132E30[code];
    data = ref->data;
    {
        int c0 = (ly << 3) + 0x100;
        int c1 = (ly << 3) + 0x400;
        u8_c125e48 *a0 = (u8_c125e48 *)((((cmbx - mbx) << 1) + (cmby - mby)) * 0x180 + base + c0);
        u8_c125e48 *a1 = (u8_c125e48 *)((((cmbx - mbx) << 1) + (cmby - mby)) * 0x180 + base + c1);
        fc->p1 = a1;
        fc->p0 = a0;
    }
    d->mb_buf[d->mb_buf_index].luma_ref[n] = data + mb * 0x180;
    d->mb_buf[d->mb_buf_index].chroma_ref[n] = data + (mb + ref->unk10) * 0x180;
    d->mb_buf[d->mb_buf_index].chroma_func[n] = D_00132E50[(avg << 2) | (xh << 1) | yh];
    d->mb_buf[d->mb_buf_index].fetch_count++;
}
