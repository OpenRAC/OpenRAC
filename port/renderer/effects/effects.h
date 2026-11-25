// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The world's effect quads (FrameInput::effects): what the game's draw callbacks send through its
// quad routine (glows, beams, the space scenes' pictures, shadows), drawn with the GS state the
// game gave each one: its texture from TEX0, filtering from TEX1, wrapping from CLAMP, blending
// from ALPHA, the texture function and alpha as the direct renderer does (direct.frag). Depth is
// tested against the world and not written, as the game's effect list draws. The strips the game
// sends to its VU1 strip program (FrameInput::strips) are drawn first, the same way.
//
// The liquids (EffectPart::Liquids) are a pass of their own after the ties: the strips the game
// draws as a solid surface (ALPHA 0x64, (Cs - Cd) FIX + Cd, with FIX above 0x60, which it pairs
// with a Z-writing TEST: the liquid grid's seas and lava), with depth written. Retail draws them
// from the draw callback list it runs after the ties, before the mobys; drawn with the effects,
// after the mobys, a sea covered the mobys' glow faces and blended parts in front of it (Umbris's
// lamps and the bolts' glints as black patches).

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/direct/gs.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/shader.h"
#include "renderer/renderer.h"

namespace openrac::renderer {

// Which of the frame's strips and quads a pass draws.
enum class EffectPart : std::uint8_t {
    All,      // every strip, then every quad
    Opaque,   // depth-writing draws that replace the destination colour
    Liquids,  // only the solid strips (liquid_strip), with depth written
    Rest,     // the strips that are not, then every quad
};

// Whether a strip drawn under TEST writes Z: not when its alpha test never passes and its AFAIL
// keeps only the colour.
bool test_writes_z(std::uint64_t test);

// A strip drawn as a solid surface: ALPHA 0x64 ((Cs - Cd) FIX + Cd) with FIX above 0x60, under a
// TEST that writes Z.
bool liquid_strip(std::uint64_t alpha, std::uint64_t test);

// Whether a strip sent before the world's mobys writes Z, as far as its list shows: a solid one
// (liquid_strip), or one whose list set ZBUF without ZMSK or alpha-tests what it draws (Dobbo's
// water, TEST 0x5310B). Not when ZBUF masks it, its TEST writes none, or the list gave neither
// (Oozla's additive water shine, ALPHA 0x48, TEST 0x513F0: Z there cut the bolts' glows in half).
bool before_mobys_writes_z(const EffectStrip& strip);

// A strip the liquids' pass draws: a solid one (liquid_strip), or one the game sent before the
// world's mobys (EffectStrip::before_mobys), drawn where the game drew it.
bool liquid_pass_strip(const EffectStrip& strip);

// A strip or quad's texture coordinates (s, t over TEX0's size) on the texture of an off-screen
// target the 2D path drew at TEX0's block, which spans kOffscreenPixels chip pixels across and
// down with its first row on top. Up Your Arsenal's frame copy (effect texture -1, 256 x 256) is
// sampled so by the world's quads and by the strips of its strip routine (fx_strip).
std::array<float, 2> offscreen_target_st(std::uint64_t tex0, float s, float t);

// A frame-copy strip's texture coordinates (EffectStrip::frame_copy: s, t over the game's picture,
// t down from its top, as Up Your Arsenal's 0x00403F20 projects a point) on the native snapshot of
// the target, which spans the whole target with its first row at the bottom. On a target wider
// than the picture the game's picture is the target's middle, `picture_scale_x` of its width.
std::array<float, 2> frame_copy_st(float s, float t, float picture_scale_x);

// Whether any strip of `strips` this pass draws samples the frame copy.
bool any_frame_copy(std::span<const EffectStrip> strips);

// Up Your Arsenal's heat haze (the Galactic Rangers' dropship's thrusters, Veldin's 0x003C8400 and
// its copies in the other levels) draws each patch as two square strips at the same place: a mask
// of effect texture 0x3B that keeps the colour and writes only the destination alpha (ALPHA 0x6A,
// (0 - 0) FIX + Cd), then the frame copy blended in by that alpha (ALPHA 0x54, (Cs - Cd) Ad + Cd),
// its ST the corners' places in the picture pushed outwards. The window draws each such pair as a
// round patch of the native heat haze (Haze): its centre, its radius (half the square's side, in
// the game's units) and its strength from the mask's alpha, which fades as the patch drifts away.
// Moves those pairs' patches into `haze` (x, y, z, radius, strength) and leaves the pairs out of
// `strips`; a frame-copy strip without its mask stays.
void frame_copy_haze(
    std::vector<EffectStrip>& strips,
    std::span<const EffectVertex> vertices,
    std::vector<std::array<float, 5>>& haze
);

bool opaque_effect_strip(const EffectStrip& strip, std::span<const EffectVertex> vertices);
bool opaque_effect_quad(const EffectQuad& quad);

// A material-owned mesh for one strip; counts let core keep its existing rendering statistics.
struct EffectMesh {
    std::size_t handle = 0;
    std::uint64_t vertices = 0;
    std::uint32_t draws = 0;  // Zero suppresses the original strip without drawing replacement work.
};

// Optional material extension supplied by enhanced rendering. Core owns no Vulkan dependency.
class EffectMaterial {
public:
    virtual ~EffectMaterial() = default;
    virtual std::string fragment_source() const = 0;

    virtual std::string vertex_source() const { return {}; }

    // Optional exact specialisation for surface-zero quads using the unextended effect vertex.
    // Providers opt in only when that vertex and this fragment preserve their full quad behaviour.
    virtual std::optional<std::string> quad_fragment_source() const { return std::nullopt; }

    // The same specialisation is exact for surface-zero strips with UV animation disabled.
    virtual bool plain_strips_supported() const { return false; }

    virtual void bind_quads(const Shader& shader, const FrameInput& input) {
        bind(shader, 0, input, {});
    }

    // Return a mesh handle when the material replaces this strip's triangle stream.
    virtual std::optional<EffectMesh> mesh(const EffectStrip&, std::span<const EffectVertex>) {
        return std::nullopt;
    }

    // Upload the small per-pass instance stream after collecting strips, before any draws.
    virtual void upload_meshes() {}

    // Snapshot the scene before any effects draw, only when a replacement mesh is visible.
    virtual void prepare_scene(const FrameInput&, const RenderTarget&, unsigned) {}

    // Opts into deferred snapshot/draw recording after material simulation and geometry preparation.
    virtual bool prepares_colour_chain() const { return false; }

    // Returns current colour/depth snapshot textures, or zeros when this pass has no snapshot.
    virtual std::array<unsigned, 2> scene_textures() const { return {}; }

    // Publish per-pass counters after drawing, avoiding work on every material bind.
    virtual void finish_pass() {}

    // Draw an earlier mesh handle using the material's resident geometry.
    virtual void draw_mesh(const Shader&, const EffectMesh&) {}

    virtual bool init(std::string& error) = 0;
    virtual void prepare(const FrameInput& input) = 0;
    virtual void bind(
        const Shader& shader,
        std::uint32_t surface,
        const FrameInput& input,
        const EffectUvMotion& motion
    ) = 0;
    virtual void release() = 0;
};

class EffectRenderer : public BucketRenderer {
public:
    explicit EffectRenderer(
        std::string name = "effects",
        EffectPart part = EffectPart::All,
        Bucket bucket = Bucket::Particles,
        std::unique_ptr<EffectMaterial> material = {},
        bool split_opaque = false
    )
        : BucketRenderer(std::move(name), bucket),
          m_part(part),
          m_material(std::move(material)),
          m_split_opaque(split_opaque) {}

    bool init(RenderState& state, std::string& error) override;
    void render(const FrameInput& input, RenderState& state) override;
    void render_depth(const FrameInput& input, RenderState& state) override;

    bool prepares_colour_chain() const override {
        return !m_material || m_material->prepares_colour_chain();
    }
    // A bucket-level GPU query would hide the snapshot and draw queries inside the bucket.
    bool owns_gpu_timing() const override {
        return m_material && m_material->prepares_colour_chain();
    }

    // Prepares geometry before declaring snapshots and retaining owned draw metadata.
    [[nodiscard]] Result<void> record_colour(const FrameInput& input, RenderState& state) override;
    void release() override;

private:
    // Draw metadata is shared by preparation and deferred recording within the enclosing chain.
    struct PreparedEffects {
        std::vector<int> strip_counts;  // Expanded stream counts, in accepted strip order.
        std::vector<std::optional<EffectMesh>> meshes;  // Matching resident-material mesh handles.
        std::size_t quads_from = 0;                     // First quad vertex after expanded strips.
        bool quads = false;
    };

    // Queues scene snapshots and their colour consumers, or retains immediate unknown materials.
    void record_colour(
        const FrameInput& input,
        RenderState& state,
        const std::shared_ptr<const PreparedEffects>& prepared
    );
    // Records the existing fused GS batching loop with prepared, frame-owned geometry.
    // Hot path: retains the existing size/nesting of effect batching to preserve draw and upload order.
    void draw_prepared(
        const FrameInput& input,
        RenderState& state,
        const PreparedEffects& prepared,
        bool has_colour_pass
    );
    // Orders shared material inputs once per selected effect shader, after provider bindings are
    // installed.
    void prepare_colour_reads();
    void use_shader(Shader& shader, const Mat4& view_projection, const Mat4& view);
    void bind_vertex_stream(unsigned vao, std::size_t first);
    void reserve_quad_indices(std::size_t quads);
    // Resolves a panel drawn by the 2D renderer before this effect pass.
    static unsigned offscreen_texture(std::uint64_t tex0_bits, std::uint32_t tbp);
    // Whether this pass draws a strip that samples the frame copy (EffectStrip::frame_copy).
    bool draws_frame_copy(const FrameInput& input) const;
    // Copies the target's colour as drawn so far into m_frame_copy (the game's frame copy, made
    // natively), for the frame-copy strips that follow; m_frame_copy_texture is then its texture,
    // else 0. Records into the colour graph where it is on.
    void snapshot_frame_copy(const RenderTarget& target);
    // Binds TEX0's texture with TEX1's filtering and CLAMP's wrapping, and sets ALPHA's blend.
    // False when the pool has nothing at TEX0's block (the draw is left out, not the placeholder).
    bool apply(
        std::uint64_t tex0,
        std::uint64_t tex1,
        std::uint64_t clamp,
        std::uint64_t alpha,
        RenderState& state,
        std::uint32_t tbp = 0,
        std::uint32_t cbp = 0,
        bool frame_copy = false
    );

    // The rest of apply, once the texture is bound: TEX0's TCC and TFX, the texture's alpha scale
    // (0.5 for a texture whose alpha is 0..255 with 0x80 as 1), ALPHA's blend.
    void apply_colour_state(const gs::Tex0& tex0, float scale, std::uint64_t alpha_bits);

    // apply for a native quad (EffectQuad::native): binds `image` (a pool texture; none when
    // `untextured`) with the state's filter and wrap, and sets its texture function, blend and
    // depth comparison (effect_gl_state). False when a textured quad's image has no texture.
    bool apply_native(
        const EffectState& material, TextureHandle image, bool untextured, RenderState& state
    );

    // Sets the depth comparison of the next draws, when it changed.
    void set_depth_function(unsigned function);

    struct Vertex {
        float x, y, z;
        float s, t;
        std::uint8_t r, g, b, a;
    };

    // Whether this pass draws `strip` (m_part).
    bool draws(const EffectStrip& strip, const FrameInput& input) const;
    bool draws(const EffectQuad& quad, const FrameInput& input) const;

    struct AppliedState {
        TextureHandle texture{};
        std::uint64_t tex1_bits = ~0ull;
        std::uint64_t clamp_bits = ~0ull;
        std::uint64_t alpha_bits = ~0ull;
        int tcc = -1;
        int tfx = -1;
        float alpha_scale = -1.0f;
        unsigned offscreen = 0;  // the off-screen target's GL texture bound (kOffscreenHandle)
    } m_applied;

    // The depth comparison in GL now (set_depth_function), across shader changes: GL_GEQUAL from
    // the start of each pass's draws.
    unsigned m_depth_function = 0;

    // m_applied.texture while an off-screen target's texture is bound (no pool handle is ~0).
    static constexpr TextureHandle kOffscreenHandle = ~TextureHandle{0};

    EffectPart m_part = EffectPart::All;
    // The pass's snapshot of the target for the frame-copy strips (snapshot_frame_copy).
    FrameBuffer m_frame_copy;
    unsigned m_frame_copy_texture = 0;
    Shader m_shader;
    Shader m_depth_shader;
    Shader m_quad_shader;
    // The quads drawn with the list's TEST (EffectQuad::registers) that test alpha: the quads'
    // shader with the alpha test kept (the others compile it out, EffectStaticControls).
    // m_test_plain: built from the material's plain quad source, bound as m_quad_shader is.
    Shader m_test_shader;
    bool m_test_plain = false;
    // The untextured quads (EffectQuad::untextured): the same fragment with `textured` a uniform
    // (0 here), not compiled to 1 as EffectStaticControls does for the textured shaders.
    Shader m_untextured_shader;
    Shader* m_active_shader = nullptr;
    bool m_colour_reads_ready = false;  // Reset whenever the effect pass changes its shader.
    std::unique_ptr<EffectMaterial> m_material;
    bool m_split_opaque = false;
    TextureFilter m_texture_filter = TextureFilter::Normal;
    int m_view_projection = -1;
    int m_textured = -1, m_tcc = -1, m_tfx = -1, m_tex_alpha_scale = -1;
    int m_fog_enable = -1, m_alpha_test = -1, m_alpha_ref = -1, m_alpha_keep_failing = -1;
    int m_view = -1, m_fog_colour = -1, m_fog_line = -1;
    int m_source_scale = -1, m_source_fix = -1;
    unsigned m_vao = 0, m_vbo = 0;
    unsigned m_quad_vao = 0, m_quad_indices = 0;
    std::size_t m_quad_capacity = 0;
    std::vector<Vertex> m_vertices;  // the frame's triangles, kept for their storage
};

}  // namespace openrac::renderer
