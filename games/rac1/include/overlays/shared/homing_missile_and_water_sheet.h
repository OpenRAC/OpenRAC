#ifndef OVERLAYS_SHARED_HOMING_MISSILE_AND_WATER_SHEET_H
#define OVERLAYS_SHARED_HOMING_MISSILE_AND_WATER_SHEET_H

/* Functions defined in src/overlays/shared/homing_missile_and_water_sheet.c. */

#include "common.h"

extern void Group_CopyZToTableSlots(void *);
extern void append_rotated_textured_screen_quad(float, float, float, float, float, int, int, long, int, int, int, int,
                                                int);
extern void append_scaled_screen_triangle_strip(float, float, float, void *, int, int, long);
extern void draw_quarter_turn_textured_quads(float, float, float, float, unsigned char, unsigned char, unsigned char,
                                             unsigned char);
extern void emit_type_74_particles_from_moby_joints(void *, void *);
extern void project_world_point_to_screen_pixels(void *, void *, void *, int);
extern void queue_oil_tanker_front_texture_2b_batches(int);
extern int func_L11_00309C18(void *, void *, void *, float, float);
extern char *func_L11_0030A318(int owner, float *dir, float *pos, float size, float len, float z);
extern int test_linked_moby_turn_trigger(void *);
extern void update_moby_hit_stats_and_state(void *);
extern void update_moby_path_transform(void *);
extern void update_moby_trail_points(void *);

#endif /* OVERLAYS_SHARED_HOMING_MISSILE_AND_WATER_SHEET_H */
