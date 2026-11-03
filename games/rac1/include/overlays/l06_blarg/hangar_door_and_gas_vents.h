#ifndef OVERLAYS_L06_BLARG_HANGAR_DOOR_AND_GAS_VENTS_H
#define OVERLAYS_L06_BLARG_HANGAR_DOOR_AND_GAS_VENTS_H

/* Functions defined in src/overlays/l06_blarg/hangar_door_and_gas_vents.c. */

#include "common.h"

extern int activate_state_15_moby_at_position(void *, void *);
extern int check_all_listed_frog_states_fe_fd_or_10(int);
extern void clear_moby_list_state_18(int);
extern void clear_moby_list_state_4(int);
extern void set_moby_and_links_to_states_4_and_5(char *);
extern void set_moby_and_links_to_states_8_and_9(char *);
extern void set_moby_list_state_15(int);
extern void set_moby_list_state_18(int);
extern void set_moby_list_state_4(int);
extern void set_moby_state_3(unsigned char *);

#endif /* OVERLAYS_L06_BLARG_HANGAR_DOOR_AND_GAS_VENTS_H */
