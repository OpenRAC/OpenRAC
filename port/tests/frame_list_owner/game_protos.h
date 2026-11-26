/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Synthetic declarations for compiling the actual RAC1 ownership wrappers as C in the unit test.
 * No translated game body or generated table is reproduced. All mapped words live in the fixture.
 */
#ifndef OPENRAC_TEST_FRAME_LIST_PROTOS_H
#define OPENRAC_TEST_FRAME_LIST_PROTOS_H

#include "openrac/guest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maps the wrapper's symbolic datum to the fixture's invented readable cursor word. */
gaddr openrac_test_data_address(gaddr canonical);
/* The two real production wrappers, each forwarding the original operation once. */
void func_002348E8(void);
void func_00234948(void);
/* Synthetic original operations; their side effects are owned by the test, not copied from retail. */
void func_002348E8__game(void);
void func_00234948__game(void);

#ifdef __cplusplus
}
#endif

/* The generated header supplies this mapping in a game; this fixture never uses a game address. */
#define OPENRAC_DATA(address) openrac_test_data_address((gaddr)(address))

#endif /* OPENRAC_TEST_FRAME_LIST_PROTOS_H */
