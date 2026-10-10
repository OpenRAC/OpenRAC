/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1-pal/hand/game_joint_matrices.c"
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

unsigned char joint_marks[256], joint_scratch[128 * 64];
static unsigned char moby[0x40], model[0x40], chains_data[4][12];
static unsigned char* chains[5];
static unsigned char expected_marks[256];
static int evaluations;

void joint_evaluate(void* m, void* marks) {
    int i, j;
    CHECK(m == moby && marks == joint_marks);
    CHECK(memcmp(joint_marks, expected_marks, sizeof(joint_marks)) == 0);
    ++evaluations;
    /* Distinct bytes in every matrix lane catch ordering, stride and partial
     * copy errors. This tests selection independently of the pose evaluator. */
    for (i = 0; i < 128; ++i) {
        for (j = 0; j < 64; ++j) {
            joint_scratch[i * 64 + j] = (unsigned char)(i * 13 + j);
        }
    }
}

static void setup(void) {
    int i;
    memset(joint_marks, 0xCC, sizeof(joint_marks));
    memset(expected_marks, 0xCC, sizeof(expected_marks));
    memset(expected_marks, 0, 128);
    memset(chains_data, 0, sizeof(chains_data));
    *(unsigned char**)(moby + 0x24) = model;
    *(unsigned char***)(model + 0x1C) = chains;
    chains[0] = NULL; /* the table's header is not a chain */
    for (i = 0; i < 4; ++i) {
        chains[i + 1] = chains_data[i];
    }
    evaluations = 0;
}

static void check_output(unsigned char* out, const int* terminals, int count) {
    int i, j;
    for (i = 0; i < count; ++i) {
        for (j = 0; j < 64; ++j) {
            CHECK(out[i * 64 + j] == (unsigned char)(terminals[i] * 13 + j));
        }
    }
    CHECK(evaluations == 1);
}

int main(void) {
    unsigned char buffer[5 * 64 + 2];
    int ids[] = {2, 0, 2, 1, 3}, terminals[] = {7, 3, 7, 5, 0};
    int i;
    setup();
    *(unsigned short*)chains_data[0] = 2;
    chains_data[0][4] = 1;
    chains_data[0][5] = 3;
    *(unsigned short*)chains_data[1] = 3;
    chains_data[1][4] = 1;
    chains_data[1][5] = 4;
    chains_data[1][6] = 5;
    *(unsigned short*)chains_data[2] = 3;
    chains_data[2][4] = 1;
    chains_data[2][5] = 3;
    chains_data[2][6] = 7;
    *(unsigned short*)chains_data[3] = 1;
    chains_data[3][4] = 0;
    expected_marks[0] = expected_marks[1] = expected_marks[3] = 1;
    expected_marks[4] = expected_marks[5] = 1;
    expected_marks[7] = 255;
    expected_marks[127] = 8;
    for (i = 0; i < 5; ++i) {
        expected_marks[128 + i] = (unsigned char)terminals[i];
    }
    memset(buffer, 0xA5, sizeof(buffer));
    func_002116A0(moby, 5, ids, buffer + 1);
    check_output(buffer + 1, terminals, 5);
    CHECK(buffer[0] == 0xA5 && buffer[sizeof(buffer) - 1] == 0xA5);

    /* Retail computes its bound from the last joint of each chain, even when
     * synthetic data gives it an earlier dependency above that terminal. */
    setup();
    ids[0] = 0;
    terminals[0] = 4;
    *(unsigned short*)chains_data[0] = 2;
    chains_data[0][4] = 12;
    chains_data[0][5] = 4;
    expected_marks[12] = 1;
    expected_marks[4] = 255;
    expected_marks[127] = 5;
    expected_marks[128] = 4;
    func_002116A0(moby, 1, ids, buffer);
    check_output(buffer, terminals, 1);

    /* A zero count still executes one iteration in the handwritten routine. */
    setup();
    terminals[0] = 0;
    expected_marks[0] = 255;
    expected_marks[127] = 1;
    expected_marks[128] = 0;
    func_002116A0(moby, 0, ids, buffer);
    check_output(buffer, terminals, 1);
    puts("joint matrix selection tests passed");
    return 0;
}
