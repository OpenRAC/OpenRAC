/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * libvu0 routines of Up Your Arsenal's engine core that its C writes with VU0 inline assembly. */
#include "openrac/guest.h"

/* sceVu0UnitMatrix (0x0012FF40, the instructions of rac1-pal's func_00125358): m = the identity. */
void func_0012FF40(gaddr m) {
    float* r = (float*)G(m);
    for (int i = 0; i < 16; ++i) {
        r[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}
