#ifndef OVERLAYS_SHARED_MOBY_FRAME_PACKET_H
#define OVERLAYS_SHARED_MOBY_FRAME_PACKET_H

/* Functions defined in src/overlays/shared/moby_frame_packet.c. */

#include "common.h"

extern void Moby_GetJointWorldPos(void *, int, void *);
extern int SlotTable_Insert(void *);
extern void advance_moby_animation_blend(void *, void *);
extern void *allocate_moby_animation_slot(void *, int);
extern void apply_nearest_environment_sample(int);
extern void compute_selected_world_matrices(int, int, void *, void *);
extern int find_nearest_animation_sequence_entry(void *, float);
extern void initialize_moby_animation_blend(int, void *, int, int, int, int);
extern void jet_clear_persistent(int);
extern void offset_moby_class_point_pairs(unsigned char *, int, int);
extern void release_moby_animation_slot(void *, void *);
extern void set_moby_local_table_entry(void *, int, void *, float, float);

#endif /* OVERLAYS_SHARED_MOBY_FRAME_PACKET_H */
