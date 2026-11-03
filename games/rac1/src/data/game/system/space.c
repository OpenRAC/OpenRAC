/* Data owned by src/game/system/space.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/vector.h"

u8 level_render_state[32] DATA_SECTION(D_0013E130) = {0};

int ship_substate[16] DATA_SECTION(D_0013E150) = {0};

u8 hero_spawn_pose[960] DATA_SECTION(D_0013E190) = {0};

u32 environment_mapping_uv_refresh_state DATA_SECTION(D_001605A4) =
#include "data_inc/D_001605A4.inc"
    ;

short env_map_was_active[] DATA_SECTION(D_001605A8) = {
#include "data_inc/D_001605A8.inc"
};

short warp_trail_widths[] DATA_SECTION(D_001605D0) = {
#include "data_inc/D_001605D0.inc"
};

short resident_quad_start_pos[] DATA_SECTION(D_001605E0) = {
#include "data_inc/D_001605E0.inc"
};

short resident_quad_pos[8] DATA_SECTION(D_001605F0) = {0};

u32 D_00160600[] DATA_SECTION(D_00160600) = {
#include "data_inc/D_00160600.inc"
};

u32 scene_variant_durations[] DATA_SECTION(D_00160610) = {
#include "data_inc/D_00160610.inc"
};

short environment_mesh_colors[] DATA_SECTION(D_00160620) = {
#include "data_inc/D_00160620.inc"
};

short environment_mesh_vertex_counts[] DATA_SECTION(D_00160630) = {
#include "data_inc/D_00160630.inc"
};

short environment_mesh_quad_counts[] DATA_SECTION(D_00160640) = {
#include "data_inc/D_00160640.inc"
};

#include "data_inc/D_00160650.refs"
u32 environment_mesh_normals[] DATA_SECTION(D_00160650) = {
#include "data_inc/D_00160650.inc"
};

#include "data_inc/D_00160660.refs"
u32 environment_mesh_positions[] DATA_SECTION(D_00160660) = {
#include "data_inc/D_00160660.inc"
};

#include "data_inc/D_00160670.refs"
u32 environment_mesh_quads[] DATA_SECTION(D_00160670) = {
#include "data_inc/D_00160670.inc"
};

u32 resident_quad_texture[2] DATA_SECTION(D_00160680) = {0};

#include "data_inc/D_00160688.refs"
u32 resident_banner_texture[] DATA_SECTION(D_00160688) = {
#include "data_inc/D_00160688.inc"
};

u32 textured_quad_giftag[] DATA_SECTION(D_00160950) = {
#include "data_inc/D_00160950.inc"
};

u32 level_init_cleared_block2[32] DATA_SECTION(D_00186450) = {0};

s32 render_sequence_slots[26] DATA_SECTION(D_0018CD98) = {0};

int level_resource_buffer_base DATA_SECTION(D_001941D4) = {0};

s32 resident_indexed_texture_addrs[256] DATA_SECTION(D_001B5D00) = {0};

u32 resident_class_material_maps[896] DATA_SECTION(D_001B6C00) = {0};

u32 space_flight_paths[240] DATA_SECTION(D_001CC730) = {0};

u32 default_dir_lights[] DATA_SECTION(D_001D9AD0) = {
#include "data_inc/D_001D9AD0.inc"
};

u32 quad_default_uvs[] DATA_SECTION(D_001D9B40) = {
#include "data_inc/D_001D9B40.inc"
};

float light_flare_corners[] DATA_SECTION(D_001D9B60) = {
#include "data_inc/D_001D9B60.inc"
};

float warp_texture_coordinates[] DATA_SECTION(D_001D9DA0) = {
#include "data_inc/D_001D9DA0.inc"
};

u32 warp_trail_colors[] DATA_SECTION(D_001D9DC0) = {
#include "data_inc/D_001D9DC0.inc"
};

float resident_quad_corners[] DATA_SECTION(D_001D9DE0) = {
#include "data_inc/D_001D9DE0.inc"
};

float resident_quad_level_scales[] DATA_SECTION(D_001D9E20) = {
#include "data_inc/D_001D9E20.inc"
};

u32 sky_shell_scroll_dirs[] DATA_SECTION(D_001D9E70) = {
#include "data_inc/D_001D9E70.inc"
};

float sky_shell_base_rot_z_by_mode[] DATA_SECTION(D_001D9EC0) = {
#include "data_inc/D_001D9EC0.inc"
};

float cam_mode_min_fov[] DATA_SECTION(D_001D9ED8) = {
#include "data_inc/D_001D9ED8.inc"
};

float flare_spark_offsets[] DATA_SECTION(D_001D9EF0) = {
#include "data_inc/D_001D9EF0.inc"
};

Vec4f env_map_world_positions[102] DATA_SECTION(D_001DC870) = {0};

float env_map_uvs[204] DATA_SECTION(D_001DCED0) = {0};

float env_map_saved_uvs[204] DATA_SECTION(D_001DD200) = {0};

u32 unloaded_sound_bank_format[] DATA_SECTION(D_001E8CB8) = {
#include "data_inc/D_001E8CB8.inc"
};
