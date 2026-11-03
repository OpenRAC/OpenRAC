#ifndef STRUCTS_HERO_VIEWS_H
#define STRUCTS_HERO_VIEWS_H

#include "common.h"

typedef struct {
    char p0[0x44];
    int f44;
    int f48;
    char p1[0x10];
    float f5C;
    int f60;
    char p2[0xC];
} HeroIdleFidget;

typedef struct {
    char p0[0x158];
    float f158;
    char p15C[4];
    float f160;
    char p164[4];
    int f168;
    char p16C[0x1E4];
    float f350[4];
} HeroPlayerCameraView;

typedef struct {
    char p0[0x60];
    float f60[4];
    char p70[0x30];
    float fA0[4];
} HeroBoneRow;

typedef struct {
    char p0[0x254];
    float f254;
    float f258;
} HeroBoneScalars;

typedef struct {
    char p0[8];
    float f8;
    float fC;
    char p10[8];
    float f18;
} HeroSpeedBands;

typedef struct {
    char p0[4];
    int f4;
    char p8[4];
    int fC;
    int f10;
    int f14;
    int f18;
    int f1C;
    int f20;
    char p24[8];
} HeroAttackRecord;

typedef struct {
    char p0[0x18];
    int f18;
    char p1C[4];
    int f20;
    int f24;
    char p28[0x24];
} HeroItemDefView;

#endif /* STRUCTS_HERO_VIEWS_H */
