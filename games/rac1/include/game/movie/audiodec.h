#ifndef GAME_MOVIE_AUDIODEC_H
#define GAME_MOVIE_AUDIODEC_H

/* Functions defined in src/game/movie/audiodec.c. */

#include "common.h"

extern void audioDecBeginPut(void *, void *, void *, void *, void *);
extern int audioDecCreate(void *, void *, int, int);
extern int audioDecDelete(void *);
extern void audioDecEndPut(void *, int);
extern void audioDecReset(void *);
extern void audioDecSend(void *);
extern void audioDecStart(void *);
extern int func_0023C2B0(void *);

#endif /* GAME_MOVIE_AUDIODEC_H */
