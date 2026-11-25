/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The native movie-audio wrappers must preserve the integer result read by Going Commando's
 * movie initialization and status callers, even though the native movie player owns the sound. */
#include "../game/rac2/host/sound.c"

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&func_001337F0), int (*)(void)),
    "movie status 4A returns an integer"
);
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&func_00133820), int (*)(void)),
    "movie status 4B returns an integer"
);
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&func_00133850), int (*)(int, int, int, int, int, int)),
    "movie initialization returns an integer"
);

int snd_test_rac2_movie_results(void) {
    return func_001337F0() | func_00133820() | func_00133850(0x400, 0x1000, 0x400, 0, 5, 3);
}
