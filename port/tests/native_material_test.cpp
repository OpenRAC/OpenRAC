// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A world quad's native material, from the C call to the renderer's state for it: the contract's
// layout and enumerations (openrac/material.h), the checks and the native state the game thread
// makes of it, the image read once into the tick's capture (game/common/frontend/
// effect_material.h), and what EffectRenderer sets for that capture (renderer/effects/
// effect_state.h, translate_blend, TexturePool::ui_image). Expected values are the GS blend
// formula (A - B) C + D written out, and arithmetic. older_passes and older_state are
// characterisations of what EffectRenderer read from a quad's GS words before native materials
// (its draw_batch_with_alpha_test and apply), written before effect_passes replaced that code.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "game/common/frontend/effect_material.h"
#include "game/common/frontend/ui_images.h"
#include "openrac/game_host.h"
#include "renderer/direct/direct.h"
#include "renderer/effects/effect_state.h"
#include "renderer/gl/gl.h"
#include "renderer/textures/texture.h"
#include "renderer/textures/texture_pool.h"
#include "renderer_gif_builder.h"
#include "tests/check.h"

namespace {

using namespace openrac::renderer;
using openrac::frontend::capture_material_quad;
using openrac::frontend::effect_state;
using openrac::frontend::UiImageCache;
using openrac::test::alpha_reg;

// The GS blend's operands (ALPHA's A, B and D, then C).
constexpr std::uint8_t kCs = 0, kCd = 1, kZero = 2;
constexpr std::uint8_t kAs = 0, kAd = 1, kFix = 2;

// An alpha of three quarters (0x80 = 1.0), the reference of the tests' alpha tests.
constexpr std::uint8_t kThreeQuarters = 0x60;

// TEX1's MMAG and MMIN bit positions, and a CLAMP that clamps both ways (WMS 1, WMT 1).
constexpr unsigned kTex1Mmag = 5;
constexpr unsigned kTex1Mmin = 6;
constexpr std::uint64_t kClampBoth = 1 | (1 << 2);

// Where the synthetic image lies in the test's game memory, and its palette.
constexpr std::uint32_t kPixels = 0x100;
constexpr std::uint32_t kPalette = 0x400;
constexpr std::size_t kRamBytes = 0x1000;
constexpr std::size_t kPaletteEntries = 256;

// A material that blends by `terms` (source k, m, destination k, m) weighed by `weight`.
openrac_material blended(std::array<int, 4> terms, std::uint8_t weight) {
    openrac_material m{};
    m.source_k = static_cast<std::int8_t>(terms[0]);
    m.source_m = static_cast<std::uint8_t>(terms[1]);
    m.dest_k = static_cast<std::int8_t>(terms[2]);
    m.dest_m = static_cast<std::uint8_t>(terms[3]);
    m.weight = weight;
    m.alpha_test = OPENRAC_MATERIAL_TEST_ALWAYS;
    m.depth_test = OPENRAC_MATERIAL_DEPTH_NEARER_OR_EQUAL;
    m.writes = OPENRAC_MATERIAL_WRITE_COLOUR;
    m.flags = OPENRAC_MATERIAL_BLEND | OPENRAC_MATERIAL_TEXTURE_ALPHA | OPENRAC_MATERIAL_REPEAT_S
              | OPENRAC_MATERIAL_REPEAT_T;
    return m;
}

// The native blend of `material`, which must be in range.
Blend blend_of(const openrac_material& material) {
    const std::optional<EffectState> state = effect_state(material);
    CHECK(state.has_value());
    return state ? state->blend : Blend{};
}

// Whether two GL blends set the same factors, equation, constant and shader weight.
bool same_gl(const GlBlend& a, const GlBlend& b) {
    return a.src == b.src && a.dst == b.dst && a.equation == b.equation && a.constant == b.constant
           && a.source_scale == b.source_scale && a.fix == b.fix && a.exact == b.exact;
}

// The game memory of the tests: an 8-bit 2 x 2 image at kPixels with a 256-colour palette at
// kPalette, entry i the colour (i, 2i, 3i, 0x80) in the game's stored order.
std::vector<std::uint8_t> synthetic_ram() {
    std::vector<std::uint8_t> ram(kRamBytes, 0);
    const std::array<std::uint8_t, 4> indices{1, 2, 3, 4};

    // The image's four pixels, one index a byte.
    for (std::size_t i = 0; i < indices.size(); ++i) {
        ram[kPixels + i] = indices[i];
    }

    // Entries 8-15 and 16-23 of each 32 trade places in the stored order (ui_item.h).
    for (std::uint32_t i = 0; i < kPaletteEntries; ++i) {
        const std::uint32_t stored = ((i >> 3) & 1) != ((i >> 4) & 1) ? i ^ 0x18 : i;
        std::uint8_t* colour = ram.data() + kPalette + stored * 4;
        colour[0] = static_cast<std::uint8_t>(i);
        colour[1] = static_cast<std::uint8_t>(2 * i);
        colour[2] = static_cast<std::uint8_t>(3 * i);
        colour[3] = 0x80;
    }

    return ram;
}

// A textured quad over the synthetic image, with `material`.
openrac_game_material_quad image_quad(const openrac_material& material) {
    openrac_game_material_quad quad{};
    quad.material = material;
    quad.image.pixels = kPixels;
    quad.image.palette = kPalette;
    quad.image.width = 2;
    quad.image.height = 2;
    quad.image.bits = 8;
    quad.image.swizzled = 1;

    // A unit square in x and y, white at full alpha.
    for (std::uint32_t k = 0; k < 4; ++k) {
        quad.corner[k][0] = static_cast<float>(k & 1);
        quad.corner[k][1] = static_cast<float>(k >> 1);
        quad.corner[k][3] = 1.0f;
        quad.rgba[k] = 0x80808080u;
    }

    quad.id = 7;
    return quad;
}

// The four bytes of the decoded texel `index` of `image`.
std::array<std::uint8_t, 4> texel(const Rgba8Image& image, std::size_t index) {
    const std::uint8_t* p = image.pixels.data() + index * 4;
    return {p[0], p[1], p[2], p[3]};
}

// The contract's sizes, and its enumerations as the renderer's (no translation table between).
void test_the_contract_matches_the_renderer_values() {
    static_assert(sizeof(openrac_material) == 16);
    static_assert(sizeof(openrac_game_material_quad) == 156);
    CHECK(OPENRAC_MATERIAL_WEIGHT_DEST_ALPHA == static_cast<int>(BlendWeight::DestinationAlpha));
    CHECK(OPENRAC_MATERIAL_WEIGHT_CONSTANT == static_cast<int>(BlendWeight::Constant));
    CHECK(OPENRAC_MATERIAL_TEST_GREATER_EQUAL == static_cast<int>(Compare::GreaterEqual));
    CHECK(OPENRAC_MATERIAL_TEST_NOT_EQUAL == static_cast<int>(Compare::NotEqual));
    CHECK(OPENRAC_MATERIAL_FAIL_RGB == static_cast<int>(FailWrites::Rgb));
    CHECK(OPENRAC_MATERIAL_FAIL_DEPTH == static_cast<int>(FailWrites::Depth));
    CHECK(OPENRAC_MATERIAL_HIGHLIGHT2 == static_cast<int>(TextureCombine::Highlight2));
    CHECK(
        OPENRAC_MATERIAL_WRITE_ALPHA == kWriteAlpha && OPENRAC_MATERIAL_WRITE_COLOUR == kWriteColour
    );
}

// The recorder's five blends are five sets of coefficients of the one native form.
void test_the_fixed_blends_are_coefficients() {
    openrac_material opaque = blended({1, 0, -1, 1}, OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA);
    opaque.flags &= ~OPENRAC_MATERIAL_BLEND;
    CHECK(blend_of(opaque) == Blend::of(BlendMode::Opaque));
    CHECK(blend_of(blended({1, 0, -1, 1}, 0)) == Blend::of(BlendMode::Mix));
    CHECK(blend_of(blended({1, 0, 0, 1}, 0)) == Blend::of(BlendMode::Add));
    CHECK(blend_of(blended({-1, 0, 0, 1}, 0)) == Blend::of(BlendMode::Subtract));
    CHECK(blend_of(blended({0, 0, -1, 1}, 0)) == Blend::of(BlendMode::Multiply));
    // An unblended material's terms weigh nothing, whatever they are.
    openrac_material odd = blended({0, 1, 0, 0}, OPENRAC_MATERIAL_WEIGHT_CONSTANT);
    odd.flags &= ~OPENRAC_MATERIAL_BLEND;
    CHECK(blend_of(odd) == Blend::of(BlendMode::Opaque));
}

// Layers weighed by the alpha already in the target keep that weight: the GS's (A - B) C + D for
// each, as the same native blend and through the same GL translation as the 2D path's.
void test_destination_alpha_layers_keep_their_weight() {
    constexpr auto dest = OPENRAC_MATERIAL_WEIGHT_DEST_ALPHA;
    constexpr auto fixed = OPENRAC_MATERIAL_WEIGHT_CONSTANT;

    struct Layer {
        std::array<int, 4> terms;
        std::uint8_t weight;
        std::uint64_t alpha;
    };

    const std::array<Layer, 4> layers{{
        {{1, 0, -1, 1}, dest, alpha_reg(kCs, kCd, kAd, kCd)},       // (Cs - Cd) Ad + Cd
        {{1, 0, 0, 1}, dest, alpha_reg(kCs, kZero, kAd, kCd)},      // Cs Ad + Cd
        {{-1, 0, 1, 1}, dest, alpha_reg(kCd, kCs, kAd, kCd)},       // (Cd - Cs) Ad + Cd
        {{0, 0, 0, 1}, fixed, alpha_reg(kZero, kZero, kFix, kCd)},  // Cd: the target's kept
    }};

    // Each layer's native terms are its GS blend's, to the GL factors.
    for (const Layer& layer : layers) {
        const Blend native = blend_of(blended(layer.terms, layer.weight));
        const Blend gs = blend_from_gs(gs::Alpha::decode(layer.alpha));
        CHECK(native == gs);
        CHECK(same_gl(translate_blend(native), translate_blend(gs)));
    }

    const GlBlend over = translate_blend(blend_of(blended({1, 0, -1, 1}, dest)));
    CHECK(over.src == openrac::gl::GL_DST_ALPHA && over.dst == openrac::gl::GL_ONE_MINUS_DST_ALPHA);
    const GlBlend added = translate_blend(blend_of(blended({1, 0, 0, 1}, dest)));
    CHECK(added.src == openrac::gl::GL_DST_ALPHA && added.dst == openrac::gl::GL_ONE);
    // Cd (Ad + 1) - Cs Ad has a term past GL's factors: drawn approximately, and said so.
    CHECK(!translate_blend(blend_of(blended({-1, 0, 1, 1}, dest))).exact);
    // A blend by the source alpha's weight takes the medium's in-scattering; one by Ad does too.
    CHECK(source_over(blend_of(blended({1, 0, -1, 1}, dest))));
    CHECK(!source_over(blend_of(blended({1, 0, 0, 1}, dest))));
}

// The draws of a batch under the list's TEST as EffectRenderer's draw_batch_with_alpha_test made
// them before effect_passes (characterisation).
EffectPasses older_passes(const gs::Test& t, bool z_write, bool tested) {
    EffectPasses out;
    const auto call = [&](int test, bool keep, bool colour, bool alpha, bool z) {
        const auto writes = static_cast<std::uint8_t>(
            (colour ? kWriteRed | kWriteGreen | kWriteBlue : 0) | (alpha ? kWriteAlpha : 0)
        );
        out.pass[out.count++] = {static_cast<std::uint8_t>(test), keep, writes, z};
    };
    const bool fail_colour = t.afail == gs::AlphaFail::FbOnly || t.afail == gs::AlphaFail::RgbOnly;
    const bool fail_alpha = t.afail == gs::AlphaFail::FbOnly;
    const bool fail_z = t.afail == gs::AlphaFail::ZbOnly && z_write;

    // As the older code branched: NEVER, then no test or no tested shader, then the double draw.
    if (t.ate && t.atst == gs::AlphaTest::Never) {
        // A failing pixel that keeps nothing was not drawn.
        if (t.afail != gs::AlphaFail::Keep) {
            call(0, false, fail_colour, fail_alpha, fail_z);
        }
    } else if (!t.ate || t.atst == gs::AlphaTest::Always || !tested) {
        call(0, false, true, true, z_write);
    } else {
        call(static_cast<int>(t.atst), false, true, true, z_write);

        // The failing pixels were drawn again only when AFAIL kept something.
        if (t.afail != gs::AlphaFail::Keep) {
            call(static_cast<int>(t.atst), true, fail_colour, fail_alpha, fail_z);
        }
    }

    return out;
}

// What EffectRenderer read from a quad's GS words before native materials (apply,
// apply_colour_state, its quad loop) when the list gave TEST and ZBUF: ALPHA's blend; TEST's alpha
// test; ZBUF's mask; depth tested as near or nearer; every channel; TEX0's function and TCC;
// TEX1's filters; CLAMP's wrap (characterisation).
EffectState older_state(const std::array<std::uint64_t, 6>& words) {
    const auto [tex0, tex1, clamp, alpha, test, zbuf] = words;
    EffectState s;
    s.blend = blend_from_gs(gs::Alpha::decode(alpha));
    const AlphaTest listed = alpha_test_from_gs(gs::Test::decode(test));

    // ALWAYS draws as no test at all.
    if (listed.compare != Compare::Always) {
        s.alpha_test = listed;
    }

    s.depth_write = !gs::Zbuf::decode(zbuf).zmsk;
    const gs::Tex0 t0 = gs::Tex0::decode(tex0);
    s.combine = static_cast<TextureCombine>(t0.tfx);
    s.texture_alpha = t0.tcc;
    const gs::Tex1 t1 = gs::Tex1::decode(tex1);
    s.linear_mag = t1.mmag;
    // MMIN 1, 4 and 5 are the linear minifications (EffectRenderer::apply).
    s.linear_min = t1.mmin == 1 || t1.mmin == 4 || t1.mmin == 5;
    const gs::Clamp wrap = gs::Clamp::decode(clamp);
    s.repeat_s = wrap.wms == gs::Wrap::Repeat;
    s.repeat_t = wrap.wmt == gs::Wrap::Repeat;
    return s;
}

// A material's alpha test, depth, writes, texture function, filters and wrap become the renderer's
// state as named, and EffectRenderer's GL values follow from them.
void test_tests_writes_and_sampling_become_native_state() {
    openrac_material m = blended({1, 0, -1, 1}, OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA);
    m.alpha_test = OPENRAC_MATERIAL_TEST_GREATER_EQUAL;
    m.alpha_reference = kThreeQuarters;
    m.alpha_fail = OPENRAC_MATERIAL_FAIL_RGB;
    m.depth_test = OPENRAC_MATERIAL_DEPTH_NEARER;
    m.writes = OPENRAC_MATERIAL_WRITE_ALPHA;
    m.combine = OPENRAC_MATERIAL_DECAL;
    m.flags = OPENRAC_MATERIAL_BLEND | OPENRAC_MATERIAL_DEPTH_WRITE | OPENRAC_MATERIAL_LINEAR_MAG
              | OPENRAC_MATERIAL_REPEAT_T;
    const std::optional<EffectState> s = effect_state(m);
    CHECK(s.has_value());

    // The checks below read the state.
    if (!s) {
        return;
    }

    CHECK((s->alpha_test == AlphaTest{Compare::GreaterEqual, kThreeQuarters, FailWrites::Rgb}));
    CHECK((s->depth == DepthTest{true, DepthCompare::Greater}) && s->depth_write);
    CHECK(s->colour_writes == kWriteAlpha && s->combine == TextureCombine::Decal);
    CHECK(!s->texture_alpha && s->linear_mag && !s->linear_min && !s->repeat_s && s->repeat_t);
    const EffectGlState gl = effect_gl_state(*s);
    CHECK(gl.blend && gl.inscatter && gl.tcc == 0 && gl.tfx == 1);
    CHECK(gl.mag_filter == openrac::gl::GL_LINEAR && gl.min_filter == openrac::gl::GL_NEAREST);
    CHECK(gl.depth_function == openrac::gl::GL_GREATER && !gl.repeat_s && gl.repeat_t);
    // ALWAYS is no test at all; a depth not tested still writes, through GL's ALWAYS.
    m.alpha_test = OPENRAC_MATERIAL_TEST_ALWAYS;
    m.depth_test = OPENRAC_MATERIAL_DEPTH_ALWAYS;
    const std::optional<EffectState> plain = effect_state(m);
    CHECK(plain && plain->alpha_test == AlphaTest{} && !plain->depth.enabled);
    CHECK(plain && effect_gl_state(*plain).depth_function == openrac::gl::GL_ALWAYS);
}

// Every field past its range is refused, and its quad is left to the caller with nothing added
// and no image read.
void test_out_of_range_materials_are_refused() {
    const openrac_material good = blended({1, 0, -1, 1}, OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA);
    std::array<openrac_material, 10> bad{};
    bad.fill(good);
    bad[0].source_k = 2;
    bad[1].dest_k = -2;
    bad[2].dest_m = 2;
    bad[3].weight = OPENRAC_MATERIAL_WEIGHT_CONSTANT + 1;
    bad[4].alpha_test = OPENRAC_MATERIAL_TEST_NOT_EQUAL + 1;
    bad[5].alpha_fail = OPENRAC_MATERIAL_FAIL_RGB + 1;
    bad[6].depth_test = OPENRAC_MATERIAL_DEPTH_NEARER + 1;
    bad[7].combine = OPENRAC_MATERIAL_HIGHLIGHT2 + 1;
    bad[8].writes = OPENRAC_MATERIAL_WRITE_COLOUR + 1;
    bad[9].flags |= OPENRAC_MATERIAL_UNTEXTURED << 1;  // a bit no flag names
    std::vector<EffectQuad> quads;
    std::vector<UiImage> images;
    int looked = 0;
    const openrac::frontend::EffectImageLookup lookup = [&](const openrac_game_ui_image&) {
        ++looked;
        return UiImage{};
    };

    // Each refused material leaves no quad and reads no image.
    for (const openrac_material& material : bad) {
        CHECK(!effect_state(material).has_value());
        CHECK(!capture_material_quad(image_quad(material), lookup, quads, images));
    }

    CHECK(quads.empty() && images.empty() && looked == 0);
    CHECK(effect_state(good).has_value());
}

// effect_passes draws every TEST a list can give as the older draws did: all 512 cases of ATE,
// ATST, AFAIL, two AREFs, the Z write and the tested shader.
void test_passes_draw_every_list_test_as_before() {
    constexpr int kCases = 512;
    int differ = 0;

    // The case's bits are, from the lowest: ATE, ATST (3), AFAIL (2), AREF, Z write, tested shader.
    for (int bits = 0; bits < kCases; ++bits) {
        const bool ate = (bits & 1) != 0;
        const auto atst = static_cast<gs::AlphaTest>((bits >> 1) & 7);
        const auto afail = static_cast<gs::AlphaFail>((bits >> 4) & 3);
        const auto aref = static_cast<std::uint8_t>((bits & 64) != 0 ? kThreeQuarters : 0);
        const bool z_write = (bits & 128) != 0;
        const bool tested = (bits & 256) != 0;
        const std::uint64_t word =
            openrac::test::test_reg(ate, atst, aref, afail, gs::DepthTest::Gequal);
        const gs::Test t = gs::Test::decode(word);
        const EffectPasses now =
            effect_passes(alpha_test_from_gs(t), z_write, kWriteColour, tested);
        const EffectPasses before = older_passes(t, z_write, tested);
        bool same = now.count == before.count;

        // Equal counts compare draw by draw.
        for (std::size_t i = 0; same && i < now.count; ++i) {
            same = now.pass[i] == before.pass[i];
        }

        differ += same ? 0 : 1;
    }

    CHECK(differ == 0);
}

// A material's channels limit each of its draws: a mask layer that writes only alpha writes only
// alpha, passing or failing; NEVER with a depth-only fail writes depth alone, with no test.
void test_a_write_mask_limits_every_pass() {
    const auto geq = static_cast<std::uint8_t>(Compare::GreaterEqual);
    const AlphaTest test{Compare::GreaterEqual, kThreeQuarters, FailWrites::Colour};
    const EffectPasses masked = effect_passes(test, true, kWriteAlpha, true);
    CHECK(masked.count == 2);
    CHECK((masked.pass[0] == EffectPass{geq, false, kWriteAlpha, true}));
    CHECK((masked.pass[1] == EffectPass{geq, true, kWriteAlpha, false}));
    const AlphaTest never{Compare::Never, 0, FailWrites::Depth};
    const EffectPasses depth = effect_passes(never, true, kWriteColour, false);
    CHECK(depth.count == 1 && (depth.pass[0] == EffectPass{0, false, 0, true}));
}

// The image is decoded when the quad is drawn and kept by the tick: game memory changing after the
// call, or the next tick drawing other content there, leaves the capture as it was.
void test_the_image_is_kept_as_it_was_at_the_call() {
    std::vector<std::uint8_t> ram = synthetic_ram();
    UiImageCache cache;
    const openrac::frontend::EffectImageLookup lookup = [&](const openrac_game_ui_image& image) {
        return cache.find(ram, image);
    };
    std::vector<EffectQuad> quads;
    std::vector<UiImage> images;
    const openrac_game_material_quad quad = image_quad(blended({1, 0, -1, 1}, 0));
    CHECK(capture_material_quad(quad, lookup, quads, images));
    CHECK(capture_material_quad(quad, lookup, quads, images));
    // Two quads of one image share it.
    CHECK(quads.size() == 2 && images.size() == 1);

    // The checks below read the captured image.
    if (quads.size() != 2 || images.size() != 1 || !images[0].image) {
        return;
    }

    CHECK(quads[0].native && quads[0].image == 1 && quads[1].image == 1 && quads[0].id == 7);
    CHECK(quads[0].tex0 == 0 && quads[0].uploads < 0 && !quads[0].registers);
    const UiImage first = images[0];
    const std::array<std::uint8_t, 4> one{1, 2, 3, 0x80};  // index 1's colour (synthetic_ram)
    CHECK(texel(*first.image, 0) == one);
    ram[kPixels] = 9;  // the game draws something else there afterwards
    cache.next_tick();
    std::vector<EffectQuad> next_quads;
    std::vector<UiImage> next_images;
    CHECK(capture_material_quad(quad, lookup, next_quads, next_images));
    CHECK(next_images.size() == 1 && next_images[0].hash != first.hash);
    const std::array<std::uint8_t, 4> nine{9, 18, 27, 0x80};
    CHECK(!next_images.empty() && next_images[0].image && texel(*next_images[0].image, 0) == nine);
    CHECK(texel(*images[0].image, 0) == one);
}

// An untextured quad samples no image: none is read, and the quad says so.
void test_an_untextured_quad_reads_no_image() {
    int looked = 0;
    const openrac::frontend::EffectImageLookup lookup = [&](const openrac_game_ui_image&) {
        ++looked;
        return UiImage{};
    };
    openrac_material m = blended({1, 0, 0, 1}, OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA);
    m.flags |= OPENRAC_MATERIAL_UNTEXTURED;
    std::vector<EffectQuad> quads;
    std::vector<UiImage> images;
    CHECK(capture_material_quad(image_quad(m), lookup, quads, images));
    CHECK(looked == 0 && images.empty() && quads.size() == 1);
    CHECK(!quads.empty() && quads[0].native && quads[0].untextured && quads[0].image == 0);
    CHECK(!quads.empty() && quads[0].state.blend == Blend::of(BlendMode::Add));
}

// An image the cache cannot read (past game memory) leaves the quad to the caller's older way.
void test_an_unreadable_image_leaves_the_quad_to_the_caller() {
    const std::vector<std::uint8_t> ram = synthetic_ram();
    UiImageCache cache;
    const openrac::frontend::EffectImageLookup lookup = [&](const openrac_game_ui_image& image) {
        return cache.find(ram, image);
    };
    openrac_game_material_quad quad = image_quad(blended({1, 0, -1, 1}, 0));
    quad.image.pixels = static_cast<std::uint32_t>(kRamBytes) * 2;
    std::vector<EffectQuad> quads;
    std::vector<UiImage> images;
    CHECK(!capture_material_quad(quad, lookup, quads, images));
    CHECK(quads.empty() && images.empty());
}

// A captured quad reaches EffectRenderer's state: the pool's texture holds the capture's pixels,
// and its state, GL values and draws are those of its twin drawn from GS words before.
void test_a_captured_quad_draws_as_its_gs_twin() {
    const std::vector<std::uint8_t> ram = synthetic_ram();
    UiImageCache cache;
    const openrac::frontend::EffectImageLookup lookup = [&](const openrac_game_ui_image& image) {
        return cache.find(ram, image);
    };
    openrac_material m = blended({1, 0, -1, 1}, OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA);
    m.alpha_test = OPENRAC_MATERIAL_TEST_GREATER_EQUAL;
    m.alpha_reference = kThreeQuarters;
    m.alpha_fail = OPENRAC_MATERIAL_FAIL_RGB;
    m.flags = OPENRAC_MATERIAL_BLEND | OPENRAC_MATERIAL_DEPTH_WRITE | OPENRAC_MATERIAL_TEXTURE_ALPHA
              | OPENRAC_MATERIAL_LINEAR_MAG | OPENRAC_MATERIAL_LINEAR_MIN;
    std::vector<EffectQuad> quads;
    std::vector<UiImage> images;
    CHECK(capture_material_quad(image_quad(m), lookup, quads, images));

    // The checks below read the capture.
    if (quads.size() != 1 || images.size() != 1 || !images[0].image) {
        return;
    }

    TexturePool pool;
    const UiImage& image = images[quads[0].image - 1];
    const TextureHandle handle = pool.ui_image(image.hash, *image.image, image.indexed.get());
    CHECK(handle != 0 && handle != pool.placeholder());
    CHECK(pool.image(handle).pixels == image.image->pixels);
    CHECK(pool.ui_image(image.hash, *image.image) == handle);
    // The twin: (Cs - Cd) As + Cd; ATE, GEQUAL, RGB_ONLY, ZTE GEQUAL; ZBUF without ZMSK; TCC and
    // MODULATE; MMAG and MMIN linear; CLAMP clamping both ways.
    const std::uint64_t alpha = alpha_reg(kCs, kCd, kAs, kCd);
    const std::uint64_t test = openrac::test::test_reg(
        true, gs::AlphaTest::Gequal, kThreeQuarters, gs::AlphaFail::RgbOnly, gs::DepthTest::Gequal
    );
    const std::uint64_t tex0 = openrac::test::tex0(0, 1, gs::kPsmt8, 1, 1, true);
    const std::uint64_t linear = std::uint64_t{1} << kTex1Mmag | std::uint64_t{1} << kTex1Mmin;
    const EffectState twin = older_state({tex0, linear, kClampBoth, alpha, test, 0});
    const EffectState& state = quads[0].state;
    CHECK(state == twin);
    CHECK(same_gl(translate_blend(state.blend), translate_blend(gs::Alpha::decode(alpha))));
    const EffectGlState gl = effect_gl_state(state);
    CHECK(gl.blend && gl.inscatter && gl.tcc == 1 && gl.tfx == 0 && !gl.repeat_s && !gl.repeat_t);
    CHECK(gl.depth_function == openrac::gl::GL_GEQUAL);
    const EffectPasses passes =
        effect_passes(state.alpha_test, state.depth_write, state.colour_writes, true);
    const EffectPasses before = older_passes(gs::Test::decode(test), true, true);
    CHECK(passes.count == 2 && before.count == 2);
    CHECK(passes.pass[0] == before.pass[0] && passes.pass[1] == before.pass[1]);
}

}  // namespace

int main() {
    test_the_contract_matches_the_renderer_values();
    test_the_fixed_blends_are_coefficients();
    test_destination_alpha_layers_keep_their_weight();
    test_tests_writes_and_sampling_become_native_state();
    test_out_of_range_materials_are_refused();
    test_passes_draw_every_list_test_as_before();
    test_a_write_mask_limits_every_pass();
    test_the_image_is_kept_as_it_was_at_the_call();
    test_an_untextured_quad_reads_no_image();
    test_an_unreadable_image_leaves_the_quad_to_the_caller();
    test_a_captured_quad_draws_as_its_gs_twin();
    return openrac::test::result();
}
