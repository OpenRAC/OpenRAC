#ifndef ASM_FUNCTIONS_LEVEL_06_H
#define ASM_FUNCTIONS_LEVEL_06_H

/* Functions with no C definition: level_06. */

#include "common.h"

extern void blarg_draw_strand_strip(void *, void *);
extern void draw_segmented_hud_meter_base();
extern void draw_tube_mesh64_twin(void);
extern void draw_tube_mesh96_twin(void);
extern void finish_alien_queen_first_recovery(void *);
extern void finish_alien_queen_second_recovery(void *);
extern void handle_gadgebot_hit_knockback(void *);
extern int process_gadgebot_interactions(void *, int);
extern void set_vec_to_origin_point(void *);
extern void start_alien_queen_first_recovery(void *);
extern void start_alien_queen_second_recovery(void *);
extern int update_alien_queen_second_recovery_camera(void *);

#endif /* ASM_FUNCTIONS_LEVEL_06_H */
