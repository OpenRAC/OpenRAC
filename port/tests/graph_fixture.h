// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Submits synthetic fixture draws and dispatches that have no production pass.
#pragma once

#include <functional>

namespace openrac::test {
void fixture_draw(const std::function<void()>& draw);
void fixture_dispatch(const std::function<void()>& dispatch);

template <class Draw, class... Args>
void graph_draw(Draw draw, Args... args) {
    fixture_draw([&] { draw(args...); });
}

template <class Dispatch, class... Args>
void graph_dispatch(Dispatch dispatch, Args... args) {
    fixture_dispatch([&] { dispatch(args...); });
}
}  // namespace openrac::test
