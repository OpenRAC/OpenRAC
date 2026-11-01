/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A stand-in for hostgen's game_protos.h (PAL's names), for unit tests that compile a piece of the
 * shared rac1 host C (port/game/rac1/host) without a disc: only what those pieces call, each
 * defined by the test. */
#ifndef OPENRAC_GAME_PROTOS_H
#define OPENRAC_GAME_PROTOS_H
#include "openrac/guest.h"

/* scene_actors.c */
void func_00118D80(int a0);
int func_0020C468(int a0, int a1);
gaddr func_0020D348(int a0);
void func_00204FC0(gaddr a0);

/* moby_pool.c */
gaddr func_L00_002422D8(int a0);
gaddr func_L00_002422D8__hooked(int a0);
gaddr func_0020E6B8(void);

/* world.c */
void func_001FD3E8(void);
void func_001FD3E8__game(void);
void func_0020D678(gaddr a0);
void func_0020D678__game(gaddr a0);
void func_00213C78(void);
void func_00218B10(void);
void func_002352C8(void);
void func_L00_00235FF8(int a0);
void func_L00_0025AAC0(gaddr a0, gaddr a1);
void func_L00_0025AAC0__game(gaddr a0, gaddr a1);
void func_L00_0025AC00(gaddr a0, float a1, int a2, int a3, gaddr a4, gaddr a5);
void func_L00_0025AC00__game(gaddr a0, float a1, int a2, int a3, gaddr a4, gaddr a5);
void func_L00_00277A88(void);
void func_L00_00277A88__game(void);

/* figure_shots.c */
gaddr func_L00_002CC3C0(gaddr a0, gaddr a1, gaddr a2, float a3, float a4, float a5, float a6);
gaddr func_L00_002CC3C0__game(gaddr a0, gaddr a1, gaddr a2, float a3, float a4, float a5, float a6);
gaddr func_L00_002AE110(int a0, gaddr a1);
void func_L00_002AE278(gaddr a0, gaddr a1, gaddr a2);
void func_L00_002AE278__game(gaddr a0, gaddr a1, gaddr a2);
gaddr func_L00_002B0F58(gaddr a0, gaddr a1, gaddr a2, float a3, float a4, float a5, float a6, float a7);
gaddr func_L00_002B0F58__game(gaddr a0, gaddr a1, gaddr a2, float a3, float a4, float a5, float a6, float a7);
gaddr func_L00_002CE390(int a0, gaddr a1, gaddr a2, float a3, float a4, float a5);
gaddr func_L00_002CE390__game(int a0, gaddr a1, gaddr a2, float a3, float a4, float a5);
void func_L00_002C96D0(gaddr a0, gaddr a1, gaddr a2);
void func_L00_002C96D0__game(gaddr a0, gaddr a1, gaddr a2);
void func_L00_002AB2A8(gaddr a0, gaddr a1, gaddr a2);
void func_L00_002AB2A8__game(gaddr a0, gaddr a1, gaddr a2);
void func_L00_002C4B90(gaddr a0, gaddr a1, gaddr a2);
void func_L00_002C4B90__game(gaddr a0, gaddr a1, gaddr a2);
gaddr func_L00_002C9578(int a0, gaddr a1, gaddr a2);
gaddr func_L00_002AB170(int a0, gaddr a1);
gaddr func_L00_002C48C8(int a0, gaddr a1, gaddr a2);
gaddr func_L00_0026C0D0(gaddr a0, gaddr a1, unsigned char a2, float a3);
void func_L00_0026E438(gaddr a0, gaddr a1, int a2);
void func_L00_002B6FF0(gaddr a0, int a1);
void func_L00_0025BA50(gaddr a0, gaddr a1, gaddr a2, int a3, gaddr a4, int a5, int a6, int a7, float a8, float a9, float a10);
void func_L00_0025BA50__game(gaddr a0, gaddr a1, gaddr a2, int a3, gaddr a4, int a5, int a6, int a7, float a8, float a9, float a10);
int func_L00_00222B80(int a0, int a1);
int func_L00_00222B80__game(int a0, int a1);
int func_L01_0023D688(int a0, int a1);
int func_L01_0023D688__game(int a0, int a1);
int func_L02_0022BE40(int a0, int a1);
int func_L02_0022BE40__game(int a0, int a1);
int func_L03_00216D38(int a0, int a1);
int func_L03_00216D38__game(int a0, int a1);
int func_L05_0024D680(int a0, int a1);
int func_L05_0024D680__game(int a0, int a1);
int func_L06_00235E08(int a0, int a1);
int func_L06_00235E08__game(int a0, int a1);
int func_L08_00231228(int a0, int a1);
int func_L08_00231228__game(int a0, int a1);
int func_L11_0024E240(int a0, int a1);
int func_L11_0024E240__game(int a0, int a1);
int func_L12_002407C0(int a0, int a1);
int func_L12_002407C0__game(int a0, int a1);
int func_L14_002306C8(int a0, int a1);
int func_L14_002306C8__game(int a0, int a1);
int func_L15_00217348(int a0, int a1);
int func_L15_00217348__game(int a0, int a1);
int func_L16_0021EA88(int a0, int a1);
int func_L16_0021EA88__game(int a0, int a1);
int func_L17_0021ED38(int a0, int a1);
int func_L17_0021ED38__game(int a0, int a1);
int func_L18_002284E0(int a0, int a1);
int func_L18_002284E0__game(int a0, int a1);
void func_L00_002110C0(int a0, int a1, gaddr a2);
void func_L00_002110C0__game(int a0, int a1, gaddr a2);
void func_L01_00231960(int a0, int a1, int a2);
void func_L01_00231960__game(int a0, int a1, int a2);
void func_L00_00217718(gaddr a0, gaddr a1, int a2, int a3);
void func_L00_00232C10(int a0, int a1, float a2);
void func_L00_00211908(void);
void func_L00_00211908__game(void);

#endif
