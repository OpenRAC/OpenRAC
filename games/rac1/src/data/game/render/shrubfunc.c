/* Data owned by src/game/render/shrubfunc.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 str_shrub_texs[] DATA_SECTION(D_001604B0) = {
#include "data_inc/D_001604B0.inc"
};

char str_shrub_draw[] DATA_SECTION(D_001604C0) =
#include "data_inc/D_001604C0.inc"
    ;

int shrub_tex_dma_tag[2] DATA_SECTION(D_001604F0) = {0};

int max_shrub_texture_dma_size[2] DATA_SECTION(D_001604F8) = {0};

int shrub_draw_enabled[2] DATA_SECTION(D_0018A3D0) = {0};

u16 shrub_paging_table[512] DATA_SECTION(D_001D8C40) = {0};

int shrub_class_index_list[64] DATA_SECTION(D_001D9140) = {0};

u32 str_shrub_texture_overflow[] DATA_SECTION(D_001E8C00) = {
#include "data_inc/D_001E8C00.inc"
};
