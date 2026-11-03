#include "common.h"
#include "structs/data/game_render_tfragfunc.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "game/render/draw.h"
#include "game/render/vuchain.h"
#include "data/executable.h"
#include "asm_functions/executable.h"
#include "game/system/stub.h"

/*
 * tfragfunc.cpp in the original source; text 0x234380-0x2347F0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern char camera_position[];

/*
 * Retail aligns SetTfragDists to 16 bytes, and stash_get_reserved_qwords's .s file
 * carried two trailing nops after its endlabel to supply that padding.
 * Those nops sit OUTSIDE the declared function size (0x28), so they were
 * silently lost when this function became C, shifting all 179 later
 * text functions by -8 and giving them spurious 1-byte jal-target diffs.
 * The .s files only ever .align 3 (8 bytes), so the 16-byte alignment
 * has to be restored explicitly here.
 */
__asm__(".align 4");

extern int tfrag_lod_dists_raw[3];
extern float fog_params;
extern float tfrag_lod_blend_matrix[4][4];
extern int func_001FA898(float);

/* SetTfragDists(void): the three LOD distances as fixed point (x1024), and
   the matrix mapping distance to the two blend weights. The store order is
   load-bearing: filling the rows first and the two translation terms last
   ([1][3] = b, then [0][3] = a) moves sched1's last use of b, and with it
   the local-alloc priority of b's quantity below ab's -- retail's b in $f21,
   1/(a-b) in $f20, and -0.5 in $f3. */
void SetTfragDists(void) {
    float a, b, c, ab, bc;

    tfrag_lod_dists_raw[0] = truncate_float_to_s32(tfrag_render_ranges[0] * 1024.0f);
    tfrag_lod_dists_raw[1] = truncate_float_to_s32(tfrag_render_ranges[1] * 1024.0f);
    tfrag_lod_dists_raw[2] = truncate_float_to_s32(tfrag_render_ranges[2] * 1024.0f);
    a = tfrag_render_ranges[0] * fog_params;
    b = tfrag_render_ranges[1] * fog_params;
    c = tfrag_render_ranges[2] * fog_params;
    ab = 1.0f / (a - b);
    bc = 1.0f / (b - c);
    FastMemZero16(tfrag_lod_blend_matrix, 0x40);
    tfrag_lod_blend_matrix[0][0] = ab * 0.5f;
    tfrag_lod_blend_matrix[0][1] = -ab;
    tfrag_lod_blend_matrix[1][0] = bc * 0.5f;
    tfrag_lod_blend_matrix[1][1] = -bc;
    tfrag_lod_blend_matrix[2][0] = b * ab * -0.5f;
    tfrag_lod_blend_matrix[2][1] = a * ab;
    tfrag_lod_blend_matrix[3][0] = c * bc * -0.5f;
    tfrag_lod_blend_matrix[3][1] = b * bc;
    tfrag_lod_blend_matrix[1][3] = b;
    tfrag_lod_blend_matrix[0][3] = a;
}

extern int g_render_packet_cursor MACRO_ADDR;
extern int tfrag_tex_dma_tag_slot MACRO_ADDR;
extern int max_tfrag_texture_dma_size MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern char str_tfrag_texture_overflow[];

/* DmaTfragTextures: the tfrag twin of DmaShrubTextures,
   which also warns "tfrag texture overflow" past 0x400000 bytes and keeps
   the largest size seen in max_tfrag_texture_dma_size. */
/* A copy of its twin DmaShrubTextures. */
void DmaTfragTextures(void) {
    int *p = (int *)g_render_packet_cursor;
    int size;

    g_render_packet_cursor += 0x10;
    ((int *)tfrag_tex_dma_tag_slot)[0] = 0x20000000;
    ((int *)tfrag_tex_dma_tag_slot)[1] = g_render_packet_cursor;
    ((int *)tfrag_tex_dma_tag_slot)[2] = 0;
    ((int *)tfrag_tex_dma_tag_slot)[3] = 0;
    if (draw_config[4] != 0 && draw_config[3] != 0) {
        ComputeTfragTextureUsage();
        size = build_tfrag_texture_dma(g_gs_texture_allocation_cursor);
        VU1_texFlush();
        if (size > 0x400000) {
            STUB_printf(str_tfrag_texture_overflow);
        }
        if (max_tfrag_texture_dma_size < size) {
            max_tfrag_texture_dma_size = size;
        }
    }
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = tfrag_tex_dma_tag_slot + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
}

typedef struct {
    short a;
    short b;
} TexRemap;

extern TexChunk tfrag_gif_chunk_list[];
extern TexRemap tfrag_tex_remap_table[];

/* Walk the null-terminated chunk list at tfrag_gif_chunk_list; for every 0x50-byte
   record in each chunk, look up the remap entry indexed by the byte at
   +0x23 and patch its two nonzero halves into the low 14 bits of the
   words at +0x00 and +0x30. */
/* PatchTfragGifs(void) */
void PatchTfragGifs(void) {
    TexChunk *c;
    char *r;
    TexRemap *e;
    int i;
    int n;

    c = tfrag_gif_chunk_list;
    while (c->items != 0) {
        n = c->count;
        r = c->items;
        for (i = 0; i < n; i++) {
            e = &tfrag_tex_remap_table[*(unsigned char *)(r + 0x23)];
            if (e->a != 0) {
                *(int *)r = (*(int *)r & 0xFFFFC000) | e->a;
            }
            if (e->b != 0) {
                *(int *)(r + 0x30) = (*(int *)(r + 0x30) & 0xFFFFC000) | e->b;
            }
            r += 0x50;
        }
        c++;
    }
}

extern int g_gs_texture_allocation_start MACRO_ADDR;
extern char str_tfrag_texs[];
extern char str_tfrag_draw[];
extern char tfrag_dma_block[];

extern void DmaTfragTextures(void);

/* DrawTfrag, in DrawShrubs' shape. */
void DrawTfrag(void) {
    float m[4][4];
    int p = g_render_packet_cursor;

    tfrag_tex_dma_tag_slot = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    p += 0x10;
    g_render_packet_cursor = p;
    ProfileMarker_Begin(str_tfrag_texs, 1);
    set_identity_matrix(m);
    FastVecScale(m[3], camera_position, -1024.0f);
    m[3][3] = 1.0f;
    sce_vu0_mul_matrix(m, camera_position - 0x100, m);
    write_vif_unpack_packet(5, m, 4);
    write_vif_unpack_packet(0x14D, m, 4);
    if (draw_config[4] != 0) {
        flush_cache(0);
        TfragProc();
    }
    ProfileMarker_Begin(str_tfrag_draw, 2);
    DmaTfragTextures();
    if (draw_config[4] != 0) {
        write_dma_channel(tfrag_dma_block, 0x3000, 0x40);
    }
    ProfileMarker_End(str_tfrag_draw, 2);
}

/* The last function in the object: retail pads to 16 bytes after it
   (two nops in its .s), and vuchain.o starts there. */
__asm__(".align 4");
