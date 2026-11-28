// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// The player of what OpenRAC Editor packs with a map, built into Ratchet & Clank's program
// (built_in_mods.cpp): its cutscenes (cutscenes.cpp, OpenRAC Editor's docs/CUTSCENE_FORMAT.md) and the behaviours
// given to its objects (behaviours.cpp, OpenRAC Editor's docs/BEHAVIOUR_FORMAT.md). Both read only the files a
// map's pack holds (openrac_api.pack_folder), so a level without them is left as the game has it.

#pragma once

#include "openrac/openrac_mod.h"

namespace openrac::editor_runtime {

void query(openrac_mod_info* info);

int load(const openrac_api* api, openrac_mod* mod);

}  // namespace openrac::editor_runtime
