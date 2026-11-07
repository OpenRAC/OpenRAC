#ifndef OVERLAYS_SHARED_CUTSCENE_AND_MOVIE_PLAYBACK_H
#define OVERLAYS_SHARED_CUTSCENE_AND_MOVIE_PLAYBACK_H

/* Functions defined in src/overlays/shared/cutscene_and_movie_playback.c. */

#include "common.h"

extern void PlayMovieEntryIfIdle(int);
extern void func_L00_00299B68(int);
extern void handle_pause_input(void);
extern void play_queued_movie(void);
extern void queue_language_zero_movie(int);
extern void queue_movie_with_game_language(int);
extern void reset_frame_counters(void);
extern void start_slideshow(void);
extern void update_cutscene_playback(void);
extern void update_game_frame_and_idle_counters(void);
extern void update_slideshow(void);

#endif /* OVERLAYS_SHARED_CUTSCENE_AND_MOVIE_PLAYBACK_H */
