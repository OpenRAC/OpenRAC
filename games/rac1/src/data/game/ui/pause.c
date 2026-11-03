/* Data owned by src/game/ui/pause.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_ui_pause.h"
#include "structs/menu.h"

extern u16 pause_weapon_menu_items[];
extern void update_menu_icon_list();
extern void draw_menu_icon_list();
extern u16 gadget_menu_items[];

u8 map_screen_disc_entries[152] DATA_SECTION(D_001384A0) = {0};

u8 map_screen_disc_entries_alt[3904] DATA_SECTION(D_00138538) = {0};

u8 g_hero_equipped_item_id[160] DATA_SECTION(D_00140508) = {0};

u8 last_weapon_id[8] DATA_SECTION(D_00141760) = {0};

u8 usage_stats_entry21_count[24] DATA_SECTION(D_001419F0) = {0};

u8 pause_iop_sprite_lists[8] DATA_SECTION(D_00141FA0) = {0};

u8 D_00141FA8[8] DATA_SECTION(D_00141FA8) = {0};

u8 gold_bolts_collected_18[8] DATA_SECTION(D_0014C008) = {0};

u32 hero_max_health[] DATA_SECTION(D_0015EEA0) = {
#include "data_inc/D_0015EEA0.inc"
};

u8 selector_available[16] DATA_SECTION(D_0015EEC0) = {0};

int vendor_stock[] DATA_SECTION(D_0015EED0) = {
#include "data_inc/D_0015EED0.inc"
};

int memcard_selected_slot DATA_SECTION(D_0015EF34) = {0};

int counter_advance_disabled[5] DATA_SECTION(D_0015F6D0) = {0};

short panel_clear_color[] DATA_SECTION(D_001602B0) = {
#include "data_inc/D_001602B0.inc"
};

u32 menu_fade_duration DATA_SECTION(D_001602B4) =
#include "data_inc/D_001602B4.inc"
    ;

short text_shadow_x[] DATA_SECTION(D_001602B8) = {
#include "data_inc/D_001602B8.inc"
};

u32 text_shadow_y DATA_SECTION(D_001602BC) =
#include "data_inc/D_001602BC.inc"
    ;

u32 menu_light_direction_init[] DATA_SECTION(D_001602C0) = {
#include "data_inc/D_001602C0.inc"
};

u32 menu_light_color_init[] DATA_SECTION(D_001602D0) = {
#include "data_inc/D_001602D0.inc"
};

short menu_label_text_inset[] DATA_SECTION(D_00160358) = {
#include "data_inc/D_00160358.inc"
};

short menu_label_marquee_gap[] DATA_SECTION(D_00160368) = {
#include "data_inc/D_00160368.inc"
};

char fmt_two_strings[] DATA_SECTION(D_00160380) =
#include "data_inc/D_00160380.inc"
    ;

char str_null_text[] DATA_SECTION(D_00160388) =
#include "data_inc/D_00160388.inc"
    ;

short menu_grid_icon_width[] DATA_SECTION(D_00160390) = {
#include "data_inc/D_00160390.inc"
};

u32 fmt_decimal DATA_SECTION(D_001603A0) =
#include "data_inc/D_001603A0.inc"
    ;

u32 gold_bolt_marker_y_offset DATA_SECTION(D_001603A4) =
#include "data_inc/D_001603A4.inc"
    ;

u32 fmt_gold_bolt_count_line[] DATA_SECTION(D_001603A8) = {
#include "data_inc/D_001603A8.inc"
};

u32 fmt_ammo_have[] DATA_SECTION(D_001603B8) = {
#include "data_inc/D_001603B8.inc"
};

u32 fmt_ammo_have_thousands[] DATA_SECTION(D_001603C0) = {
#include "data_inc/D_001603C0.inc"
};

u32 fmt_number_thousands[] DATA_SECTION(D_001603D0) = {
#include "data_inc/D_001603D0.inc"
};

u32 pause_ring_left_glyph[] DATA_SECTION(D_001603D8) = {
#include "data_inc/D_001603D8.inc"
};

u32 pause_ring_right_glyph[] DATA_SECTION(D_001603E0) = {
#include "data_inc/D_001603E0.inc"
};

u8 menu_side_indicator_left_glyph[] DATA_SECTION(D_001603F0) = {
#include "data_inc/D_001603F0.inc"
};

u8 menu_side_indicator_right_glyph[] DATA_SECTION(D_001603F8) = {
#include "data_inc/D_001603F8.inc"
};

u32 fmt_save_play_time[] DATA_SECTION(D_00160400) = {
#include "data_inc/D_00160400.inc"
};

char fmt_save_bolt_count[] DATA_SECTION(D_00160410) =
#include "data_inc/D_00160410.inc"
    ;

u32 fmt_save_date[] DATA_SECTION(D_00160420) = {
#include "data_inc/D_00160420.inc"
};

short setup_strip_pass_words[] DATA_SECTION(D_00160460) = {
#include "data_inc/D_00160460.inc"
};

float direction_unit_y DATA_SECTION(D_00160474) = {0};

float direction_unit_z[2] DATA_SECTION(D_00160478) = {0};

float direction_perpendicular_y DATA_SECTION(D_00160484) = {0};

float direction_perpendicular_z[2] DATA_SECTION(D_00160488) = {0};

float direction_scaled_y DATA_SECTION(D_00160494) = {0};

float direction_scaled_z[2] DATA_SECTION(D_00160498) = {0};

u32 item_info_table[2] DATA_SECTION(D_001864D0) = {0};

u32 item_info_table_slot[2] DATA_SECTION(D_001864D8) = {0};

u32 D_001864E0[] DATA_SECTION(D_001864E0) = {
#include "data_inc/D_001864E0.inc"
};

s32 active_camera_basis_column1_x[4] DATA_SECTION(D_001873A0) = {0};

s32 active_camera_basis_column2_x[20] DATA_SECTION(D_001873B0) = {0};

u32 menu_light_direction[4] DATA_SECTION(D_0019C250) = {0};

u32 D_0019C260[4] DATA_SECTION(D_0019C260) = {0};

u32 D_0019C270[20] DATA_SECTION(D_0019C270) = {0};

int selected_level_index DATA_SECTION(D_001A0414) = {0};

int pause_memory_card_state[12] DATA_SECTION(D_001A0418) = {0};

u32 transition_overlay_tex0[8] DATA_SECTION(D_001A0448) = {0};

s32 panel_slot_enabled[] DATA_SECTION(D_001CE640) = {
#include "data_inc/D_001CE640.inc"
};

#include "data_inc/D_001CE938.refs"
u32 pause_menu_screen_a[] DATA_SECTION(D_001CE938) = {
#include "data_inc/D_001CE938.inc"
};

#include "data_inc/D_001CEAC8.refs"
u32 pause_menu_screen_b[] DATA_SECTION(D_001CEAC8) = {
#include "data_inc/D_001CEAC8.inc"
};

#include "data_inc/D_001CEB18.refs"
u32 pause_menu_screen_optional[] DATA_SECTION(D_001CEB18) = {
#include "data_inc/D_001CEB18.inc"
};

#include "data_inc/D_001CEFA0.refs"
u32 gadget_menu_slots_row0[] DATA_SECTION(D_001CEFA0) = {
#include "data_inc/D_001CEFA0.inc"
};

Item0A gadget_menu_slots_row1[] DATA_SECTION(D_001CEFE0) = {
#include "data_inc/D_001CEFE0.inc"
};

Item0A gadget_menu_slots_row2[] DATA_SECTION(D_001CF000) = {
#include "data_inc/D_001CF000.inc"
};

#include "data_inc/D_001CF020.refs"
u32 gadget_menu_slots_row3[] DATA_SECTION(D_001CF020) = {
#include "data_inc/D_001CF020.inc"
};

#include "data_inc/D_001CF4A0.refs"
u32 pause_weapon_menu_slots[] DATA_SECTION(D_001CF4A0) = {
#include "data_inc/D_001CF4A0.inc"
};

#include "data_inc/D_001CF798.refs"
u32 map_menu_page[] DATA_SECTION(D_001CF798) = {
#include "data_inc/D_001CF798.inc"
};

#include "data_inc/D_001CF9F8.refs"
u32 mission_menu_page[] DATA_SECTION(D_001CF9F8) = {
#include "data_inc/D_001CF9F8.inc"
};

int selected_mission_index[4] DATA_SECTION(D_001CFAD8) = {0};

u16 active_message_menu_items[] DATA_SECTION(D_001CFFC0) = {
#include "data_inc/D_001CFFC0.inc"
};

int pause_entry_cleared_word[4] DATA_SECTION(D_001D0718) = {0};

void *pause_weapon_menu_grid[] DATA_SECTION(D_001D1080) = {
    update_menu_icon_list,
    draw_menu_icon_list,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    pause_weapon_menu_items,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

void *gadget_menu_grid[] DATA_SECTION(D_001D1408) = {
    update_menu_icon_list,
    draw_menu_icon_list,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    gadget_menu_items,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

#include "data_inc/D_001D28F8.refs"
u32 save_overwrite_confirm_page[] DATA_SECTION(D_001D28F8) = {
#include "data_inc/D_001D28F8.inc"
};

int save_confirm_slot[4] DATA_SECTION(D_001D29C0) = {0};

u16 reward_menu_items[] DATA_SECTION(D_001D2B80) = {
#include "data_inc/D_001D2B80.inc"
};

u32 reward_menu_page[] DATA_SECTION(D_001D2BF8) = {
#include "data_inc/D_001D2BF8.inc"
};

Entry14 cheat_toggle_menu_items[13] DATA_SECTION(D_001D3E90) = {0};

#include "data_inc/D_001D48A8.refs"
int pause_level_sprite_lists[] DATA_SECTION(D_001D48A8) = {
#include "data_inc/D_001D48A8.inc"
};

#include "data_inc/D_001D4948.refs"
struct MenuPage pause_menu_start_page DATA_SECTION(D_001D4948) = {
#include "data_inc/D_001D4948.inc"
};

#include "data_inc/D_001D4B90.refs"
u32 pause_action_list_pal[] DATA_SECTION(D_001D4B90) = {
#include "data_inc/D_001D4B90.inc"
};

#include "data_inc/D_001D4BC0.refs"
u32 pause_action_list_short[] DATA_SECTION(D_001D4BC0) = {
#include "data_inc/D_001D4BC0.inc"
};

#include "data_inc/D_001D51B8.refs"
u32 save_menu_overwrite_confirm_page[] DATA_SECTION(D_001D51B8) = {
#include "data_inc/D_001D51B8.inc"
};

#include "data_inc/D_001D5240.refs"
u32 restart_confirm_page[] DATA_SECTION(D_001D5240) = {
#include "data_inc/D_001D5240.inc"
};

#include "data_inc/D_001D5318.refs"
u32 save_menu_load_confirm_page[] DATA_SECTION(D_001D5318) = {
#include "data_inc/D_001D5318.inc"
};

int menu_preview_slot_enabled[] DATA_SECTION(D_001D53A0) = {
#include "data_inc/D_001D53A0.inc"
};

u32 timed_sequence_page1[] DATA_SECTION(D_001D5418) = {
#include "data_inc/D_001D5418.inc"
};

u32 timed_sequence_page2[] DATA_SECTION(D_001D54C8) = {
#include "data_inc/D_001D54C8.inc"
};

u32 timed_sequence_page3[] DATA_SECTION(D_001D5588) = {
#include "data_inc/D_001D5588.inc"
};

u32 timed_sequence_page4[] DATA_SECTION(D_001D5618) = {
#include "data_inc/D_001D5618.inc"
};

u32 timed_sequence_page5[] DATA_SECTION(D_001D5630) = {
#include "data_inc/D_001D5630.inc"
};

HandGadgetAnimation gadget_animations[] DATA_SECTION(D_001D5668) = {
#include "data_inc/D_001D5668.inc"
};

#include "data_inc/D_001D5D58.refs"
u32 preview_resource_bindings[] DATA_SECTION(D_001D5D58) = {
#include "data_inc/D_001D5D58.inc"
};

s32 preserved_item_available_indices[] DATA_SECTION(D_001D5F20) = {
#include "data_inc/D_001D5F20.inc"
};

u32 menu_system DATA_SECTION(D_001D5F70) = 0;

u32 current_menu_page DATA_SECTION(D_001D5F74) = 0;

u32 D_001D6020[22] DATA_SECTION(D_001D6020) = {0};

int menu_system_help_text_buffer[18] DATA_SECTION(D_001D6078) = {0};

u32 preview_resource_ids[22] DATA_SECTION(D_001D60C8) = {0};

int panel_slots[16] DATA_SECTION(D_001D6120) = {0};

u8 class_pose_manipulator[64] DATA_SECTION(D_001D6160) = {0};

u8 first_attachment_manipulator[32] DATA_SECTION(D_001D61A0) = {0};

u8 D_001D61C0[8] DATA_SECTION(D_001D61C0) = {0};

u8 D_001D61C8[24] DATA_SECTION(D_001D61C8) = {0};

u8 second_attachment_manipulator[32] DATA_SECTION(D_001D61E0) = {0};

u8 D_001D6200[8] DATA_SECTION(D_001D6200) = {0};

u8 D_001D6208[16] DATA_SECTION(D_001D6208) = {0};

float ammo_preview_offsets[6] DATA_SECTION(D_001D6220) = {0};

f32 ammo_preview_velocities[6] DATA_SECTION(D_001D6238) = {0};

Rec38 preview_animation_requests[8] DATA_SECTION(D_001D6250) = {0};

int menu_stream_buffers[10] DATA_SECTION(D_001D6448) = {0};

u16 pause_weapon_menu_items[76] DATA_SECTION(D_001D6470) = {0};

int pause_weapon_menu_item_field40[16] DATA_SECTION(D_001D6508) = {0};

u16 gadget_menu_items[72] DATA_SECTION(D_001D6548) = {0};

int gadget_menu_item_field40[14] DATA_SECTION(D_001D65D8) = {0};

int gadget_menu_item_field44[14] DATA_SECTION(D_001D6610) = {0};

Out0C galaxy_menu_items[23] DATA_SECTION(D_001D6648) = {0};

int scene_state_buffer_2[64] DATA_SECTION(D_001D6760) = {0};

int scene_state_buffer_0[792] DATA_SECTION(D_001D6860) = {0};

int scene_state_buffer_1[792] DATA_SECTION(D_001D74C0) = {0};

u32 graphics_setup_dma_packets[] DATA_SECTION(D_001D8120) = {
#include "data_inc/D_001D8120.inc"
};

#include "data_inc/D_001D81E0.refs"
u32 ntsc_graphics_setup_packet[] DATA_SECTION(D_001D81E0) = {
#include "data_inc/D_001D81E0.inc"
};

#include "data_inc/D_001D8250.refs"
u32 pal_graphics_setup_packet[] DATA_SECTION(D_001D8250) = {
#include "data_inc/D_001D8250.inc"
};

u32 planet_icon_rows[] DATA_SECTION(D_001DE0C0) = {
#include "data_inc/D_001DE0C0.inc"
};

int map_screen_layer_insets[] DATA_SECTION(D_001DE2E8) = {
#include "data_inc/D_001DE2E8.inc"
};

float preview_placements[] DATA_SECTION(D_001E0708) = {
#include "data_inc/D_001E0708.inc"
};

int map_gold_bolt_totals[] DATA_SECTION(D_001E0B88) = {
#include "data_inc/D_001E0B88.inc"
};

Blk32 cheat_menu_ids_init DATA_SECTION(D_001E8A58) = {
#include "data_inc/D_001E8A58.inc"
};

u32 cheat_menu_text_ids_init[] DATA_SECTION(D_001E8A78) = {
#include "data_inc/D_001E8A78.inc"
};

u32 language_resource_offsets[] DATA_SECTION(D_001E8B00) = {
#include "data_inc/D_001E8B00.inc"
};
