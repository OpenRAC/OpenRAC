#ifndef OVERLAYS_SHARED_HERO_UPDATE_BONE_ATTACHMENT_H
#define OVERLAYS_SHARED_HERO_UPDATE_BONE_ATTACHMENT_H

/* Functions defined in src/overlays/shared/hero_update_bone_attachment.c. */

#include "common.h"

extern void adjust_hero_planar_velocity_from_probes(int);
extern void restore_saved_hero_moby_and_clear_vehicle_mode(void);
extern void set_hero_state_for_vehicle_mode(void);

#endif /* OVERLAYS_SHARED_HERO_UPDATE_BONE_ATTACHMENT_H */
