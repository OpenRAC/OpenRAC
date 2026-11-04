// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// env.h's values stay readable in exit handlers (common/env.h, env::value). main registers
// program::finish with std::atexit before anything reads OPENRAC_STATE_DUMP, and finish writes the
// last state dump through that value. A handler registered before an object of static storage is
// made runs after that object's destructor, so a static copy of the path, or a snapshot destroyed
// at exit, left finish reading a freed string (on the Mac: "cannot write" and a garbage name).
// This test registers its handler first, reads the variable after, and checks the value from the
// handler once the heap has been churned over the freed blocks.

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "check.h"
#include "common/env.h"

namespace {

namespace env = openrac::env;

// Longer than any short-string buffer, so the value lives on the heap.
constexpr std::string_view kPath = "/scratch/openrac-env-at-exit-test/a-state-dump-path.json";

// Blocks allocated in the handler, of the value's size, to take over a freed buffer.
constexpr int kChurnBlocks = 256;

// The view read in main, checked at exit.
std::optional<std::string_view> g_read;

void set_variable(const char* name, std::string_view value) {
    const std::string text(value);
#ifdef _WIN32
    CHECK(_putenv_s(name, text.c_str()) == 0);
#else
    CHECK(setenv(name, text.c_str(), 1) == 0);
#endif
}

// The exit handler: the value read in main is still the variable's, and reading it again gives the
// same. A failure ends the program with 1, which is what CTest sees from a handler.
void check_at_exit() {
    std::vector<std::string> churn;
    churn.reserve(kChurnBlocks);
    for (int i = 0; i < kChurnBlocks; ++i) {
        churn.emplace_back(kPath.size(), '#');
    }

    const std::optional<std::string_view> again = env::value(env::Var::StateDump);
    const bool kept = g_read && *g_read == kPath;
    const bool read_again = again && *again == kPath;

    if (!kept || !read_again) {
        std::fprintf(
            stderr, "env.h's value was gone at exit: kept %d, read again %d\n", kept, read_again
        );
        std::_Exit(1);
    }
}

}  // namespace

int main() {
    set_variable("OPENRAC_STATE_DUMP", kPath);
    // First the handler, then the first read of the environment, as the program's main does.
    CHECK(std::atexit(check_at_exit) == 0);
    g_read = env::value(env::Var::StateDump);
    CHECK(g_read && *g_read == kPath);
    return openrac::test::result();
}
