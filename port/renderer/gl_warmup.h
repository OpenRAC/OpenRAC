// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL's states warmed up behind the loading screen (gl-state-warmup, docs/port/RENDER_PERF.md).
// An OpenGL driver compiles a program again, in secret, the first time it is drawn in a state its
// machine code depends on: the target's formats and samples, the blend, the colour mask, the vertex
// formats. That compile lands in the middle of a frame, a hitch the program binaries
// (program_cache.h) cannot save. So the draws are watched: each new state a program is drawn in is
// noted, by the program's sources (program_cache::Key) and the state's values, and kept in a file
// per driver beside the program binaries. At a level's load, warm() draws each noted state of each
// program built again, once, one empty triangle into a private 4x4 target of the same formats, and
// gives back every state it changed: the driver makes its variants then. Nothing drawn in the game
// changes. The Vulkan backend has its own (pipelines made ahead, vkgl.h).

#pragma once

#include "renderer/gl.h"
#include "renderer/program_cache.h"

namespace openrac::renderer::gl_warmup {

// After gl::load() and gl_stats::install() on an OpenGL context (not the Vulkan backend): puts a
// watching function in front of the draws and the state they note, and reads the driver's file.
// Once per context.
void install(gl::GetProcAddress get);

// For Shader::build: `program` is the program of sources `key`, until it is deleted.
void note_program(gl::GLuint program, const program_cache::Key& key);

// For a level's load, once its programs are built: draws each noted state of each of them once,
// as above, and saves the file when the run noted new states. Nothing before install().
void warm();

// What the run did, for the tests and the log.
struct Counts {
    int noted = 0;   // States noted since install().
    int warmed = 0;  // Draws warm() made.
};

Counts counts();

}  // namespace openrac::renderer::gl_warmup
