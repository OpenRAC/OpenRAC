// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GL's objects on Metal: textures, samplers, buffers, framebuffers, renderbuffers, and the copies
// between them and to the CPU.

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <vector>

#include "common/float_encoding.h"
#include "renderer/metal/mtlgl_internal.h"

namespace openrac::mtlgl::detail {

using namespace openrac::gl;

Format format_of(GLenum internal) {
    switch (internal) {
        case GL_RGBA8:
        case GL_RGBA:
        case GL_RGB:
            return {MTLPixelFormatRGBA8Unorm, 4};
        case kGlR8:
        case kGlRed:
            return {MTLPixelFormatR8Unorm, 1};
        case wgl::GL_R32UI:
            return {MTLPixelFormatR32Uint, 4};
        case wgl::GL_RGBA32UI:
            return {MTLPixelFormatRGBA32Uint, 16};
        case wgl::GL_RGBA32F:
            return {MTLPixelFormatRGBA32Float, 16};
        case kGlRgba16f:
            return {MTLPixelFormatRGBA16Float, 8};
        case kGlR11fG11fB10f:
            return {MTLPixelFormatRG11B10Float, 4};
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            return g->device.supportsBCTextureCompression
                       ? Format{MTLPixelFormatBC6H_RGBUfloat, 16, 4}
                       : Format{};
        case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
            return [g->device supportsFamily:MTLGPUFamilyApple2]
                       ? Format{MTLPixelFormatASTC_4x4_LDR, 16, 4}
                       : Format{};
        case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
            return [g->device supportsFamily:MTLGPUFamilyApple6]
                       ? Format{MTLPixelFormatASTC_8x8_HDR, 16, 8}
                       : Format{};
        case GL_RG8:
            return {MTLPixelFormatRG8Unorm, 2};
        case GL_RG16:
            return {MTLPixelFormatRG16Unorm, 4};
        case kGlRg32f:
            return {MTLPixelFormatRG32Float, 8};
        case kGlR32f:
            return {MTLPixelFormatR32Float, 4};
        case kGlRg16f:
            return {MTLPixelFormatRG16Float, 4};
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F:
            return {MTLPixelFormatDepth32Float, 4};
        // Every Mac GPU has 32-bit float depth with stencil; Depth24Unorm_Stencil8 is AMD's only.
        case GL_DEPTH24_STENCIL8:
        case kGlDepthStencil:
        case GL_DEPTH32F_STENCIL8:
            return {MTLPixelFormatDepth32Float_Stencil8, 8, 1, true};
        default:
            return {};
    }
}

namespace {

bool is_depth(MTLPixelFormat f) {
    return f == MTLPixelFormatDepth32Float || f == MTLPixelFormatDepth32Float_Stencil8;
}

bool extent_valid(int width, int height) {
    return width > 0 && height > 0 && width <= 16384 && height <= 16384;
}

id<MTLTexture> make_texture(const Format& f, int width, int height, int levels, int samples) {
    MTLTextureDescriptor* d = [MTLTextureDescriptor new];
    d.textureType = samples > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;
    d.pixelFormat = f.pixel;
    d.width = static_cast<NSUInteger>(width);
    d.height = static_cast<NSUInteger>(height);
    d.mipmapLevelCount = static_cast<NSUInteger>(levels);
    d.sampleCount = static_cast<NSUInteger>(samples);
    d.storageMode = MTLStorageModePrivate;
    d.usage = MTLTextureUsageShaderRead;
    if (f.block == 1) {
        d.usage |= MTLTextureUsageRenderTarget;
    }
    return [g->device newTextureWithDescriptor:d];
}

int full_chain(id<MTLTexture> t) {
    return std::bit_width(static_cast<unsigned>(std::max(t.width, t.height)));
}

// `pixels` as rows of `to`'s texels, from GL's unpack layout.
std::vector<std::uint8_t> stage_pixels(
    const Format& to, int width, int height, GLenum format, GLenum type, const void* pixels
) {
    const bool float_rgba = format == GL_RGBA && type == GL_FLOAT;
    int from_bytes = to.bytes;
    if (float_rgba) {
        from_bytes = 16;
    }
    if (format == GL_RGB && type == GL_UNSIGNED_BYTE) {
        from_bytes = 3;
    }
    const int row_pixels = g->state.unpack_row_length > 0 ? g->state.unpack_row_length : width;
    const std::size_t align = static_cast<std::size_t>(std::max(g->state.unpack_alignment, 1));
    const std::size_t pitch =
        (static_cast<std::size_t>(row_pixels) * static_cast<std::size_t>(from_bytes) + align - 1)
        / align * align;
    const std::size_t row = static_cast<std::size_t>(width) * static_cast<std::size_t>(to.bytes);
    std::vector<std::uint8_t> out(row * static_cast<std::size_t>(height));
    const auto* src = static_cast<const std::uint8_t*>(pixels);
    const bool packs = float_rgba
                       && (to.pixel == MTLPixelFormatRG11B10Float || to.pixel == MTLPixelFormatRG32Float
                           || to.pixel == MTLPixelFormatRGBA16Float);
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* in = src + pitch * static_cast<std::size_t>(y);
        std::uint8_t* o = out.data() + row * static_cast<std::size_t>(y);
        if (packs) {
            for (int x = 0; x < width; ++x) {
                float rgba[4];
                std::memcpy(rgba, in + x * 16, sizeof rgba);
                if (to.pixel == MTLPixelFormatRG11B10Float) {
                    const std::uint32_t packed = float_encoding::pack_small_float(rgba[0], 6, false)
                                                 | (float_encoding::pack_small_float(rgba[1], 6, false) << 11)
                                                 | (float_encoding::pack_small_float(rgba[2], 5, false) << 22);
                    std::memcpy(o + x * 4, &packed, sizeof packed);
                } else if (to.pixel == MTLPixelFormatRG32Float) {
                    std::memcpy(o + x * 8, rgba, sizeof(float) * 2);
                } else {
                    std::uint16_t packed[4];
                    for (unsigned c = 0; c < 4; ++c) {
                        packed[c] =
                            static_cast<std::uint16_t>(float_encoding::pack_small_float(rgba[c], 10, true));
                    }
                    std::memcpy(o + x * 8, packed, sizeof packed);
                }
            }
        } else if (from_bytes == 3 && to.bytes == 4) {
            for (int x = 0; x < width; ++x) {
                o[x * 4 + 0] = in[x * 3 + 0];
                o[x * 4 + 1] = in[x * 3 + 1];
                o[x * 4 + 2] = in[x * 3 + 2];
                o[x * 4 + 3] = 0xFF;
            }
        } else {
            std::memcpy(o, in, row);
        }
    }
    return out;
}

// Copies `size` bytes of rows `row_bytes` long into `t`'s `level` at (x, y).
void upload(Texture& t, int level, int x, int y, int width, int height, const void* bytes,
            std::size_t size, std::size_t row_bytes) {
    id<MTLBuffer> staging = [g->device newBufferWithBytes:bytes
                                                   length:size
                                                  options:MTLResourceStorageModeShared];
    [blit_for(t) copyFromBuffer:staging
                   sourceOffset:0
              sourceBytesPerRow:row_bytes
            sourceBytesPerImage:size
                     sourceSize:MTLSizeMake(static_cast<NSUInteger>(width),
                                            static_cast<NSUInteger>(height), 1)
                      toTexture:t.texture
               destinationSlice:0
               destinationLevel:static_cast<NSUInteger>(level)
              destinationOrigin:MTLOriginMake(static_cast<NSUInteger>(x),
                                              static_cast<NSUInteger>(y), 0)];
}

// `t`'s texture with every mip level, level 0 kept.
void with_chain(Texture& t) {
    if (static_cast<int>(t.texture.mipmapLevelCount) == full_chain(t.texture)) {
        return;
    }
    const Format f{t.texture.pixelFormat};
    id<MTLTexture> chained = make_texture(f, static_cast<int>(t.texture.width),
                                          static_cast<int>(t.texture.height), full_chain(t.texture), 1);
    [blit_for(t) copyFromTexture:t.texture
                     sourceSlice:0
                     sourceLevel:0
                       toTexture:chained
                destinationSlice:0
                destinationLevel:0
                      sliceCount:1
                      levelCount:1];
    t.texture = chained;
}

Texture* bound_texture(GLenum target) {
    const State& s = g->state;
    switch (target) {
        case GL_TEXTURE_2D:
            return g->textures.find(s.unit_2d[s.active_unit]);
        case wgl::GL_TEXTURE_BUFFER:
            return g->textures.find(s.unit_buffer[s.active_unit]);
        case wgl::GL_TEXTURE_2D_ARRAY:
            return g->textures.find(s.unit_array[s.active_unit]);
        default:
            return nullptr;
    }
}

std::size_t compressed_bytes(GLenum format, int width, int height) {
    const int side = format == GL_COMPRESSED_RGBA_ASTC_8x8_KHR ? 8 : 4;
    return std::size_t((width + side - 1) / side) * ((height + side - 1) / side) * 16;
}

bool is_compressed(GLenum internal) {
    return internal == GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT || internal == GL_COMPRESSED_RGBA_ASTC_8x8_KHR
           || internal == GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
}

void set_param(SamplerParams& p, int* base, int* max, GLenum name, float value) {
    const GLenum e = static_cast<GLenum>(value);
    switch (name) {
        case GL_TEXTURE_MIN_FILTER:
            p.min = e;
            break;
        case GL_TEXTURE_MAG_FILTER:
            p.mag = e;
            break;
        case GL_TEXTURE_WRAP_S:
            p.wrap_s = e;
            break;
        case GL_TEXTURE_WRAP_T:
            p.wrap_t = e;
            break;
        case GL_TEXTURE_MAX_ANISOTROPY:
            p.anisotropy = value;
            break;
        case wgl::GL_TEXTURE_BASE_LEVEL:
            if (base != nullptr) {
                *base = static_cast<int>(value);
            }
            break;
        case GL_TEXTURE_MAX_LEVEL:
            if (max != nullptr) {
                *max = static_cast<int>(value);
            }
            break;
        default:
            break;
    }
}

GLuint* buffer_binding(GLenum target) {
    State& s = g->state;
    switch (target) {
        case GL_ARRAY_BUFFER:
            return &s.array_buffer;
        case GL_ELEMENT_ARRAY_BUFFER:
            return &vertex_array().elements;
        case wgl::GL_UNIFORM_BUFFER:
            return &s.uniform_buffer;
        case wgl::GL_TEXTURE_BUFFER:
            return &s.texture_buffer;
        case GL_SHADER_STORAGE_BUFFER:
            return &s.storage_buffer;
        default:
            return nullptr;
    }
}

Framebuffer* bound_framebuffer(GLenum target) {
    return g->framebuffers.find(
        target == GL_READ_FRAMEBUFFER ? g->state.read_framebuffer : g->state.draw_framebuffer);
}

// A half float's value (IEEE 754 binary16).
float half_to_float(std::uint16_t h) {
    const int exponent = (h >> 10) & 0x1F;
    const int mantissa = h & 0x3FF;
    const float sign = (h & 0x8000) != 0 ? -1.0f : 1.0f;
    if (exponent == 0) {
        return sign * std::ldexp(static_cast<float>(mantissa), -24);
    }
    if (exponent == 31) {
        return mantissa != 0 ? NAN : sign * INFINITY;
    }
    return sign * std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, exponent - 15);
}

}  // namespace

#define MTLGL OPENRAC_GLAPI

void MTLGL read_pixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void* pixels) {
    id<MTLTexture> image = target_of(g->state.read_framebuffer).colour;
    // A float frame (an HDR one) is read as bytes, clamped as an 8-bit one's would be, or as
    // floats (GL_FLOAT).
    const bool half = image != nil && image.pixelFormat == MTLPixelFormatRGBA16Float;
    if (image == nil || w <= 0 || h <= 0 || format != GL_RGBA
        || (type != GL_UNSIGNED_BYTE && !(half && type == GL_FLOAT))
        || (!half && image.pixelFormat != MTLPixelFormatRGBA8Unorm) || image.sampleCount != 1) {
        fail(kGlInvalidEnum);
        return;
    }
    const int width = static_cast<int>(image.width), height = static_cast<int>(image.height);
    const std::size_t texel = half ? 8 : 4;
    const std::size_t out_texel = type == GL_FLOAT ? 16 : 4;
    const int x0 = std::clamp(x, 0, width), y0 = std::clamp(y, 0, height);
    const int x1 = std::clamp(x + w, x0, width), y1 = std::clamp(y + h, y0, height);
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    const int cw = x1 - x0, ch = y1 - y0;
    const std::size_t row = static_cast<std::size_t>(cw) * texel;
    id<MTLBuffer> read = [g->device newBufferWithLength:row * static_cast<std::size_t>(ch)
                                                options:MTLResourceStorageModeShared];
    [blit() copyFromTexture:image
                     sourceSlice:0
                     sourceLevel:0
                    sourceOrigin:MTLOriginMake(static_cast<NSUInteger>(x0), static_cast<NSUInteger>(y0), 0)
                      sourceSize:MTLSizeMake(static_cast<NSUInteger>(cw), static_cast<NSUInteger>(ch), 1)
                        toBuffer:read
               destinationOffset:0
          destinationBytesPerRow:row
        destinationBytesPerImage:row * static_cast<std::size_t>(ch)];
    submit(true);
    const std::size_t align = static_cast<std::size_t>(std::max(g->state.pack_alignment, 1));
    const std::size_t pitch = (static_cast<std::size_t>(w) * out_texel + align - 1) / align * align;
    const auto* src = static_cast<const std::uint8_t*>(read.contents);
    auto* dst = static_cast<std::uint8_t*>(pixels);
    for (int r = 0; r < ch; ++r) {
        std::uint8_t* to = dst + pitch * static_cast<std::size_t>(r + (y0 - y))
                           + static_cast<std::size_t>(x0 - x) * out_texel;
        const std::uint8_t* from = src + static_cast<std::size_t>(r) * row;
        if (!half) {
            std::memcpy(to, from, row);
            continue;
        }
        for (std::size_t n = 0; n < static_cast<std::size_t>(cw) * 4; ++n) {
            std::uint16_t bits = 0;
            std::memcpy(&bits, from + n * 2, 2);
            const float value = half_to_float(bits);
            if (type == GL_FLOAT) {
                std::memcpy(to + n * 4, &value, 4);
            } else {
                to[n] = static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
            }
        }
    }
}

namespace {
void draw_blit(id<MTLTexture> src, id<MTLTexture> dst, GLint sx0, GLint sy0, GLint sx1, GLint sy1,
               GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLenum filter, NSUInteger level = 0);
}  // namespace

void MTLGL copy_tex_sub_image(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y,
                              GLsizei w, GLsizei h) {
    Texture* t = bound_texture(target);
    id<MTLTexture> from = target_of(g->state.read_framebuffer).colour;
    if (t == nullptr || t->texture == nil || from == nil || w <= 0 || h <= 0 || from.sampleCount != 1) {
        return;
    }
    id<MTLTexture> to = t->texture;
    // Clipped to both images, as GL leaves what lies outside the source undefined.
    const int sx = std::max(x, 0), sy = std::max(y, 0);
    const int dx = xoff + (sx - x), dy = yoff + (sy - y);
    const int cw = std::min({w - (sx - x), static_cast<int>(from.width) - sx,
                             std::max(static_cast<int>(to.width) >> level, 1) - dx});
    const int ch = std::min({h - (sy - y), static_cast<int>(from.height) - sy,
                             std::max(static_cast<int>(to.height) >> level, 1) - dy});
    if (cw <= 0 || ch <= 0 || dx < 0 || dy < 0 || level < 0 || level >= static_cast<int>(to.mipmapLevelCount)) {
        return;
    }
    t->used = g->serial;
    if (from.pixelFormat != to.pixelFormat) {
        if (is_integer(from.pixelFormat) || is_integer(to.pixelFormat)
            || (to.usage & MTLTextureUsageRenderTarget) == 0) {
            fail(kGlInvalidOperation);
            return;
        }
        draw_blit(from, to, sx, sy, sx + cw, sy + ch, dx, dy, dx + cw, dy + ch, GL_NEAREST,
                  static_cast<NSUInteger>(level));
        return;
    }
    [blit() copyFromTexture:from
                sourceSlice:0
                sourceLevel:0
               sourceOrigin:MTLOriginMake(static_cast<NSUInteger>(sx), static_cast<NSUInteger>(sy), 0)
                 sourceSize:MTLSizeMake(static_cast<NSUInteger>(cw), static_cast<NSUInteger>(ch), 1)
                  toTexture:to
           destinationSlice:0
           destinationLevel:static_cast<NSUInteger>(level)
          destinationOrigin:MTLOriginMake(static_cast<NSUInteger>(dx), static_cast<NSUInteger>(dy), 0)];
}

// ---- Textures ----

void MTLGL gen_textures(GLsizei n, GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        names[i] = g->textures.add();
    }
}

void MTLGL delete_textures(GLsizei n, const GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        if (g->textures.objects.erase(names[i]) == 0) {
            continue;
        }
        for (GLuint* units : {g->state.unit_2d, g->state.unit_buffer, g->state.unit_array}) {
            std::replace(units, units + kUnits, names[i], 0u);
        }
    }
}

void MTLGL bind_texture(GLenum target, GLuint name) {
    State& s = g->state;
    if (Texture* t = g->textures.find(name); t != nullptr && t->target == 0) {
        t->target = target;
    }
    switch (target) {
        case GL_TEXTURE_2D:
            s.unit_2d[s.active_unit] = name;
            break;
        case wgl::GL_TEXTURE_BUFFER:
            s.unit_buffer[s.active_unit] = name;
            break;
        case wgl::GL_TEXTURE_2D_ARRAY:
            s.unit_array[s.active_unit] = name;
            break;
        default:
            break;
    }
}

void MTLGL active_texture(GLenum unit) {
    g->state.active_unit = std::min<GLuint>(unit - GL_TEXTURE0, kUnits - 1);
}

void MTLGL tex_image_2d(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint,
                        GLenum format, GLenum type, const void* pixels) {
    Texture* t = bound_texture(target);
    if (t == nullptr || level < 0) {
        return;
    }
    if (level >= 15 || !extent_valid(w, h)) {
        fail(kGlInvalidValue);
        return;
    }
    const Format f = format_of(static_cast<GLenum>(internal));
    if (f.pixel == MTLPixelFormatInvalid || f.block != 1) {
        fail(kGlInvalidEnum);
        return;
    }
    if (level == 0) {
        id<MTLTexture> old = t->texture;
        // Same size and format: written in place, in order with the frame's use of it. Else a new
        // texture, which what was recorded with the old one does not see.
        if (old == nil || static_cast<int>(old.width) != w || static_cast<int>(old.height) != h
            || old.pixelFormat != f.pixel) {
            t->texture = make_texture(f, w, h, 1, 1);
            t->used = 0;
            if (pixels == nullptr) {
                zero(t->texture);
            }
        }
        t->internal = static_cast<GLenum>(internal);
    } else {
        if (t->texture == nil || std::max(static_cast<int>(t->texture.width) >> level, 1) != w
            || std::max(static_cast<int>(t->texture.height) >> level, 1) != h) {
            return;  // a level that does not belong to the first one's chain
        }
        with_chain(*t);
    }
    if (pixels == nullptr) {
        return;
    }
    const auto staged = stage_pixels(f, w, h, format, type, pixels);
    upload(*t, level, 0, 0, w, h, staged.data(), staged.size(),
           static_cast<std::size_t>(w) * static_cast<std::size_t>(f.bytes));
}

void MTLGL compressed_tex_image_2d(GLenum target, GLint level, GLenum internal, GLsizei w, GLsizei h,
                                   GLint border, GLsizei size, const void* pixels) {
    Texture* t = bound_texture(target);
    const Format f = format_of(internal);
    if (!extent_valid(w, h)) {
        fail(kGlInvalidValue);
        return;
    }
    // Only whole single-level atlases come this way (vkgl's compressed_tex_image_2d).
    if (t == nullptr || level != 0 || border != 0 || !is_compressed(internal)
        || f.pixel == MTLPixelFormatInvalid || size < 0
        || static_cast<std::size_t>(size) != compressed_bytes(internal, w, h)) {
        fail(kGlInvalidEnum);
        return;
    }
    t->texture = make_texture(f, w, h, 1, 1);
    t->internal = internal;
    t->used = 0;
    if (pixels != nullptr) {
        upload(*t, 0, 0, 0, w, h, pixels, static_cast<std::size_t>(size),
               static_cast<std::size_t>((w + f.block - 1) / f.block) * 16);
    }
}

void MTLGL compressed_tex_sub_image_2d(GLenum target, GLint level, GLint x, GLint y, GLsizei w,
                                       GLsizei h, GLenum internal, GLsizei size, const void* pixels) {
    Texture* t = bound_texture(target);
    const int side = internal == GL_COMPRESSED_RGBA_ASTC_8x8_KHR ? 8 : 4;
    const int tw = t != nullptr && t->texture != nil ? static_cast<int>(t->texture.width) : 0;
    const int th = t != nullptr && t->texture != nil ? static_cast<int>(t->texture.height) : 0;
    if (t == nullptr || t->texture == nil || pixels == nullptr || level != 0 || internal != t->internal
        || !is_compressed(internal) || x < 0 || y < 0 || w <= 0 || h <= 0 || size < 0 || x % side != 0
        || y % side != 0 || x > tw || w > tw - x || y > th || h > th - y
        || (w % side != 0 && x + w != tw) || (h % side != 0 && y + h != th)
        || static_cast<std::size_t>(size) != compressed_bytes(internal, w, h)) {
        fail(kGlInvalidEnum);
        return;
    }
    upload(*t, 0, x, y, w, h, pixels, static_cast<std::size_t>(size),
           static_cast<std::size_t>((w + side - 1) / side) * 16);
}

void MTLGL tex_sub_image_2d(GLenum target, GLint level, GLint x, GLint y, GLsizei w, GLsizei h,
                            GLenum format, GLenum type, const void* pixels) {
    Texture* t = bound_texture(target);
    if (t == nullptr || t->texture == nil || pixels == nullptr || w <= 0 || h <= 0 || level < 0
        || level >= static_cast<int>(t->texture.mipmapLevelCount)) {
        return;
    }
    if (x < 0 || y < 0 || x + w > std::max(static_cast<int>(t->texture.width) >> level, 1)
        || y + h > std::max(static_cast<int>(t->texture.height) >> level, 1)) {
        return;
    }
    const Format f = format_of(t->internal);
    const auto staged = stage_pixels(f, w, h, format, type, pixels);
    upload(*t, level, x, y, w, h, staged.data(), staged.size(),
           static_cast<std::size_t>(w) * static_cast<std::size_t>(f.bytes));
}

void MTLGL tex_image_3d(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum,
                        const void*) {
    fail(kGlInvalidEnum);  // no renderer draws with a texture array yet, as in vkgl
}

void MTLGL tex_parameter_i(GLenum target, GLenum name, GLint value) {
    if (Texture* t = bound_texture(target)) {
        set_param(t->params, &t->base_level, &t->max_level, name, static_cast<float>(value));
    }
}

void MTLGL tex_parameter_f(GLenum target, GLenum name, GLfloat value) {
    if (Texture* t = bound_texture(target)) {
        set_param(t->params, &t->base_level, &t->max_level, name, value);
    }
}

void MTLGL generate_mipmap(GLenum target) {
    Texture* t = bound_texture(target);
    if (t == nullptr || t->texture == nil || is_depth(t->texture.pixelFormat)) {
        return;
    }
    with_chain(*t);
    if (t->texture.mipmapLevelCount > 1) {
        [blit_for(*t) generateMipmapsForTexture:t->texture];
    }
}

void MTLGL tex_buffer(GLenum target, GLenum format, GLuint buffer) {
    if (Texture* t = bound_texture(target)) {
        t->buffer = buffer;
        t->buffer_format = format;
    }
}

// ---- Samplers ----

void MTLGL gen_samplers(GLsizei n, GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        names[i] = g->samplers.add();
    }
}

void MTLGL delete_samplers(GLsizei n, const GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        g->samplers.objects.erase(names[i]);
        std::replace(g->state.unit_sampler, g->state.unit_sampler + kUnits, names[i], 0u);
    }
}

void MTLGL bind_sampler(GLuint unit, GLuint sampler) {
    if (unit < kUnits) {
        g->state.unit_sampler[unit] = sampler;
    }
}

void MTLGL sampler_parameter_i(GLuint sampler, GLenum name, GLint value) {
    if (SamplerParams* p = g->samplers.find(sampler)) {
        set_param(*p, nullptr, nullptr, name, static_cast<float>(value));
    }
}

// ---- Buffers ----

void MTLGL gen_buffers(GLsizei n, GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        names[i] = g->buffers.add();
    }
}

void MTLGL delete_buffers(GLsizei n, const GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        g->buffers.objects.erase(names[i]);
    }
}

void MTLGL bind_buffer(GLenum target, GLuint name) {
    if (GLuint* b = buffer_binding(target)) {
        *b = name;
    }
}

void MTLGL bind_buffer_base(GLenum target, GLuint index, GLuint buffer) {
    if (target == GL_SHADER_STORAGE_BUFFER && index < kUniformBindings) {
        g->state.storage_bindings[index] = buffer;
    }
    if (target == wgl::GL_UNIFORM_BUFFER && index < kUniformBindings) {
        g->state.uniform_bindings[index] = buffer;
    }
    bind_buffer(target, buffer);
}

// Always a new MTLBuffer: the frames in flight keep the old one, so nothing waits for the GPU.
void MTLGL buffer_data(GLenum target, GLsizeiptr size, const void* data, GLenum) {
    GLuint* binding = buffer_binding(target);
    Buffer* b = binding != nullptr ? g->buffers.find(*binding) : nullptr;
    if (b == nullptr || size < 0) {
        return;
    }
    const auto length = static_cast<std::size_t>(size);
    // Rounded up so that a texel buffer's row, which Metal aligns (256 bytes at most), fits.
    b->buffer = length == 0 ? nil
                            : [g->device newBufferWithLength:(length + 255) & ~std::size_t{255}
                                                     options:MTLResourceStorageModeShared];
    if (data != nullptr && length != 0) {
        std::memcpy(b->buffer.contents, data, length);
    }
    b->size = length;
    b->used = 0;
}

void MTLGL buffer_sub_data(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    GLuint* binding = buffer_binding(target);
    Buffer* b = binding != nullptr ? g->buffers.find(*binding) : nullptr;
    if (size == 0) {
        return;
    }
    // Subtraction checks the range without overflowing offset + size on malformed input.
    if (b == nullptr || offset < 0 || size < 0 || data == nullptr
        || static_cast<std::size_t>(offset) > b->size
        || static_cast<std::size_t>(size) > b->size - static_cast<std::size_t>(offset)) {
        fail(kGlInvalidValue);
        return;
    }
    // Commands recorded or still running read the bytes they were recorded with: a buffer they
    // use is copied, and the copy written.
    if (b->used > g->completed.load(std::memory_order_acquire)) {
        b->buffer = [g->device newBufferWithBytes:b->buffer.contents
                                           length:b->buffer.length
                                          options:MTLResourceStorageModeShared];
        b->used = 0;
    }
    std::memcpy(static_cast<std::uint8_t*>(b->buffer.contents) + offset, data,
                static_cast<std::size_t>(size));
}

// ---- Framebuffers and renderbuffers ----

void MTLGL gen_framebuffers(GLsizei n, GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        names[i] = g->framebuffers.add();
    }
}

void MTLGL delete_framebuffers(GLsizei n, const GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        g->framebuffers.objects.erase(names[i]);
        if (g->state.draw_framebuffer == names[i]) {
            g->state.draw_framebuffer = 0;
        }
        if (g->state.read_framebuffer == names[i]) {
            g->state.read_framebuffer = 0;
        }
    }
}

void MTLGL bind_framebuffer(GLenum target, GLuint name) {
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        g->state.draw_framebuffer = name;
    }
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        g->state.read_framebuffer = name;
    }
}

void MTLGL framebuffer_texture_2d(GLenum target, GLenum attachment, GLenum, GLuint texture, GLint level) {
    Framebuffer* fb = bound_framebuffer(target);
    if (fb == nullptr) {
        return;
    }
    if (attachment == GL_COLOR_ATTACHMENT0) {
        fb->colour_texture = texture;
        fb->colour_level = level;
        fb->colour_renderbuffer = 0;
    } else if (attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        fb->depth_texture = texture;
        fb->depth_renderbuffer = 0;
    }
}

void MTLGL framebuffer_renderbuffer(GLenum target, GLenum attachment, GLenum, GLuint renderbuffer) {
    Framebuffer* fb = bound_framebuffer(target);
    if (fb == nullptr) {
        return;
    }
    if (attachment == GL_COLOR_ATTACHMENT0) {
        fb->colour_renderbuffer = renderbuffer;
        fb->colour_texture = 0;
    } else if (attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        fb->depth_renderbuffer = renderbuffer;
        fb->depth_texture = 0;
    }
}

GLenum MTLGL check_framebuffer_status(GLenum target) {
    constexpr GLenum kIncompleteAttachment = 0x8CD6;
    constexpr GLenum kMissingAttachment = 0x8CD7;
    constexpr GLenum kIncompleteMultisample = 0x8D56;
    const Target t =
        target_of(target == GL_READ_FRAMEBUFFER ? g->state.read_framebuffer : g->state.draw_framebuffer);
    const Framebuffer* fb = bound_framebuffer(target);
    if (fb != nullptr
        && (((fb->colour_texture != 0 || fb->colour_renderbuffer != 0) && t.colour == nil)
            || ((fb->depth_texture != 0 || fb->depth_renderbuffer != 0) && t.depth == nil))) {
        return kIncompleteAttachment;
    }
    if (t.colour != nil && t.depth != nil && t.colour.sampleCount != t.depth.sampleCount) {
        return kIncompleteMultisample;
    }
    return t.colour != nil || t.depth != nil ? GL_FRAMEBUFFER_COMPLETE : kMissingAttachment;
}

void MTLGL gen_renderbuffers(GLsizei n, GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        names[i] = g->renderbuffers.add();
    }
}

void MTLGL delete_renderbuffers(GLsizei n, const GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        g->renderbuffers.objects.erase(names[i]);
    }
}

void MTLGL bind_renderbuffer(GLenum, GLuint name) {
    g->state.renderbuffer = name;
}

void MTLGL renderbuffer_storage_multisample(GLenum, GLsizei samples, GLenum format, GLsizei w, GLsizei h) {
    Renderbuffer* r = g->renderbuffers.find(g->state.renderbuffer);
    const Format f = format_of(format);
    if (r == nullptr || f.pixel == MTLPixelFormatInvalid || f.block != 1) {
        return;
    }
    if (!extent_valid(w, h)) {
        fail(kGlInvalidValue);
        return;
    }
    // The most samples the device has at or below those asked for, as GL rounds up to one it has.
    int count = 1;
    for (const int c : {8, 4, 2}) {
        if (samples >= c && [g->device supportsTextureSampleCount:static_cast<NSUInteger>(c)]) {
            count = c;
            break;
        }
    }
    r->texture = make_texture(f, w, h, 1, count);
    zero(r->texture);
}

void MTLGL renderbuffer_storage(GLenum target, GLenum format, GLsizei w, GLsizei h) {
    renderbuffer_storage_multisample(target, 0, format, w, h);
}

namespace {

// A scaled, flipped or converting colour blit, drawn: `src` sampled into the destination box,
// clipped to `dst`.
void draw_blit(id<MTLTexture> src, id<MTLTexture> dst, GLint sx0, GLint sy0, GLint sx1, GLint sy1,
               GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLenum filter, NSUInteger level) {
    // Along one axis: the destination's pixels lo..hi inside `size`, and the source's coordinates
    // (0..1) at them, as d0..d1 shows s0..s1.
    struct Axis {
        int lo, hi;
        float a, b;
    };
    auto axis = [](int s0, int s1, int d0, int d1, NSUInteger size, NSUInteger source) {
        const int lo = std::max(std::min(d0, d1), 0);
        const int hi = std::min(std::max(d0, d1), static_cast<int>(size));
        auto at = [&](int d) {
            return static_cast<float>((s0 + (s1 - s0) * static_cast<double>(d - d0) / (d1 - d0)) / source);
        };
        return hi > lo ? Axis{lo, hi, at(lo), at(hi)} : Axis{0, 0, 0, 0};
    };
    const Axis x = axis(sx0, sx1, dx0, dx1, std::max<NSUInteger>(dst.width >> level, 1), src.width);
    const Axis y = axis(sy0, sy1, dy0, dy1, std::max<NSUInteger>(dst.height >> level, 1), src.height);
    if (x.hi == x.lo || y.hi == y.lo) {
        return;
    }
    const bool depth = is_depth(dst.pixelFormat);
    __strong id<MTLRenderPipelineState>& pipeline = g->blits[dst.pixelFormat | dst.sampleCount << 16];
    if (pipeline == nil) {
        MTLRenderPipelineDescriptor* d = [MTLRenderPipelineDescriptor new];
        d.vertexFunction = [g->library newFunctionWithName:@"blit_vertex"];
        d.fragmentFunction = [g->library newFunctionWithName:depth                         ? @"blit_depth"
                                                             : is_integer(dst.pixelFormat) ? @"blit_uint"
                                                                                           : @"blit_fragment"];
        d.rasterSampleCount = dst.sampleCount;
        if (depth) {
            d.depthAttachmentPixelFormat = dst.pixelFormat;
        } else {
            d.colorAttachments[0].pixelFormat = dst.pixelFormat;
        }
        pipeline = [g->device newRenderPipelineStateWithDescriptor:d error:nil];
    }
    end_encoders();
    MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLRenderPassAttachmentDescriptor* to = depth ? d.depthAttachment : d.colorAttachments[0];
    to.texture = dst;
    to.level = level;
    to.loadAction = MTLLoadActionLoad;
    to.storeAction = MTLStoreActionStore;
    id<MTLRenderCommandEncoder> pass = [commands() renderCommandEncoderWithDescriptor:d];
    if (depth) {
        __strong id<MTLDepthStencilState>& state = g->depth_states["blit"];
        if (state == nil) {
            MTLDepthStencilDescriptor* written = [MTLDepthStencilDescriptor new];
            written.depthWriteEnabled = YES;
            state = [g->device newDepthStencilStateWithDescriptor:written];
        }
        [pass setDepthStencilState:state];
    }
    const float uv[4] = {x.a, y.a, x.b, y.b};
    const std::uint32_t linear = filter == GL_LINEAR ? 1 : 0;
    [pass setRenderPipelineState:pipeline];
    [pass setViewport:{static_cast<double>(x.lo), static_cast<double>(y.lo), static_cast<double>(x.hi - x.lo),
                       static_cast<double>(y.hi - y.lo), 0.0, 1.0}];
    [pass setVertexBytes:uv length:sizeof uv atIndex:0];
    [pass setFragmentTexture:src atIndex:0];
    [pass setFragmentBytes:&linear length:sizeof linear atIndex:0];
    [pass drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [pass endEncoding];
}

}  // namespace

// Copies of one size and format and resolves; other blits drawn. A scaled, flipped or converting
// blit of stencil, a resolve of part of depth and part of one multisampled image copied into
// another are GL_INVALID_OPERATION (README.md).
void MTLGL blit_framebuffer(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0,
                            GLint dx1, GLint dy1, GLbitfield mask, GLenum filter) {
    const Target from = target_of(g->state.read_framebuffer);
    const Target to = target_of(g->state.draw_framebuffer);
    auto blit_one = [&](id<MTLTexture> src, id<MTLTexture> dst, bool stencil) {
        if (src == nil || dst == nil || src == dst) {
            return;
        }
        const bool exact = is_depth(src.pixelFormat) || is_integer(src.pixelFormat);
        if (is_integer(src.pixelFormat) != is_integer(dst.pixelFormat) || (exact && filter != GL_NEAREST)) {
            fail(kGlInvalidOperation);
            return;
        }
        const int w = sx1 - sx0, h = sy1 - sy0;
        const bool same_size = w > 0 && h > 0 && dx1 - dx0 == w && dy1 - dy0 == h;
        // GL's rules for multisampled blits.
        if ((src.sampleCount > 1 || dst.sampleCount > 1)
            && (!same_size
                || (src.sampleCount > 1 && (src.pixelFormat != dst.pixelFormat
                                            || (dst.sampleCount > 1 && dst.sampleCount != src.sampleCount))))) {
            fail(kGlInvalidOperation);
            return;
        }
        if (!same_size || src.pixelFormat != dst.pixelFormat || src.sampleCount < dst.sampleCount) {
            if (stencil) {
                fail(kGlInvalidOperation);
                return;
            }
            draw_blit(src, dst, sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, filter);
            return;
        }
        const bool whole = sx0 == 0 && sy0 == 0 && dx0 == 0 && dy0 == 0
                           && static_cast<NSUInteger>(w) == src.width && src.width == dst.width
                           && static_cast<NSUInteger>(h) == src.height && src.height == dst.height;
        if (src.sampleCount > dst.sampleCount) {
            if (is_depth(src.pixelFormat)) {
                if (!whole) {
                    fail(kGlInvalidOperation);
                    return;
                }
                end_encoders();
                MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
                auto resolve = [&](MTLRenderPassAttachmentDescriptor* a) {
                    a.texture = src;
                    a.loadAction = MTLLoadActionLoad;
                    a.storeAction = MTLStoreActionStoreAndMultisampleResolve;
                    a.resolveTexture = dst;
                };
                if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
                    resolve(d.depthAttachment);
                }
                if (stencil && src.pixelFormat == MTLPixelFormatDepth32Float_Stencil8) {
                    resolve(d.stencilAttachment);
                }
                [[commands() renderCommandEncoderWithDescriptor:d] endEncoding];
                return;
            }
            // Part of an image is resolved whole into a scratch one, then copied.
            id<MTLTexture> resolved = dst;
            if (!whole) {
                MTLTextureDescriptor* t = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:src.pixelFormat
                                                                                             width:src.width
                                                                                            height:src.height
                                                                                         mipmapped:NO];
                t.storageMode = MTLStorageModePrivate;
                t.usage = MTLTextureUsageRenderTarget;
                resolved = [g->device newTextureWithDescriptor:t];
            }
            __strong id<MTLRenderPipelineState>& pipeline = g->blits[src.pixelFormat | NSUInteger{1} << 24];
            if (pipeline == nil) {
                MTLRenderPipelineDescriptor* p = [MTLRenderPipelineDescriptor new];
                p.vertexFunction = [g->library newFunctionWithName:@"flip_vertex"];
                const bool bytes = src.pixelFormat == MTLPixelFormatRGBA8Unorm
                                   && ![g->device supportsFamily:MTLGPUFamilyApple1];
                p.fragmentFunction = [g->library newFunctionWithName:bytes ? @"resolve_bytes" : @"resolve_fragment"];
                p.colorAttachments[0].pixelFormat = src.pixelFormat;
                pipeline = [g->device newRenderPipelineStateWithDescriptor:p error:nil];
            }
            end_encoders();
            MTLRenderPassDescriptor* d = [MTLRenderPassDescriptor renderPassDescriptor];
            d.colorAttachments[0].texture = resolved;
            d.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            d.colorAttachments[0].storeAction = MTLStoreActionStore;
            id<MTLRenderCommandEncoder> pass = [commands() renderCommandEncoderWithDescriptor:d];
            [pass setRenderPipelineState:pipeline];
            [pass setFragmentTexture:src atIndex:0];
            [pass drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [pass endEncoding];
            if (whole) {
                return;
            }
            src = resolved;
        } else if (src.sampleCount > 1) {
            if (!whole) {
                fail(kGlInvalidOperation);
                return;
            }
            [blit() copyFromTexture:src toTexture:dst];
            return;
        }
        // Clipped to both images, as copy_tex_sub_image is.
        const int x0 = std::max(sx0, 0), y0 = std::max(sy0, 0);
        const int ox = dx0 + (x0 - sx0), oy = dy0 + (y0 - sy0);
        const int cw = std::min(sx1, static_cast<int>(src.width)) - x0 - std::max(0, ox + (std::min(sx1, static_cast<int>(src.width)) - x0) - static_cast<int>(dst.width));
        const int ch = std::min(sy1, static_cast<int>(src.height)) - y0 - std::max(0, oy + (std::min(sy1, static_cast<int>(src.height)) - y0) - static_cast<int>(dst.height));
        if (cw <= 0 || ch <= 0 || ox < 0 || oy < 0) {
            return;
        }
        [blit() copyFromTexture:src
                    sourceSlice:0
                    sourceLevel:0
                   sourceOrigin:MTLOriginMake(static_cast<NSUInteger>(x0), static_cast<NSUInteger>(y0), 0)
                     sourceSize:MTLSizeMake(static_cast<NSUInteger>(cw), static_cast<NSUInteger>(ch), 1)
                      toTexture:dst
               destinationSlice:0
               destinationLevel:0
              destinationOrigin:MTLOriginMake(static_cast<NSUInteger>(ox), static_cast<NSUInteger>(oy), 0)];
    };
    if ((mask & GL_COLOR_BUFFER_BIT) != 0) {
        blit_one(from.colour, to.colour, false);
    }
    if ((mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0) {
        blit_one(from.depth, to.depth, (mask & GL_STENCIL_BUFFER_BIT) != 0);
    }
}

#undef MTLGL

}  // namespace openrac::mtlgl::detail
