#ifndef GAME_AUDIO_MUSIC_H
#define GAME_AUDIO_MUSIC_H

/* Functions defined in src/game/audio/music.c. */

#include "common.h"

extern int Music_ContinueStreamIfReady(void);
extern void Music_QueueTrack(int, int);
extern void music_Pause(int);
extern void music_PreseekTrack(int, int, int);
extern void music_StartTrack(int, int, int);
extern void music_Stop(void);
extern void music_Unpause(void);
extern void music_Update(void);
extern void music_start_track_by_id(int, int, int);
extern void register_audio_stream_callback(void);
extern void reset_music(void);

#endif /* GAME_AUDIO_MUSIC_H */
