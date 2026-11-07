/* Functions of level 13 added after the first split; stubs added by tools/overlay_variants.py, replaced by C as
   functions are matched. */
#include "common.h"
#include "include_asm.h"
#include "game/render/skyfunc.h"
#include "overlays/shared/sky_particles.h"
#include "enums/level.h"
#include "game/render/vuchain.h"

extern int g_current_level_index MACRO_ADDR;

extern int depth_buffer_address MACRO_ADDR;
/* Draws the Gemlik sky shells and particles on level 13, then restores the GS draw registers. */
void draw_paged_sky_shells(void) {
    SetupSkyGifPaging();
    if (g_current_level_index == LEVEL_GEMLIK) {
        draw_three_sky_shells_with_particles();
    } else {
        update_sky_shells_static();
    }
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
}
