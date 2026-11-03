#ifndef GAME_SYSTEM_STASH_H
#define GAME_SYSTEM_STASH_H

/* Functions defined in src/game/system/stash.c. */

#include "common.h"

extern int Stash_SendData(int arg0, int arg1, int arg2, int arg3);
extern void init_stash(void);
extern void initialize_sif_rpc(void);
extern int stash_get_reserved_qwords(unsigned int);
extern void stash_receive_data(int, int, int, int, int);

#endif /* GAME_SYSTEM_STASH_H */
