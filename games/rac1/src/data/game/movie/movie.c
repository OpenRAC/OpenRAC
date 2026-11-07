/* Data owned by src/game/movie/movie.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

char movie_error_message[] DATA_SECTION(D_001612F8) =
#include "data_inc/D_001612F8.inc"
    ;

int decoder_buffer_address DATA_SECTION(D_00161308) = {0};

int D_00161310 DATA_SECTION(D_00161310) = {0};

int movie_play_step DATA_SECTION(D_00161314) = {0};

int movie_frame_count[2] DATA_SECTION(D_00161318) = {0};

u32 movie_open_error_text[] DATA_SECTION(D_001E8E20) = {
#include "data_inc/D_001E8E20.inc"
};
