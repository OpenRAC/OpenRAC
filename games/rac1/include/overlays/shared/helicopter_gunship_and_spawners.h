#ifndef OVERLAYS_SHARED_HELICOPTER_GUNSHIP_AND_SPAWNERS_H
#define OVERLAYS_SHARED_HELICOPTER_GUNSHIP_AND_SPAWNERS_H

/* Functions defined in src/overlays/shared/helicopter_gunship_and_spawners.c. */

#include "common.h"

extern void attach_shared_manipulator_to_matching_class_moby(void *);
extern void draw_textured_grid(char *);
extern int is_moby_class_23e_in_state_14(void *);
extern void set_moby_state_14(void *);
extern void *spawn_homing_spark_projectile(void *, void *, int, int, float);

#endif /* OVERLAYS_SHARED_HELICOPTER_GUNSHIP_AND_SPAWNERS_H */
