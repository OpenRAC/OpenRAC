/* Data owned by src/game/movie/videodec.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 mpeg_sequence_end_code[] DATA_SECTION(D_00161320) = {
#include "data_inc/D_00161320.inc"
};

u32 video_error_message[] DATA_SECTION(D_00161328) = {
#include "data_inc/D_00161328.inc"
};

char str_decode_thread_aborted[] DATA_SECTION(D_001E8E68) =
#include "data_inc/D_001E8E68.inc"
    ;

u32 str_mpeg_get_picture_error[] DATA_SECTION(D_001E8E80) = {
#include "data_inc/D_001E8E80.inc"
};
