/* Data owned by src/game/render/skyfunc.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

char str_sky_texs[] DATA_SECTION(D_00160550) =
#include "data_inc/D_00160550.inc"
    ;

int sky_gif_paging_tag[4] DATA_SECTION(D_00160570) = {0};

u32 sky_matrix[] DATA_SECTION(D_001D9A70) = {
#include "data_inc/D_001D9A70.inc"
};
