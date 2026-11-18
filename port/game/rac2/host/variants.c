/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: places the decompilation's catalogue (config/levels/functions.tsv) folds into
 * another function by its fingerprint although their code differs from it in a field offset
 * (split_places.tsv). Each is the function it was folded into with the words that differ, read
 * from the retail copies; the places are theirs alone (hostgen, read_split_places).
 */
#include "openrac/guest.h"
#include "rac2.h"

/* func_L00_00424BB8's form in levels 16, 22, 23 and 26 (0x42FB10, 0x425FB8, 0x43B5F0, 0x4178B0):
 * the same test of the record D_L00_001B8E10 (each level's own copy: 0x1B9B50, 0x1B9CD0, 0x1BA690,
 * 0x1B8BD0), but the two non-zero floats go to its object's +0x3C and +0x40 (addiu 32, then +28
 * and +32) instead of +0x144 and +0x148. */
void func_L16_0042FB10(float a, float b) {
    const gaddr rec = GREF(gaddr, OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001B8E10u)));

    if (GREF(int16_t, rec + 0x86) == 0) {
        const gaddr q = GREF(gaddr, rec + 0x70) + 0x20;

        if (a != 0.0f) {
            GREF(float, q + 0x1C) = a;
        }
        if (b != 0.0f) {
            GREF(float, q + 0x20) = b;
        }
    }
}
