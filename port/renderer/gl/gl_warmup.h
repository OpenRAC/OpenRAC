// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL state warm-up (gl-state-warmup, docs/port/RENDER_PERF.md). A GL driver silently
// recompiles a program the first time it is drawn with new formats, blend, colour mask or vertex
// formats. We save each such state per driver and draw it once at level load, into a private 4x4
// target, so the recompile happens behind the loading screen. Not used on Vulkan.

#pragma once

#include "renderer/gl/gl.h"
#include "renderer/gl/program_cache.h"

namespace openrac::renderer::gl_warmup {

// Call once after gl::load() and gl_stats::install() on OpenGL (not Vulkan). Wraps the draw and
// state calls and loads the driver's file.
void install(gl::GetProcAddress get);

// Called by Shader::build: `program` was built from `key`.
void note_program(gl::GLuint program, const program_cache::Key& key);

// Called at level load: draws each saved state of each built program once, and saves the file
// if there are new states. Does nothing before install().
void warm();

// What the run did, for the tests and the log.
struct Counts {
    int noted = 0;   // States noted since install().
    int warmed = 0;  // Draws warm() made.
};

Counts counts();

}  // namespace openrac::renderer::gl_warmup
