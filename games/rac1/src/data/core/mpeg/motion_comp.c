/* Data owned by src/core/mpeg/motion_comp.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

extern void mpeg_mc_luma_copy();
extern void mpeg_mc_luma_half_y();
extern void mpeg_mc_luma_half_x();
extern void mpeg_mc_luma_half_xy();
extern void mpeg_mc_luma_avg();
extern void mpeg_mc_luma_avg_half_y();
extern void mpeg_mc_luma_avg_half_x();
extern void mpeg_mc_luma_avg_half_xy();
extern void mpeg_mc_chroma_copy();
extern void mpeg_mc_chroma_half_y();
extern void mpeg_mc_chroma_half_x();
extern void mpeg_mc_chroma_half_xy();
extern void mpeg_mc_chroma_avg();
extern void mpeg_mc_chroma_avg_half_y();
extern void mpeg_mc_chroma_avg_half_x();
extern void mpeg_mc_chroma_avg_half_xy();

void *mpeg_luma_mc_routines[] DATA_SECTION(D_00132F30) = {
    mpeg_mc_luma_copy, mpeg_mc_luma_half_y,     mpeg_mc_luma_half_x,     mpeg_mc_luma_half_xy,
    mpeg_mc_luma_avg,  mpeg_mc_luma_avg_half_y, mpeg_mc_luma_avg_half_x, mpeg_mc_luma_avg_half_xy,
};

void *mpeg_chroma_mc_routines[] DATA_SECTION(D_00132F50) = {
    mpeg_mc_chroma_copy, mpeg_mc_chroma_half_y,     mpeg_mc_chroma_half_x,     mpeg_mc_chroma_half_xy,
    mpeg_mc_chroma_avg,  mpeg_mc_chroma_avg_half_y, mpeg_mc_chroma_avg_half_x, mpeg_mc_chroma_avg_half_xy,
};

int IpuCommandTable[] DATA_SECTION(D_00132F70) = {
#include "data_inc/D_00132F70.inc"
};

unsigned int mpeg_display_field_count_table[] DATA_SECTION(D_00132F98) = {
#include "data_inc/D_00132F98.inc"
};

u32 str_mpeg_invalid_motion_type[] DATA_SECTION(D_001537E8) = {
#include "data_inc/D_001537E8.inc"
};

char str_mpeg_invalid_motion_type_a[] DATA_SECTION(D_00153810) =
#include "data_inc/D_00153810.inc"
    ;

char str_mpeg_invalid_motion_type_b[] DATA_SECTION(D_00153830) =
#include "data_inc/D_00153830.inc"
    ;

char str_mpeg_invalid_motion_type_c[] DATA_SECTION(D_00153850) =
#include "data_inc/D_00153850.inc"
    ;

u32 str_mpeg_bdec_error[] DATA_SECTION(D_00153888) = {
#include "data_inc/D_00153888.inc"
};

u32 str_mpeg_invalid_mb_addr_inc[] DATA_SECTION(D_001538A8) = {
#include "data_inc/D_001538A8.inc"
};

char str_mpeg_skip_to_next_picture[] DATA_SECTION(D_001538E0) =
#include "data_inc/D_001538E0.inc"
    ;

char str_mpeg_slice_start_code_range[] DATA_SECTION(D_00153900) =
#include "data_inc/D_00153900.inc"
    ;

u32 str_mpeg_slice_a0_error[] DATA_SECTION(D_00153928) = {
#include "data_inc/D_00153928.inc"
};

char str_mpeg_too_many_macroblocks[] DATA_SECTION(D_00153948) =
#include "data_inc/D_00153948.inc"
    ;

char str_mpeg_skipped_mb_in_i_picture[] DATA_SECTION(D_00153968) =
#include "data_inc/D_00153968.inc"
    ;

char str_mpeg_invalid_mb_type[] DATA_SECTION(D_00153998) =
#include "data_inc/D_00153998.inc"
    ;

char str_mpeg_load_chroma_intra_qm[] DATA_SECTION(D_001539E8) =
#include "data_inc/D_001539E8.inc"
    ;

u32 str_mpeg_load_chroma_non_intra_qm[] DATA_SECTION(D_00153A10) = {
#include "data_inc/D_00153A10.inc"
};

char str_mpeg_odd_field_pictures[] DATA_SECTION(D_00153A40) =
#include "data_inc/D_00153A40.inc"
    ;

u32 str_mpeg_unknown_picture_structure[] DATA_SECTION(D_00153A60) = {
#include "data_inc/D_00153A60.inc"
};

u32 str_mpeg_buffer_too_small_fmt[] DATA_SECTION(D_00153A80) = {
#include "data_inc/D_00153A80.inc"
};
