#ifndef OVERLAYS_SHARED_SHIP_PICKUP_AND_ROCKET_H
#define OVERLAYS_SHARED_SHIP_PICKUP_AND_ROCKET_H

/* Functions defined in src/overlays/shared/ship_pickup_and_rocket.c. */

#include "common.h"

extern void emit_transformed_2d_vertex_packet(void *, int, int, long, float, float, float, float);
extern void flag_vendor_item(int);
extern float interpolate_value_with_repeated_squaring(int, int, float, float, float, float, float);
extern unsigned char *spawn_ship_pickup(char *position, int moby_class);
extern int func_L13_002E7E90(void *, void *, float, int, void *, int);

#endif /* OVERLAYS_SHARED_SHIP_PICKUP_AND_ROCKET_H */
