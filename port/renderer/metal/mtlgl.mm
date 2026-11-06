// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/metal/mtlgl.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <SDL3/SDL_metal.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <format>
#include <utility>

#include "renderer/gl.h"

namespace openrac::mtlgl {

namespace {

using namespace openrac::gl;

constexpr GLenum kGlInvalidEnum = 0x0500;
constexpr GLenum kGlInvalidOperation = 0x0502;
constexpr GLenum kGlFramebufferBinding = 0x8CA6;
constexpr GLenum kGlViewportQuery = 0x0BA2;

struct State {
    bool blend = false, depth_test = false, cull = false, scissor_test = false, stencil_test = false;
    GLint viewport[4] = {}, scissor[4] = {};
    float clear_colour[4] = {}, clear_depth = 1.0f;
    GLint clear_stencil = 0;
    bool colour_mask[4] = {true, true, true, true}, depth_mask = true;
    GLuint stencil_mask = 0xFFFFFFFF;
    GLint pack_alignment = 4, unpack_alignment = 4;
    GLuint draw_framebuffer = 0, read_framebuffer = 0;
};

struct Context {
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    SDL_Window* window = nullptr;
    SDL_MetalView view = nullptr;
    CAMetalLayer* layer = nil;
    id<MTLRenderPipelineState> flip;  // the default framebuffer to the drawable, upside down
    id<MTLTexture> colour, depth;     // the default framebuffer
    id<MTLCommandBuffer> commands;
    id<MTLRenderCommandEncoder> pass;
    State state;
    GLenum error = GL_NO_ERROR;
    std::string version, renderer;
};

Context* g = nullptr;

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

void end_pass() {
    [g->pass endEncoding];
    g->pass = nil;
}

void submit(bool wait) {
    end_pass();
    if (g->commands == nil) {
        return;
    }
    [g->commands commit];
    if (wait) {
        [g->commands waitUntilCompleted];
    }
    g->commands = nil;
}

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

// A pass on the default framebuffer, clearing what `clear` names and keeping the rest.
void begin_pass(GLbitfield clear) {
    end_pass();
    const State& s = g->state;
    MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
    d.colorAttachments[0].texture = g->colour;
    d.colorAttachments[0].loadAction =
        (clear & GL_COLOR_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
    d.colorAttachments[0].clearColor = MTLClearColorMake(
        s.clear_colour[0], s.clear_colour[1], s.clear_colour[2], s.clear_colour[3]);
    d.depthAttachment.texture = g->depth;
    d.depthAttachment.loadAction =
        (clear & GL_DEPTH_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
    d.depthAttachment.clearDepth = s.clear_depth;
    d.stencilAttachment.texture = g->depth;
    d.stencilAttachment.loadAction =
        (clear & GL_STENCIL_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
    d.stencilAttachment.clearStencil = static_cast<std::uint32_t>(s.clear_stencil) & 0xFF;
    g->pass = [commands() renderCommandEncoderWithDescriptor:d];
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

void MTLGL get_integerv(GLenum name, GLint* data) {
    switch (name) {
        // MTLGPUFamilyMac2's largest 2D texture (Apple's Metal feature set tables).
        case GL_MAX_TEXTURE_SIZE:
            *data = 16384;
            return;
        case GL_SAMPLES:
            *data = 1;
            return;
        case GL_MAX_SAMPLES:
            *data = 1;
            for (const GLint count : {8, 4, 2}) {
                if ([g->device supportsTextureSampleCount:static_cast<NSUInteger>(count)]) {
                    *data = count;
                    break;
                }
            }
            return;
        case GL_MAJOR_VERSION:
            *data = 4;
            return;
        case GL_MINOR_VERSION:
            *data = 1;
            return;
        case kGlFramebufferBinding:
            *data = static_cast<GLint>(g->state.draw_framebuffer);
            return;
        case kGlViewportQuery:
            std::memcpy(data, g->state.viewport, sizeof g->state.viewport);
            return;
        default:
            *data = 0;
            return;
    }
}

void MTLGL get_floatv(GLenum name, GLfloat* data) {
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
                         || ((mask & GL_STENCIL_BUFFER_BIT) != 0 && (s.stencil_mask & 0xFF) != 0xFF);
    if (partial || s.draw_framebuffer != 0) {
        fail(kGlInvalidOperation);
        return;
    }
    begin_pass(mask);
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

void MTLGL colour_mask(GLboolean r, GLboolean gr, GLboolean b, GLboolean a) {
    g->state.colour_mask[0] = r != GL_FALSE;
    g->state.colour_mask[1] = gr != GL_FALSE;
    g->state.colour_mask[2] = b != GL_FALSE;
    g->state.colour_mask[3] = a != GL_FALSE;
}

void MTLGL depth_mask(GLboolean flag) {
    g->state.depth_mask = flag != GL_FALSE;
}

void MTLGL stencil_mask(GLuint mask) {
    g->state.stencil_mask = mask;
}

void MTLGL pixel_store(GLenum name, GLint value) {
    switch (name) {
        case GL_UNPACK_ALIGNMENT:
            g->state.unpack_alignment = value;
            break;
        case GL_PACK_ALIGNMENT:
            g->state.pack_alignment = value;
            break;
        default:
            break;
    }
}

void MTLGL
read_pixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void* pixels) {
    if (format != GL_RGBA || type != GL_UNSIGNED_BYTE || w <= 0 || h <= 0) {
        fail(kGlInvalidEnum);
        return;
    }
    if (g->state.read_framebuffer != 0) {
        fail(kGlInvalidOperation);
        return;
    }
    id<MTLTexture> image = g->colour;
    const int width = static_cast<int>(image.width), height = static_cast<int>(image.height);
    const int x0 = std::clamp(x, 0, width), y0 = std::clamp(y, 0, height);
    const int x1 = std::clamp(x + w, x0, width), y1 = std::clamp(y + h, y0, height);
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    const auto cw = static_cast<NSUInteger>(x1 - x0), ch = static_cast<NSUInteger>(y1 - y0);
    id<MTLBuffer> buffer = [g->device newBufferWithLength:cw * ch * 4
                                                  options:MTLResourceStorageModeShared];
    end_pass();
    id<MTLBlitCommandEncoder> blit = [commands() blitCommandEncoder];
    [blit copyFromTexture:image
                     sourceSlice:0
                     sourceLevel:0
                    sourceOrigin:MTLOriginMake(static_cast<NSUInteger>(x0), static_cast<NSUInteger>(y0), 0)
                      sourceSize:MTLSizeMake(cw, ch, 1)
                        toBuffer:buffer
               destinationOffset:0
          destinationBytesPerRow:cw * 4
        destinationBytesPerImage:cw * ch * 4];
    [blit endEncoding];
    submit(true);
    // The texture's rows are GL's (mtlgl.h): row 0 is the bottom one GL gives first.
    const int align = std::max(g->state.pack_alignment, 1);
    const std::size_t pitch = (static_cast<std::size_t>(w) * 4 + align - 1) / align * align;
    const auto* src = static_cast<const std::uint8_t*>(buffer.contents);
    auto* dst = static_cast<std::uint8_t*>(pixels);
    for (NSUInteger row = 0; row < ch; ++row) {
        std::memcpy(dst + pitch * (row + static_cast<std::size_t>(y0 - y))
                        + static_cast<std::size_t>(x0 - x) * 4,
                    src + row * cw * 4, cw * 4);
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
    MTLGL_ENTRY(glColorMask, colour_mask),
    MTLGL_ENTRY(glDepthMask, depth_mask),
    MTLGL_ENTRY(glStencilMask, stencil_mask),
    MTLGL_ENTRY(glPixelStorei, pixel_store),
    MTLGL_ENTRY(glReadPixels, read_pixels),
    MTLGL_ENTRY(glFinish, finish),
};
#undef MTLGL_ENTRY

}  // namespace

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
    end_pass();
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
