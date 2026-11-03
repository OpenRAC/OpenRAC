#ifndef OVERLAYS_SHARED_NANOTECH_BALL_AND_SUCK_CANNON_H
#define OVERLAYS_SHARED_NANOTECH_BALL_AND_SUCK_CANNON_H

/* Functions defined in src/overlays/shared/nanotech_ball_and_suck_cannon.c. */

#include "common.h"

extern int CarriedMobyUpdate(void *, void *);
extern int cancel_suck_cannon_suction(void *);
extern void create_linked_class_0326_moby(void *);
extern char *get_moby_suck_cannon_record(char *);
extern int is_moby_in_state_one(void *);
extern int launch_suck_cannon_projectile(char *, int, int, float);
extern int process_moby_suck_cannon_suction(void *, void *, void *, float);
extern void quad_moby_update(void);
extern char *spawn_randomly_rotated_colored_moby(void *, void *, void *, float, int, unsigned char, unsigned char,
                                                 unsigned char, int);
extern int store_moby_in_suck_cannon(void *, void *, void *, float);
extern void suck_beam_queue_draw(int);
extern void suck_reset(void *);

#endif /* OVERLAYS_SHARED_NANOTECH_BALL_AND_SUCK_CANNON_H */
