/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Synthetic declarations for compiling the actual RAC1 letterbox wrapper as C in the unit test.
 * No translated game body or generated table is reproduced. All mapped words live in the fixture.
 */
#ifndef OPENRAC_TEST_RAC1_LETTERBOX_PROTOS_H
#define OPENRAC_TEST_RAC1_LETTERBOX_PROTOS_H

#include "openrac/guest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maps the wrapper's symbolic data to the fixture's readable words. */
gaddr openrac_test_data_address(gaddr canonical);
/* The real production wrapper, forwarding the original operation once. */
void func_001F5148(void);
/* The synthetic original operation; its side effects are owned by the test. */
void func_001F5148__game(void);

#ifdef __cplusplus
}
#endif

/* The generated header supplies this mapping in a game; here the fixture owns every word. */
#define OPENRAC_DATA(address) openrac_test_data_address((gaddr)(address))

#endif /* OPENRAC_TEST_RAC1_LETTERBOX_PROTOS_H */
