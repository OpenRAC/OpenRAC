#ifndef OVERLAYS_L16_KALEBO3_TEST_DUMMIES_AND_CONVEYORS_H
#define OVERLAYS_L16_KALEBO3_TEST_DUMMIES_AND_CONVEYORS_H

/* Functions defined in src/overlays/l16_kalebo3/test_dummies_and_conveyors.c. */

#include "common.h"

extern void challenge_set_layout_param(float);
extern int get_target_reposition_status(void *);
extern int is_moby_in_state_6(void *);
extern void reposition_moby_with_collision_check(void *, void *);
extern void request_target_reposition(void *);

#endif /* OVERLAYS_L16_KALEBO3_TEST_DUMMIES_AND_CONVEYORS_H */
