/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The games' printf conversions (game/common/lib/guest_format.c) on a small game memory: the
 * conversions the menus and message boxes use, and a %s argument that is no game address. Up Your
 * Arsenal's message box (func_L01_004CDF20) once reached sprintf with leftovers where a string
 * should be, and the 0xFFFFFFC0 it read took the host down. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/guest.h"

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "guest format test failed at line %d: %s\n", __LINE__, #condition);    \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

/* The game's memory: the console's 32 MiB, of which the test writes the first kilobyte. */
static uint8_t* memory;
static uint8_t scratchpad[GUEST_SCRATCHPAD_SIZE];

static void* test_guest(gaddr address) {
    if (address >= GUEST_SCRATCHPAD && address - GUEST_SCRATCHPAD < GUEST_SCRATCHPAD_SIZE) {
        return scratchpad + (address - GUEST_SCRATCHPAD);
    }
    CHECK(address < GUEST_RAM_SIZE);
    return memory + address;
}

#undef G
#define G(address) test_guest((gaddr)(address))
#include "../game/common/lib/guest_format.c"

static int format(char* out, size_t size, const char* f, ...) {
    va_list args;
    int n;

    va_start(args, f);
    n = openrac_guest_vformat(out, size, f, args);
    va_end(args);
    return n;
}

int main(void) {
    char out[64];

    memory = calloc(GUEST_RAM_SIZE, 1);
    CHECK(memory != NULL);
    openrac_guest_ram_size = GUEST_RAM_SIZE;
    strcpy((char*)memory + 0x100, "Yes");

    CHECK(format(out, sizeof out, "%s%c%s%c%d", 0x100, '-', 0x100, ' ', 42) == 10);
    CHECK(strcmp(out, "Yes-Yes 42") == 0);
    CHECK(format(out, sizeof out, "[%5.2f|%-4d|%04x|%%]", 1.5, 7, 0xAB) == 19);
    CHECK(strcmp(out, "[ 1.50|7   |00ab|%]") == 0);

    /* The same string by its uncached alias. */
    CHECK(format(out, sizeof out, "%s", GUEST_RAM_UNCACHED + 0x100) == 3);
    CHECK(strcmp(out, "Yes") == 0);

    /* No string: 0, and addresses that are no memory (the leftover that crashed). */
    CHECK(format(out, sizeof out, "<%s>", 0) == 8);
    CHECK(strcmp(out, "<(null)>") == 0);
    CHECK(format(out, sizeof out, "<%s>", (int)0xFFFFFFC0u) == 8);
    CHECK(strcmp(out, "<(null)>") == 0);
    CHECK(format(out, sizeof out, "<%s>", (int)0x7FFFFFF0) == 8);
    CHECK(strcmp(out, "<(null)>") == 0);

    /* Text in the scratchpad, where the save notice of Up Your Arsenal's message box is built. */
    strcpy((char*)scratchpad + 0x10, "Progress is saving.");
    CHECK(format(out, sizeof out, "%s%c%s", GUEST_SCRATCHPAD + 0x10, '\n', 0x100) == 23);
    CHECK(strcmp(out, "Progress is saving.\nYes") == 0);

    /* The output is cut to the buffer and terminated. */
    CHECK(format(out, 5, "%s%s", 0x100, 0x100) == 4);
    CHECK(strcmp(out, "YesY") == 0);
    free(memory);
    return 0;
}
