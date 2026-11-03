#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "game/render/draw.h"
#include "game/render/vuchain.h"
#include "asm_functions/executable.h"
#include "data/executable.h"
#include "game/system/stub.h"

/*
 * tiefunc.cpp in the original source; text 0x236958-0x236F00.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern int g_render_packet_cursor MACRO_ADDR;
extern int tie_tex_dma_tag_slot MACRO_ADDR;
extern int max_tie_texture_dma_size MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern char str_tie_texture_overflow[];

/* DmaTieTextures: the same splice for ties, largest size in max_tie_texture_dma_size. */
void DmaTieTextures(void) {
    int *p = (int *)g_render_packet_cursor;
    int size;

    g_render_packet_cursor += 0x10;
    ((int *)tie_tex_dma_tag_slot)[0] = 0x20000000;
    ((int *)tie_tex_dma_tag_slot)[1] = g_render_packet_cursor;
    ((int *)tie_tex_dma_tag_slot)[2] = 0;
    ((int *)tie_tex_dma_tag_slot)[3] = 0;
    if (draw_config[6] != 0 && draw_config[5] != 0) {
        size = BuildTieTextureDma(g_gs_texture_allocation_cursor);
        VU1_texFlush();
        if (size > 0x400000) {
            STUB_printf(str_tie_texture_overflow);
        }
        if (max_tie_texture_dma_size < size) {
            max_tie_texture_dma_size = size;
        }
    }
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = tie_tex_dma_tag_slot + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
}

extern ObjectRenderClass *object_render_classes[];
extern char tie_gif_patch_list[];
extern char tie_tex_remap_table[];

/* PatchTieGifs: patch_shrub_gifs' shape (patch_shrub_gifs). For each tie index
   in the -1-terminated list at tie_gif_patch_list, walk the tie's 0x50-byte
   records and patch the nonzero halves of the remap entry picked by byte
   0x33 into the low 14 bits of the words at +0x00 and +0x20. */
void PatchTieGifs(void) {
    int *p;
    for (p = (int *)tie_gif_patch_list; *p >= 0; p++) {
        ObjectRenderClass *tie = object_render_classes[*p];
        char *r = (char *)tie->packets_offset;
        int i;
        for (i = 0; i < tie->packet_count; i++) {
            short *ent = (short *)(tie_tex_remap_table + *(unsigned char *)(r + 0x33) * 4);
            if (ent[0] != 0)
                *(int *)r = (*(int *)r & 0xFFFFC000) | ent[0];
            if (ent[1] != 0)
                *(int *)(r + 0x20) = (*(int *)(r + 0x20) & 0xFFFFC000) | ent[1];
            r += 0x50;
        }
    }
}

extern void PatchTieGifs(void);
extern char tie_gif_patch_list_saved[];
extern char tie_tex_remap_table_saved[];

/* Patch the tie GIF tags, save the patch list and texture remap table, then patch again. */
void copy_render_buffer_pair(void) {
    PatchTieGifs();
    FastMemCopy(tie_gif_patch_list, tie_gif_patch_list_saved, 0x200);
    FastMemCopy(tie_tex_remap_table, tie_tex_remap_table_saved, 0x400);
    PatchTieGifs();
}

extern char tie_light_data[];
extern char tie_light_data2[];

/* Light the ties from both tie light data blocks. */
void register_entity_render_resources(void) {
    LightTies(tie_light_data);
    LightTies(tie_light_data2);
}

extern int g_gs_texture_allocation_start MACRO_ADDR;
extern int tie_proc_enabled;
extern char str_tie_texs[];
extern char str_tie_draw[];
extern char tie_packet_tail_tags[];

extern void DmaTieTextures(void); /* DmaTieTextures */

/* DrawTies_1: DrawShrubs' shape, the packet pointer advanced through a
   local. */
void DrawTies_1(void) {
    int p = g_render_packet_cursor;

    tie_tex_dma_tag_slot = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    p += 0x10;
    g_render_packet_cursor = p;
    ProfileMarker_Begin(str_tie_texs, 1);
    if (tie_proc_enabled != 0) {
        flush_cache(0);
        TieProc();
        write_dma_channel(tie_light_data, 0x3600, 0x40);
    }
    ProfileMarker_Begin(str_tie_draw, 5);
    DmaTieTextures();
    FastMemCopy((void *)g_render_packet_cursor, tie_packet_tail_tags, 0x20);
    ProfileMarker_End(str_tie_draw, 5);
}

extern int registered_object_render_class_count MACRO_ADDR;

/* draw_config read as a struct: a member access is MEM_IN_STRUCT_P, so
   sched2 knows it cannot alias the fixed-address scalar stores before it
   (a plain `int *` index is not, and the load then waits for them). */
typedef struct {
    int unk00[6];
    int unk18;
} DrawCfg;

/* DrawTies_2: DrawTies_1's two passes, the first with the odd ties' 0x8
   flag set (and the saved tie data swapped in around it), the second with
   the even ones'. Each flag loop reads the tie count into its own local
   (the loop's stores could alias it) and the flag is a `short` (so the
   `&= ~8` stays an int AND with -9, not andi 0xFFF7). */
void DrawTies_2(void) {
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 1; i < n; i += 2) {
            object_render_classes[i]->flags |= 8;
        }
    }
    {
        int p = g_render_packet_cursor;
        tie_tex_dma_tag_slot = p;
        g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
        p += 0x10;
        g_render_packet_cursor = p;
    }
    ProfileMarker_Begin(str_tie_texs, 1);
    {
        DrawCfg *d = (DrawCfg *)draw_config;
        if (d->unk18 != 0) {
            flush_cache(0);
            TieProc();
            write_dma_channel(tie_light_data2, 0x3600, 0x40);
        }
    }
    DmaTieTextures();
    FastMemCopy(tie_gif_patch_list_saved, tie_gif_patch_list, 0x200);
    FastMemCopy(tie_tex_remap_table_saved, tie_tex_remap_table, 0x400);
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 1; i < n; i += 2) {
            object_render_classes[i]->flags &= ~8;
        }
    }
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 0; i < n; i += 2) {
            object_render_classes[i]->flags |= 8;
        }
    }
    {
        DrawCfg *d = (DrawCfg *)draw_config;
        int p = g_render_packet_cursor;
        tie_tex_dma_tag_slot = p;
        g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
        p += 0x10;
        g_render_packet_cursor = p;
        if (d->unk18 != 0) {
            flush_cache(0);
            TieProc();
            write_dma_channel(tie_light_data, 0x3600, 0x40);
        }
    }
    ProfileMarker_Begin(str_tie_draw, 5);
    DmaTieTextures();
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 0; i < n; i += 2) {
            object_render_classes[i]->flags &= ~8;
        }
    }
    FastMemCopy((void *)g_render_packet_cursor, tie_packet_tail_tags, 0x20);
    ProfileMarker_End(str_tie_draw, 5);
}
