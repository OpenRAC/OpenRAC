#ifndef GAME_MOVIE_MOVIE_H
#define GAME_MOVIE_MOVIE_H

/* Functions defined in src/game/movie/movie.c. */

#include "common.h"

extern void ErrMessage(char *);
extern int Movie_Play(int, int, int, char *, int);
extern void switchThread(void);

#endif /* GAME_MOVIE_MOVIE_H */
