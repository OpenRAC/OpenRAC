#include "common.h"
#include "vector.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * drawquad.cpp in the original source; text 0x1F7C60-0x1F9810.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char voice_pool[];
extern int bank_sound_count;
extern void FastMemCopy(void *, void *, int);
extern char backup_cam[];
extern char gCamBackupState[];
extern void *cur_cam NOT_SDA;
extern DispatchRec camera_types[];
extern int draw_config[];
extern void FastMemSet();
extern void BuildOcclVisibility(void);
extern int occl_update NOT_SDA;
extern char occl_visibility[];
extern long gs_display_settings[3];
extern int space_load_in_progress;
extern short draw_enable_mask;
extern void append_gif_transfer_packet(void);
extern void DrawDebugProfiler(void);
extern int draw_callback_count;
extern int draw_callback_args[];
extern int draw_callback_funcs[];
extern short GlobalStateFlag;
extern int measure_text_width(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char normal_font_metrics[];
extern unsigned char small_font_metrics[];
extern unsigned char large_font_metrics[];
extern void FontPrint(void *, void *, void *, void *, void *, int,
                          unsigned char *);
extern int measure_text_width_regular(unsigned char *, int);
extern int measure_text_width_small(unsigned char *, int);
extern int GetEffectTex(int);
extern void PutDrawBufferLarge(void);
extern void InitViewContext(void);
extern void UpdateViewContext(void);
extern int bit_swap_lut[];

ASM_FUNC("asm/handwritten/text", func_001F7C60);

ASM_FUNC("asm/handwritten/text", func_001F7DD8);

/* Store in dst the point on segment a-b closest to c (VU0); return its clamped 0..1 position t. */
float closest_point_on_segment(float *dst, float *a, float *b, float *c) {
    float t;
    __asm__ __volatile__(
        "vaddw.xyz $vf24,$vf0,$vf0w\n\t"
        "lqc2 $vf1,0(%2)\n\t"
        "lqc2 $vf2,0(%3)\n\t"
        "lqc2 $vf3,0(%4)\n\t"
        "vsub.xyz $vf4,$vf2,$vf1\n\t"
        "vsub.xyz $vf5,$vf3,$vf1\n\t"
        "vmul.xyz $vf6,$vf4,$vf4\n\t"
        "vmul.xyz $vf7,$vf4,$vf5\n\t"
        "vadday.x ACC,$vf6,$vf6y\n\t"
        "vmaddz.x $vf6,$vf24,$vf6z\n\t"
        "vadday.x ACC,$vf7,$vf7y\n\t"
        "vmaddz.x $vf7,$vf24,$vf7z\n\t"
        "vdiv Q,$vf7x,$vf6x\n\t"
        "vwaitq\n\t"
        "vaddq.x $vf8,$vf0,Q\n\t"
        "vminiw.x $vf7,$vf8,$vf0w\n\t"
        "qmfc2.ni $8,$vf8\n\t"
        "nop\n\t"
        "vmaxx.x $vf7,$vf7,$vf0x\n\t"
        "mtc1 $8,%0\n\t"
        "vmulax.xyz ACC,$vf4,$vf7x\n\t"
        "vmaddw.xyz $vf1,$vf1,$vf0w\n\t"
        "sqc2 $vf1,0(%1)"
        : "=f"(t) : "r"(dst), "r"(a), "r"(b), "r"(c) : "$8", "memory");
    return t;
}

ASM_FUNC("asm/handwritten/text", func_001F7EF8);

ASM_FUNC("asm/handwritten/text", func_001F84AC);

ASM_FUNC("asm/handwritten/text", func_001F852C);

ASM_FUNC("asm/handwritten/text", func_001F856C);

ASM_FUNC("asm/handwritten/text", func_001F8B6C);

ASM_FUNC("asm/handwritten/text", func_001F91B8);

typedef struct {
    Vec4f position;
    s16 active_count;
    s16 alpha;
    u8 pad14[4];
    f32 angle;
    f32 radius_scale;
} BillboardRecord;
typedef struct {
    u8 pad0[0x1A8];
    f32 depth_offset;
    u8 pad1AC[0x64];
    f32 projection_scale;
} BillboardViewContext;
struct DmaTag {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct TagPtr {
    struct DmaTag *p;
};
extern struct TagPtr g_render_packet_cursor MACRO_ADDR;
extern char billboard_quad_header[];
extern u8 camera_position[];
extern BillboardViewContext view_context;
extern BillboardRecord billboard_records[];
extern void FastMemSet(void *, s32, s32);
extern s64 GetEffectTex_r(s32) __asm__("func_001F4868");
extern s32 is_vector_outside_clip(Vec4f *);
extern void FastVecSub(Vec4f *, void *, void *);
extern void FastVecScale(Vec4f *, Vec4f *, f32);
extern void multiply_vector_components(Vec4f *, Vec4f *, void *);
extern f32 FastVecLength(Vec4f *);
extern void transform_vector(Vec4f *, Vec4f *, void *);
extern f32 FastCos(f32);
extern f32 FastSin(f32);
extern f32 IntToFloat(s32);
extern s32 truncate_float_to_s32(f32);
extern void VU1_addGSregister(s32, u64);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/append_billboard_batch.c, append_billboard_batch. */
void append_billboard_batch(void) {
    Vec4f projected_position;
    Vec4f clip_position;
    f32 distance;
    f32 radius;
    s32 record_idx;
    BillboardRecord *record;
    s64 color;
    s64 x;
    s32 y;
    s64 packed_position;
    s32 sine_offset;
    s32 cosine_offset;
    struct DmaTag *tag;
    s64 *packet_words;
    BillboardRecord *records;

    VU1_addGSregister(0x42, 0x8000000048);
    for (record_idx = 0; record_idx < 16; record_idx++) {
        records = billboard_records;
        record = records + record_idx;
        if (record->active_count <= 0) {
            continue;
        }
        FastVecSub(&projected_position, record, camera_position);
        projected_position.w = 1.0f;
        distance = FastVecLength(&projected_position);
        FastVecScale(&projected_position, &projected_position, 1024.0f);
        transform_vector(&projected_position, &projected_position, camera_position - 0x100);
        multiply_vector_components(&clip_position, &projected_position,
                                   (char *)&view_context + 0x180);
        if (is_vector_outside_clip(&clip_position) != 0) {
            FastMemSet(record, 0, 0x20);
            continue;
        }
        FastVecScale(&projected_position, &projected_position,
                       view_context.projection_scale / projected_position.w);
        color = (record->alpha << 24) | 0x808080;
        x = truncate_float_to_s32(projected_position.x * 16.0f) + 0x8000;
        y = truncate_float_to_s32(projected_position.y * 16.0f) + 0x8000;
        packed_position = ((s64)truncate_float_to_s32(projected_position.z * 0.9997f +
                                                         view_context.depth_offset)
                           << 32) |
                          ((s64)y << 16) | x;
        if (distance > 18.0f) {
            distance = 18.0f;
        } else if (distance < 2.0f) {
            distance = 2.0f;
        }
        radius = record->radius_scale * (IntToFloat(record->alpha + 16) * 0.015625f) *
                 ((24.0f - distance) * 16.0f);
        sine_offset = truncate_float_to_s32(radius * FastSin(record->angle));
        cosine_offset = truncate_float_to_s32(radius * FastCos(record->angle));
        g_render_packet_cursor.p->tag = 0x10000009;
        g_render_packet_cursor.p->addr = 0;
        g_render_packet_cursor.p->vif0 = 0;
        g_render_packet_cursor.p->vif1 = 0x50000009;
        tag = g_render_packet_cursor.p;
        g_render_packet_cursor.p = tag + 1;
        qcopy(tag + 1, billboard_quad_header);
        packet_words = (s64 *)(tag + 2);
        g_render_packet_cursor.p = tag + 2;
        packet_words[0] = 5;
        packet_words[1] = GetEffectTex_r(0x13);
        packet_words[2] = 0x154;
        packet_words[3] = color;
        packet_words[4] = 0;
        packet_words[5] = packed_position + (cosine_offset << 16) + sine_offset;
        packet_words[6] = color;
        packet_words[7] = 0x200;
        packet_words[8] = packed_position + (-sine_offset << 16) + cosine_offset;
        packet_words[9] = color;
        packet_words[10] = 0x2000000;
        packet_words[11] = packed_position + (sine_offset << 16) - cosine_offset;
        packet_words[12] = color;
        packet_words[13] = 0x2000200;
        packet_words[14] = packed_position + (-cosine_offset << 16) - sine_offset;
        packet_words[15] = 0;
        g_render_packet_cursor.p = (struct DmaTag *)((u8 *)g_render_packet_cursor.p + 0x80);
    }
    VU1_addGSregister(0x42, 0x8000000044);
}
