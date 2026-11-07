/* Data owned by src/game/ui/map.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_ui_map.h"

u8 map_icon_flag800_width[] DATA_SECTION(D_0015FE40) = {
#include "data_inc/D_0015FE40.inc"
};

u8 map_icon_flag400_width[] DATA_SECTION(D_0015FE48) = {
#include "data_inc/D_0015FE48.inc"
};

u8 map_icon_flag1000_width[] DATA_SECTION(D_0015FE50) = {
#include "data_inc/D_0015FE50.inc"
};

TexSlot pending_texture_uploads[64] DATA_SECTION(D_0018D140) = {0};

int D_001A0468[16] DATA_SECTION(D_001A0468) = {0};

u32 D_001E8398[] DATA_SECTION(D_001E8398) = {
#include "data_inc/D_001E8398.inc"
};
