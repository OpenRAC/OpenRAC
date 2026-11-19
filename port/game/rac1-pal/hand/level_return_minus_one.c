/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* coll_return_minus_one (8 bytes in every level program: jr $ra with addiu $v0, $zero, -1 in its delay
 * slot). The collision queries and the bitmap run-length encoder (func_L00_002003C8's candidate) reach
 * it by a tail call for their -1 result. The decompilation keeps it as hand-written assembly. Its
 * candidate C passed fcheck (port/game/rac1-pal/checks/fcheck.tsv, 2026-10-18) and then left functional.tsv, so the
 * port ran a stub that returned 0 there. hostgen's assembly guard found it. */
int func_L00_001F3980(void) {
    return -1;
}
