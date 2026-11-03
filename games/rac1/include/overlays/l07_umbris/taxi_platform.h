#ifndef OVERLAYS_L07_UMBRIS_TAXI_PLATFORM_H
#define OVERLAYS_L07_UMBRIS_TAXI_PLATFORM_H

/* Functions defined in src/overlays/l07_umbris/taxi_platform.c. */

#include "common.h"

extern void configure_timed_point_light(float, void *, void *, short, void *, short, unsigned int, int);
extern void rumble_by_distance(void *);
extern void *spawn_machinegun_turret_projectile(void *, void *, void *, int);
extern void *spawn_moby_class_1046(int, void *, int, float, float, float, float, float);
extern int update_alternating_joint_emission_burst(void *, void *, void *);
extern void update_timed_point_light_color(void *);

#endif /* OVERLAYS_L07_UMBRIS_TAXI_PLATFORM_H */
