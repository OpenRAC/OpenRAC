#ifndef OVERLAYS_SHARED_TRIGGER_SWITCHES_AND_HOVERBOARD_H
#define OVERLAYS_SHARED_TRIGGER_SWITCHES_AND_HOVERBOARD_H

/* Functions defined in src/overlays/shared/trigger_switches_and_hoverboard.c. */

#include "common.h"

extern int AdvanceMobyPath(char *);
extern char *FindMobyByDataId(void *, int);
extern void add_moby_frames_for_group_proximity(void *);
extern void apply_moby_position_and_rotation_wobble(char *);
extern void attach_big_head_cheat_manipulator(void *);
extern void create_rocket_1475_moby(void *, void *, void *, float, float);
extern void initialize_moby_launch_motion(void *, int, void *, void *, int);
extern void request_moby_variant_help_message(void *);
extern void reset_selected_moby_to_current_placement(char *);
extern void transform_moby_vector_table_to_world_space(char *);

#endif /* OVERLAYS_SHARED_TRIGGER_SWITCHES_AND_HOVERBOARD_H */
