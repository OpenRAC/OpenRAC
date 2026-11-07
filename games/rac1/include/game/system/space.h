#ifndef GAME_SYSTEM_SPACE_H
#define GAME_SYSTEM_SPACE_H

/* Functions defined in src/game/system/space.c. */

#include "common.h"

extern void DoSpaceTransition(void);
extern void dispatch_game_state_update(void);
extern void draw_light_flare(void);
extern void func_0022F4A0(int);
extern void render_environment_mapped_object(void);
extern void update_resident_gameplay_state(void);

#endif /* GAME_SYSTEM_SPACE_H */
