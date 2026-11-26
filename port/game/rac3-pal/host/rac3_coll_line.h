/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * coll_line with its gather bounded to the scratchpad (coll_line.c): the host C of each copy's wrap
 * row calls this with the copy's own translation. */
#ifndef RAC3_COLL_LINE_H
#define RAC3_COLL_LINE_H

typedef int (*rac3_coll_line_fn)(int from, int to, int flags, int moby, int x);

int rac3_coll_line(rac3_coll_line_fn query, int from, int to, int flags, int moby, int x);

#endif
