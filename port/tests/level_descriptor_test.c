/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * A synthetic 25-program descriptor exercises host bounds beyond RAC1's retail count.
 * A map changes the displayed name, not the program addresses or the catalog's bounds. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

const openrac_front_end* openrac_game_front_end;
const struct openrac_map_config* openrac_game_map;

int main(void) {
    const char* names[25] = {0};
    names[0] = "First";
    names[19] = "Twentieth";
    names[24] = "Last";
    const openrac_front_end front_end = {.levels = 25, .level_names = names};
    CHECK(!openrac_game_has_level(0));
    openrac_game_front_end = &front_end;
    CHECK(openrac_game_has_level(0));
    CHECK(openrac_game_has_level(19));
    CHECK(openrac_game_has_level(24));
    CHECK(!openrac_game_has_level(25));
    CHECK(!openrac_game_has_level(-1));
    CHECK(!openrac_game_has_level(-2));
    CHECK(!openrac_game_has_level(98));
    CHECK(!openrac_game_has_level(99));
    CHECK(strcmp(openrac_game_level_name(19), "Twentieth") == 0);
    const struct openrac_map_config map = {.name = "Test island", .level = 0};
    openrac_game_map = &map;
    CHECK(strcmp(openrac_game_level_name(0), "Test island") == 0);
    CHECK(strcmp(openrac_game_level_name(19), "Twentieth") == 0);
    openrac_game_map = NULL;
    CHECK(strcmp(openrac_game_level_name(0), "First") == 0);
    CHECK(strcmp(openrac_game_level_name(25), "boot") == 0);
    return 0;
}
