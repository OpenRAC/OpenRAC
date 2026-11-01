/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * rac1/host/shared.c on an array in place of game memory: sections read at their sizes and given
 * upward only, the levels' flags set to the game's 0xFF, a size or a level that does not fit
 * refused, nothing done outside a level.
 */
#include <stdio.h>
#include <string.h>

#include <rac1.h>

#include "../game/rac1/host/shared.c"

uint8_t test_memory[0x200000];
int test_level_running = 1;

static int failures = 0;

#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c); \
            ++failures;                                                  \
        }                                                                \
    } while (0)

static uint8_t* at(uint32_t address) {
    return test_memory + address;
}

static void sizes_read(void) {
    uint8_t out[400];
    memset(test_memory, 0, sizeof test_memory);
    at(0x0013E620u)[3] = 1;
    CHECK(rac1_shared_read(OPENRAC_GAME_SHARED_GOLD_WEAPONS, out, sizeof out) == 40 && out[3] == 1);
    CHECK(rac1_shared_read(OPENRAC_GAME_SHARED_SKILL_POINTS, out, sizeof out) == 32);
    CHECK(rac1_shared_read(OPENRAC_GAME_SHARED_LEVEL_FLAGS, out, sizeof out) == 320);
    CHECK(rac1_shared_read(OPENRAC_GAME_SHARED_SPECIAL_ITEMS, out, sizeof out) == 128);
    CHECK(rac1_shared_read(99, out, sizeof out) == -1);
    CHECK(rac1_shared_read(OPENRAC_GAME_SHARED_GOLD_WEAPONS, out, 39) == -1);
}

static void upward_only(void) {
    uint8_t in[40] = {0};
    memset(test_memory, 0, sizeof test_memory);
    at(0x0013E620u)[1] = 7;
    in[0] = 1;
    in[1] = 1;
    CHECK(rac1_shared_give(OPENRAC_GAME_SHARED_GOLD_WEAPONS, in, sizeof in) == 1);
    CHECK(at(0x0013E620u)[0] == 1);
    /* What it had stays as it was, and a zero takes nothing away. */
    CHECK(at(0x0013E620u)[1] == 7);
    in[0] = 0;
    CHECK(rac1_shared_give(OPENRAC_GAME_SHARED_GOLD_WEAPONS, in, sizeof in) == 1);
    CHECK(at(0x0013E620u)[0] == 1);
}

static void flags_are_the_games(void) {
    uint8_t row[16] = {0};
    uint8_t out[16];
    memset(test_memory, 0, sizeof test_memory);
    row[2] = 1;
    CHECK(rac1_mission_state_write(3, row) == 1);
    /* Level 3's row, the game's 0xFF, and no other level's. */
    CHECK(at(0x0014C150u)[3 * 16 + 2] == 0xFF);
    CHECK(at(0x0014C150u)[2 * 16 + 2] == 0 && at(0x0014C150u)[4 * 16 + 2] == 0);
    CHECK(rac1_mission_state_read(3, out) == 1 && out[2] == 0xFF && out[1] == 0);
    /* The copy the level took (mission_state) is not in here to be touched: only the row moved. */
    uint8_t rows[320] = {0};
    rows[19 * 16 + 15] = 0x80;
    CHECK(rac1_shared_give(OPENRAC_GAME_SHARED_LEVEL_FLAGS, rows, sizeof rows) == 1);
    CHECK(at(0x0014C150u)[19 * 16 + 15] == 0xFF);
}

static void refused(void) {
    uint8_t bytes[400] = {1};
    uint8_t out[16];
    memset(test_memory, 0, sizeof test_memory);
    /* A wrong size is someone else's layout: nothing is guessed. */
    CHECK(rac1_shared_give(OPENRAC_GAME_SHARED_GOLD_WEAPONS, bytes, 39) == 0);
    CHECK(rac1_shared_give(OPENRAC_GAME_SHARED_GOLD_WEAPONS, bytes, 41) == 0);
    CHECK(rac1_shared_give(99, bytes, 40) == 0);
    CHECK(at(0x0013E620u)[0] == 0);
    CHECK(rac1_mission_state_write(-1, bytes) == 0);
    CHECK(rac1_mission_state_write(20, bytes) == 0);
    CHECK(rac1_mission_state_read(20, out) == 0);
    test_level_running = 0;
    CHECK(rac1_shared_give(OPENRAC_GAME_SHARED_GOLD_WEAPONS, bytes, 40) == 0);
    CHECK(rac1_shared_read(OPENRAC_GAME_SHARED_GOLD_WEAPONS, bytes, 40) == -1);
    CHECK(rac1_mission_state_write(1, bytes) == 0);
    test_level_running = 1;
}

int main(void) {
    sizes_read();
    upward_only();
    flags_are_the_games();
    refused();
    if (failures != 0) {
        fprintf(stderr, "%d failed\n", failures);
        return 1;
    }
    return 0;
}
