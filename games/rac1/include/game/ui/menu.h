#ifndef GAME_UI_MENU_H
#define GAME_UI_MENU_H

/* Functions defined in src/game/ui/menu.c. */

#include "common.h"

extern void func_00209040(void);
extern void FormatMenuItemText(int, void *);
extern int Menu_RunStateHandler(void);
extern void copy_level_object_positions(void);
extern void draw_map_markers(int, int, int, int);
extern void load_map_chunk(void *, void *, int);
extern void save_map_coverage(void *);
extern void world_to_map_coords(void *, void *, int, float, float);

#endif /* GAME_UI_MENU_H */
