#ifndef OVERLAYS_SHARED_TRESPASSER_PAD_AND_BACKGROUND_SHIPS_H
#define OVERLAYS_SHARED_TRESPASSER_PAD_AND_BACKGROUND_SHIPS_H

/* Functions defined in src/overlays/shared/trespasser_pad_and_background_ships.c. */

#include "common.h"

extern void attach_big_head_cheat_manipulator_to_matching_moby(void *);
extern void build_interpolated_dma_packet(void *);
extern void initialize_collectible_sprite_state(void *);
extern void prepare_scaled_float_table_with_target_byte(void *, float, int);
extern void prepare_scaled_table_with_byte_128(void *, float);
extern void restore_view_after_textured_grid(void *);
extern void set_nonzero_follow_camera_parameters(float, float, float);
extern void set_selected_object_float_triplets(float, float, float, float, float, float);
extern void update_collectible_sprite_effect(void *);

#endif /* OVERLAYS_SHARED_TRESPASSER_PAD_AND_BACKGROUND_SHIPS_H */
