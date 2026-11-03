#ifndef GAME_SYSTEM_MEMCARD_H
#define GAME_SYSTEM_MEMCARD_H

/* Functions defined in src/game/system/memcard.c. */

#include "common.h"

extern int check_memory_card(void);
extern void load_and_initialize_level_chunk(void);
extern void memcard_Init(void);
extern void memcard_MakeWholeSave(void *);
extern void memcard_RestoreGame(int);
extern int memcard_Save(int, int);
extern void memcard_get_name(void *);
extern void memcard_update_state(void);

#endif /* GAME_SYSTEM_MEMCARD_H */
