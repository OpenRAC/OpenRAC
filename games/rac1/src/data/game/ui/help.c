/* Data owned by src/game/ui/help.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_ui_help.h"

u8 help_records[16] DATA_SECTION(D_00141A68) = {0};

u8 active_message_records[152] DATA_SECTION(D_00141F08) = {0};

int active_message_record_count DATA_SECTION(D_0015EF30) = {
#include "data_inc/D_0015EF30.inc"
};

short map_label_text_y_offset[] DATA_SECTION(D_0015F770) = {
#include "data_inc/D_0015F770.inc"
};

u32 D_00199800[4] DATA_SECTION(D_00199800) = {0};

short message_id_code_table[] DATA_SECTION(D_00199810) = {
#include "data_inc/D_00199810.inc"
};

u32 s_paradox_this_message_does_not[] DATA_SECTION(D_00199A68) = {
#include "data_inc/D_00199A68.inc"
};

LevelMapMarker level_map_markers[] DATA_SECTION(D_001DE1A8) = {
#include "data_inc/D_001DE1A8.inc"
};
