#ifndef OVERLAYS_SHARED_SOUND_PLAYBACK_H
#define OVERLAYS_SHARED_SOUND_PLAYBACK_H

/* Functions defined in src/overlays/shared/sound_playback.c. */

#include "common.h"

extern int func_L00_0028EB98(void *, int);
extern int Sound_PlayClassSound(int, int, int, int);
extern int Sound_SetPitchBend(int, int);
extern void Sound_StopSlot(int);
extern int play_voice(int a, int b, int c, int d);
extern int request_grouped_voice(int, int, int, int, int);
extern void resolve_lookup_record(void *);
extern void update_moby_proximity_interaction(void);

#endif /* OVERLAYS_SHARED_SOUND_PLAYBACK_H */
