#ifndef OVERLAYS_SHARED_CRATES_AND_VELDIN_SHIP_H
#define OVERLAYS_SHARED_CRATES_AND_VELDIN_SHIP_H

/* Functions defined in src/overlays/shared/crates_and_veldin_ship.c. */

#include "common.h"

extern void break_crate(void *, int, int);
extern void ground_attached_update(void *);
extern void initialize_moby_particle_effect(void *);
extern void mine_camera_blast(void *, void *, int, int);
extern void *spawn_burst_moby(void *, float);
extern void update_crate_stack_motion(void *);

#endif /* OVERLAYS_SHARED_CRATES_AND_VELDIN_SHIP_H */
