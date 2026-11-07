#ifndef GAME_AUDIO_SOUND_H
#define GAME_AUDIO_SOUND_H

/* Functions defined in src/game/audio/sound.c. */

#include "common.h"

extern int Moby_PlaySound(int, int, int);
extern int func_0022EE28(int, int, int);
extern int allocate_voice_for_group_entry(int, int, void *);
extern int calculate_voice_distance_volume(void *, float, float, float);
extern void enqueue_voice_request(void);
extern void initialize_gameplay_sound_system(void);
extern int load_audio_bank_by_location(int);
extern void func_0022EAB0(int);
extern void sound_StopAllSounds(void);

#endif /* GAME_AUDIO_SOUND_H */
