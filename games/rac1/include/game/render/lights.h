#ifndef GAME_RENDER_LIGHTS_H
#define GAME_RENDER_LIGHTS_H

/* Functions defined in src/game/render/lights.c. */

#include "common.h"

extern void DetachPointLight(int);
extern void RefreshPointLight(int);
extern void UpdateAllPointLights(void);

#endif /* GAME_RENDER_LIGHTS_H */
