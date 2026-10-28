/* Functions of level 10 added after the first split; stubs added by tools/overlay_variants.py, replaced by C as functions are matched. */
#include "common.h"
#include "include_asm.h"

extern void SetupSkyGifPaging(void);
extern int g_current_level_index_m __asm__("D_0015EE84") MACRO_ADDR;
extern void draw_sky_shells_with_scaled_phase_rotation(void);
extern void update_sky_shells_static(void);
extern void DoSkyGifPaging(void);
extern void VU1_addGSregister(int, long);
extern int depth_buffer_address MACRO_ADDR;
/* Draw the Orxon sky: phase-rotated shells in level 10, static shells otherwise. */
void SkyFrameDraw(void) {
    SetupSkyGifPaging();
    if (g_current_level_index_m == 10) {
        draw_sky_shells_with_scaled_phase_rotation();
    } else {
        update_sky_shells_static();
    }
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
}
