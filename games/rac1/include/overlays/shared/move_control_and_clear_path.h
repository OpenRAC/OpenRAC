#ifndef OVERLAYS_SHARED_MOVE_CONTROL_AND_CLEAR_PATH_H
#define OVERLAYS_SHARED_MOVE_CONTROL_AND_CLEAR_PATH_H

/* Functions defined in src/overlays/shared/move_control_and_clear_path.c. */

#include "common.h"

extern void func_L01_0026F040(int, int);
extern void MoveCtl_Init(void *);
extern void func_L01_00279398(float, void *);
extern int check_scaled_vector_bsphere(void *, float);
extern int compute_clear_path_point_mask(float, void *, int, int, void *);
extern int count_active_moby_group_members(int, int);
extern int follow_path(void *, int, int, void *, int, void *, float);
extern int is_hero_on_moby(void *);
extern void set_moby_list_state(int, int);
extern void spawn_moby_from_placement(void *, int);
extern void spawn_proximity_gated_bolts(void *);

#endif /* OVERLAYS_SHARED_MOVE_CONTROL_AND_CLEAR_PATH_H */
