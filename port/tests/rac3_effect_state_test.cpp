// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Up Your Arsenal's draw state capture (game/rac3-pal/host/effect_state.h) on the production path
// of a world quad: the registers the levels' VU1_addGSregister is given, in the order and at the
// places in the list they land, then a DrawWorldQuad record; the material the capture proves; the
// frontend's capture of that material (capture_material_quad, what openrac_game_material_effect_quad
// runs); and EffectRenderer's plan for it (effect_passes, effect_gl_state). Expected values are the
// renderer's own GS readings (blend_from_gs, alpha_test_from_gs, depth_test_from_gs) and the draws
// EffectRenderer made of the same words before native materials (older_passes, a
// characterisation). A quad whose state the capture does not prove stays on its older path.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "game/common/frontend/effect_material.h"
#include "game/common/frontend/ui_images.h"
#include "game/rac3-pal/host/effect_state.h"
#include "openrac/game_host.h"
#include "renderer/direct/direct.h"
#include "renderer/effects/effect_state.h"
#include "renderer/gl/gl.h"
#include "renderer_gif_builder.h"
#include "tests/check.h"

namespace {

using namespace openrac::renderer;
using openrac::frontend::capture_material_quad;
using openrac::frontend::effect_state;
using openrac::frontend::UiImageCache;
using openrac::test::alpha_reg;
using openrac::test::test_reg;

// The registers' A+D addresses.
constexpr int kTex0 = 0x06, kClamp1 = 0x08, kAlpha1 = 0x42, kTest1 = 0x47, kFba1 = 0x4A;
constexpr int kFrame1 = 0x4C, kZbuf1 = 0x4E;

// VU1_addGSregister's packet: a CNT tag, a DIRECT, a GIF tag and the A+D pair (48 bytes).
constexpr std::uint32_t kPacket = 48;

// The list pointer's word and the list's place when DrawFrame starts, in the test's addresses.
constexpr std::uint32_t kList = 0x001E1068;
constexpr std::uint32_t kListStart = 0x00400000;

// The refraction masks' TEST_1 (ATE, NEVER, FB_ONLY, ZTE ALWAYS), and the masks' record words:
// ALPHA 0x4A ((0 - 0) As + Cd: the target's colour kept), TEX1 0xFF9000000260 (linear), CLAMP 0.
constexpr std::uint64_t kMaskTest = 0x31001;
constexpr std::uint64_t kMaskAlpha = 0x800000004A;
constexpr std::uint64_t kLinear = 0xFF9000000260;

// The ZBUF_1 the levels' DrawFrame sets (PSMZ24, depth written) and that word with ZMSK.
constexpr std::uint64_t kZbufWritten = 0x1000000 | 0x8C;
constexpr std::uint64_t kZbufMasked = kZbufWritten | std::uint64_t{1} << 32;

// The DrawFrame's own TEST_1 for its draw callbacks (alpha GEQUAL 0x60, RGB_ONLY, depth GEQUAL).
constexpr std::uint64_t kFrameTest = 0x5360B;

// Where the synthetic image lies in the test's game memory, and its palette.
constexpr std::uint32_t kPixels = 0x100;
constexpr std::uint32_t kPalette = 0x400;
constexpr std::size_t kRamBytes = 0x1000;

// A frame open at kListStart, the list pointer kept by hand as VU1_addGSregister moves it.
struct Frame {
    rac3_effect_state state{};
    std::uint32_t at = kListStart;

    Frame() { rac3_effect_state_open(&state, kList, at); }

    // VU1_addGSregister(reg, value): its packet appended where the list is.
    void set(int reg, std::uint64_t value) {
        rac3_effect_state_set(&state, kList, at, at + kPacket, reg, value);
        at += kPacket;
    }

    // Something else appended `bytes` (a sprite, a prebuilt packet by reference, a texture block).
    void append(std::uint32_t bytes) { at += bytes; }

    // The material the capture proves for a DrawWorldQuad here with `words`, if it does.
    std::optional<openrac_material> mask(const rac3_quad_words& words) const {
        openrac_material m{};

        // Not proven: the quad keeps its older path.
        if (rac3_effect_mask_material(&state, kList, at, &words, &m) == 0) {
            return std::nullopt;
        }

        return m;
    }
};

// A mask record: an 8-bit image with TCC and MODULATE (EffectTex0's handles), linear, repeated.
rac3_quad_words mask_words() {
    return {0, openrac::test::tex0(0x2000, 1, gs::kPsmt8, 5, 5, true), kLinear, kMaskAlpha};
}

// The draws of a batch under the list's TEST as EffectRenderer's draw_batch_with_alpha_test made
// them before effect_passes (characterisation, as native_material_test's).
EffectPasses older_passes(const gs::Test& t, bool z_write) {
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

    // As the older code branched: NEVER, then no test, then the double draw.
    if (t.ate && t.atst == gs::AlphaTest::Never) {
        // A failing pixel that keeps nothing was not drawn.
        if (t.afail != gs::AlphaFail::Keep) {
            call(0, false, fail_colour, fail_alpha, fail_z);
        }
    } else if (!t.ate || t.atst == gs::AlphaTest::Always) {
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

// Whether two GL blends set the same factors, equation, constant and shader weight.
bool same_gl(const GlBlend& a, const GlBlend& b) {
    return a.src == b.src && a.dst == b.dst && a.equation == b.equation && a.constant == b.constant
           && a.source_scale == b.source_scale && a.fix == b.fix && a.exact == b.exact;
}

// The GL depth comparison EffectRenderer sets for `depth` (GL_ALWAYS when not tested).
unsigned depth_function(const DepthTest& depth) {
    EffectState s;
    s.depth = depth;
    return effect_gl_state(s).depth_function;
}

// A material with the blend that keeps the target's colour and the capture's TEST_1 fields.
openrac_material tested(const rac3_effect_state& s) {
    openrac_material m{};
    m.dest_m = 1;
    m.alpha_test = s.alpha_test;
    m.alpha_reference = s.alpha_reference;
    m.alpha_fail = s.alpha_fail;
    m.depth_test = s.depth_test;
    m.writes = OPENRAC_MATERIAL_WRITE_COLOUR;
    m.flags = OPENRAC_MATERIAL_BLEND;
    return m;
}

// A textured quad over a 2 x 2 8-bit image of the test's memory, with `material`.
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

    quad.id = 3;
    return quad;
}

// The refraction mask as func_L01_00332160 draws it: DrawFrame's ZBUF, other packets, then the
// mask's TEST_1 and its quad with nothing between. The material is the record's and that TEST's.
void test_a_mask_set_right_before_its_quad_has_a_native_material() {
    Frame f;
    f.set(kZbuf1, kZbufWritten);
    f.append(0x40);
    f.set(kTest1, kMaskTest);
    const std::optional<openrac_material> m = f.mask(mask_words());
    CHECK(m.has_value());

    // The checks below read the material.
    if (!m) {
        return;
    }

    // The target's colour kept, the drawn alpha written: 0 Cs + (0 w + 1) Cd.
    CHECK(m->source_k == 0 && m->source_m == 0 && m->dest_k == 0 && m->dest_m == 1);
    CHECK(m->weight == OPENRAC_MATERIAL_WEIGHT_SOURCE_ALPHA && m->constant == 0);
    // Every pixel fails and writes its colour and alpha; depth neither tested nor written.
    CHECK(m->alpha_test == OPENRAC_MATERIAL_TEST_NEVER);
    CHECK(m->alpha_fail == OPENRAC_MATERIAL_FAIL_COLOUR && m->alpha_reference == 0);
    CHECK(m->depth_test == OPENRAC_MATERIAL_DEPTH_ALWAYS);
    CHECK(m->writes == OPENRAC_MATERIAL_WRITE_COLOUR && m->combine == OPENRAC_MATERIAL_MODULATE);
    CHECK(
        m->flags
        == (OPENRAC_MATERIAL_BLEND | OPENRAC_MATERIAL_TEXTURE_ALPHA | OPENRAC_MATERIAL_LINEAR_MAG
            | OPENRAC_MATERIAL_LINEAR_MIN | OPENRAC_MATERIAL_REPEAT_S | OPENRAC_MATERIAL_REPEAT_T)
    );
}

// The mask's material through the frontend's capture to EffectRenderer's plan: the list's blend
// and alpha test as the renderer reads the words, the draws EffectRenderer made of that TEST before,
// whichever ZBUF DrawFrame left (a pixel that fails writes no depth); only the depth comparison is
// the TEST's own ALWAYS where the older draw compared GEQUAL.
void test_the_mask_reaches_the_renderer_as_the_list_drew_it() {
    const std::vector<std::uint8_t> ram(kRamBytes, 0);
    UiImageCache cache;
    const openrac::frontend::EffectImageLookup lookup = [&](const openrac_game_ui_image& image) {
        return cache.find(ram, image);
    };
    const rac3_quad_words words = mask_words();
    const gs::Test t = gs::Test::decode(kMaskTest);

    // DrawFrame's ZBUF with depth written, then masked: the same material and the same draws.
    for (const std::uint64_t zbuf : {kZbufWritten, kZbufMasked}) {
        Frame f;
        f.set(kZbuf1, zbuf);
        f.set(kTest1, kMaskTest);
        const std::optional<openrac_material> m = f.mask(words);
        CHECK(m.has_value());
        std::vector<EffectQuad> quads;
        std::vector<UiImage> images;
        CHECK(m && capture_material_quad(image_quad(*m), lookup, quads, images));

        // The checks below read the capture.
        if (quads.size() != 1 || images.size() != 1) {
            return;
        }

        const EffectState& s = quads[0].state;
        CHECK(quads[0].native && quads[0].image == 1);
        CHECK(s.blend == blend_from_gs(gs::Alpha::decode(words.alpha)));
        CHECK(same_gl(translate_blend(s.blend), translate_blend(gs::Alpha::decode(words.alpha))));
        CHECK(s.alpha_test == alpha_test_from_gs(t));
        CHECK(depth_function(s.depth) == depth_function(depth_test_from_gs(t)));
        const EffectPasses passes =
            effect_passes(s.alpha_test, s.depth_write, s.colour_writes, true);
        const EffectPasses before = older_passes(t, !gs::Zbuf::decode(zbuf).zmsk);
        CHECK(passes.count == 1 && before.count == 1 && passes.pass[0] == before.pass[0]);
        CHECK(!passes.pass[0].depth_write && passes.pass[0].colour_writes == kWriteColour);
        const EffectGlState gl = effect_gl_state(s);
        CHECK(gl.blend && gl.tcc == 1 && gl.tfx == 0 && gl.repeat_s && gl.repeat_t);
        CHECK(gl.mag_filter == openrac::gl::GL_LINEAR && gl.min_filter == openrac::gl::GL_LINEAR);
        CHECK(gl.depth_function == openrac::gl::GL_ALWAYS);
    }
}

// Every TEST_1 the capture is given (ATE, ATST, AREF, AFAIL, ZTST) becomes the alpha test and the
// depth comparison the renderer reads from the same word.
void test_the_capture_reads_every_test_as_the_renderer() {
    // Each combination once: ATE, ATST, AFAIL, ZTST, then AREF 0 or 0x60.
    for (unsigned bits = 0; bits < 2 * 8 * 4 * 4 * 2; ++bits) {
        const bool ate = (bits & 1) != 0;
        const auto atst = static_cast<gs::AlphaTest>((bits >> 1) & 7);
        const auto afail = static_cast<gs::AlphaFail>((bits >> 4) & 3);
        const auto ztst = static_cast<gs::DepthTest>((bits >> 6) & 3);
        const std::uint8_t aref = (bits >> 8) != 0 ? 0x60 : 0;
        const std::uint64_t word = test_reg(ate, atst, aref, afail, ztst);
        Frame f;
        f.set(kTest1, word);
        CHECK((f.state.known & RAC3_EFFECT_TEST) != 0 && f.state.dest_alpha_test == 0);
        const std::optional<EffectState> s = effect_state(tested(f.state));
        CHECK(s.has_value());

        // The checks below read the state.
        if (!s) {
            continue;
        }

        const gs::Test t = gs::Test::decode(word);
        AlphaTest expected = alpha_test_from_gs(t);

        // ALWAYS draws as no test at all, whatever its reference and fail mode said.
        if (expected.compare == Compare::Always) {
            expected = AlphaTest{};
        }

        CHECK(s->alpha_test == expected);
        CHECK(depth_function(s->depth) == depth_function(depth_test_from_gs(t)));
    }
}

// Every ALPHA_1 of a mask record is the renderer's blend of the same word, and GL's; a reserved
// operand (3) has no native form.
void test_the_record_blends_are_the_renderers() {
    // A, B, C and D each 0 to 2 (81 blends), FIX 0x40.
    for (unsigned terms = 0; terms < 81; ++terms) {
        const auto operand = [terms](unsigned place) {
            return static_cast<std::uint8_t>(terms / place % 3);
        };
        rac3_quad_words words = mask_words();
        words.alpha = alpha_reg(operand(1), operand(3), operand(9), operand(27), 0x40);
        Frame f;
        f.set(kTest1, kMaskTest);
        const std::optional<openrac_material> m = f.mask(words);
        const std::optional<EffectState> s = m ? effect_state(*m) : std::optional<EffectState>{};
        CHECK(s.has_value());
        const gs::Alpha gs_alpha = gs::Alpha::decode(words.alpha);
        CHECK(s && s->blend == blend_from_gs(gs_alpha));
        CHECK(s && same_gl(translate_blend(s->blend), translate_blend(gs_alpha)));
    }

    // Each operand reserved (3) in turn.
    for (int operand = 0; operand < 4; ++operand) {
        rac3_quad_words words = mask_words();
        words.alpha = kMaskAlpha | std::uint64_t{3} << (2 * operand);
        Frame f;
        f.set(kTest1, kMaskTest);
        CHECK(!f.mask(words).has_value());
    }
}

// A quad whose state the capture does not prove keeps its older path (openrac_game_effect_quad
// and its marker): an inherited TEST, one with something appended after it (a sprite, a prebuilt
// packet by reference such as the one before each draw callback list, which sets TEST_1 itself),
// a TEST that is not a mask's, and outside DrawFrame or on another list.
void test_unproved_quads_stay_on_the_older_path() {
    // Nothing set yet in this frame: DrawFrame's first packets are data the capture does not read.
    Frame fresh;
    CHECK(!fresh.mask(mask_words()).has_value());

    // The mask's TEST, then a packet the capture did not see before the quad.
    Frame sprite;
    sprite.set(kTest1, kMaskTest);
    sprite.append(0x60);
    CHECK(!sprite.mask(mask_words()).has_value());

    // The mask's TEST, then a gap, then a register the capture did see: the TEST is forgotten.
    Frame gap;
    gap.set(kTest1, kMaskTest);
    gap.append(0x10);
    gap.set(kZbuf1, kZbufWritten);
    CHECK((gap.state.known & RAC3_EFFECT_TEST) == 0 && !gap.mask(mask_words()).has_value());

    // The draw callbacks' TEST as DrawFrame sets it before a list: a glow drawn under it inherits
    // it across the list's prebuilt packet, and even set right before, it is not a mask's.
    Frame glow;
    glow.set(kTest1, kFrameTest);
    glow.append(16);
    CHECK(!glow.mask(mask_words()).has_value());
    Frame own;
    own.set(kTest1, kFrameTest);
    CHECK(!own.mask(mask_words()).has_value());

    // A destination alpha test has no native form.
    Frame date;
    date.set(kTest1, kMaskTest | std::uint64_t{1} << 14);
    CHECK(!date.mask(mask_words()).has_value());

    // DrawFrame has returned: what it set is not the next quad's, nor is a set made outside it.
    Frame closed;
    closed.set(kTest1, kMaskTest);
    rac3_effect_state_close(&closed.state);
    CHECK(!closed.mask(mask_words()).has_value());
    closed.set(kTest1, kMaskTest);
    CHECK(!closed.mask(mask_words()).has_value());

    // Another list pointer (the boot program's, a multiplayer map's): neither read nor kept.
    Frame other;
    other.set(kTest1, kMaskTest);
    openrac_material m{};
    const rac3_quad_words words = mask_words();
    CHECK(rac3_effect_mask_material(&other.state, kList + 4, other.at, &words, &m) == 0);
    rac3_effect_state_set(&other.state, kList + 4, other.at, other.at + kPacket, kTest1, 0);
    CHECK(other.mask(mask_words()).has_value());

    // A record whose CLAMP holds to a region has no native wrap.
    Frame region;
    region.set(kTest1, kMaskTest);
    rac3_quad_words held = mask_words();
    held.clamp = 2;
    CHECK(!region.mask(held).has_value());
}

// What the capture saw keeps the chain: other registers set alone between the TEST and the quad,
// and the quads drawn under one TEST one after another (DrawWorldQuad's own packet sends its
// record's ALPHA_1, so the list's ALPHA_1 is forgotten).
void test_registers_the_capture_saw_keep_the_mask_proven() {
    Frame f;
    f.set(kTest1, kMaskTest);
    f.set(kClamp1, 0);
    f.set(kTex0, mask_words().tex0);
    f.set(kAlpha1, kMaskAlpha);
    CHECK((f.state.known & RAC3_EFFECT_ALPHA) != 0 && f.mask(mask_words()).has_value());
    rac3_effect_state_quad(&f.state);
    CHECK((f.state.known & RAC3_EFFECT_ALPHA) == 0 && (f.state.known & RAC3_EFFECT_TEST) != 0);
    CHECK(f.mask(mask_words()).has_value());

    // A new DrawFrame starts from nothing known.
    rac3_effect_state_open(&f.state, kList, f.at);
    CHECK(f.state.known == 0 && !f.mask(mask_words()).has_value());
}

// FBA_1 and FRAME_1 as their sets give them: a forced alpha bit and a channel masked in part have
// no native form; whole channels masked are the material's writes.
void test_fba_and_frame_sets_shape_or_refuse_the_mask() {
    Frame fba;
    fba.set(kFba1, 1);
    fba.set(kTest1, kMaskTest);
    CHECK(!fba.mask(mask_words()).has_value());
    Frame no_fba;
    no_fba.set(kFba1, 0);
    no_fba.set(kTest1, kMaskTest);
    CHECK(no_fba.mask(mask_words()).has_value());

    // 32-bit colour with the alpha byte masked: red, green and blue written.
    Frame rgb;
    rgb.set(kFrame1, std::uint64_t{0xFF000000} << 32);
    rgb.set(kTest1, kMaskTest);
    const std::optional<openrac_material> m = rgb.mask(mask_words());
    CHECK(
        m
        && m->writes
               == (OPENRAC_MATERIAL_WRITE_RED | OPENRAC_MATERIAL_WRITE_GREEN
                   | OPENRAC_MATERIAL_WRITE_BLUE)
    );

    // Half a byte masked, or 16-bit colour (PSMCT16, 2): refused.
    Frame part;
    part.set(kFrame1, std::uint64_t{0x0F} << 32);
    part.set(kTest1, kMaskTest);
    CHECK(!part.mask(mask_words()).has_value());
    Frame sixteen;
    sixteen.set(kFrame1, std::uint64_t{2} << 24);
    sixteen.set(kTest1, kMaskTest);
    CHECK(!sixteen.mask(mask_words()).has_value());
}

}  // namespace

int main() {
    test_a_mask_set_right_before_its_quad_has_a_native_material();
    test_the_mask_reaches_the_renderer_as_the_list_drew_it();
    test_the_capture_reads_every_test_as_the_renderer();
    test_the_record_blends_are_the_renderers();
    test_unproved_quads_stay_on_the_older_path();
    test_registers_the_capture_saw_keep_the_mask_proven();
    test_fba_and_frame_sets_shape_or_refuse_the_mask();
    return openrac::test::result();
}
