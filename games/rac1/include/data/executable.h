#ifndef DATA_EXECUTABLE_H
#define DATA_EXECUTABLE_H

/* Data declared the same way by several files: executable. */

#include "common.h"

extern int SbrkErrno;
extern int __malloc_current_mallinfo;
extern char *__malloc_sbrk_base;
extern unsigned char active_message_records[];
extern int camera_position_publication_suppressed;
extern char camera_rows[];
extern int D_001313D0;
extern int D_001313F0;
extern char D_00132590[];
extern int cd_ncmd_sema;
extern int D_001325C0;
extern char D_00132E08[];
extern int cd_scmd_sema;
extern char coll_hit_pos[];
extern unsigned char debug_font_buffer[];
extern int draw_config[];
extern short draw_enable_mask;
extern char draw_environment[];
extern char first_clear_packet[];
extern char fs_aa_clear_packet[];
extern char fs_aa_draw_packet[];
extern char fs_aa_resample_packet[];
extern char fs_aa_transfer_packet[];
extern int gGalaxyList[];
extern char g_help_state[];
extern f32 g_hero_capsule_top[];
extern int hero_current_state;
extern unsigned char large_font_metrics[];
extern int last_weapon_id;
extern s32 level_bolts[];
extern s32 level_hero_hit_counts[];
extern char D_00141FC0[];
extern char level_render_state_base[];
extern char moby_vu1_data[];
extern unsigned char normal_font_metrics[];
extern int pause_iop_sprite_lists[];
extern int radial_select_choice[];
extern int resident_class_original_field_2c[];
extern int D_001B3900[];
extern char sif_lib_version_string[];
extern unsigned char small_font_metrics[];
extern short sound_volume_10;
extern float tfrag_render_ranges[3];
extern char vendor_state[];

#endif /* DATA_EXECUTABLE_H */
