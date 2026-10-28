/* Functions of level 07 added after the first split; stubs added by tools/overlay_variants.py, replaced by C as functions are matched. */
#include "common.h"
#include "include_asm.h"

extern void SetupSkyGifPaging(void);
extern int g_current_level_index_m __asm__("D_0015EE84") MACRO_ADDR;
extern void draw_three_sky_shells_with_particles_and_modulo_50000_rotation(void);
extern void update_sky_shells_static(void);
extern void DoSkyGifPaging(void);
extern void VU1_addGSregister(int, long);
extern int depth_buffer_address MACRO_ADDR;
/* Draw the Umbris sky shells (animated in level 7, static elsewhere) and restore GS test and Z. */
void DrawSkyUmbris(void) {
    SetupSkyGifPaging();
    if (g_current_level_index_m == 7) {
        draw_three_sky_shells_with_particles_and_modulo_50000_rotation();
    } else {
        update_sky_shells_static();
    }
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
}
