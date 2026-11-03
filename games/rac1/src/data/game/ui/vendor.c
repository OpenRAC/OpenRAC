/* Data owned by src/game/ui/vendor.c. Its values come from your own baserom (tools/data_in_c.py). */
#include "common.h"

u32 attachment_vertex_indices[] DATA_SECTION(D_00160FD0) = {
#include "data_inc/D_00160FD0.inc"
};

char fmt_thousands_grouped_int[] DATA_SECTION(D_00161178) =
#include "data_inc/D_00161178.inc"
    ;

u32 fmt_plain_int[] DATA_SECTION(D_00161180) = {
#include "data_inc/D_00161180.inc"
};

int capture_moby_pass_enabled[2] DATA_SECTION(D_001611A8) = {0};

short surface_height_layer[] DATA_SECTION(D_001611E0) = {
#include "data_inc/D_001611E0.inc"
};

int status_message_text_ids[] DATA_SECTION(D_001E62B8) = {
#include "data_inc/D_001E62B8.inc"
};

s32 capture_glyph_coordinates[] DATA_SECTION(D_001E6318) = {
#include "data_inc/D_001E6318.inc"
};

s32 capture_glyph_advances[] DATA_SECTION(D_001E6418) = {
#include "data_inc/D_001E6418.inc"
};

u32 capture_bounds_adjustments[] DATA_SECTION(D_001E6518) = {
#include "data_inc/D_001E6518.inc"
};

s32 vendor_flash_timers[8] DATA_SECTION(D_001E6920) = {0};

s32 vendor_scroll_timers[] DATA_SECTION(D_001E6940) = {
#include "data_inc/D_001E6940.inc"
};

u32 surface_height_grid[] DATA_SECTION(D_001E69E0) = {
#include "data_inc/D_001E69E0.inc"
};
