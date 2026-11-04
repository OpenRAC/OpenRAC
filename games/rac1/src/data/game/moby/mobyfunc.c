/* Data owned by src/game/moby/mobyfunc.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 graphics_setup_profile_label[] DATA_SECTION(D_0015FFC0) = {
#include "data_inc/D_0015FFC0.inc"
};

u32 draw_mobys_profile_label[] DATA_SECTION(D_0015FFD0) = {
#include "data_inc/D_0015FFD0.inc"
};

u32 moby_texture_dma_profile_label[] DATA_SECTION(D_0015FFE0) = {
#include "data_inc/D_0015FFE0.inc"
};

char draw_mobys_cleanup_profile_label[] DATA_SECTION(D_0015FFF0) =
#include "data_inc/D_0015FFF0.inc"
    ;

int num_spawnable_mobys DATA_SECTION(D_0015FFFC) = {0};

int moby_anim_head DATA_SECTION(D_00160014) = {0};

u32 normal_lut[] DATA_SECTION(D_00165600) = {
#include "data_inc/D_00165600.inc"
};

int moby_draw_enabled[4] DATA_SECTION(D_0018A3D8) = {0};

int moby_render_slots[16] DATA_SECTION(D_001B2F40) = {0};

u32 moby_backup_buffer[224] DATA_SECTION(D_001B3200) = {0};

u16 moby_gs_loc_stash[64] DATA_SECTION(D_001B6100) = {0};

u16 D_001B6180[448] DATA_SECTION(D_001B6180) = {0};

int moby_class_rectify[224] DATA_SECTION(D_001B6880) = {0};

u32 graphics_setup_command_stream[2304] DATA_SECTION(D_001C8A00) = {0};

u32 s_tried_to_create_moby[] DATA_SECTION(D_001E86F0) = {
#include "data_inc/D_001E86F0.inc"
};

u32 vu_chain_overflow_message[] DATA_SECTION(D_001E8730) = {
#include "data_inc/D_001E8730.inc"
};
