#ifndef OVERLAYS_L17_FLEET_ROBOT_DOG_AND_SHUTTLE_H
#define OVERLAYS_L17_FLEET_ROBOT_DOG_AND_SHUTTLE_H

/* Functions defined in src/overlays/l17_fleet/robot_dog_and_shuttle.c. */

#include "common.h"

extern int move_robot_dog_toward_position(void *, void *, float);
extern void *try_activate_blarg_ship_from_list(int);
extern void update_robot_dog_aim_joint(void *);
extern void update_robot_dog_glow_effects(void *);
extern void update_robot_dog_hit_reaction_and_target_detection(void *, int);

#endif /* OVERLAYS_L17_FLEET_ROBOT_DOG_AND_SHUTTLE_H */
