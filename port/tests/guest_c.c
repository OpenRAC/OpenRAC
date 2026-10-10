/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * guest.h from C, the way translated game code uses it. */
#include "openrac/guest.h"

/* A local whose address the game takes lives in a frame on the game stack;
 * the frame is given back when the function returns. */
gaddr test_frame_address(gaddr* sp_inside) {
    GFRAME(frame, 24);
    GREF(int, frame + 4) = 7;
    *sp_inside = openrac_guest_sp;
    return frame;
}

gaddr test_string(void) {
    return GSTR("ratchet");
}

static int add(int a, int b) {
    return a + b;
}

static const openrac_fn_entry k_functions[] = {
    {0x00100000u, (openrac_host_fn)add, "add"},
};

void test_register(void) {
    openrac_guest_register(OPENRAC_OVERLAY_EXE, k_functions, 1);
}

int test_call(gaddr fn, int a, int b) {
    return GFN(int (*)(int, int), fn)(a, b);
}
