#ifndef STRUCTS_DATA_CORE_SOUND_989SND_H
#define STRUCTS_DATA_CORE_SOUND_989SND_H

#include "common.h"

typedef struct {
    volatile unsigned int count;
    int unk04[3];
    volatile int error;
} CdSafeState;

#endif /* STRUCTS_DATA_CORE_SOUND_989SND_H */
