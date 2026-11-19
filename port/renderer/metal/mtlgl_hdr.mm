// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR output on Metal (mtlgl.h, docs/port/HDR.md): the window's CAMetalLayer set up for macOS's
// EDR when HDR is asked for and the display offers it, and the encode pass that writes the default
// framebuffer's picture into the drawable. The pass is the Vulkan backend's own
// (renderer/vulkan/shaders/hdr_encode.comp, scRGB form), compiled by its glslang and translated to
// MSL, so the two renderers send the screen the same values. With HDR not asked for, nothing here
// runs: the layer and the present are as they always were.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "common/log.h"
#include "renderer/metal/mtlgl_internal.h"
#include "renderer/vulkan/pipelines/shader_compiler.h"
#include "vkgl_shaders.h"

namespace openrac::mtlgl {

namespace detail {

namespace {

// The encode pass's push constants (hdr_encode.comp's Encode, as hdr_output.cpp fills them).
struct EncodeConstants {
    float paper_white;
    float sdr_gamma;
    std::int32_t same_size;
    float headroom;
    float highlights;
    float sdr_hue;
    float sdr_chroma;
    float blowout;
    std::int32_t plain;
};

// hdr_encode.comp's bindings: the picture (texture and sampler slot 0) and the screen (texture
// slot 1); its push constants are buffer slot 0 (shader_translate.h, translate_native_compute).
constexpr NSUInteger kPictureSlot = 0;
constexpr NSUInteger kScreenSlot = 1;
constexpr NSUInteger kConstantsSlot = 0;

// The encode pipeline, made the first time HDR is shown. False, once and for good, when it cannot
// be made: the picture then goes out in SDR.
bool make_encode() {
    HdrState& hdr = g->hdr;
    if (hdr.encode != nil) {
        return true;
    }
    if (hdr.failed) {
        return false;
    }
    std::string error;
    vkgl::NativeComputeShader shader;
    MslProgram msl;
    // No definitions: the scRGB form, which stores half floats.
    if (!vkgl::compile_native_compute(vkgl::shaders::hdr_encode_comp, {}, {}, shader, error)
        || !translate_native_compute(shader.code, msl, error)) {
        log::warn("metal: no HDR encode pass ({}); the picture goes out in SDR", error);
        hdr.failed = true;
        return false;
    }
    MTLCompileOptions* exact = [MTLCompileOptions new];
    exact.fastMathEnabled = NO;
    NSError* failure = nil;
    id<MTLLibrary> library = [g->device newLibraryWithSource:@(msl.compute.c_str())
                                                     options:exact
                                                       error:&failure];
    id<MTLFunction> function =
        library == nil ? nil : [library newFunctionWithName:@(msl.compute_entry.c_str())];
    id<MTLComputePipelineState> pipeline =
        function == nil ? nil
                        : [g->device newComputePipelineStateWithFunction:function error:&failure];
    if (pipeline == nil) {
        log::warn(
            "metal: no HDR encode pass ({}); the picture goes out in SDR",
            failure != nil ? failure.localizedDescription.UTF8String : "no entry point"
        );
        hdr.failed = true;
        return false;
    }
    MTLSamplerDescriptor* linear = [MTLSamplerDescriptor new];
    linear.minFilter = MTLSamplerMinMagFilterLinear;
    linear.magFilter = MTLSamplerMinMagFilterLinear;
    linear.sAddressMode = MTLSamplerAddressModeClampToEdge;
    linear.tAddressMode = MTLSamplerAddressModeClampToEdge;
    hdr.linear = [g->device newSamplerStateWithDescriptor:linear];
    hdr.workgroup = MTLSizeMake(msl.workgroup[0], msl.workgroup[1], msl.workgroup[2]);
    hdr.encode = pipeline;
    return true;
}

// The layer set up as `setup` says. SDR gives the layer back its own colour space.
void configure_layer(const LayerSetup& setup) {
    CAMetalLayer* layer = g->layer;
    if (layer == nil) {
        return;
    }
    layer.wantsExtendedDynamicRangeContent = setup.extended_range;
    layer.pixelFormat = setup.half_float ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = !setup.shader_writes;
    if (!setup.linear_srgb) {
        layer.colorspace = g->hdr.sdr_space.get();
        return;
    }
    const ColourSpace linear(CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearSRGB));
    layer.colorspace = linear.get();
}

}  // namespace

bool encode_hdr(id<MTLTexture> screen) {
    HdrState& hdr = g->hdr;
    if (!make_encode()) {
        return false;
    }
    const hdr::Levels& levels = hdr.levels;
    const bool same_size = screen.width == g->colour.width && screen.height == g->colour.height;
    const EncodeConstants constants{
        levels.paper_white,
        hdr::kSdrGamma,
        same_size ? 1 : 0,
        hdr::headroom(levels),
        levels.look.highlights,
        levels.look.sdr_hue,
        levels.look.sdr_chroma,
        levels.look.blowout,
        levels.look.plain() ? 1 : 0,
    };
    end_encoders();
    id<MTLComputeCommandEncoder> pass = [commands() computeCommandEncoder];
    [pass setComputePipelineState:hdr.encode];
    [pass setTexture:g->colour atIndex:kPictureSlot];
    [pass setSamplerState:hdr.linear atIndex:kPictureSlot];
    [pass setTexture:screen atIndex:kScreenSlot];
    [pass setBytes:&constants length:sizeof(constants) atIndex:kConstantsSlot];
    const MTLSize& group = hdr.workgroup;
    // Whole groups over the screen; the pass leaves out the pixels past its edges.
    [pass dispatchThreadgroups:MTLSizeMake(
                                   (screen.width + group.width - 1) / group.width,
                                   (screen.height + group.height - 1) / group.height,
                                   1
                               )
         threadsPerThreadgroup:group];
    [pass endEncoding];
    return true;
}

}  // namespace detail

using namespace detail;

hdr::Output shown_output(hdr::Output wanted, bool edr) {
    // EDR is linear extended sRGB whatever was asked: HDR10 is not one of its forms.
    return wanted != hdr::Output::Sdr && edr ? hdr::Output::ScRgb : hdr::Output::Sdr;
}

LayerSetup layer_setup(hdr::Output shown) {
    // Anything but SDR is EDR's scRGB (shown_output).
    if (shown == hdr::Output::Sdr) {
        return {};
    }
    return {true, true, true, true};
}

void set_hdr(hdr::Output wanted, const hdr::Levels& levels, bool edr) {
    if (g == nullptr) {
        return;
    }
    HdrState& hdr = g->hdr;
    hdr.offered = edr;
    hdr.levels = levels;
    hdr::Output shown = shown_output(wanted, edr);
    // The pass is built before the layer changes, so a failure leaves the layer in SDR.
    if (shown != hdr::Output::Sdr && !make_encode()) {
        shown = hdr::Output::Sdr;
    }
    if (shown == hdr.shown) {
        return;
    }
    hdr.shown = shown;
    configure_layer(layer_setup(shown));
    if (shown != hdr::Output::Sdr) {
        log::info(
            "metal: HDR output scRGB (EDR) at {:.0f} nits paper white (peak {:.0f})",
            levels.paper_white,
            levels.peak
        );
    } else {
        log::info("metal: HDR output off, SDR");
    }
}

hdr::Output hdr_output() {
    return g == nullptr ? hdr::Output::Sdr : g->hdr.shown;
}

bool hdr_offered() {
    return g != nullptr && g->hdr.offered;
}

LayerSetup current_layer() {
    if (g == nullptr || g->layer == nil) {
        return {};
    }
    CAMetalLayer* layer = g->layer;
    LayerSetup setup;
    setup.extended_range = layer.wantsExtendedDynamicRangeContent;
    setup.half_float = layer.pixelFormat == MTLPixelFormatRGBA16Float;
    const ColourSpace linear(CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearSRGB));
    setup.linear_srgb = layer.colorspace != nullptr && CFEqual(layer.colorspace, linear.get());
    setup.shader_writes = !layer.framebufferOnly;
    return setup;
}

bool test_hdr_encode(
    const hdr::Levels& levels, int width, int height, std::vector<std::uint8_t>& out
) {
    out.clear();
    if (g == nullptr || width <= 0 || height <= 0) {
        return false;
    }
    // A shared texture, readable by the CPU once the GPU is done, standing in for the drawable.
    MTLTextureDescriptor* d =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                           width:static_cast<NSUInteger>(width)
                                                          height:static_cast<NSUInteger>(height)
                                                       mipmapped:NO];
    d.storageMode = MTLStorageModeShared;
    d.usage = MTLTextureUsageShaderWrite;
    id<MTLTexture> screen = [g->device newTextureWithDescriptor:d];
    const hdr::Levels kept = g->hdr.levels;
    g->hdr.levels = levels;
    const bool encoded = screen != nil && encode_hdr(screen);
    g->hdr.levels = kept;
    if (!encoded) {
        return false;
    }
    submit(true);
    // Four half floats a pixel.
    constexpr std::size_t kTexel = 8;
    const std::size_t row = static_cast<std::size_t>(width) * kTexel;
    out.resize(row * static_cast<std::size_t>(height));
    [screen getBytes:out.data()
         bytesPerRow:row
          fromRegion:MTLRegionMake2D(
                         0, 0, static_cast<NSUInteger>(width), static_cast<NSUInteger>(height)
                     )
         mipmapLevel:0];
    return true;
}

}  // namespace openrac::mtlgl
