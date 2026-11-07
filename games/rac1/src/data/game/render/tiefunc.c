/* Data owned by src/game/render/tiefunc.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 str_tie_texs[] DATA_SECTION(D_00161030) = {
#include "data_inc/D_00161030.inc"
};

u32 str_tie_draw[] DATA_SECTION(D_00161040) = {
#include "data_inc/D_00161040.inc"
};

int tie_tex_dma_tag_slot[2] DATA_SECTION(D_00161068) = {0};

int max_tie_texture_dma_size DATA_SECTION(D_00161074) = {0};

int tie_proc_enabled[2] DATA_SECTION(D_0018A3C8) = {0};

u32 tie_packet_tail_tags[] DATA_SECTION(D_001DF3B0) = {
#include "data_inc/D_001DF3B0.inc"
};

u16 tie_tex_remap_table[512] DATA_SECTION(D_001E2D00) = {0};

u32 tie_gif_patch_list[128] DATA_SECTION(D_001E3300) = {0};

u32 tie_tex_remap_table_saved[256] DATA_SECTION(D_001E4100) = {0};

u32 tie_gif_patch_list_saved[128] DATA_SECTION(D_001E4500) = {0};

u32 tie_light_data2[] DATA_SECTION(D_001E4700) = {
#include "data_inc/D_001E4700.inc"
};

u32 D_001E8D80[] DATA_SECTION(D_001E8D80) = {
#include "data_inc/D_001E8D80.inc"
};
