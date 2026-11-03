#ifndef GAME_UI_HELP_H
#define GAME_UI_HELP_H

/* Functions defined in src/game/ui/help.c. */

#include "common.h"

extern void Help_Update(void);
extern void calculate_ring_buffer_bounds(int, void *, void *);
extern void draw_help(void);
extern void find_help_entry(int, int, void *);
extern void *msg_string(int);
extern void push_help_history(int);

#endif /* GAME_UI_HELP_H */
