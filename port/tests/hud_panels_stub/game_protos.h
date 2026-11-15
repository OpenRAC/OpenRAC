/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#pragma once
typedef unsigned int gaddr;
int game_FUN_L00_00239810(gaddr element);
int game_FUN_L00_00239810__game(gaddr element);
int game_FUN_001ff960(int group, int frame);
void game_FUN_001ffc30(int frame, int x, int y, int w, int h, int alpha);
void game_FUN_001ffe18(int frame, int x, int y, int w, int h, int alpha);
unsigned long long game_FUN_001ffa10(int frame);
void game_FUN_001f5450(
    int x, int y, int w, int h, int u, int v, int uw, int vh, long long colour, long long texture
);
