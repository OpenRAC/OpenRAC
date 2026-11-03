#ifndef GAME_GAMEPLAY_FREEZE_H
#define GAME_GAMEPLAY_FREEZE_H

/* Functions defined in src/game/gameplay/freeze.c. */

#include "common.h"

extern void draw_freeze_message_box(void);
extern void mode_freeze_init(int, void *, int);
extern void update_mode_freeze(void);

#endif /* GAME_GAMEPLAY_FREEZE_H */
