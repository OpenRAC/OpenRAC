#ifndef OVERLAYS_SHARED_WALLOPER_AND_BEAM_EFFECTS_H
#define OVERLAYS_SHARED_WALLOPER_AND_BEAM_EFFECTS_H

/* Functions defined in src/overlays/shared/walloper_and_beam_effects.c. */

#include "common.h"

extern void build_beam_and_arc_points(void *);
extern int compute_moby_state_from_vertical_probe(void *);
extern void draw_beam_and_arc_effects(void);
extern void draw_beam_and_arc_two_pass_quads(void *, int, int, int, float, float, float);
extern void initialize_moby_hop_trajectory(void *, void *, void *);
extern int probe_moby_raised_sphere_collision(void *, int, void *);
extern void restore_saved_state_fields_from_hero(void *);
extern void restore_state_fields_from_hero(void *);
extern void save_state_fields_to_hero(void *);
extern void save_state_vector_and_fields_to_hero(void *);
extern void *select_class5_target_or_hero(void *, void *, void *, void *, int);
extern float select_moby_heading_by_collision_probes(void *, void *, float, float, float, float, float, void *, void *,
                                                     float);
extern void *spawn_doom_bot(int, void *, void *);
extern void *spawn_linked_moby_from_basis(void *);
extern int try_start_moby_hop(void *, void *, void *, int);
extern void update_beam_ring_rail_and_pulse_effects(void *);

#endif /* OVERLAYS_SHARED_WALLOPER_AND_BEAM_EFFECTS_H */
