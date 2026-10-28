// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Developer tools: a snapshot of the frame's effect strips and quads (what the effects pass draws:
// the game's draw callbacks), as JSON, for a diagnostics recording. Each item carries its GS state,
// its bounds and how far it is from Ratchet, so one drawn where it should not be can be told apart
// and traced back to the code that sent it.
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

// The game's own notes for the next snapshot (GameProfile::diagnostics), from the tick's thread.
void note(std::string json);

// The game's particles near Ratchet for the next snapshot (particle_quads), as JSON.
void particles(std::string json);

// Render thread: called with the frame's input once its effects are set; `hero` is Ratchet's
// position when the frame has one.
void offer(const renderer::FrameInput& input, const std::optional<renderer::Vec3>& hero);

// The snapshots taken, as one JSON document (empty when none was taken).
std::string latest();

}  // namespace openrac::frontend::effects_dump
