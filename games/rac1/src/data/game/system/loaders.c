/* Data owned by src/game/system/loaders.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_system_loaders.h"

u32 sound_archive_base DATA_SECTION(D_0015EF50) = 0;

u32 level_archive_base DATA_SECTION(D_0015EF54) = 0;

u32 hud_bank_alloc_file[] DATA_SECTION(D_0015FC70) = {
#include "data_inc/D_0015FC70.inc"
};

u32 hud_bank1_stash_comment[] DATA_SECTION(D_0015FC80) = {
#include "data_inc/D_0015FC80.inc"
};

u32 hud_bank3_stash_comment[] DATA_SECTION(D_0015FC90) = {
#include "data_inc/D_0015FC90.inc"
};

u32 hud_bank4_stash_comment[] DATA_SECTION(D_0015FCA0) = {
#include "data_inc/D_0015FCA0.inc"
};

u32 hud_bank5_stash_comment[] DATA_SECTION(D_0015FCB0) = {
#include "data_inc/D_0015FCB0.inc"
};

int runtime_resource_tag DATA_SECTION(D_00160044) = {0};

int class_resource_count DATA_SECTION(D_00160048) = {0};

int particle_texture_range_count[] DATA_SECTION(D_001601C0) = {
#include "data_inc/D_001601C0.inc"
};

int D_00160588[] DATA_SECTION(D_00160588) = {
#include "data_inc/D_00160588.inc"
};

int vu1_chain_buffer_2[3] DATA_SECTION(D_001941C8) = {0};

u32 hud_bank_table_ptr[8] DATA_SECTION(D_0019A500) = {0};

int hud_bank_sizes[8] DATA_SECTION(D_0019A520) = {0};

u32 resident_material_templates[96] DATA_SECTION(D_0019E640) = {0};

u32 alternate_special_material_template[6] DATA_SECTION(D_0019E7D8) = {0};

u32 gold_weapon_texture_state[] DATA_SECTION(D_0019E7F0) = {
#include "data_inc/D_0019E7F0.inc"
};

u16 resident_class_id_by_slot[224] DATA_SECTION(D_001B3C80) = {0};

int class_resource_ids[24] DATA_SECTION(D_001CBE40) = {0};

int compressed_class_resources[48] DATA_SECTION(D_001CBEA0) = {0};

u32 class_material_maps[96] DATA_SECTION(D_001CBF60) = {0};

u16 class_runtime_indices[400] DATA_SECTION(D_001CC0E0) = {0};

Pair_2F00 D_001CDD00[128] DATA_SECTION(D_001CDD00) = {0};

u32 D_001CE500[80] DATA_SECTION(D_001CE500) = {0};

u16 shrub_render_class_ids[64] DATA_SECTION(D_001D83C0) = {0};

s32 shrub_render_class_fixed_thresholds[64] DATA_SECTION(D_001D9040) = {0};

u32 shrub_render_class_material_maps[64] DATA_SECTION(D_001D9640) = {0};

u32 D_001D9740[] DATA_SECTION(D_001D9740) = {
#include "data_inc/D_001D9740.inc"
};

u16 object_render_class_ids[128] DATA_SECTION(D_001E1C00) = {0};

u32 object_render_class_fixed_thresholds[128] DATA_SECTION(D_001E3100) = {0};

u32 object_render_class_material_maps[512] DATA_SECTION(D_001E3900) = {0};
