// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's test harness: CHECK records a failure and goes on; a test
// program returns test_result() from main, so CTest sees the failures.

#pragma once

#include <cstdio>

namespace openrac::test {

inline int& failures() {
    static int count = 0;
    return count;
}

inline int result() {
    if (failures() != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures());
        return 1;
    }
    return 0;
}

}  // namespace openrac::test

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);          \
            ++openrac::test::failures();                                                           \
        }                                                                                          \
    } while (0)
