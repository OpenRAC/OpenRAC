#ifndef OVERLAYS_SHARED_SHIP_MODE_AND_LEVEL_ENTRY_H
#define OVERLAYS_SHARED_SHIP_MODE_AND_LEVEL_ENTRY_H

/* Functions defined in src/overlays/shared/ship_mode_and_level_entry.c. */

#include "common.h"

extern void deactivate_recorded_moby(void);
extern void initialize_moby_state_record(char *);
extern void prepare_level_entry_transition(void);
extern void prepare_level_transition_with_help_cleanup(void);
extern void prepare_target_level_transition(int);

#endif /* OVERLAYS_SHARED_SHIP_MODE_AND_LEVEL_ENTRY_H */
