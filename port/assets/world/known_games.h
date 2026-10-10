// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Which games' world data the readers of assets/world know. Every layout here
// is Ratchet & Clank's (rac1); the sequels lay their levels out differently
// (other gameplay section orders, chunked collision, other record sizes) and
// are not known yet, so a reader asked for one of them fails instead of
// guessing.

#pragma once

#include <string_view>

#include "assets/version.h"

namespace openrac::assets {

// True when assets/world knows `game`'s layouts (rac1 only).
bool world_layout_known(Game game);

// Throws AssetError naming `what` unless `game`'s layout of it is known.
void require_world_layout(Game game, std::string_view what);

}  // namespace openrac::assets
