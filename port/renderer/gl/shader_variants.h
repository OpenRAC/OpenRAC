// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exact OpenGL and ES feature variants. Shader owns the logical program; the context registry
// follows uniform writes and binds a constant-only program before each draw or dispatch.
// Vulkan, including MoltenVK, performs this selection in its pipeline backend instead.

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "renderer/gl/gl.h"

namespace openrac::renderer::shader_variants {

// Compiles or loads one exact set of feature values; zero and a diagnostic on failure.
using Compiler = std::function<gl::GLuint(const std::vector<int>&, std::string&)>;

// Context-local registry and wrapped entry points, owned by the GL program cache's state.
// The rendering thread alone uses it; reset forgets names after the old context has gone.
class Context {
public:
    struct State;

    Context();
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    // Forgets the previous context and installs no hooks until a mode shader is registered.
    void reset();

    // Gives internal wrappers their context state; never shared between rendering threads.
    State& state();

private:
    std::unique_ptr<State> m_state;
};

// Records the initial exact variant and precompiles observed combinations when possible.
void register_program(
    gl::GLuint program, std::string source, std::vector<std::string> modes, Compiler compiler
);

// A named integer upload or block binding saved with an observed draw, independent of GL indices.
struct WarmupBinding {
    std::array<char, 64> name{};  // Null-terminated GLSL name, including any array element.
    gl::GLint value = 0;          // Integer uniform value or uniform buffer binding point.
    unsigned block = 0;           // One for a uniform block, zero for an integer uniform.

    // Compares the complete recorded binding when refreshing a saved state.
    bool operator==(const WarmupBinding&) const = default;
};

// Reads a uniform from a native program through the context's GL loader.
using UniformQuery = void(OPENRAC_GLAPI*)(gl::GLuint, gl::GLint, gl::GLint*);

// Captures the effective integer values and explicit block bindings, or -1 if unavailable.
int capture_warmup_bindings(
    gl::GLuint program, UniformQuery read_uniform, std::span<WarmupBinding> bindings
);

// Replaces scalar feature declarations by GLSL integer constants, preserving source line numbers.
std::string specialise(
    std::string source, const std::vector<std::string>& modes, const std::vector<int>& values
);

// Driver-state replay binds concrete program handles without changing logical uniforms.
class ReplayScope {
public:
    ReplayScope();
    ~ReplayScope();
    ReplayScope(const ReplayScope&) = delete;
    ReplayScope& operator=(const ReplayScope&) = delete;

private:
    bool m_previous = false;  // Restored when nested replay ends.
};

// Observed exact programs, cache hits and failed compiles since this context loaded.
struct Counts {
    unsigned selected = 0;  // New exact feature combinations compiled or loaded.
    unsigned reused = 0;    // Selections of already compiled combinations.
    unsigned failed = 0;    // Variants refused by the driver, never drawn with another shader.
};

Counts counts();

// Whether a native or logical handle belongs to a feature group, which must be rebuilt together.
bool owns(gl::GLuint program);

}  // namespace openrac::renderer::shader_variants
