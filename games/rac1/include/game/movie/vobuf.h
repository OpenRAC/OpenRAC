#ifndef GAME_MOVIE_VOBUF_H
#define GAME_MOVIE_VOBUF_H

/* Functions defined in src/game/movie/vobuf.c. */

#include "common.h"

extern void voBufCreate(void *, int, int, int);
extern void voBufDelete(void *);
extern void *voBufGetData(void *);
extern void voBufIncCount(void *);
extern int voBufIsFull(void *);
extern void voBufReset(void *);

#endif /* GAME_MOVIE_VOBUF_H */
