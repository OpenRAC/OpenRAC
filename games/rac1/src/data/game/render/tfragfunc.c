/* Data owned by src/game/render/tfragfunc.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_render_tfragfunc.h"

u32 str_tfrag_texs[] DATA_SECTION(D_00160F70) = {
#include "data_inc/D_00160F70.inc"
};

char str_tfrag_draw[] DATA_SECTION(D_00160F80) =
#include "data_inc/D_00160F80.inc"
    ;

float fog_params[24] DATA_SECTION(D_0018D020) = {0};

u32 tfrag_lod_blend_matrix[] DATA_SECTION(D_001DEB70) = {
#include "data_inc/D_001DEB70.inc"
};

u16 tfrag_tex_remap_table[384] DATA_SECTION(D_001E0F00) = {0};

TexChunk tfrag_gif_chunk_list[128] DATA_SECTION(D_001E1200) = {0};

char str_tfrag_texture_overflow[] DATA_SECTION(D_001E8CE0) =
#include "data_inc/D_001E8CE0.inc"
    ;
