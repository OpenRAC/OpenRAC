#ifndef OVERLAYS_SHARED_HERO_WRENCH_AND_GRAPPLE_H
#define OVERLAYS_SHARED_HERO_WRENCH_AND_GRAPPLE_H

/* Functions defined in src/overlays/shared/hero_wrench_and_grapple.c. */

#include "common.h"

extern void hero_update_hop_velocity(void);
extern int process_hero_item_action_input(void);
extern void update_hero_stick_target_magnitude(void);

#endif /* OVERLAYS_SHARED_HERO_WRENCH_AND_GRAPPLE_H */
