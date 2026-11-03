#ifndef GAME_MOVIE_VIBUF_H
#define GAME_MOVIE_VIBUF_H

/* Functions defined in src/game/movie/vibuf.c. */

#include "common.h"

extern int viBufAddDMA(void *);
extern void viBufBeginPut(void *, void *, void *, void *, void *);
extern int viBufCount(void *);
extern int viBufCreate(void *, void *, void *, int, void *, int);
extern int viBufDelete(void *);
extern void viBufEndPut(void *, int);
extern void viBufFlush(void *);
extern void viBufGetTs(void *, void *);
extern void viBufPutTs(void *, void *);
extern void viBufReset(void *);
extern int viBufRestartDMA(void *);
extern int viBufStopDMA(void *);

#endif /* GAME_MOVIE_VIBUF_H */
