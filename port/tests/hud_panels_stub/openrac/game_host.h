/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#pragma once
#include "game_protos.h"
void* test_memory(gaddr address);
gaddr test_level_data(int level, gaddr address);
const unsigned char* openrac_game_fix(const char* key);
#define GREF(type, address) (*(type*)test_memory(address))
#define OPENRAC_LDATA(level, address) test_level_data(level, address)
