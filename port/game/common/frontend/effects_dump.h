// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Developer tools: snapshots for a diagnostics recording, as JSON, every so many frames. Each has
// the frame's effect strips and quads near Ratchet (what the effects pass draws: the game's draw
// callbacks) with their GS state, bounds and distance, so one drawn where it should not be can be
// traced back to the code that sent it; the particles near him; and the game's own notes
// (GameProfile::diagnostics: for rac1 the moby table's spawn range and the collision queries near
// him).
#pragma once

#include <optional>
#include <string>

#include "renderer/renderer.h"

namespace openrac::frontend::effects_dump {

// Snapshots are taken while armed (a recording runs), every so many frames; each lists the strips
// and quads near Ratchet.
void arm(bool on);

// Whether a recording runs (a game's notes are worth reading).
bool armed();

// The game's own notes for the next snapshot (GameProfile::diagnostics), from the game's thread.
void note(std::string json);

// The game's particles near Ratchet for the next snapshot (particle_quads), as JSON.
void particles(std::string json);

// Render thread: called with the frame's input once its effects are set; `hero` is Ratchet's
// position when the frame has one.
void offer(const renderer::FrameInput& input, const std::optional<renderer::Vec3>& hero);

// The snapshots taken, as one JSON document (empty when none was taken).
std::string latest();

}  // namespace openrac::frontend::effects_dump
