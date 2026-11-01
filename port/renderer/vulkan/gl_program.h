// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private GL shader/program entry points shared by the dispatch table and internal blits.
// Resource ownership and reflection stay inside the Vulkan backend.
#pragma once

#include "renderer/gl.h"

namespace openrac::vkgl::detail {

// Creates an owned stage whose source is supplied through the GL table.
gl::GLuint OPENRAC_GLAPI create_shader(gl::GLenum type);

// Keeps stage text until program linking compiles the complete interface.
void OPENRAC_GLAPI shader_source(
    gl::GLuint name, gl::GLsizei count, const gl::GLchar* const* strings, const gl::GLint* lengths
);

// Checks one stage and retains diagnostics for GL shader queries.
void OPENRAC_GLAPI compile_shader(gl::GLuint name);

// Reports stage compilation status and diagnostic length.
void OPENRAC_GLAPI get_shader_iv(gl::GLuint name, gl::GLenum what, gl::GLint* out);

// Copies stage diagnostics into the caller's bounded GL buffer.
void OPENRAC_GLAPI
get_shader_info_log(gl::GLuint name, gl::GLsizei size, gl::GLsizei* length, gl::GLchar* log);

// Releases the stage text after its owning program has linked.
void OPENRAC_GLAPI delete_shader(gl::GLuint name);

// Creates an owned program before attaching its stages.
gl::GLuint OPENRAC_GLAPI create_program();

// Associates an existing stage with the program that will link it.
void OPENRAC_GLAPI attach_shader(gl::GLuint program, gl::GLuint shader);

// Accepts GL attribute bindings; shaders supply explicit locations.
void OPENRAC_GLAPI bind_attrib_location(gl::GLuint, gl::GLuint, const gl::GLchar*);

// Builds the executable only after every driver prerequisite succeeds.
void OPENRAC_GLAPI link_program(gl::GLuint name);

// Reports link status and diagnostic length through the GL table.
void OPENRAC_GLAPI get_program_iv(gl::GLuint name, gl::GLenum what, gl::GLint* out);

// Copies link diagnostics into the caller's bounded GL buffer.
void OPENRAC_GLAPI
get_program_info_log(gl::GLuint name, gl::GLsizei size, gl::GLsizei* length, gl::GLchar* log);

// Retires linked resources after the frames that can reference them.
void OPENRAC_GLAPI delete_program(gl::GLuint name);

// Selects the program that subsequent draws and uniform writes consume.
void OPENRAC_GLAPI use_program(gl::GLuint name);

// Encodes the reflected uniform and array element in a GL location.
gl::GLint OPENRAC_GLAPI get_uniform_location(gl::GLuint program, const gl::GLchar* name);

// Writes an integer uniform or selects a sampler's texture unit.
void OPENRAC_GLAPI uniform_1i(gl::GLint location, gl::GLint v);

// Writes an unsigned integer into the reflected default block.
void OPENRAC_GLAPI uniform_1ui(gl::GLint location, gl::GLuint v);

// Writes one float into the reflected default block.
void OPENRAC_GLAPI uniform_1f(gl::GLint location, gl::GLfloat v);

// Writes two floats into the reflected default block.
void OPENRAC_GLAPI uniform_2f(gl::GLint location, gl::GLfloat a, gl::GLfloat b);

// Writes three floats into the reflected default block.
void OPENRAC_GLAPI uniform_3f(gl::GLint location, gl::GLfloat a, gl::GLfloat b, gl::GLfloat c);

// Writes four floats into the reflected default block.
void OPENRAC_GLAPI
uniform_4f(gl::GLint location, gl::GLfloat a, gl::GLfloat b, gl::GLfloat c, gl::GLfloat d);

// Writes vector array elements using the reflected stride.
void OPENRAC_GLAPI uniform_4fv(gl::GLint location, gl::GLsizei count, const gl::GLfloat* v);

// Writes matrix array elements, transposing when requested by GL.
void OPENRAC_GLAPI uniform_matrix_4fv(
    gl::GLint location, gl::GLsizei count, gl::GLboolean transpose, const gl::GLfloat* v
);

// Finds a named reflected block or returns the GL invalid index.
gl::GLuint OPENRAC_GLAPI get_uniform_block_index(gl::GLuint program, const gl::GLchar* name);

// Assigns the GL buffer binding consumed by a reflected block.
void OPENRAC_GLAPI uniform_block_binding(gl::GLuint program, gl::GLuint block, gl::GLuint binding);

}  // namespace openrac::vkgl::detail
