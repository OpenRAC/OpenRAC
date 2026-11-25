// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// The games' menu-page ABI remains linkable in a build with no windowed frontend.
// There are no rendered mod pages in that build, so it exposes an empty provider.
#ifndef OPENRAC_FRONTEND
#include "openrac/game_host.h"

const openrac_mod_pages_host* openrac_game_mod_pages = nullptr;

int openrac_mod_pages_items(int) {
    return 0;
}

int openrac_mod_pages_label(int, int, char* out, int capacity) {
    if (out != nullptr && capacity > 0) {
        out[0] = '\0';
    }

    return 0;
}

int openrac_mod_pages_after(int) {
    return 2;
}

int openrac_mod_pages_view_of(openrac_mod_pages_view*) {
    return 0;
}
#endif
