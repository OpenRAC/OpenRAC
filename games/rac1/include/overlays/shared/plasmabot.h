#ifndef OVERLAYS_SHARED_PLASMABOT_H
#define OVERLAYS_SHARED_PLASMABOT_H

/* Functions defined in src/overlays/shared/plasmabot.c. */

#include "common.h"

extern void emit_moby_offset_particles(void *);
extern void initialize_moby_path_state(void *);
extern void moby_set_state_one(void *);
extern char *func_L14_00300F80(void *, void *);
extern void *spawn_path_or_ballistic_debris(void *, void *, int);
extern void *spawn_plasmabot_death_debris(void *, void *);
extern void update_linked_moby_position(void *);

#endif /* OVERLAYS_SHARED_PLASMABOT_H */
