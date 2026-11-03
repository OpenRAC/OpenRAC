/* Functions of level 10 added after the first split; stubs added by tools/overlay_variants.py, replaced by C as
   functions are matched. */
#include "common.h"
#include "include_asm.h"
#include "game/render/skyfunc.h"
#include "overlays/shared/sky_particles.h"
#include "enums/level.h"
#include "game/render/vuchain.h"

extern int D_0015EE84 MACRO_ADDR;

extern int depth_buffer_address MACRO_ADDR;
/* Draw the Orxon sky: phase-rotated shells in level 10, static shells otherwise. */
void SkyFrameDraw(void) {
    SetupSkyGifPaging();
    if (g_current_level_index == LEVEL_ORXON) {
        draw_sky_shells_with_scaled_phase_rotation();
    } else {
        update_sky_shells_static();
    }
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
}
