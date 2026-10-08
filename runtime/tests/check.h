// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstdio>

// The whole test framework: two macros and a table of functions.

inline int g_failures = 0;
inline int g_checks = 0;

#define CHECK(cond)                                                          \
  do {                                                                       \
    g_checks++;                                                              \
    if (!(cond)) {                                                           \
      g_failures++;                                                          \
      std::printf("  FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
    }                                                                        \
  } while (0)

#define CHECK_EQ(a, b)                                                       \
  do {                                                                       \
    g_checks++;                                                              \
    auto va = (a);                                                           \
    auto vb = (b);                                                           \
    if (!(va == vb)) {                                                       \
      g_failures++;                                                          \
      std::printf("  FAILED %s:%d: %s == %s (%llx vs %llx)\n", __FILE__, __LINE__, #a, #b, \
                  static_cast<unsigned long long>(va), static_cast<unsigned long long>(vb)); \
    }                                                                        \
  } while (0)

struct TestCase {
  const char* name;
  void (*run)();
};

template <std::size_t N>
int run_tests(const TestCase (&tests)[N]) {
  for (const TestCase& t : tests) {
    int before = g_failures;
    t.run();
    std::printf("%s %s\n", g_failures == before ? "ok    " : "FAILED", t.name);
  }
  std::printf("%d checks, %d failed\n", g_checks, g_failures);
  return g_failures ? 1 : 0;
}
