#ifndef GAME_MOVIE_STRFILE_H
#define GAME_MOVIE_STRFILE_H

/* Functions defined in src/game/movie/strfile.c. */

#include "common.h"

extern int get_state_callback_result(void *);
extern int read_cd_stream_sectors(void *, int, int, int);
extern int set_state_fields(void *, int, int);

#endif /* GAME_MOVIE_STRFILE_H */
