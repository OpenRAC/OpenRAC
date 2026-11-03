#ifndef GAME_MOVIE_VIDEODEC_H
#define GAME_MOVIE_VIDEODEC_H

/* Functions defined in src/game/movie/videodec.c. */

#include "common.h"

extern void videoDecAbort(void *);
extern void videoDecBeginPut(void *, void *, void *, void *, void *);
extern int videoDecCreate(void *, int, int, void *, int, int, void *, int);
extern int videoDecDelete(void *);
extern void videoDecEndPut(void *, int);
extern int videoDecFlush(void *);
extern int videoDecGetState(void *);
extern int videoDecIsFlushed(void *);
extern int videoDecPutTs(void *, long, long, void *, int);
extern int videoDecSetStream(void *, int, int, void *, void *);

#endif /* GAME_MOVIE_VIDEODEC_H */
