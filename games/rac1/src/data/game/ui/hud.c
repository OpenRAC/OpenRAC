/* Data owned by src/game/ui/hud.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

int hud_msg_count DATA_SECTION(D_0015F760) = {0};

int hud_msg_alpha DATA_SECTION(D_0015F764) = {0};

int hud_msg_y[2] DATA_SECTION(D_0015F768) = {0};

u32 hud_file_str[] DATA_SECTION(D_0015F7B8) = {
#include "data_inc/D_0015F7B8.inc"
};

short ResourceCounter[] DATA_SECTION(D_0015F9D0) = {
#include "data_inc/D_0015F9D0.inc"
};

u32 hud_bank_main DATA_SECTION(D_0015FAB8) = 0;

u32 hud_bank_cursor DATA_SECTION(D_0015FABC) = 0;

u32 hud_bank_end[32] DATA_SECTION(D_0015FAC0) = {0};

u32 hud_msg_text1[20] DATA_SECTION(D_001994D8) = {0};

u8 hud_msg_text2[80] DATA_SECTION(D_00199528) = {0};

#include "data_inc/D_00199C60.refs"
u32 hud_chan_slots[] DATA_SECTION(D_00199C60) = {
#include "data_inc/D_00199C60.inc"
};

u32 hud_state[6] DATA_SECTION(D_0019A4E8) = {0};

u32 message_text_buffer[] DATA_SECTION(D_0019A540) = {
#include "data_inc/D_0019A540.inc"
};

u32 message_too_long_label[] DATA_SECTION(D_001E7DD8) = {
#include "data_inc/D_001E7DD8.inc"
};
