/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Entries of Going Commando's hand-written assembly that are not functions of their own, and the one
 * that cannot run as C, bound by libraries.tsv "host" rows. Each reports when it is reached.
 *
 * libgcc's setjmp/longjmp exception support (_eh.o, the unwinder's __throw and __rethrow): the
 * catalogue lists the landing points of __builtin_setjmp inside these routines as functions, but
 * they are entered only by a longjmp from their own routine, with its frame registers set. __sjthrow
 * itself ends in a longjmp to a guest handler, which C has no form for; nothing in the game throws a
 * C++ exception, so none of this is reached.
 *
 * func_00130004 is the shared epilogue of libmpeg's func_0012FEC0, whose decompiled C already
 * contains it. */
#include "openrac/guest.h"

static void reached(openrac_reach_site* site) {
    openrac_guest_reach(site);
}

void func_00120528(void) {
    static openrac_reach_site site =
        {"func_00120528",
         "unreachable",
         "__sjthrow: a C++ exception throw, which longjmps to a guest handler",
         0,
         0};
    reached(&site);
}

void func_001205E0(void) {
    static openrac_reach_site site =
        {"func_001205E0",
         "unreachable",
         "a fragment of func_00120528, entered only from its assembly",
         0,
         0};
    reached(&site);
}

void func_00120748(void) {
    static openrac_reach_site site =
        {"func_00120748",
         "unreachable",
         "a fragment of __sjpopnthrow, entered only from its assembly",
         0,
         0};
    reached(&site);
}

void func_00120FA4(void) {
    static openrac_reach_site site =
        {"func_00120FA4",
         "unreachable",
         "a fragment of __throw, entered only from its assembly",
         0,
         0};
    reached(&site);
}

void func_0012118C(void) {
    static openrac_reach_site site =
        {"func_0012118C",
         "unreachable",
         "a fragment of __rethrow, entered only from its assembly",
         0,
         0};
    reached(&site);
}

void func_00130004(void) {
    static openrac_reach_site site =
        {"func_00130004",
         "unreachable",
         "a fragment of func_0012FEC0, entered only from its assembly",
         0,
         0};
    reached(&site);
}
