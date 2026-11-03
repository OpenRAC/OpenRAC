#ifndef STRUCTS_LEVEL_RECORDS_H
#define STRUCTS_LEVEL_RECORDS_H

#include "common.h"
#include "structs/vector.h"
#include "structs/records.h"

typedef struct {
    unsigned char r, g, b, pad;
    float f[4];
} FogPreset;

struct MusicCdReadMode {
    u8 trycount; /* 0x20 from reset_music */
    u8 spindlctrl;
    u8 datapattern;
    u8 pad;
};

typedef struct {
    float v[4][4];
    int rgba[4];
    float uv[4][2];
    long gs[4];
} QuadPkt;

#endif /* STRUCTS_LEVEL_RECORDS_H */
