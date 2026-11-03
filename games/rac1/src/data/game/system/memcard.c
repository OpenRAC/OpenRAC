/* Data owned by src/game/system/memcard.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u8 memcard_save_dir_name[] DATA_SECTION(D_0013D2D0) = {
#include "data_inc/D_0013D2D0.inc"
};

u8 memcard_dir_search_path[] DATA_SECTION(D_0013D2E8) = {
#include "data_inc/D_0013D2E8.inc"
};

u32 memcard_icon_path_a[] DATA_SECTION(D_0013D300) = {
#include "data_inc/D_0013D300.inc"
};

u32 memcard_icon_path_b[] DATA_SECTION(D_0013D320) = {
#include "data_inc/D_0013D320.inc"
};

u8 memcard_slot_path_format[] DATA_SECTION(D_0013D370) = {
#include "data_inc/D_0013D370.inc"
};

u8 memcard_game_data_buffer[6144] DATA_SECTION(D_0014EFD0) = {0};

u8 memcard_level_data_buffer[4096] DATA_SECTION(D_001507D0) = {0};

short memcard_restore_mismatch_count[4] DATA_SECTION(D_0015FF50) = {0};

char memcard_slot_file_prefix[] DATA_SECTION(D_0015FF58) =
#include "data_inc/D_0015FF58.inc"
    ;

char memcard_slot_file_suffix[] DATA_SECTION(D_0015FF60) =
#include "data_inc/D_0015FF60.inc"
    ;

char fmt_memcard_path[] DATA_SECTION(D_0015FF68) =
#include "data_inc/D_0015FF68.inc"
    ;

u32 str_memcard_deleting_file[] DATA_SECTION(D_0015FF70) = {
#include "data_inc/D_0015FF70.inc"
};

#include "data_inc/D_001A05C0.refs"
int mc_gamedata[] DATA_SECTION(D_001A05C0) = {
#include "data_inc/D_001A05C0.inc"
};

#include "data_inc/D_001A08C0.refs"
int mc_leveldata[] DATA_SECTION(D_001A08C0) = {
#include "data_inc/D_001A08C0.inc"
};

u32 memcard_dir_entries[8] DATA_SECTION(D_001A0980) = {0};

u32 memcard_save_file_name_buffer[168] DATA_SECTION(D_001A09A0) = {0};

u32 memcard_restore_size_mismatch_msg[] DATA_SECTION(D_001E8500) = {
#include "data_inc/D_001E8500.inc"
};

u32 str_memcard_deleting_folder[] DATA_SECTION(D_001E8518) = {
#include "data_inc/D_001E8518.inc"
};

u32 memcard_error_message[] DATA_SECTION(D_001E8690) = {
#include "data_inc/D_001E8690.inc"
};
