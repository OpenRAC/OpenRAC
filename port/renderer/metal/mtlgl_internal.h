// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mtlgl's files share (mtlgl.h): the context, GL's objects, and the command stream.
//
// The command stream: one command buffer for the frame, its encoders in GL's call order. A texture
// written before anything this frame reads it is written in an upload command buffer that runs
// first, so that the frame's render passes are not cut by its copy.

#pragma once

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <SDL3/SDL_metal.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "renderer/gl.h"
#include "renderer/metal/mtlgl.h"
#include "renderer/world/gl_world.h"

namespace openrac::mtlgl::detail {

using namespace openrac::gl;
namespace wgl = openrac::renderer::world::gl;

inline constexpr GLenum kGlInvalidEnum = 0x0500;
inline constexpr GLenum kGlInvalidValue = 0x0501;
inline constexpr GLenum kGlInvalidOperation = 0x0502;
inline constexpr GLenum kGlRed = 0x1903;
inline constexpr GLenum kGlR8 = 0x8229;
inline constexpr GLenum kGlR32f = 0x822E;
inline constexpr GLenum kGlRg16f = 0x822F;
inline constexpr GLenum kGlRg32f = 0x8230;
inline constexpr GLenum kGlRgba16f = 0x881A;
inline constexpr GLenum kGlR11fG11fB10f = 0x8C3A;
inline constexpr GLenum kGlDepthStencil = 0x84F9;
inline constexpr GLenum kGlNearestMipmapLinear = 0x2702;
inline constexpr GLenum kGlMirroredRepeat = 0x8370;
inline constexpr GLenum kGlUnpackRowLength = 0x0CF2;
inline constexpr GLenum kGlFramebufferBinding = 0x8CA6;
inline constexpr GLenum kGlTextureBinding2d = 0x8069;
inline constexpr GLenum kGlUniformBufferOffsetAlignment = 0x8A34;
inline constexpr GLenum kGlMaxCombinedTextureUnits = 0x8B4D;
inline constexpr GLenum kGlMaxVertexAttribs = 0x8869;
inline constexpr GLenum kGlMaxUniformBlockSize = 0x8A30;

inline constexpr GLuint kUnits = 32;
inline constexpr GLuint kUniformBindings = 16;

// GL names of objects of type T.
template <typename T>
struct Names {
    std::unordered_map<GLuint, T> objects;
    GLuint next = 1;

    GLuint add() {
        objects[next];
        return next++;
    }

    T* find(GLuint name) {
        const auto it = objects.find(name);
        return it == objects.end() ? nullptr : &it->second;
    }
};

struct Format {
    MTLPixelFormat pixel = MTLPixelFormatInvalid;
    int bytes = 0;      // a texel's, or a compressed block's
    int block = 1;      // the side of a compressed block
    bool stencil = false;
};

// The format of GL's internal format `internal`, invalid for one this device has not.
Format format_of(GLenum internal);

struct SamplerParams {
    GLenum min = kGlNearestMipmapLinear, mag = GL_LINEAR;  // GL's defaults
    GLenum wrap_s = GL_REPEAT, wrap_t = GL_REPEAT;
    float anisotropy = 1.0f;
};

// `used` is the serial of the last frame whose commands read or wrote the object (Context::serial).
struct Buffer {
    id<MTLBuffer> buffer;
    std::size_t size = 0;
    std::uint64_t used = 0;
};

struct Texture {
    id<MTLTexture> texture;
    GLenum target = 0, internal = 0;
    SamplerParams params;
    int base_level = 0, max_level = 1000;
    GLuint buffer = 0;  // GL_TEXTURE_BUFFER's buffer, and its texels' format
    GLenum buffer_format = 0;
    std::uint64_t used = 0;
};

struct Renderbuffer {
    id<MTLTexture> texture;
};

struct Framebuffer {
    GLuint colour_texture = 0, colour_renderbuffer = 0;
    GLuint depth_texture = 0, depth_renderbuffer = 0;
};

// The textures a framebuffer draws into (nil for an attachment it has not).
struct Target {
    id<MTLTexture> colour, depth;
};

struct StencilFace {
    GLenum fail = GL_KEEP, depth_fail = GL_KEEP, pass = GL_KEEP;
};

struct State {
    bool blend = false, depth_test = false, cull = false, scissor_test = false;
    bool stencil_test = false, primitive_restart = false, depth_clamp = false;
    bool clip_zero_to_one = false;
    GLenum blend_src_rgb = GL_ONE, blend_dst_rgb = GL_ZERO, blend_src_a = GL_ONE, blend_dst_a = GL_ZERO;
    GLenum blend_equation = GL_FUNC_ADD;
    float blend_colour[4] = {};
    GLenum depth_func = GL_LESS;
    bool depth_mask = true;
    bool colour_mask[4] = {true, true, true, true};
    GLenum cull_face = GL_BACK, front_face = GL_CCW, polygon_mode = GL_FILL;
    GLint viewport[4] = {}, scissor[4] = {};
    float clear_colour[4] = {}, clear_depth = 1.0f;
    GLint clear_stencil = 0;
    GLenum stencil_func = GL_ALWAYS;
    GLint stencil_ref = 0;
    GLuint stencil_value_mask = 0xFFFFFFFF, stencil_write_mask = 0xFFFFFFFF;
    StencilFace stencil_front, stencil_back;
    GLint pack_alignment = 4, unpack_alignment = 4, unpack_row_length = 0;
    GLuint active_unit = 0;
    GLuint unit_2d[kUnits] = {}, unit_buffer[kUnits] = {}, unit_array[kUnits] = {};
    GLuint unit_sampler[kUnits] = {};
    GLuint array_buffer = 0, element_buffer = 0, uniform_buffer = 0, texture_buffer = 0;
    GLuint storage_buffer = 0;
    GLuint uniform_bindings[kUniformBindings] = {}, storage_bindings[kUniformBindings] = {};
    GLuint draw_framebuffer = 0, read_framebuffer = 0, renderbuffer = 0;
};

struct Context {
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    SDL_Window* window = nullptr;
    SDL_MetalView view = nullptr;
    CAMetalLayer* layer = nil;
    id<MTLRenderPipelineState> flip;  // the default framebuffer to the drawable, upside down
    id<MTLTexture> colour, depth;     // the default framebuffer

    id<MTLCommandBuffer> commands, uploads;
    id<MTLRenderCommandEncoder> pass;  // open on pass_target
    Target pass_target;
    id<MTLBlitCommandEncoder> blit, upload_blit;
    std::uint64_t serial = 1;  // the frame's commands'; those before it have smaller ones
    std::atomic<std::uint64_t> completed{0};  // the last serial the GPU has finished

    State state;
    Names<Buffer> buffers;
    Names<Texture> textures;
    Names<SamplerParams> samplers;
    Names<Renderbuffer> renderbuffers;
    Names<Framebuffer> framebuffers;
    GLenum error = GL_NO_ERROR;
    std::string version, renderer;
};

extern Context* g;

void fail(GLenum error);

// The frame's command buffer.
id<MTLCommandBuffer> commands();
// Ends the open encoder, a render pass or a copy.
void end_encoders();
// A copy encoder in the frame's command order.
id<MTLBlitCommandEncoder> blit();
// A copy encoder for writing `texture`: the upload command buffer's while nothing this frame has
// used it, so its render passes stay whole, else blit()'s.
id<MTLBlitCommandEncoder> blit_for(Texture& texture);
// Commits the frame's commands, waiting for the GPU when `wait`.
void submit(bool wait);

// The textures of framebuffer `name`; 0 is the default framebuffer.
Target target_of(GLuint name);
// Opens a render pass on `target`, clearing what `clear` (GL's clear bits) names and keeping the
// rest.
void begin_pass(const Target& target, GLbitfield clear);

// mtlgl_resources.mm's GL functions, for get_proc.
#define MTLGL OPENRAC_GLAPI
void MTLGL read_pixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void* pixels);
void MTLGL copy_tex_sub_image(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y,
                              GLsizei w, GLsizei h);
void MTLGL gen_textures(GLsizei n, GLuint* names);
void MTLGL delete_textures(GLsizei n, const GLuint* names);
void MTLGL bind_texture(GLenum target, GLuint name);
void MTLGL active_texture(GLenum unit);
void MTLGL tex_image_2d(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border,
                        GLenum format, GLenum type, const void* pixels);
void MTLGL compressed_tex_image_2d(GLenum target, GLint level, GLenum internal, GLsizei w, GLsizei h,
                                   GLint border, GLsizei size, const void* pixels);
void MTLGL compressed_tex_sub_image_2d(GLenum target, GLint level, GLint x, GLint y, GLsizei w,
                                       GLsizei h, GLenum internal, GLsizei size, const void* pixels);
void MTLGL tex_sub_image_2d(GLenum target, GLint level, GLint x, GLint y, GLsizei w, GLsizei h,
                            GLenum format, GLenum type, const void* pixels);
void MTLGL tex_image_3d(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLsizei d,
                        GLint border, GLenum format, GLenum type, const void* pixels);
void MTLGL tex_parameter_i(GLenum target, GLenum name, GLint value);
void MTLGL tex_parameter_f(GLenum target, GLenum name, GLfloat value);
void MTLGL generate_mipmap(GLenum target);
void MTLGL tex_buffer(GLenum target, GLenum format, GLuint buffer);
void MTLGL gen_samplers(GLsizei n, GLuint* names);
void MTLGL delete_samplers(GLsizei n, const GLuint* names);
void MTLGL bind_sampler(GLuint unit, GLuint sampler);
void MTLGL sampler_parameter_i(GLuint sampler, GLenum name, GLint value);
void MTLGL gen_buffers(GLsizei n, GLuint* names);
void MTLGL delete_buffers(GLsizei n, const GLuint* names);
void MTLGL bind_buffer(GLenum target, GLuint name);
void MTLGL bind_buffer_base(GLenum target, GLuint index, GLuint buffer);
void MTLGL buffer_data(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void MTLGL buffer_sub_data(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
void MTLGL gen_framebuffers(GLsizei n, GLuint* names);
void MTLGL delete_framebuffers(GLsizei n, const GLuint* names);
void MTLGL bind_framebuffer(GLenum target, GLuint name);
void MTLGL framebuffer_texture_2d(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
                                  GLint level);
void MTLGL framebuffer_renderbuffer(GLenum target, GLenum attachment, GLenum rbtarget,
                                    GLuint renderbuffer);
GLenum MTLGL check_framebuffer_status(GLenum target);
void MTLGL gen_renderbuffers(GLsizei n, GLuint* names);
void MTLGL delete_renderbuffers(GLsizei n, const GLuint* names);
void MTLGL bind_renderbuffer(GLenum target, GLuint name);
void MTLGL renderbuffer_storage(GLenum target, GLenum format, GLsizei w, GLsizei h);
void MTLGL renderbuffer_storage_multisample(GLenum target, GLsizei samples, GLenum format, GLsizei w,
                                            GLsizei h);
void MTLGL blit_framebuffer(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0,
                            GLint dx1, GLint dy1, GLbitfield mask, GLenum filter);
#undef MTLGL

}  // namespace openrac::mtlgl::detail
