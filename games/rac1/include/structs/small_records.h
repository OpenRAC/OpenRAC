#ifndef STRUCTS_SMALL_RECORDS_H
#define STRUCTS_SMALL_RECORDS_H

#include "common.h"

typedef struct {
    int a;
    int b;
} IntPair;

typedef struct {
    float u;
    float v;
} QuadUv;

typedef struct {
    float f[4];
} UvScrollFactors;

typedef struct {
    int v[6];
} IntVec6;

#endif /* STRUCTS_SMALL_RECORDS_H */
