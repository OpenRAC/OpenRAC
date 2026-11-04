/* Data owned by src/game/gameplay/freeze.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

#include "structs/data/game_gameplay_freeze.h"

u8 D_00141150[16] DATA_SECTION(D_00141150) = {0};

u8 D_00141160[16] DATA_SECTION(D_00141160) = {0};

int rilgar_hoverboard_races_finished DATA_SECTION(D_0015EF38) = {0};

int kalebo3_hoverboard_races_finished DATA_SECTION(D_0015EF3C) = {0};

short freeze_box_line1_y[] DATA_SECTION(D_0015F5E8) = {
#include "data_inc/D_0015F5E8.inc"
};

u32 freeze_box_line2_y DATA_SECTION(D_0015F5EC) =
#include "data_inc/D_0015F5EC.inc"
    ;

short freeze_text_fade_color_start[] DATA_SECTION(D_0015F5F0) = {
#include "data_inc/D_0015F5F0.inc"
};

u32 freeze_text_fade_color_end DATA_SECTION(D_0015F5F4) =
#include "data_inc/D_0015F5F4.inc"
    ;

short freeze_short_box_center_x[] DATA_SECTION(D_0015F5F8) = {
#include "data_inc/D_0015F5F8.inc"
};

u32 freeze_short_box_center_y DATA_SECTION(D_0015F5FC) =
#include "data_inc/D_0015F5FC.inc"
    ;

short freeze_short_box_half_width[] DATA_SECTION(D_0015F600) = {
#include "data_inc/D_0015F600.inc"
};

u32 freeze_short_box_half_height DATA_SECTION(D_0015F604) =
#include "data_inc/D_0015F604.inc"
    ;

short freeze_short_box_text_y[] DATA_SECTION(D_0015F608) = {
#include "data_inc/D_0015F608.inc"
};

u32 freeze_long_box_center_x DATA_SECTION(D_0015F60C) =
#include "data_inc/D_0015F60C.inc"
    ;

short freeze_long_box_center_y[] DATA_SECTION(D_0015F610) = {
#include "data_inc/D_0015F610.inc"
};

u32 freeze_long_box_half_width DATA_SECTION(D_0015F614) =
#include "data_inc/D_0015F614.inc"
    ;

short freeze_long_box_half_height[] DATA_SECTION(D_0015F618) = {
#include "data_inc/D_0015F618.inc"
};

u32 freeze_long_box_text_y DATA_SECTION(D_0015F61C) =
#include "data_inc/D_0015F61C.inc"
    ;

short freeze_box_pulse_period[] DATA_SECTION(D_0015F620) = {
#include "data_inc/D_0015F620.inc"
};

u32 freeze_box_pulse_color_a DATA_SECTION(D_0015F624) =
#include "data_inc/D_0015F624.inc"
    ;

short freeze_box_pulse_color_b[] DATA_SECTION(D_0015F628) = {
#include "data_inc/D_0015F628.inc"
};

u32 freeze_box_text_color DATA_SECTION(D_0015F62C) =
#include "data_inc/D_0015F62C.inc"
    ;

short freeze_box_text_flash_color[] DATA_SECTION(D_0015F630) = {
#include "data_inc/D_0015F630.inc"
};

u32 freeze_box_record_color DATA_SECTION(D_0015F634) =
#include "data_inc/D_0015F634.inc"
    ;

short freeze_box_record_flash_color[] DATA_SECTION(D_0015F638) = {
#include "data_inc/D_0015F638.inc"
};

u32 freeze_box_no_record_color DATA_SECTION(D_0015F63C) =
#include "data_inc/D_0015F63C.inc"
    ;

short freeze_box_no_record_flash_color[] DATA_SECTION(D_0015F640) = {
#include "data_inc/D_0015F640.inc"
};

u32 freeze_prompt_number_format[] DATA_SECTION(D_0015F650) = {
#include "data_inc/D_0015F650.inc"
};

char freeze_rank_line_format[] DATA_SECTION(D_0015F660) =
#include "data_inc/D_0015F660.inc"
    ;

u32 freeze_text_line_format[] DATA_SECTION(D_0015F668) = {
#include "data_inc/D_0015F668.inc"
};

char freeze_race_time_format[] DATA_SECTION(D_0015F670) =
#include "data_inc/D_0015F670.inc"
    ;

u32 freeze_record_value_format[] DATA_SECTION(D_0015F680) = {
#include "data_inc/D_0015F680.inc"
};

u32 mode_freeze_info[] DATA_SECTION(D_00193400) = {
#include "data_inc/D_00193400.inc"
};

s32 freeze_prompt_override_flag[2] DATA_SECTION(D_001D60C0) = {0};

struct TextRegionP freeze_info_text_region DATA_SECTION(D_001E7BF8) = {
#include "data_inc/D_001E7BF8.inc"
};

u32 freeze_choice_text_region[] DATA_SECTION(D_001E7C10) = {
#include "data_inc/D_001E7C10.inc"
};
