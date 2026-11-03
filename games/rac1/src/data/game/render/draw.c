/* Data owned by src/game/render/draw.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_render_draw.h"
#include "structs/vector.h"

u8 first_clear_packet[] DATA_SECTION(D_0013CD90) = {
#include "data_inc/D_0013CD90.inc"
};

u8 reset_gs_regs_fixed[] DATA_SECTION(D_0013D0C0) = {
#include "data_inc/D_0013D0C0.inc"
};

u8 depth_buffer_register[] DATA_SECTION(D_0013D200) = {
#include "data_inc/D_0013D200.inc"
};

u8 masked_depth_buffer_register[] DATA_SECTION(D_0013D270) = {
#include "data_inc/D_0013D270.inc"
};

u32 gs_display_settings[94] DATA_SECTION(D_00151888) = {0};

u8 subtitles_enabled[8] DATA_SECTION(D_0015EF40) = {0};

#include "data_inc/D_0015F100.refs"
u32 font_text_cursor DATA_SECTION(D_0015F100) =
#include "data_inc/D_0015F100.inc"
    ;

u32 font_text_slot_index DATA_SECTION(D_0015F104) = 0;

u32 font_text_format[] DATA_SECTION(D_0015F108) = {
#include "data_inc/D_0015F108.inc"
};

int screen_tint_quad_enabled[4] DATA_SECTION(D_0015F430) = {0};

short font_depth_bias[2] DATA_SECTION(D_0015F448) = {0};

u32 screen_overlay_enabled DATA_SECTION(D_0015F44C) = 0;

#include "data_inc/D_0015F450.refs"
u32 screen_color_effect_now[] DATA_SECTION(D_0015F450) = {
#include "data_inc/D_0015F450.inc"
};

short draw_unused_reset_flag[8] DATA_SECTION(D_0015F460) = {0};

short occl_debug_overlay_enabled[8] DATA_SECTION(D_0015F470) = {0};

char profile_mark_frame_start[] DATA_SECTION(D_0015F480) =
#include "data_inc/D_0015F480.inc"
    ;

u32 profile_mark_after_sky[] DATA_SECTION(D_0015F490) = {
#include "data_inc/D_0015F490.inc"
};

u32 profile_mark_after_ties[] DATA_SECTION(D_0015F4A0) = {
#include "data_inc/D_0015F4A0.inc"
};

u32 profile_mark_after_mobys[] DATA_SECTION(D_0015F4B0) = {
#include "data_inc/D_0015F4B0.inc"
};

char profile_mark_after_lights[] DATA_SECTION(D_0015F4C0) =
#include "data_inc/D_0015F4C0.inc"
    ;

u32 profile_mark_after_particles[] DATA_SECTION(D_0015F4D0) = {
#include "data_inc/D_0015F4D0.inc"
};

char profile_mark_after_sprites[] DATA_SECTION(D_0015F4E0) =
#include "data_inc/D_0015F4E0.inc"
    ;

char profile_mark_after_blur[] DATA_SECTION(D_0015F4F0) =
#include "data_inc/D_0015F4F0.inc"
    ;

u32 profile_mark_after_hud[] DATA_SECTION(D_0015F4F8) = {
#include "data_inc/D_0015F4F8.inc"
};

char profile_mark_after_fades[] DATA_SECTION(D_0015F500) =
#include "data_inc/D_0015F500.inc"
    ;

u32 profile_mark_vu1_sync[2] DATA_SECTION(D_0015F510) = {0};

char profile_mark_tie_lighting[] DATA_SECTION(D_0015F518) =
#include "data_inc/D_0015F518.inc"
    ;

char profile_mark_moby_patch[] DATA_SECTION(D_0015F528) =
#include "data_inc/D_0015F528.inc"
    ;

int letterbox_bar_height[2] DATA_SECTION(D_0015F548) = {0};

u32 gif_paging_head_tag DATA_SECTION(D_0015F550) = 0;

u32 gif_paging_tail_tag DATA_SECTION(D_0015F554) = 0;

int draw_callback2_count DATA_SECTION(D_0015F568) = {0};

int draw_callback3_count DATA_SECTION(D_0015F56C) = {0};

int draw_callback4_count DATA_SECTION(D_0015F570) = {0};

int effect_quad_count DATA_SECTION(D_0015F574) = {0};

int level_fog_mode DATA_SECTION(D_0015F598) = {0};

float draw_frame_time_ratio[6] DATA_SECTION(D_0015F6F8) = {0};

u32 occl_grid_data DATA_SECTION(D_0015F720) = 0;

u32 occl_oct DATA_SECTION(D_0015F724) = 0;

int occl_invalid_grid DATA_SECTION(D_0015F72C) = {0};

u32 occl_previous_grid[12] DATA_SECTION(D_0015F730) = {0};

u32 rect_overlay_gif_regs_a[] DATA_SECTION(D_00160920) = {
#include "data_inc/D_00160920.inc"
};

u32 rect_overlay_gif_regs_b[] DATA_SECTION(D_00160930) = {
#include "data_inc/D_00160930.inc"
};

u32 textured_quad_header[] DATA_SECTION(D_00160940) = {
#include "data_inc/D_00160940.inc"
};

u32 textured_rect_gif_regs[] DATA_SECTION(D_00160960) = {
#include "data_inc/D_00160960.inc"
};

u32 underwater_screen_tint_r DATA_SECTION(D_001611C0) =
#include "data_inc/D_001611C0.inc"
    ;

u32 water_fog_color_red DATA_SECTION(D_001611C4) =
#include "data_inc/D_001611C4.inc"
    ;

float water_fog_near_dist DATA_SECTION(D_001611C8) = {0};

float water_fog_far_dist DATA_SECTION(D_001611CC) = {
#include "data_inc/D_001611CC.inc"
};

u32 water_fog_near_intensity[] DATA_SECTION(D_001611D0) = {
#include "data_inc/D_001611D0.inc"
};

int occl_cell_table DATA_SECTION(D_00161294) = {0};

int font_char_widths[] DATA_SECTION(D_00189EC0) = {
#include "data_inc/D_00189EC0.inc"
};

u32 D_0018A400[512] DATA_SECTION(D_0018A400) = {0};

u32 font_text_slots[] DATA_SECTION(D_0018AC00) = {
#include "data_inc/D_0018AC00.inc"
};

u32 D_0018CB40[] DATA_SECTION(D_0018CB40) = {
#include "data_inc/D_0018CB40.inc"
};

u32 effect_quad_corners[] DATA_SECTION(D_0018CBA0) = {
#include "data_inc/D_0018CBA0.inc"
};

s32 font_palette_colors[] DATA_SECTION(D_0018CBF8) = {
#include "data_inc/D_0018CBF8.inc"
};

u32 view_context_field_c0[48] DATA_SECTION(D_0018CEC0) = {0};

float screen_projection_focal[4] DATA_SECTION(D_0018D010) = {0};

u32 sky_view_mtx[4] DATA_SECTION(D_0018D080) = {0};

u32 D_0018D090[4] DATA_SECTION(D_0018D090) = {0};

u32 D_0018D0A0[4] DATA_SECTION(D_0018D0A0) = {0};

u32 D_0018D0B0[3] DATA_SECTION(D_0018D0B0) = {0};

u32 D_0018D0BC DATA_SECTION(D_0018D0BC) = 0;

Vec4f view_clip_constants[8] DATA_SECTION(D_0018D0C0) = {0};

PageSlot effect_texture_definitions[112] DATA_SECTION(D_0018D540) = {0};

u32 draw_callback_funcs[64] DATA_SECTION(D_0018DC40) = {0};

u32 draw_callback_args[64] DATA_SECTION(D_0018DD40) = {0};

u32 draw_callback2_funcs[64] DATA_SECTION(D_0018DE40) = {0};

u32 draw_callback2_args[64] DATA_SECTION(D_0018DF40) = {0};

u32 draw_callback3_funcs[64] DATA_SECTION(D_0018E040) = {0};

u32 draw_callback3_args[64] DATA_SECTION(D_0018E140) = {0};

u32 draw_callback4_funcs[64] DATA_SECTION(D_0018E240) = {0};

u32 draw_callback4_args[64] DATA_SECTION(D_0018E340) = {0};

u32 effect_quad_entries[4] DATA_SECTION(D_0018E440) = {0};

u32 D_0018E450[252] DATA_SECTION(D_0018E450) = {0};

int bit_swap_lut[] DATA_SECTION(D_0018E840) = {
#include "data_inc/D_0018E840.inc"
};

u32 occl_visibility_merged[16] DATA_SECTION(D_00194140) = {0};

u32 D_00194180[16] DATA_SECTION(D_00194180) = {0};

u32 camera_sample_points[8] DATA_SECTION(D_001941E0) = {0};

Vec4f vu1_view_screen_consts[3] DATA_SECTION(D_001DEA90) = {0};

u32 tfrag_vu_const_block[] DATA_SECTION(D_001DEAC0) = {
#include "data_inc/D_001DEAC0.inc"
};

u32 D_001DEB20[] DATA_SECTION(D_001DEB20) = {
#include "data_inc/D_001DEB20.inc"
};

u32 shrub_vu1_constants[] DATA_SECTION(D_001DED30) = {
#include "data_inc/D_001DED30.inc"
};

u32 shrub_vu1_constants_alt[] DATA_SECTION(D_001DED80) = {
#include "data_inc/D_001DED80.inc"
};

u32 shrub_vu1_constants_alt_screen[] DATA_SECTION(D_001DEDC0) = {
#include "data_inc/D_001DEDC0.inc"
};

float view_fog_multiplier_copy[] DATA_SECTION(D_001E0250) = {
#include "data_inc/D_001E0250.inc"
};

u32 view_fog_multiplier_copy2[] DATA_SECTION(D_001E6E70) = {
#include "data_inc/D_001E6E70.inc"
};

Tbl1088 render_debug_menu_row_counts DATA_SECTION(D_001E7AC0) = {
#include "data_inc/D_001E7AC0.inc"
};

char camera_sample_text_name[] DATA_SECTION(D_001E7AF8) =
#include "data_inc/D_001E7AF8.inc"
    ;

u32 profile_mark_tfrag_lighting[] DATA_SECTION(D_001E7BA0) = {
#include "data_inc/D_001E7BA0.inc"
};

u32 profile_mark_shrub_lighting[] DATA_SECTION(D_001E7BB8) = {
#include "data_inc/D_001E7BB8.inc"
};
