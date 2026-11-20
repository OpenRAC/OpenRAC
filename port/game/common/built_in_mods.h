// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods built into a game's program (mods.h, BuiltIn): port/game/<game>/built_in_mods.cpp, or
// built_in_mods_none.cpp for a game with none. Programs with a window only.

#pragma once

#include <vector>

#include "mods.h"

namespace openrac::mods {

std::vector<BuiltIn> built_in_mods();

}  // namespace openrac::mods
