// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Every texture the renderer draws with, converted to RGBA8 and uploaded to
// the GPU once. After OpenGOAL's TexturePool (OPENGOAL_NOTES.md, section 5.2),
// with its central rule: a texture's base pointer in the chip's memory (TBP)
// is an identifier, not an address. Ratchet & Clank streams each pass's
// textures into the same area of that memory (RENDERER.md, section 5), so
// the pool never keeps an image of the memory; it keeps, per base pointer, what
// the game last put there:
//
//   - a texture converted ahead of time, from the assets extracted from the
//     player's disc (add() then place(): the way the game's texture paging
//     will hand the renderer its textures), or
//   - the pixels of an image transfer the game sent itself (upload(): the 2D
//     path's fonts and HUD images, sent in GIF IMAGE packets).
//
// A draw asks for a texture by its TEX0 register (resolve()); the pool finds
// what is at that base pointer, converts it with the CLUT at the CLUT pointer,
// and caches the result by content, so a texture the game sends every frame
// is converted once.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "renderer/direct/gs.h"
#include "renderer/textures/texture.h"
#include "renderer/textures/texture_filter.h"
#include "renderer/textures/texture_pack.h"

namespace openrac::renderer {

class Shader;

// A rectangle the game sent to the chip with an image transfer.
struct ImageUpload {
    std::uint32_t dbp = 0;  // destination base, in 256-byte blocks
    std::uint32_t dbw = 1;  // destination buffer width, in 64-pixel units
    std::uint8_t dpsm = gs::kPsmct32;
    std::uint32_t x = 0;  // destination rectangle
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> data;  // raster order, in dpsm

    // Past the pages this transfer can replace, as TexturePool::upload_at resolves them.
    std::uint32_t end_block() const;
};

// A converted texture, valid for the life of the pool (or until end_frame()
// drops it after a long time unused). 0 is never a texture.
using TextureHandle = std::uint32_t;

// What TexturePool::prepare made, for the log line of a level's load.
struct PreparedTextures {
    std::size_t textures = 0;        // GL textures created (those that had one are not counted).
    std::size_t staged_bytes = 0;    // Their texels as uploaded: the first level, RGBA8.
    std::size_t resident_bytes = 0;  // What they hold on the GPU, mip chains included.
    std::size_t batches = 0;         // Runs of uploads with a glFinish between two of them.
};

class TexturePool {
public:
    TexturePool();
    ~TexturePool();
    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    // ---- What the game puts in the chip's memory ----

    // An image transfer. It replaces whatever was last uploaded at the same
    // base pointer.
    void upload(ImageUpload upload);

    // Same ordering/replacement semantics as upload(). For repeated immutable
    // frame inputs, reuse identical stored bytes without copying or rehashing.
    // Returns true when the existing storage was reused.
    bool upload_cached(const ImageUpload& upload);

    // A texture converted ahead of time. `scale` says how its alpha runs;
    // `mipmaps` gives its GL texture a mipmap chain and trilinear filtering
    // (3D geometry), otherwise it has one level, filtered as each draw says (nearest until then;
    // `linear`: bilinear, as the GS samples a texture whose TEX1 has MXL 0 and linear filters).
    TextureHandle add(Rgba8Image image, AlphaScale scale, std::string name, bool mipmaps = false,
                      bool linear = false);

    // Gives `texture` the frames of a texture pack's animated replacement (texture_pack.h): its
    // image stays the first frame, and gl_texture() gives the frame of the game's tick. Null:
    // none. Each frame's GL texture is made at its first use and freed with the texture.
    void animate(TextureHandle texture, std::shared_ptr<const texture_pack::Animation> animation);

    // Gives `texture` a texture pack KTX2's picture in GPU blocks (texture_pack.h): its GL texture
    // holds those, and its mip levels, instead of RGBA8 (a quarter of the memory); its image stays
    // for what the CPU reads. Null, blocks of another size than the image, or a texture with a
    // mip chain whose blocks have one level only (the GPU cannot make the chain from blocks):
    // RGBA8 as before.
    void set_blocks(TextureHandle texture, std::shared_ptr<const CompressedImage> blocks);

    // Whether `texture`'s GL texture holds blocks (set_blocks): its levels are its own, so the
    // game's mip levels are not put over them.
    bool has_blocks(TextureHandle texture) const { return m_entries[texture - 1].gl_blocks; }

    // The block format a pack's KTX2 is transcoded to on this GPU: BC7, else ASTC 4x4, else None
    // (RGBA8). OPENRAC_GL_HIDE_EXTENSIONS hides one to try the next. The context must be current.
    static BlockFormat gpu_block_format();

    // What texture_pack::replace() found for `texture`, a texture added by its owner (an effect's,
    // a particle's): its pack hash, for watch(), and its frames when animated (animate()); with
    // hashes of the texels and the palette it was made from, so that the dump can tell a palette
    // cycle (0: not known).
    void set_replacement(
        TextureHandle texture,
        texture_pack::Replacement replacement,
        std::uint64_t texels_hash = 0,
        std::uint64_t palette_hash = 0
    );

    // For a pack maker's dump (texture_pack::CycleWatch): `texture` drawn this tick in `slot`, a
    // place its owner names in at most 33 bits (an effect's quad). Nothing unless textures are
    // dumped.
    void watch(std::uint64_t slot, TextureHandle texture);

    // The game's tick the frame shows (FrameInput::frame): the clock of animated replacements, so
    // that they pause with the game and run alike at any frame rate.
    void set_tick(std::uint64_t tick) { m_tick = tick; }

    // The anisotropic filter of every texture with a mipmap chain (the world's): 1 is none, up to
    // what the driver offers (max_anisotropy(); 1 when it has no such filter). The textures on the
    // GPU now are set again; later ones get it as they are uploaded. The context must be current.
    void set_anisotropy(float samples);
    float anisotropy() const { return m_anisotropy; }
    static float max_anisotropy();

    // The game has put `texture` at base pointer `tbp`; draws that sample
    // there get it, whatever their TEX0 says about format and CLUT.
    void place(std::uint32_t tbp, TextureHandle texture);

    // The texture placed at `tbp`, if any.
    std::optional<TextureHandle> placed(std::uint32_t tbp) const {
        const auto found = m_placed.find(tbp);
        return found != m_placed.end() ? std::optional<TextureHandle>(found->second) : std::nullopt;
    }
    void clear_placements();

    // A TEX0 with CLD set has loaded the CLUT at its CBP into the chip's CLUT buffer: the draws
    // after it read their palette from what was there then, even after the game puts something
    // else at that block (cld: 1 to 3 always load, 4 and 5 when CBP differs from CBP0 or CBP1,
    // which the caller decides).
    void load_clut(const gs::Tex0& tex0);

    // ---- What a draw samples ----

    // The texture TEX0 describes, converted with TEXA. Never 0: when nothing
    // is known at that base pointer, the placeholder (a checkerboard).
    TextureHandle resolve(const gs::Tex0& tex0, const gs::Texa& texa);

    TextureHandle placeholder() const { return m_placeholder; }

    // A distance field of `source` for drawing text sharp at any size, four times its size: in
    // alpha the signed distance to the glyphs' edge (0.5 on the edge, 1/8 per source texel), in red
    // the glyphs' own opacity (0..1) held up to that edge; made once per content of the source.
    TextureHandle distance_field(TextureHandle source);

    // The distance fields made from scratch so far: a field asked for again soon after it was
    // dropped is taken from those resting (kRestingFields), not made again.
    std::size_t fields_made() const { return m_fields_made; }

    // The dropped fields whose texels are kept, the latest: text that comes back after a pause
    // (a counter shown again) gets its field without the frame's hitch of making it, about 9 ms on
    // a fast phone and four times that on a slow one. A 256 x 128 page's field is 2 MB.
    static constexpr std::size_t kRestingFields = 4;

    const Rgba8Image& image(TextureHandle texture) const;
    bool has_cropped_replacement(TextureHandle texture) const;
    std::array<float, 2> uv_scale(TextureHandle texture) const;

    // Whether `texture` has an animated replacement's frames (animate()).
    bool animated(TextureHandle texture) const {
        return m_entries[texture - 1].animation != nullptr;
    }
    AlphaScale alpha_scale(TextureHandle texture) const;
    const std::string& name(TextureHandle texture) const;

    // ---- The GPU side; the GL context must be current ----

    // The GL texture, created on first use. Filtering and wrapping are set
    // by whoever binds it. An animated texture's is its frame at the tick (set_tick).
    unsigned gl_texture(TextureHandle texture);

    // Creates the GL textures of `textures` that have none yet, as gl_texture() would at their
    // first use (the same texels, mip chain and sampler parameters, by the same code), so that
    // the frames after a level's load make none (docs/port/RENDER_PERF.md, R-13). The uploads go
    // in batches of at most kPrepareBatchBytes texels, with a glFinish between two batches. A
    // handle given twice is made once. Leaves no texture bound to unit 0's 2D target.
    PreparedTextures prepare(std::span<const TextureHandle> textures);

    // The texel bytes prepare() uploads before it waits for the GPU, so that the staging a
    // driver holds stays bounded: level_01's 607 textures (26 MB as uploaded) are one run.
    static constexpr std::size_t kPrepareBatchBytes = std::size_t{32} << 20;

    // Bind cached RGB on unit 13, keeping the original on unit 0 for alpha.
    // Call after binding a material; restores the material program after a cache miss.
    void bind_upscaled_texture_colours(
        TextureHandle texture,
        const Shader& shader,
        TextureFilter filter,
        bool repeat_s = true,
        bool repeat_t = true
    );
    void set_upscaled_texture_colours(std::unique_ptr<UpscaledTextureColours> source);
    // Drop cached world colours while keeping the game's source textures.
    void clear_upscaled_texture_colours();

    bool has_upscaled_texture_colours() const { return m_upscaled_texture_colours != nullptr; }

    // Delete every GL texture (before the context goes away).
    void release_gl();

    // Once per frame: forget conversions of game uploads, and distance fields, that no draw has
    // used for kKeepFrames frames.
    void end_frame();

    static constexpr std::uint32_t kKeepFrames = 120;

    // Live textures, the placeholder included.
    std::size_t size() const;

private:
    struct Key {
        std::uint32_t tbp, tbw, tw, th, cbp;
        std::uint8_t psm, cpsm;
        gs::Texa texa;
        std::uint64_t pixels_hash, clut_hash, crop_border_hash;

        bool operator==(const Key&) const = default;
    };

    struct KeyHash {
        std::size_t operator()(const Key& key) const;
    };

    struct Stored {
        ImageUpload upload;
        std::uint64_t hash = 0;
        std::uint64_t sequence = 0;  // uploads in order: the later of two over one block is there
        std::uint32_t end = 0;       // past the last block its pages reach
    };

    struct CropBlock {
        int x, y;
        std::uint32_t offset;
        const Stored* upload;
    };

    std::vector<CropBlock> crop_border(const gs::Tex0& tex0, std::array<int, 2> extent) const;

    // A CLUT as loaded into the chip's buffer: its rectangle in its format.
    struct Clut {
        std::vector<std::uint8_t> data;
        std::uint8_t cpsm = 0;
        int width = 0;
        int entries = 0;
        std::uint64_t hash = 0;
    };

    // The upload a texture or CLUT at `block` is in: the one there, else the latest whose pages
    // reach over it (`offset` says how far into it, in blocks). Null when there is none.
    const Stored* upload_at(std::uint32_t block, std::uint32_t& offset) const;
    // Whether `upload` is the transfer already stored at its base pointer, byte for byte: then
    // only its order is renewed (and a texture placed there forgotten), as a new upload's would be.
    bool repeat_upload(const ImageUpload& upload);
    // The CLUT at tex0.cbp as it is in memory now; false when nothing was put there.
    bool clut_in_memory(const gs::Tex0& tex0, Clut& out) const;

    struct Entry {
        bool live = false;
        bool from_upload = false;  // converted from game uploads: may be dropped
        bool field = false;        // a distance field (distance_field): may be dropped as well
        std::uint64_t field_hash = 0;  // its key in m_distance_fields
        bool mipmaps = false;
        bool linear = false;
        std::uint8_t wrap = 0;  // the GL texture's clamp: bit 0 across, bit 1 down (0: repeats)
        Rgba8Image image;
        bool cropped_replacement = false;
        std::array<float, 2> uv_scale{1.0f, 1.0f};
        AlphaScale scale = AlphaScale::Gs;
        std::string name;
        unsigned gl = 0;
        std::uint64_t last_used = 0;
        Key key{};
        std::uint64_t pack_hash = 0;  // its texture pack hash (texture_pack::Replacement), or 0
        // An animated replacement's frames (animate()): GL textures of frames past the first
        // (frame_gl[i - 1] is frame i; 0 until drawn) and the tick its clock started at.
        std::shared_ptr<const texture_pack::Animation> animation;
        // A pack KTX2's blocks (set_blocks), and whether the GL texture was made from them.
        std::shared_ptr<const CompressedImage> blocks;
        bool gl_blocks = false;
        std::vector<unsigned> frame_gl;
        std::uint64_t animation_start = 0;
        bool animation_started = false;
    };

    TextureHandle new_entry();
    // Creates entry `e`'s GL texture and uploads its texels: the one place a texture is made,
    // for gl_texture() at a first use and for prepare() at a level's load.
    void create_gl(Entry& e);
    // A GL texture of `image` as entry `e`'s would be made (its mip chain and filters).
    unsigned upload_gl(const Entry& e, const Rgba8Image& image);
    // Entry `e`'s GL texture made from its blocks, or 0 when the GPU cannot take them (then it is
    // made of RGBA8). Sets the filters upload_gl would.
    unsigned upload_blocks(const Entry& e);
    // The GL texture's sampler parameters, as upload_gl sets them, for a texture of `levels`.
    void set_filters(const Entry& e, int levels);
    // The GL textures entry `e` holds (its own and its frames'), deleted.
    void delete_gl(Entry& e);
    // For the dump (texture_pack::CycleWatch): `texture` drawn from the upload at `tex0`.
    void watch_cycle(const gs::Tex0& tex0, TextureHandle texture);
    // The draw's turn at `place` this frame: how many other textures were drawn there before it.
    unsigned turn_at(std::uint64_t place, std::uint64_t hash);
    // `hash` seen in `slot`: a cycle found there is logged (`where` says where) and dumped.
    void see_cycle(std::uint64_t slot, std::uint64_t hash, const std::string& where);
    void set_wrap(Entry& e, bool repeat_s, bool repeat_t);
    TextureHandle convert_upload(const gs::Tex0& tex0, const gs::Texa& texa);
    std::vector<std::uint8_t> texture_raster(
        const Stored& stored, const gs::Tex0& tex0, int width, int height
    ) const;
    // The CLUT an indexed texture reads: the one loaded from its block, else memory's (into `now`).
    // Null when there is none (drawn transparent).
    const Clut* palette_clut(const gs::Tex0& tex0, Clut& now);
    // Adds a distance field of `source` (whose content hashes to `hash`, sampled as `sample`) and
    // remembers it by both.
    TextureHandle add_field(
        Rgba8Image image,
        std::string name,
        std::uint64_t hash,
        std::uint64_t sample,
        TextureHandle source
    );

    std::unique_ptr<UpscaledTextureColours> m_upscaled_texture_colours;

    std::vector<Entry> m_entries;  // handle - 1
    float m_anisotropy = 1.0f;
    std::vector<TextureHandle> m_free;
    std::unordered_map<std::uint32_t, Stored> m_uploads;        // by base pointer
    std::uint64_t m_sequence = 0;
    std::unordered_map<std::uint32_t, Clut> m_cluts;            // loaded, by CBP (load_clut)
    std::unordered_map<std::uint32_t, TextureHandle> m_placed;  // by base pointer
    std::unordered_map<Key, TextureHandle, KeyHash> m_converted;
    TextureHandle m_placeholder = 0;
    std::uint64_t m_frame = 0;
    std::uint64_t m_tick = 0;  // set_tick
    // Textures drawn at each base pointer this frame, in order, and the cycles found among them:
    // only while textures are dumped (texture_pack::dumping).
    std::unordered_map<std::uint64_t, std::pair<std::uint64_t, unsigned>> m_slot_turns;
    texture_pack::CycleWatch m_cycles;
    std::unordered_map<std::uint64_t, TextureHandle> m_distance_fields;  // by the source's content
    // The field last given for a handle, with a sample of its texels (distance_field).
    std::unordered_map<TextureHandle, std::pair<std::uint64_t, TextureHandle>> m_field_of_handle;

    // Dropped fields, the oldest first (kRestingFields).
    struct RestingField {
        std::uint64_t hash = 0;  // the source's content, as m_distance_fields keys it
        Rgba8Image image;
        std::string name;
    };

    std::vector<RestingField> m_resting_fields;
    std::size_t m_fields_made = 0;
};

// A 64-bit hash of the bytes, for the cache keys.
std::uint64_t hash_bytes(const std::uint8_t* data, std::size_t size);

}  // namespace openrac::renderer
