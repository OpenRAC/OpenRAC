// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The context, the command stream (mtlgl_internal.h), state, and the present. Resources are
// mtlgl_resources.mm's.

#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <utility>
#include <vector>

#include "renderer/metal/mtlgl_internal.h"

namespace openrac::mtlgl::detail {

Context* g = nullptr;

void fail(GLenum error) {
    if (g->error == GL_NO_ERROR) {
        g->error = error;
    }
}

id<MTLCommandBuffer> commands() {
    if (g->commands == nil) {
        g->commands = [g->queue commandBuffer];
    }
    return g->commands;
}

void end_encoders() {
    [g->pass endEncoding];
    g->pass = nil;
    g->pass_target = {};
    [g->blit endEncoding];
    g->blit = nil;
}

id<MTLBlitCommandEncoder> blit() {
    if (g->blit == nil) {
        end_encoders();
        g->blit = [commands() blitCommandEncoder];
    }
    return g->blit;
}

id<MTLBlitCommandEncoder> blit_for(Texture& texture) {
    if (texture.used == g->serial) {
        return blit();
    }
    if (g->uploads == nil) {
        g->uploads = [g->queue commandBuffer];
        // Its place on the queue is taken now, before the frame's command buffer, which takes
        // its own at commit.
        [g->uploads enqueue];
        g->upload_blit = [g->uploads blitCommandEncoder];
    }
    return g->upload_blit;
}

void submit(bool wait) {
    end_encoders();
    if (g->uploads != nil) {
        [g->upload_blit endEncoding];
        g->upload_blit = nil;
        [g->uploads commit];
        g->uploads = nil;
    }
    id<MTLCommandBuffer> frame = commands();
    Context* context = g;
    const std::uint64_t serial = g->serial++;
    [frame addCompletedHandler:^(id<MTLCommandBuffer>) {
      context->completed.store(serial, std::memory_order_release);
    }];
    [frame commit];
    if (wait) {
        [frame waitUntilCompleted];
    }
    g->commands = nil;
}

Target target_of(GLuint name) {
    if (name == 0) {
        return {g->colour, g->depth};
    }
    const Framebuffer* fb = g->framebuffers.find(name);
    if (fb == nullptr) {
        return {};
    }
    auto image = [](GLuint texture, GLuint renderbuffer) -> id<MTLTexture> {
        // Counted as used now, so that a later upload to it keeps its place after the pass.
        if (Texture* t = g->textures.find(texture)) {
            t->used = g->serial;
            return t->texture;
        }
        const Renderbuffer* r = g->renderbuffers.find(renderbuffer);
        return r != nullptr ? r->texture : nil;
    };
    return {image(fb->colour_texture, fb->colour_renderbuffer),
            image(fb->depth_texture, fb->depth_renderbuffer)};
}

void begin_pass(const Target& target, GLbitfield clear) {
    end_encoders();
    const State& s = g->state;
    MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
    if (target.colour != nil) {
        d.colorAttachments[0].texture = target.colour;
        d.colorAttachments[0].loadAction =
            (clear & GL_COLOR_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
        d.colorAttachments[0].clearColor = MTLClearColorMake(
            s.clear_colour[0], s.clear_colour[1], s.clear_colour[2], s.clear_colour[3]);
    }
    if (target.depth != nil) {
        d.depthAttachment.texture = target.depth;
        d.depthAttachment.loadAction =
            (clear & GL_DEPTH_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
        d.depthAttachment.clearDepth = s.clear_depth;
        if (target.depth.pixelFormat == MTLPixelFormatDepth32Float_Stencil8) {
            d.stencilAttachment.texture = target.depth;
            d.stencilAttachment.loadAction =
                (clear & GL_STENCIL_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
            d.stencilAttachment.clearStencil = static_cast<std::uint32_t>(s.clear_stencil) & 0xFF;
        }
    }
    const bool multisampled = (target.colour != nil ? target.colour : target.depth).sampleCount > 1;
    if (multisampled) {
        d.colorAttachments[0].storeAction = MTLStoreActionStore;
    }
    g->pass = [commands() renderCommandEncoderWithDescriptor:d];
    g->pass_target = target;
}

namespace {

constexpr const char* kFlipSource = R"(
#include <metal_stdlib>
using namespace metal;
struct Out { float4 position [[position]]; float2 uv; };
vertex Out flip_vertex(uint id [[vertex_id]]) {
    const float2 p = float2((id << 1) & 2, id & 2);
    return {float4(p * 2.0 - 1.0, 0.0, 1.0), p};
}
fragment float4 flip_fragment(Out in [[stage_in]], texture2d<float> frame [[texture(0)]]) {
    constexpr sampler nearest(filter::nearest);
    return frame.sample(nearest, in.uv);
}
)";

void make_default_framebuffer(int width, int height) {
    MTLTextureDescriptor* d = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:static_cast<NSUInteger>(width)
                                    height:static_cast<NSUInteger>(height)
                                 mipmapped:NO];
    d.storageMode = MTLStorageModePrivate;
    d.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    g->colour = [g->device newTextureWithDescriptor:d];
    d.pixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    d.usage = MTLTextureUsageRenderTarget;
    g->depth = [g->device newTextureWithDescriptor:d];
}

// The compressed formats this device samples, by GL's extension names.
std::vector<const char*> extensions() {
    std::vector<const char*> out;
    if (g->device.supportsBCTextureCompression) {
        out.push_back("GL_ARB_texture_compression_bptc");
    }
    if ([g->device supportsFamily:MTLGPUFamilyApple2]) {
        out.push_back("GL_KHR_texture_compression_astc_ldr");
    }
    if ([g->device supportsFamily:MTLGPUFamilyApple6]) {
        out.push_back("GL_KHR_texture_compression_astc_hdr");
    }
    return out;
}

// ---- The GL functions ----

#define MTLGL OPENRAC_GLAPI

const GLubyte* MTLGL get_string(GLenum name) {
    switch (name) {
        case GL_VENDOR:
            return reinterpret_cast<const GLubyte*>("OpenRAC");
        case GL_RENDERER:
            return reinterpret_cast<const GLubyte*>(g->renderer.c_str());
        case GL_VERSION:
            return reinterpret_cast<const GLubyte*>(g->version.c_str());
        case GL_SHADING_LANGUAGE_VERSION:
            return reinterpret_cast<const GLubyte*>("4.10");
        default:
            fail(kGlInvalidEnum);
            return nullptr;
    }
}

const GLubyte* MTLGL get_stringi(GLenum, GLuint index) {
    const auto names = extensions();
    return index < names.size() ? reinterpret_cast<const GLubyte*>(names[index]) : nullptr;
}

void MTLGL get_integerv(GLenum name, GLint* data) {
    switch (name) {
        // MTLGPUFamilyMac2's largest 2D texture (Apple's Metal feature set tables).
        case GL_MAX_TEXTURE_SIZE:
            *data = 16384;
            return;
        case GL_SAMPLES: {
            const Target t = target_of(g->state.draw_framebuffer);
            id<MTLTexture> any = t.colour != nil ? t.colour : t.depth;
            *data = any != nil ? static_cast<GLint>(any.sampleCount) : 1;
            return;
        }
        case GL_MAX_SAMPLES:
            *data = 1;
            for (const GLint count : {8, 4, 2}) {
                if ([g->device supportsTextureSampleCount:static_cast<NSUInteger>(count)]) {
                    *data = count;
                    break;
                }
            }
            return;
        case GL_NUM_EXTENSIONS:
            *data = static_cast<GLint>(extensions().size());
            return;
        case GL_MAJOR_VERSION:
            *data = 4;
            return;
        case GL_MINOR_VERSION:
            *data = 1;
            return;
        case GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS:
            *data = kUniformBindings;
            return;
        // Metal's alignment for a buffer offset bound for constant reads on a Mac (Apple's Metal
        // feature set tables).
        case kGlUniformBufferOffsetAlignment:
            *data = 256;
            return;
        case kGlMaxCombinedTextureUnits:
            *data = kUnits;
            return;
        case GL_MAX_TEXTURE_BUFFER_SIZE:
            *data = static_cast<GLint>(std::min<NSUInteger>(g->device.maxBufferLength / 16, 0x7FFFFFFF));
            return;
        case kGlMaxVertexAttribs:
            *data = 16;
            return;
        case kGlMaxUniformBlockSize:
            *data = 65536;
            return;
        case kGlFramebufferBinding:
            *data = static_cast<GLint>(g->state.draw_framebuffer);
            return;
        case kGlTextureBinding2d:
            *data = static_cast<GLint>(g->state.unit_2d[g->state.active_unit]);
            return;
        case GL_VIEWPORT:
            std::memcpy(data, g->state.viewport, sizeof g->state.viewport);
            return;
        default:
            *data = 0;
            return;
    }
}

void MTLGL get_floatv(GLenum name, GLfloat* data) {
    if (name == GL_MAX_TEXTURE_MAX_ANISOTROPY) {
        *data = 16.0f;  // MTLSamplerDescriptor.maxAnisotropy's largest
        return;
    }
    GLint v = 0;
    get_integerv(name, &v);
    *data = static_cast<GLfloat>(v);
}

GLenum MTLGL get_error() {
    return std::exchange(g->error, GL_NO_ERROR);
}

void enable_cap(GLenum cap, bool on) {
    State& s = g->state;
    switch (cap) {
        case GL_BLEND:
            s.blend = on;
            break;
        case GL_DEPTH_TEST:
            s.depth_test = on;
            break;
        case GL_CULL_FACE:
            s.cull = on;
            break;
        case GL_SCISSOR_TEST:
            s.scissor_test = on;
            break;
        case GL_STENCIL_TEST:
            s.stencil_test = on;
            break;
        case GL_PRIMITIVE_RESTART:
            s.primitive_restart = on;
            break;
        case GL_DEPTH_CLAMP:
            s.depth_clamp = on;
            break;
        default:
            break;
    }
}

void MTLGL enable(GLenum cap) {
    enable_cap(cap, true);
}

void MTLGL disable(GLenum cap) {
    enable_cap(cap, false);
}

// A load action clears whole attachments only: a scissored or masked clear is a draw, which this
// backend does not have yet (README.md).
void MTLGL clear(GLbitfield mask) {
    const State& s = g->state;
    const bool partial = s.scissor_test
                         || ((mask & GL_COLOR_BUFFER_BIT) != 0
                             && !std::ranges::all_of(s.colour_mask, std::identity{}))
                         || ((mask & GL_DEPTH_BUFFER_BIT) != 0 && !s.depth_mask)
                         || ((mask & GL_STENCIL_BUFFER_BIT) != 0
                             && (s.stencil_write_mask & 0xFF) != 0xFF);
    const Target target = target_of(s.draw_framebuffer);
    if (partial) {
        fail(kGlInvalidOperation);
        return;
    }
    if (target.colour == nil && target.depth == nil) {
        return;
    }
    begin_pass(target, mask);
}

void MTLGL clear_color(GLfloat r, GLfloat gr, GLfloat b, GLfloat a) {
    g->state.clear_colour[0] = r;
    g->state.clear_colour[1] = gr;
    g->state.clear_colour[2] = b;
    g->state.clear_colour[3] = a;
}

void MTLGL clear_depth(GLdouble d) {
    g->state.clear_depth = static_cast<float>(std::clamp(d, 0.0, 1.0));
}

void MTLGL clear_stencil(GLint s) {
    g->state.clear_stencil = s;
}

void MTLGL viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    g->state.viewport[0] = x;
    g->state.viewport[1] = y;
    g->state.viewport[2] = w;
    g->state.viewport[3] = h;
}

void MTLGL scissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    g->state.scissor[0] = x;
    g->state.scissor[1] = y;
    g->state.scissor[2] = w;
    g->state.scissor[3] = h;
}

void MTLGL depth_func(GLenum f) {
    g->state.depth_func = f;
}

void MTLGL depth_mask(GLboolean flag) {
    g->state.depth_mask = flag != GL_FALSE;
}

void MTLGL colour_mask(GLboolean r, GLboolean gr, GLboolean b, GLboolean a) {
    g->state.colour_mask[0] = r != GL_FALSE;
    g->state.colour_mask[1] = gr != GL_FALSE;
    g->state.colour_mask[2] = b != GL_FALSE;
    g->state.colour_mask[3] = a != GL_FALSE;
}

void MTLGL blend_func_separate(GLenum sr, GLenum dr, GLenum sa, GLenum da) {
    State& s = g->state;
    s.blend_src_rgb = sr;
    s.blend_dst_rgb = dr;
    s.blend_src_a = sa;
    s.blend_dst_a = da;
}

void MTLGL blend_equation(GLenum mode) {
    g->state.blend_equation = mode;
}

void MTLGL blend_color(GLfloat r, GLfloat gr, GLfloat b, GLfloat a) {
    g->state.blend_colour[0] = r;
    g->state.blend_colour[1] = gr;
    g->state.blend_colour[2] = b;
    g->state.blend_colour[3] = a;
}

void MTLGL cull_face(GLenum mode) {
    g->state.cull_face = mode;
}

void MTLGL front_face(GLenum mode) {
    g->state.front_face = mode;
}

// Only the depth convention: the origin stays lower left (mtlgl.h).
void MTLGL clip_control(GLenum, GLenum depth) {
    g->state.clip_zero_to_one = depth == GL_ZERO_TO_ONE;
}

void MTLGL polygon_mode(GLenum, GLenum mode) {
    g->state.polygon_mode = mode;
}

void MTLGL stencil_func(GLenum func, GLint ref, GLuint mask) {
    g->state.stencil_func = func;
    g->state.stencil_ref = ref;
    g->state.stencil_value_mask = mask;
}

void MTLGL stencil_op_separate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    const StencilFace f{sfail, dpfail, dppass};
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
        g->state.stencil_front = f;
    }
    if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
        g->state.stencil_back = f;
    }
}

void MTLGL stencil_mask(GLuint mask) {
    g->state.stencil_write_mask = mask;
}

void MTLGL pixel_store(GLenum name, GLint value) {
    switch (name) {
        case GL_UNPACK_ALIGNMENT:
            g->state.unpack_alignment = value;
            break;
        case GL_PACK_ALIGNMENT:
            g->state.pack_alignment = value;
            break;
        case kGlUnpackRowLength:
            g->state.unpack_row_length = value;
            break;
        default:
            break;
    }
}

void MTLGL finish() {
    submit(true);
}

#undef MTLGL

struct Entry {
    const char* name;
    Proc proc;
};

#define MTLGL_ENTRY(name, fn) {#name, reinterpret_cast<Proc>(&fn)}
const Entry kEntries[] = {
    MTLGL_ENTRY(glGetString, get_string),
    MTLGL_ENTRY(glGetStringi, get_stringi),
    MTLGL_ENTRY(glGetIntegerv, get_integerv),
    MTLGL_ENTRY(glGetFloatv, get_floatv),
    MTLGL_ENTRY(glGetError, get_error),
    MTLGL_ENTRY(glEnable, enable),
    MTLGL_ENTRY(glDisable, disable),
    MTLGL_ENTRY(glClear, clear),
    MTLGL_ENTRY(glClearColor, clear_color),
    MTLGL_ENTRY(glClearDepth, clear_depth),
    MTLGL_ENTRY(glClearStencil, clear_stencil),
    MTLGL_ENTRY(glViewport, viewport),
    MTLGL_ENTRY(glScissor, scissor),
    MTLGL_ENTRY(glDepthFunc, depth_func),
    MTLGL_ENTRY(glDepthMask, depth_mask),
    MTLGL_ENTRY(glColorMask, colour_mask),
    MTLGL_ENTRY(glBlendFuncSeparate, blend_func_separate),
    MTLGL_ENTRY(glBlendEquation, blend_equation),
    MTLGL_ENTRY(glBlendColor, blend_color),
    MTLGL_ENTRY(glCullFace, cull_face),
    MTLGL_ENTRY(glFrontFace, front_face),
    MTLGL_ENTRY(glClipControl, clip_control),
    MTLGL_ENTRY(glPolygonMode, polygon_mode),
    MTLGL_ENTRY(glStencilFunc, stencil_func),
    MTLGL_ENTRY(glStencilOpSeparate, stencil_op_separate),
    MTLGL_ENTRY(glStencilMask, stencil_mask),
    MTLGL_ENTRY(glPixelStorei, pixel_store),
    MTLGL_ENTRY(glFinish, finish),
    MTLGL_ENTRY(glReadPixels, read_pixels),
    MTLGL_ENTRY(glCopyTexSubImage2D, copy_tex_sub_image),
    MTLGL_ENTRY(glGenTextures, gen_textures),
    MTLGL_ENTRY(glDeleteTextures, delete_textures),
    MTLGL_ENTRY(glBindTexture, bind_texture),
    MTLGL_ENTRY(glActiveTexture, active_texture),
    MTLGL_ENTRY(glTexImage2D, tex_image_2d),
    MTLGL_ENTRY(glCompressedTexImage2D, compressed_tex_image_2d),
    MTLGL_ENTRY(glCompressedTexSubImage2D, compressed_tex_sub_image_2d),
    MTLGL_ENTRY(glTexSubImage2D, tex_sub_image_2d),
    MTLGL_ENTRY(glTexImage3D, tex_image_3d),
    MTLGL_ENTRY(glTexParameteri, tex_parameter_i),
    MTLGL_ENTRY(glTexParameterf, tex_parameter_f),
    MTLGL_ENTRY(glGenerateMipmap, generate_mipmap),
    MTLGL_ENTRY(glTexBuffer, tex_buffer),
    MTLGL_ENTRY(glGenSamplers, gen_samplers),
    MTLGL_ENTRY(glDeleteSamplers, delete_samplers),
    MTLGL_ENTRY(glBindSampler, bind_sampler),
    MTLGL_ENTRY(glSamplerParameteri, sampler_parameter_i),
    MTLGL_ENTRY(glGenBuffers, gen_buffers),
    MTLGL_ENTRY(glDeleteBuffers, delete_buffers),
    MTLGL_ENTRY(glBindBuffer, bind_buffer),
    MTLGL_ENTRY(glBindBufferBase, bind_buffer_base),
    MTLGL_ENTRY(glBufferData, buffer_data),
    MTLGL_ENTRY(glBufferSubData, buffer_sub_data),
    MTLGL_ENTRY(glGenFramebuffers, gen_framebuffers),
    MTLGL_ENTRY(glDeleteFramebuffers, delete_framebuffers),
    MTLGL_ENTRY(glBindFramebuffer, bind_framebuffer),
    MTLGL_ENTRY(glFramebufferTexture2D, framebuffer_texture_2d),
    MTLGL_ENTRY(glFramebufferRenderbuffer, framebuffer_renderbuffer),
    MTLGL_ENTRY(glCheckFramebufferStatus, check_framebuffer_status),
    MTLGL_ENTRY(glGenRenderbuffers, gen_renderbuffers),
    MTLGL_ENTRY(glDeleteRenderbuffers, delete_renderbuffers),
    MTLGL_ENTRY(glBindRenderbuffer, bind_renderbuffer),
    MTLGL_ENTRY(glRenderbufferStorage, renderbuffer_storage),
    MTLGL_ENTRY(glRenderbufferStorageMultisample, renderbuffer_storage_multisample),
    MTLGL_ENTRY(glBlitFramebuffer, blit_framebuffer),
};
#undef MTLGL_ENTRY

}  // namespace

}  // namespace openrac::mtlgl::detail

namespace openrac::mtlgl {

using namespace detail;

bool open(const Config& config, std::string& error) {
    close();
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        error = "no Metal device";
        return false;
    }
    // Deprecated for MTLGPUFamilyApple7, which no Intel Mac's GPU is: Mac2 is still the family
    // the shaders' MSL 2.4 is written for (README.md).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const bool mac2 = [device supportsFamily:MTLGPUFamilyMac2];
#pragma clang diagnostic pop
    if (!mac2) {
        error = std::format("{} is not MTLGPUFamilyMac2", device.name.UTF8String);
        return false;
    }
    NSError* failure = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:@(kFlipSource) options:nil error:&failure];
    if (library == nil) {
        error = std::format("the present shader: {}", failure.localizedDescription.UTF8String);
        return false;
    }
    MTLRenderPipelineDescriptor* flip = [MTLRenderPipelineDescriptor new];
    flip.vertexFunction = [library newFunctionWithName:@"flip_vertex"];
    flip.fragmentFunction = [library newFunctionWithName:@"flip_fragment"];
    flip.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:flip
                                                                                 error:&failure];
    if (pipeline == nil) {
        error = std::format("the present pipeline: {}", failure.localizedDescription.UTF8String);
        return false;
    }

    g = new Context;
    g->device = device;
    g->queue = [device newCommandQueue];
    g->flip = pipeline;
    g->renderer = device.name.UTF8String;
    g->version = std::format("4.1 OpenRAC on Metal, {}", g->renderer);
    int width = config.width, height = config.height;
    if (config.window != nullptr) {
        g->window = config.window;
        g->view = SDL_Metal_CreateView(config.window);
        g->layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(g->view);
        g->layer.device = device;
        g->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        g->layer.displaySyncEnabled = config.vsync;
        SDL_GetWindowSizeInPixels(config.window, &width, &height);
        g->layer.drawableSize = CGSizeMake(width, height);
    }
    make_default_framebuffer(std::max(width, 1), std::max(height, 1));
    g->state.viewport[2] = g->state.scissor[2] = width;
    g->state.viewport[3] = g->state.scissor[3] = height;
    return true;
}

void close() {
    if (g == nullptr) {
        return;
    }
    submit(true);
    if (g->view != nullptr) {
        SDL_Metal_DestroyView(g->view);
    }
    delete g;
    g = nullptr;
}

bool active() {
    return g != nullptr;
}

Proc get_proc(const char* name) {
    for (const Entry& e : kEntries) {
        if (std::strcmp(e.name, name) == 0) {
            return e.proc;
        }
    }
    return nullptr;
}

void present() {
    end_encoders();
    if (g->layer != nil) {
        @autoreleasepool {
            id<CAMetalDrawable> drawable = [g->layer nextDrawable];
            if (drawable != nil) {
                MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
                d.colorAttachments[0].texture = drawable.texture;
                d.colorAttachments[0].loadAction = MTLLoadActionDontCare;
                id<MTLRenderCommandEncoder> pass = [commands() renderCommandEncoderWithDescriptor:d];
                [pass setRenderPipelineState:g->flip];
                [pass setFragmentTexture:g->colour atIndex:0];
                [pass drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
                [pass endEncoding];
                [commands() presentDrawable:drawable];
            }
        }
    }
    submit(false);
    if (g->window != nullptr) {
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(g->window, &width, &height);
        if (width > 0 && height > 0
            && (static_cast<NSUInteger>(width) != g->colour.width
                || static_cast<NSUInteger>(height) != g->colour.height)) {
            g->layer.drawableSize = CGSizeMake(width, height);
            make_default_framebuffer(width, height);
        }
    }
}

void set_swap_interval(int interval) {
    if (g->layer != nil) {
        g->layer.displaySyncEnabled = interval != 0;
    }
}

std::string device_name() {
    return g == nullptr ? std::string() : g->renderer;
}

}  // namespace openrac::mtlgl
