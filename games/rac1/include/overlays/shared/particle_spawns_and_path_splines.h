#ifndef OVERLAYS_SHARED_PARTICLE_SPAWNS_AND_PATH_SPLINES_H
#define OVERLAYS_SHARED_PARTICLE_SPAWNS_AND_PATH_SPLINES_H

/* Functions defined in src/overlays/shared/particle_spawns_and_path_splines.c. */

#include "common.h"

extern void PartType41Spawn(void *, void *, int, unsigned int, int, int, int, float);
extern int advance_path_node_index(void *);
extern void compute_path_node_tangent(void *, void *, int, float, float);
extern float compute_sampled_spline_path_lengths(float, float, void *, int, void *);
extern void evaluate_cubic_hermite_segment(float, void *, void *, void *, void *, void *);
extern void initialize_path_descriptor(void *);
extern int resolve_path_node_index(void *, int);
extern int select_list_point_toward_target(void *, void *, void *, void *);
extern int select_point_index_by_distance(void *, void *, float);
extern void spawn_type_10_particle(void *, void *);
extern void spawn_type_14_particle(int, void *);
extern void spawn_type_52_particle(float *, float, float, int, int, int);
extern void spawn_type_74_particle(void *, void *, void *, void *, int);

#endif /* OVERLAYS_SHARED_PARTICLE_SPAWNS_AND_PATH_SPLINES_H */
