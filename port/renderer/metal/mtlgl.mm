// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The context, the command stream (mtlgl_internal.h), state, and the present. Resources are
// mtlgl_resources.mm's.

#include <OpenGL/OpenGL.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <memory>
#include <utility>
#include <vector>

#include "renderer/metal/mtlgl_internal.h"

namespace openrac::mtlgl::detail {

using namespace openrac::gl;

std::unique_ptr<Context> context;
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

id<MTLCommandBuffer> uploads() {
    if (g->uploads == nil) {
        g->uploads = [g->queue commandBuffer];
        // Its place on the queue is taken now, before the frame's command buffer, which takes
        // its own at commit.
        [g->uploads enqueue];
    }
    return g->uploads;
}

id<MTLBlitCommandEncoder> blit_for(Texture& texture) {
    if (texture.used == g->serial) {
        return blit();
    }
    if (g->upload_blit == nil) {
        g->upload_blit = [uploads() blitCommandEncoder];
    }
    return g->upload_blit;
}

void zero(id<MTLTexture> texture) {
    [g->upload_blit endEncoding];
    g->upload_blit = nil;
    const MTLPixelFormat f = texture.pixelFormat;
    const bool depth = f == MTLPixelFormatDepth32Float || f == MTLPixelFormatDepth32Float_Stencil8;
    for (NSUInteger level = 0; level < texture.mipmapLevelCount; ++level) {
        MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
        MTLRenderPassAttachmentDescriptor* a = depth ? d.depthAttachment : d.colorAttachments[0];
        a.texture = texture;
        a.level = level;
        a.loadAction = MTLLoadActionClear;
        a.storeAction = MTLStoreActionStore;
        d.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
        if (f == MTLPixelFormatDepth32Float_Stencil8) {
            d.stencilAttachment.texture = texture;
            d.stencilAttachment.level = level;
            d.stencilAttachment.loadAction = MTLLoadActionClear;
            d.stencilAttachment.storeAction = MTLStoreActionStore;
        }
        [[uploads() renderCommandEncoderWithDescriptor:d] endEncoding];
        // Apple's OpenGL starts depth at all ones, a NaN that passes only LESS, LEQUAL and
        // NOTEQUAL. A render pass may not clear to it, a fragment's NaN is written as 0 and its
        // infinity as 1, and a depth image has no colour view, so it is copied in, which a
        // multisampled image does not take: that keeps the clear's 1.
        if (depth && texture.sampleCount == 1) {
            const NSUInteger w = std::max<NSUInteger>(texture.width >> level, 1);
            const NSUInteger h = std::max<NSUInteger>(texture.height >> level, 1);
            id<MTLBuffer> ones = [g->device newBufferWithLength:w * h * 4 options:MTLResourceStorageModeShared];
            std::memset(ones.contents, 0xFF, w * h * 4);
            id<MTLBlitCommandEncoder> copy = [uploads() blitCommandEncoder];
            [copy copyFromBuffer:ones
                       sourceOffset:0
                  sourceBytesPerRow:w * 4
                sourceBytesPerImage:w * h * 4
                         sourceSize:MTLSizeMake(w, h, 1)
                          toTexture:texture
                   destinationSlice:0
                   destinationLevel:level
                  destinationOrigin:MTLOriginMake(0, 0, 0)
                            options:f == MTLPixelFormatDepth32Float_Stencil8
                                        ? MTLBlitOptionDepthFromDepthStencil
                                        : MTLBlitOptionNone];
            [copy endEncoding];
        }
    }
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
    Framebuffer* fb = g->framebuffers.find(name);
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
    id<MTLTexture> colour = image(fb->colour_texture, fb->colour_renderbuffer);
    if (fb->colour_level != 0 && colour != nil) {
        const auto level = static_cast<NSUInteger>(fb->colour_level);
        if (level < colour.mipmapLevelCount && fb->colour_view.parentTexture != colour) {
            fb->colour_view = [colour newTextureViewWithPixelFormat:colour.pixelFormat
                                                        textureType:MTLTextureType2D
                                                             levels:NSMakeRange(level, 1)
                                                             slices:NSMakeRange(0, 1)];
        }
        colour = level < colour.mipmapLevelCount ? fb->colour_view : nil;
    }
    return {colour, image(fb->depth_texture, fb->depth_renderbuffer)};
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
        d.colorAttachments[0].storeAction = MTLStoreActionStore;
    }
    if (target.depth != nil) {
        d.depthAttachment.texture = target.depth;
        d.depthAttachment.loadAction =
            (clear & GL_DEPTH_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
        d.depthAttachment.clearDepth = s.clear_depth;
        // Metal's default store for depth and stencil is DontCare: GL keeps them past the pass.
        d.depthAttachment.storeAction = MTLStoreActionStore;
        if (target.depth.pixelFormat == MTLPixelFormatDepth32Float_Stencil8) {
            d.stencilAttachment.texture = target.depth;
            d.stencilAttachment.loadAction =
                (clear & GL_STENCIL_BUFFER_BIT) != 0 ? MTLLoadActionClear : MTLLoadActionLoad;
            d.stencilAttachment.clearStencil = static_cast<std::uint32_t>(s.clear_stencil) & 0xFF;
            d.stencilAttachment.storeAction = MTLStoreActionStore;
        }
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
// glBlitFramebuffer's scaled, flipped or converting copies: the viewport is the destination box,
// `uv` the source's coordinates at its first and last corners.
vertex Out blit_vertex(uint id [[vertex_id]], constant float4& uv [[buffer(0)]]) {
    const float2 p = float2((id << 1) & 2, id & 2);
    return {float4(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0, 0.0, 1.0), mix(uv.xy, uv.zw, p)};
}
fragment float4 blit_fragment(Out in [[stage_in]], texture2d<float> from [[texture(0)]],
                              constant uint& linear [[buffer(0)]]) {
    constexpr sampler nearest(filter::nearest);
    constexpr sampler smooth(filter::linear);
    return linear != 0 ? from.sample(smooth, in.uv) : from.sample(nearest, in.uv);
}
fragment uint4 blit_uint(Out in [[stage_in]], texture2d<uint> from [[texture(0)]]) {
    return from.read(uint2(in.uv * float2(from.get_width(), from.get_height())));
}
struct Depth { float depth [[depth(any)]]; };
fragment Depth blit_depth(Out in [[stage_in]], depth2d<float> from [[texture(0)]]) {
    constexpr sampler nearest(filter::nearest);
    return {from.sample(nearest, in.uv)};
}
// A scissored or masked glClear: the target covered at the clear depth, in the clear colour.
vertex float4 clear_vertex(uint id [[vertex_id]], constant float& depth [[buffer(0)]]) {
    const float2 p = float2((id << 1) & 2, id & 2);
    return float4(p * 2.0 - 1.0, depth, 1.0);
}
fragment float4 clear_fragment(constant float4& colour [[buffer(0)]]) {
    return colour;
}
// Apple's OpenGL resolves as the samples' floats summed in order, then scaled; Metal's own resolve
// rounds their average differently.
fragment float4 resolve_fragment(float4 at [[position]], texture2d_ms<float> from [[texture(0)]]) {
    const uint2 p = uint2(at.xy);
    float4 sum = 0.0;
    for (uint i = 0; i < from.get_num_samples(); ++i) {
        sum += from.read(p, i);
    }
    return sum * (1.0 / from.get_num_samples());
}
// AMD's OpenGL rounds the average of 8-bit colour toward zero as a float, then to the nearest
// byte: a half rounds down, but for 127.5, which is 0.5 exactly.
fragment float4 resolve_bytes(float4 at [[position]], texture2d_ms<float> from [[texture(0)]]) {
    const uint2 p = uint2(at.xy);
    float4 sum = 0.0;
    for (uint i = 0; i < from.get_num_samples(); ++i) {
        sum += round(from.read(p, i) * 255.0);
    }
    const float4 average = sum / from.get_num_samples();
    return select(ceil(average - 0.5), float4(128.0), average == 127.5) / 255.0;
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

// A clear through the scissor and the write masks, drawn: a load action clears whole attachments.
void draw_clear(const Target& t, GLbitfield mask) {
    const State& s = g->state;
    const id<MTLTexture> size = t.colour != nil ? t.colour : t.depth;
    const auto width = static_cast<GLint>(size.width), height = static_cast<GLint>(size.height);
    GLint x0 = 0, y0 = 0, x1 = width, y1 = height;
    if (s.scissor_test) {
        x0 = std::clamp(s.scissor[0], 0, width);
        y0 = std::clamp(s.scissor[1], 0, height);
        x1 = std::clamp(s.scissor[0] + s.scissor[2], x0, width);
        y1 = std::clamp(s.scissor[1] + s.scissor[3], y0, height);
    }
    if (x1 == x0 || y1 == y0) {
        return;
    }
    const bool stencil_format = t.depth != nil && t.depth.pixelFormat == MTLPixelFormatDepth32Float_Stencil8;
    const bool depth = (mask & GL_DEPTH_BUFFER_BIT) != 0 && s.depth_mask;
    const bool stencil = (mask & GL_STENCIL_BUFFER_BIT) != 0 && stencil_format;
    const MTLColorWriteMask write =
        (mask & GL_COLOR_BUFFER_BIT) == 0
            ? MTLColorWriteMaskNone
            : (s.colour_mask[0] ? MTLColorWriteMaskRed : 0) | (s.colour_mask[1] ? MTLColorWriteMaskGreen : 0)
                  | (s.colour_mask[2] ? MTLColorWriteMaskBlue : 0) | (s.colour_mask[3] ? MTLColorWriteMaskAlpha : 0);
    const MTLPixelFormat colour_format = t.colour != nil ? t.colour.pixelFormat : MTLPixelFormatInvalid;
    const MTLPixelFormat depth_format = t.depth != nil ? t.depth.pixelFormat : MTLPixelFormatInvalid;
    // Above blit pipelines' keys, which are formats and sample counts.
    const NSUInteger key = NSUInteger{1} << 63 | NSUInteger{write} << 48 | size.sampleCount << 40
                           | NSUInteger{depth_format} << 20 | colour_format;
    __strong id<MTLRenderPipelineState>& pipeline = g->blits[key];
    if (pipeline == nil) {
        MTLRenderPipelineDescriptor* d = [MTLRenderPipelineDescriptor new];
        d.vertexFunction = [g->library newFunctionWithName:@"clear_vertex"];
        d.fragmentFunction = [g->library newFunctionWithName:@"clear_fragment"];
        d.rasterSampleCount = size.sampleCount;
        d.colorAttachments[0].pixelFormat = colour_format;
        d.colorAttachments[0].writeMask = write;
        d.depthAttachmentPixelFormat = depth_format;
        d.stencilAttachmentPixelFormat = stencil_format ? depth_format : MTLPixelFormatInvalid;
        pipeline = [g->device newRenderPipelineStateWithDescriptor:d error:nil];
    }
    if (pipeline == nil) {
        fail(kGlInvalidOperation);  // an integer image's: its clear colour is not floats
        return;
    }
    const std::string state_key = std::format("clear {} {} {}", depth, stencil, s.stencil_write_mask & 0xFF);
    __strong id<MTLDepthStencilState>& state = g->depth_states[state_key];
    if (state == nil) {
        MTLDepthStencilDescriptor* d = [MTLDepthStencilDescriptor new];
        d.depthWriteEnabled = depth;
        if (stencil) {
            MTLStencilDescriptor* face = [MTLStencilDescriptor new];
            face.depthStencilPassOperation = MTLStencilOperationReplace;
            face.writeMask = s.stencil_write_mask & 0xFF;
            d.frontFaceStencil = d.backFaceStencil = face;
        }
        state = [g->device newDepthStencilStateWithDescriptor:d];
    }
    if (g->pass == nil || g->pass_target.colour != t.colour || g->pass_target.depth != t.depth) {
        begin_pass(t, 0);
    }
    id<MTLRenderCommandEncoder> pass = g->pass;
    [pass setRenderPipelineState:pipeline];
    [pass setDepthStencilState:state];
    [pass setStencilReferenceValue:static_cast<std::uint32_t>(s.clear_stencil) & 0xFF];
    [pass setCullMode:MTLCullModeNone];
    [pass setTriangleFillMode:MTLTriangleFillModeFill];
    [pass setDepthClipMode:MTLDepthClipModeClip];
    [pass setViewport:{0.0, 0.0, static_cast<double>(width), static_cast<double>(height), 0.0, 1.0}];
    [pass setScissorRect:{static_cast<NSUInteger>(x0), static_cast<NSUInteger>(y0), static_cast<NSUInteger>(x1 - x0),
                          static_cast<NSUInteger>(y1 - y0)}];
    [pass setVertexBytes:&s.clear_depth length:sizeof s.clear_depth atIndex:0];
    [pass setFragmentBytes:s.clear_colour length:sizeof s.clear_colour atIndex:0];
    [pass drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}

void MTLGL clear(GLbitfield mask) {
    const State& s = g->state;
    const bool partial = s.scissor_test
                         || ((mask & GL_COLOR_BUFFER_BIT) != 0
                             && !std::ranges::all_of(s.colour_mask, std::identity{}))
                         || ((mask & GL_DEPTH_BUFFER_BIT) != 0 && !s.depth_mask)
                         || ((mask & GL_STENCIL_BUFFER_BIT) != 0
                             && (s.stencil_write_mask & 0xFF) != 0xFF);
    const Target target = target_of(s.draw_framebuffer);
    if (target.colour == nil && target.depth == nil) {
        return;
    }
    if (partial) {
        draw_clear(target, mask);
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
    MTLGL_ENTRY(glCreateShader, create_shader),
    MTLGL_ENTRY(glShaderSource, shader_source),
    MTLGL_ENTRY(glCompileShader, compile_shader),
    MTLGL_ENTRY(glGetShaderiv, get_shader_iv),
    MTLGL_ENTRY(glGetShaderInfoLog, get_shader_info_log),
    MTLGL_ENTRY(glDeleteShader, delete_shader),
    MTLGL_ENTRY(glCreateProgram, create_program),
    MTLGL_ENTRY(glAttachShader, attach_shader),
    MTLGL_ENTRY(glBindAttribLocation, bind_attrib_location),
    MTLGL_ENTRY(glLinkProgram, link_program),
    MTLGL_ENTRY(glGetProgramiv, get_program_iv),
    MTLGL_ENTRY(glGetProgramInfoLog, get_program_info_log),
    MTLGL_ENTRY(glDeleteProgram, delete_program),
    MTLGL_ENTRY(glUseProgram, use_program),
    MTLGL_ENTRY(glGetUniformLocation, get_uniform_location),
    MTLGL_ENTRY(glUniform1i, uniform_1i),
    MTLGL_ENTRY(glUniform1ui, uniform_1ui),
    MTLGL_ENTRY(glUniform1f, uniform_1f),
    MTLGL_ENTRY(glUniform2f, uniform_2f),
    MTLGL_ENTRY(glUniform3f, uniform_3f),
    MTLGL_ENTRY(glUniform4f, uniform_4f),
    MTLGL_ENTRY(glUniform4fv, uniform_4fv),
    MTLGL_ENTRY(glUniformMatrix4fv, uniform_matrix_4fv),
    MTLGL_ENTRY(glGetUniformBlockIndex, get_uniform_block_index),
    MTLGL_ENTRY(glUniformBlockBinding, uniform_block_binding),
    MTLGL_ENTRY(glGenVertexArrays, gen_vertex_arrays),
    MTLGL_ENTRY(glDeleteVertexArrays, delete_vertex_arrays),
    MTLGL_ENTRY(glBindVertexArray, bind_vertex_array),
    MTLGL_ENTRY(glEnableVertexAttribArray, enable_vertex_attrib_array),
    MTLGL_ENTRY(glVertexAttribPointer, vertex_attrib_pointer),
    MTLGL_ENTRY(glVertexAttribIPointer, vertex_attrib_i_pointer),
    MTLGL_ENTRY(glVertexAttribDivisor, vertex_attrib_divisor),
    MTLGL_ENTRY(glDrawArrays, draw_arrays),
    MTLGL_ENTRY(glDrawArraysInstanced, draw_arrays_instanced),
    MTLGL_ENTRY(glDrawElements, draw_elements),
    MTLGL_ENTRY(glDrawElementsInstanced, draw_elements_instanced),
};
#undef MTLGL_ENTRY

// The GPU OpenGL would draw with: Metal's default is the discrete one even where `pmset -a
// gpuswitch 0` keeps a MacBook Pro on its integrated one, which OpenGL then uses.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
id<MTLDevice> opengl_device() {
    const CGLPixelFormatAttribute attributes[] = {
        kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_GL4_Core), kCGLPFAAccelerated,
        static_cast<CGLPixelFormatAttribute>(0)};
    CGLPixelFormatObj format = nullptr;
    int screens = 0, renderer = 0, count = 0;
    if (CGLChoosePixelFormat(attributes, &format, &screens) != kCGLNoError || format == nullptr) {
        return nil;
    }
    CGLDescribePixelFormat(format, 0, kCGLPFARendererID, &renderer);
    CGLReleasePixelFormat(format);
    CGLRendererInfoObj info = nullptr;
    if (CGLQueryRendererInfo(0xFFFFFFFF, &info, &count) != kCGLNoError) {
        return nil;
    }
    std::uint64_t registry = 0;
    for (int i = 0; i < count; ++i) {
        int rid = 0, low = 0, high = 0;
        CGLDescribeRenderer(info, i, kCGLRPRendererID, &rid);
        CGLDescribeRenderer(info, i, kCGLRPRegistryIDLow, &low);
        CGLDescribeRenderer(info, i, kCGLRPRegistryIDHigh, &high);
        if (rid == renderer) {
            registry = std::uint64_t{static_cast<std::uint32_t>(high)} << 32 | static_cast<std::uint32_t>(low);
        }
    }
    CGLDestroyRendererInfo(info);
    for (id<MTLDevice> device in MTLCopyAllDevices()) {
        if (device.registryID == registry) {
            return device;
        }
    }
    return nil;
}
#pragma clang diagnostic pop

}  // namespace

}  // namespace openrac::mtlgl::detail

namespace openrac::mtlgl {

using namespace detail;

bool open(const Config& config, std::string& error) {
    close();
    id<MTLDevice> device = opengl_device();
    if (device == nil) {
        device = MTLCreateSystemDefaultDevice();
    }
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
    MTLCompileOptions* exact = [MTLCompileOptions new];
    exact.fastMathEnabled = NO;
    id<MTLLibrary> library = [device newLibraryWithSource:@(kFlipSource) options:exact error:&failure];
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

    context = std::make_unique<Context>();
    g = context.get();
    g->device = device;
    g->queue = [device newCommandQueue];
    g->flip = pipeline;
    g->library = library;
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
    context.reset();
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
