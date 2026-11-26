// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <stdexcept>

#include "../game/common/mod_play_queries.h"
#include "check.h"

using openrac::mods::PlayQueries;

struct State {
    PlayQueries* rules;
    int calls = 0;
    bool nested = false;
    bool fail = false;
    int value = 1;
};

void callback(void* user, openrac_play_query* query) {
    auto& state = *static_cast<State*>(user);
    ++state.calls;
    CHECK(query->size == sizeof *query && query->level == 24 && query->index == 3);

    if (state.nested) {
        CHECK(state.rules->query(query->kind, 24, 0, 3, 7, 8) == 7);
    }

    if (state.fail) {
        throw std::runtime_error("fixture");
    }

    query->value = state.value;
    query->original = 999;  // Input mutations cannot affect fallback or another callback.
}

int main() {
    PlayQueries rules;
    auto* a = reinterpret_cast<openrac_mod*>(1);
    auto* b = reinterpret_cast<openrac_mod*>(2);
    State state{&rules};
    CHECK(rules.query(1, 24, 0, 3, 0, 1) == 0);
    CHECK(rules.add(a, 0, callback, &state) == OPENRAC_HOOK_UNKNOWN);
    CHECK(rules.add(a, 256, callback, &state) == OPENRAC_HOOK_UNKNOWN);
    CHECK(rules.add(a, 1, callback, &state) == OPENRAC_HOOK_OK);
    CHECK(rules.add(b, 3, callback, &state) == OPENRAC_HOOK_TAKEN);
    CHECK(rules.query(2, 24, 0, 3, 0, 1) == 0 && state.calls == 0);
    state.nested = true;
    CHECK(rules.query(1, 24, 0, 3, 0, 1) == 1 && state.calls == 1);
    state.value = 2;
    CHECK(rules.query(1, 24, 0, 3, 0, 1) == 0);  // Invalid booleans preserve the game.
    state.fail = true;
    CHECK(rules.query(1, 24, 0, 3, 0, 1) == 0);
    const int calls = state.calls;
    CHECK(rules.query(1, 24, 0, 3, 1, 0) == 1 && state.calls == calls);
    rules.remove(a);
    state.fail = false;
    state.nested = false;
    state.value = 0;
    CHECK(rules.add(b, 1, callback, &state) == OPENRAC_HOOK_OK);
    CHECK(rules.query(1, 24, 0, 3, 1, 0) == 0);
    rules.remove(b);
    CHECK(rules.query(1, 24, 0, 3, 1, 0) == 1);
    state.value = -1;
    CHECK(rules.add(a, 4, callback, &state) == 0);
    CHECK(
        rules.query(4, 24, 0, 3, 500, 1000) == 500
    );  // Invalid reward values cannot remove bolts.
    rules.remove(a);
    return openrac::test::result();
}
