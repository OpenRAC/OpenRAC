/* Data owned by src/game/ui/menu.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_ui_menu.h"

u8 max_map_coverage_size[80] DATA_SECTION(D_0013D668) = {0};

u32 menu_item_price_fmt[] DATA_SECTION(D_0015FE60) = {
#include "data_inc/D_0015FE60.inc"
};

char menu_token_error_text[] DATA_SECTION(D_0015FE68) =
#include "data_inc/D_0015FE68.inc"
    ;

int marker_half_size[] DATA_SECTION(D_0015FE70) = {
#include "data_inc/D_0015FE70.inc"
};

u32 menu_action_messages[] DATA_SECTION(D_00199578) = {
#include "data_inc/D_00199578.inc"
};

u32 level_map_transforms[] DATA_SECTION(D_0019EA70) = {
#include "data_inc/D_0019EA70.inc"
};

int map_coverage_active[127] DATA_SECTION(D_001A0218) = {0};

int map_region_enabled_0 DATA_SECTION(D_001A04A8) = {0};

int map_region_enabled_1 DATA_SECTION(D_001A04AC) = {0};

s32 region_enabled DATA_SECTION(D_001A04B0) = {0};

int map_region_enabled_3 DATA_SECTION(D_001A04B4) = {0};

int map_region_enabled_4 DATA_SECTION(D_001A04B8) = {0};

int map_region_enabled_5 DATA_SECTION(D_001A04BC) = {0};

#include "data_inc/D_001A04C0.refs"
int map_region_enabled_6[] DATA_SECTION(D_001A04C0) = {
#include "data_inc/D_001A04C0.inc"
};

#include "data_inc/D_001A0538.refs"
u32 menu_state_handlers[] DATA_SECTION(D_001A0538) = {
#include "data_inc/D_001A0538.inc"
};

int level_object_index_ranges[] DATA_SECTION(D_001E06B8) = {
#include "data_inc/D_001E06B8.inc"
};

NibbleBits nibble_popcount_table DATA_SECTION(D_001E83B0) = {
#include "data_inc/D_001E83B0.inc"
};
