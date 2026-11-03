#ifndef OVERLAYS_L11_POKITARU_SHIP_FLIGHT_AND_OIL_TANKER_H
#define OVERLAYS_L11_POKITARU_SHIP_FLIGHT_AND_OIL_TANKER_H

/* Functions defined in src/overlays/l11_pokitaru/ship_flight_and_oil_tanker.c. */

#include "common.h"

extern void emit_level11_layered_effect_batches_with_pair_offset(int);
extern int is_moby_in_bob_target_exclusion_state(void *);
extern void set_moby_position_and_refresh_removal_timer(int, void *);
extern void set_moby_state_two_data_and_timer(void *, void *);
extern void *spawn_class_511_moby_with_sparks(void *, void *, int);

#endif /* OVERLAYS_L11_POKITARU_SHIP_FLIGHT_AND_OIL_TANKER_H */
