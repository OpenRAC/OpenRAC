/* Data owned by src/game/system/bmain.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u8 intro_movie_disc_entry_ntsc[8] DATA_SECTION(D_00139478) = {0};

u8 intro_movie_disc_entry_pal[4296] DATA_SECTION(D_00139480) = {0};

s32 startlevel_code_addr DATA_SECTION(D_0015F05C) = {0};

u32 startlevel_sound_defs[] DATA_SECTION(D_00186200) = {
#include "data_inc/D_00186200.inc"
};

char str_loading_sound_bank[] DATA_SECTION(D_001E79C0) =
#include "data_inc/D_001E79C0.inc"
    ;
