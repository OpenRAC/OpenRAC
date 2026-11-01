// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL programs kept as the driver's own binaries (glGetProgramBinary), so that a program built
// again from the same sources is loaded rather than compiled: each level's renderers build the same
// programs again when the level loads, and each run builds them all at its first level. The
// compile, half a second and more on some drivers, held the frame then.
//
// Kept in memory for the run, and in files for later runs: common/cache_folder.h's folder,
// gl_programs, a folder per driver (its vendor, renderer and version) and a file per program,
// named by a hash of its sources. A program loaded from a binary is the program the driver built
// from those sources (the binary is its own output), so nothing drawn changes. The driver checks a
// binary as it takes it: one it refuses (a driver updated under the same version, a damaged file)
// is forgotten, and the program is compiled and kept again. Files nobody loaded for 30 days go.
//
// OpenGL and OpenGL ES with at least one binary format; the Vulkan backend keeps its own SPIR-V
// and pipeline caches. OPENRAC_GL_PROGRAM_CACHE names another folder, or off for none at all.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "renderer/gl.h"

namespace openrac::renderer::program_cache {

// The key of a program made of these final stage sources (as handed to glShaderSource).
struct Key {
    std::uint64_t name = 0;   // names the file
    std::uint64_t check = 0;  // a second hash, compared on loading
};

Key key(std::string_view vertex, std::string_view fragment);

// True when programs are kept: a GL context with binary formats, and the cache not turned off.
bool enabled();

// A new program loaded from the binary kept under `key` and linked; 0 when there is none, or the
// driver refused it (which forgets it).
gl::GLuint load(const Key& key);

// Before glLinkProgram: asks the driver to keep the program's binary retrievable.
void prepare(gl::GLuint program);

// Keeps linked `program`'s binary under `key`, in memory and in its file.
void store(const Key& key, gl::GLuint program);

// What load() and store() did since the start (the tests, and the log at the first level).
struct Counts {
    int loaded = 0;    // programs made from a binary
    int compiled = 0;  // binaries kept after a compile
    int refused = 0;   // binaries the driver did not take
};

Counts counts();

// Forgets the binaries kept in memory and the driver they belong to, and reads the environment
// again: a test that changes OPENRAC_GL_PROGRAM_CACHE, or a context of another driver.
void reset();

}  // namespace openrac::renderer::program_cache
