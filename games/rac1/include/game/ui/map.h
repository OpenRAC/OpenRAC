#ifndef GAME_UI_MAP_H
#define GAME_UI_MAP_H

/* Functions defined in src/game/ui/map.c. */

#include "common.h"

extern void compose_bitmap_from_mask(unsigned char *, unsigned char *, unsigned char *, unsigned char *);
extern void decode_map_mask(int *, unsigned char *, short *);
extern void decode_map_rle_row(unsigned char *dst, int row, unsigned char *src, short *offs);
extern void draw_map_overlay(void);
extern int find_id_in_terminated_table(int);
extern int find_map_entry_slot(int);
extern long load_texture_file(char *);
extern void move_map_entry_slot(int, int);
extern int pick_map_slot_to_evict(void);
extern int pick_next_map(void);
extern int promote_first_available_map_entry(void);
extern long register_8bit_texture(int, int, int, int, int, int);
extern int update_map_zoom_and_pan(void);

#endif /* GAME_UI_MAP_H */
