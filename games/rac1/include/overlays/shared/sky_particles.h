#ifndef OVERLAYS_SHARED_SKY_PARTICLES_H
#define OVERLAYS_SHARED_SKY_PARTICLES_H

/* Functions defined in src/overlays/shared/sky_particles.c. */

#include "common.h"

extern void SkyShellsEntryUpdate(void);
extern void SkyShellsRotatePair(void);
extern void draw_four_sky_shells_with_animated_particles(void);
extern void draw_seven_sky_shells_with_modulo_40000_and_60000_rotation(void);
extern void draw_sky_shells_with_fast_and_slow_rotation(void);
extern void draw_sky_shells_with_modulo_25000_rotation(void);
extern void draw_sky_shells_with_modulo_40000_rotation(void);
extern void draw_sky_shells_with_particles_and_modulo_50000_rotation(void);
extern void draw_sky_shells_with_scaled_phase_rotation(void);
extern void draw_sky_shells_with_slow_medium_and_fast_rotation(void);
extern void draw_three_sky_shells_with_particles(void);
extern void draw_three_sky_shells_with_particles_and_modulo_50000_rotation(void);
extern void draw_three_sky_shells_with_three_particle_types(void);
extern void draw_two_sky_shells_with_particles(void);
extern void render_three_shell_particle_sky(void);
extern void sky_particles_init(int, int);
extern void sky_particles_update(void);
extern void sky_shells_rotate_two(void);
extern void update_sky_shells_animated(void);
extern void update_sky_shells_static(void);

#endif /* OVERLAYS_SHARED_SKY_PARTICLES_H */
