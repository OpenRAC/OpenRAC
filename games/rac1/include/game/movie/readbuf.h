#ifndef GAME_MOVIE_READBUF_H
#define GAME_MOVIE_READBUF_H

/* Functions defined in src/game/movie/readbuf.c. */

#include "common.h"

extern int readBufBeginGet(void *, void *);
extern int readBufBeginPut(void *, void *);
extern int readBufCreate(void *);
extern void readBufDelete(void *);
extern int readBufEndGet(void *, int);
extern int readBufEndPut(void *, int);

#endif /* GAME_MOVIE_READBUF_H */
