// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/texture_pool.h"

#include <algorithm>
#include <utility>

#include "common/log.h"
#include "renderer/gl.h"

namespace openrac::renderer {

using namespace openrac::gl;

std::uint64_t hash_bytes(const std::uint8_t* data, std::size_t size) {
    std::uint64_t h = 0xcbf29ce484222325ull;
    for (std::size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

std::size_t TexturePool::KeyHash::operator()(const Key& key) const {
    const std::uint64_t words[] = {
        key.tbp | (std::uint64_t{key.tbw} << 16) | (std::uint64_t{key.tw} << 32)
            | (std::uint64_t{key.th} << 40) | (std::uint64_t{key.psm} << 48)
            | (std::uint64_t{key.cpsm} << 56),
        key.cbp | (std::uint64_t{key.texa.ta0} << 16) | (std::uint64_t{key.texa.ta1} << 24)
            | (std::uint64_t{key.texa.aem} << 32),
        key.pixels_hash,
        key.clut_hash,
    };
    return static_cast<std::size_t>(
        hash_bytes(reinterpret_cast<const std::uint8_t*>(words), sizeof(words))
    );
}

TexturePool::TexturePool() {
    // Magenta and black, 8 x 8 texels of 4 x 4: obviously wrong on screen.
    Rgba8Image checker(8, 8);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const bool on = ((x / 4) + (y / 4)) % 2 == 0;
            checker.set_texel(x, y, on ? rgba(0xFF, 0x00, 0xFF, 0x80) : rgba(0, 0, 0, 0x80));
        }
    }
    m_placeholder = add(std::move(checker), AlphaScale::Gs, "placeholder");
}

TexturePool::~TexturePool() {
    // GL objects are released by release_gl() while the context exists; by
    // now it may not.
}

TextureHandle TexturePool::new_entry() {
    if (!m_free.empty()) {
        const TextureHandle h = m_free.back();
        m_free.pop_back();
        return h;
    }
    m_entries.emplace_back();
    return static_cast<TextureHandle>(m_entries.size());
}

TextureHandle TexturePool::add(Rgba8Image image, AlphaScale scale, std::string name, bool mipmaps) {
    const TextureHandle h = new_entry();
    Entry& e = m_entries[h - 1];
    e = Entry{};
    e.live = true;
    e.image = std::move(image);
    e.scale = scale;
    e.name = std::move(name);
    e.mipmaps = mipmaps;
    e.last_used = m_frame;
    return h;
}

void TexturePool::place(std::uint32_t tbp, TextureHandle texture) {
    m_placed[tbp] = texture;
}

void TexturePool::clear_placements() {
    m_placed.clear();
}

void TexturePool::upload(ImageUpload upload) {
    if (upload.x != 0 || upload.y != 0) {
        // Uploads into the middle of a buffer (texture atlases) would need the
        // rectangle placed by its offset; the 2D path seen so far sends whole
        // textures. Kept, and read as if at the origin.
        log::debug("image transfer at offset {},{} in block {:#x}", upload.x, upload.y, upload.dbp);
    }
    Stored stored;
    const std::uint64_t shape[] = {upload.width, upload.height, upload.dpsm, upload.dbw};
    stored.hash = hash_bytes(upload.data.data(), upload.data.size())
                  ^ (hash_bytes(reinterpret_cast<const std::uint8_t*>(shape), sizeof(shape)) << 1);
    stored.upload = std::move(upload);
    const std::uint32_t dbp = stored.upload.dbp;
    m_uploads[dbp] = std::move(stored);
    // An upload replaces a texture placed at the same base pointer.
    m_placed.erase(dbp);
}

std::vector<std::uint8_t> TexturePool::texture_raster(const Stored& stored, const gs::Tex0& tex0)
    const {
    const ImageUpload& up = stored.upload;
    const int tw = tex0.width();
    const int th = tex0.height();
    const int up_w = static_cast<int>(up.width);
    const int up_h = static_cast<int>(up.height);
    const bool both_16 = gs::bits_per_pixel(up.dpsm) == 16 && gs::bits_per_pixel(tex0.psm) == 16;
    if (up.dpsm != tex0.psm && !both_16) {
        // Sent in one format and read in another (an 8-bit texture sent as
        // 32-bit pixels, say): the only case that needs the chip's layout.
        return reinterpret(
            up.data,
            up.dpsm,
            up_w,
            up_h,
            up.dbw,
            tex0.psm,
            tw,
            th,
            std::max<std::uint32_t>(tex0.tbw, 1)
        );
    }
    // Same format: the rectangle, cropped or padded to the texture's size.
    std::vector<std::uint8_t> out(image_bytes(tex0.psm, tw, th));
    const int raster = raster_bits(tex0.psm);
    if (raster % 8 == 0) {
        const auto bytes = static_cast<std::size_t>(raster / 8);
        for (int y = 0; y < std::min(th, up_h); ++y) {
            const std::size_t src = static_cast<std::size_t>(y * up_w) * bytes;
            const std::size_t dst = static_cast<std::size_t>(y * tw) * bytes;
            const std::size_t count = static_cast<std::size_t>(std::min(tw, up_w)) * bytes;
            if (src + count <= up.data.size()) {
                std::copy_n(
                    up.data.begin() + static_cast<std::ptrdiff_t>(src),
                    count,
                    out.begin() + static_cast<std::ptrdiff_t>(dst)
                );
            }
        }
    } else {
        // 4-bit: nibble by nibble.
        for (int y = 0; y < std::min(th, up_h); ++y) {
            for (int x = 0; x < std::min(tw, up_w); ++x) {
                const auto s = static_cast<std::size_t>(y * up_w + x);
                const auto d = static_cast<std::size_t>(y * tw + x);
                if (s / 2 >= up.data.size()) {
                    continue;
                }
                const std::uint8_t v = (s & 1) != 0 ? up.data[s / 2] >> 4 : up.data[s / 2] & 0x0F;
                out[d / 2] = static_cast<std::uint8_t>(
                    (d & 1) != 0 ? (out[d / 2] & 0x0F) | (v << 4) : (out[d / 2] & 0xF0) | v
                );
            }
        }
    }
    return out;
}

std::vector<std::uint32_t> TexturePool::palette(
    const gs::Tex0& tex0, const gs::Texa& texa, std::uint64_t& hash
) {
    const int entries = palette_entries(tex0.psm);
    hash = 0;
    if (entries == 0) {
        return {};
    }
    auto it = m_uploads.find(tex0.cbp);
    if (it == m_uploads.end()) {
        log::debug("no CLUT at block {:#x}", tex0.cbp);
        return std::vector<std::uint32_t>(static_cast<std::size_t>(entries), 0);
    }
    if (tex0.csm) {
        // CSM2 (a CLUT as one row of a larger image, placed by TEXCLUT) is
        // not used by the games read so far.
        log::debug("CSM2 CLUT at block {:#x} read as CSM1", tex0.cbp);
    }
    hash = it->second.hash;
    const ImageUpload& up = it->second.upload;
    return clut_csm1(up.data, up.dpsm, std::max<int>(static_cast<int>(up.width), 1), entries, texa);
}

TextureHandle TexturePool::convert_upload(const gs::Tex0& tex0, const gs::Texa& texa) {
    auto it = m_uploads.find(tex0.tbp0);
    if (it == m_uploads.end()) {
        return 0;
    }
    std::uint64_t clut_hash = 0;
    std::vector<std::uint32_t> colours;
    const bool indexed = palette_entries(tex0.psm) != 0;
    if (indexed) {
        colours = palette(tex0, texa, clut_hash);
    }
    const Key key{
        tex0.tbp0,
        tex0.tbw,
        tex0.tw,
        tex0.th,
        indexed ? tex0.cbp : 0,
        tex0.psm,
        indexed ? tex0.cpsm : std::uint8_t{0},
        texa,
        it->second.hash,
        clut_hash
    };
    if (auto found = m_converted.find(key); found != m_converted.end()) {
        return found->second;
    }
    const std::vector<std::uint8_t> raster = texture_raster(it->second, tex0);
    Rgba8Image image =
        convert(raster, tex0.psm, tex0.width(), tex0.height(), colours, texa, AlphaScale::Gs);
    const TextureHandle h = add(std::move(image), AlphaScale::Gs, "upload");
    m_entries[h - 1].from_upload = true;
    m_entries[h - 1].key = key;
    m_converted.emplace(key, h);
    return h;
}

TextureHandle TexturePool::resolve(const gs::Tex0& tex0, const gs::Texa& texa) {
    TextureHandle h = 0;
    if (auto placed = m_placed.find(tex0.tbp0); placed != m_placed.end()) {
        h = placed->second;
    } else {
        h = convert_upload(tex0, texa);
    }
    if (h == 0) {
        h = m_placeholder;
    }
    m_entries[h - 1].last_used = m_frame;
    return h;
}

const Rgba8Image& TexturePool::image(TextureHandle texture) const {
    return m_entries[texture - 1].image;
}

AlphaScale TexturePool::alpha_scale(TextureHandle texture) const {
    return m_entries[texture - 1].scale;
}

const std::string& TexturePool::name(TextureHandle texture) const {
    return m_entries[texture - 1].name;
}

unsigned TexturePool::gl_texture(TextureHandle texture) {
    Entry& e = m_entries[texture - 1];
    e.last_used = m_frame;
    if (e.gl == 0) {
        glGenTextures(1, &e.gl);
        glBindTexture(GL_TEXTURE_2D, e.gl);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            static_cast<GLint>(GL_RGBA8),
            e.image.width,
            e.image.height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            e.image.pixels.data()
        );
        if (e.mipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR)
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_REPEAT));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_REPEAT));
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
        }
    }
    return e.gl;
}

void TexturePool::release_gl() {
    for (Entry& e : m_entries) {
        if (e.gl != 0) {
            glDeleteTextures(1, &e.gl);
            e.gl = 0;
        }
    }
}

void TexturePool::end_frame() {
    ++m_frame;
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        Entry& e = m_entries[i];
        if (!e.live || !e.from_upload || m_frame - e.last_used < kKeepFrames) {
            continue;
        }
        if (e.gl != 0 && gl::loaded()) {
            glDeleteTextures(1, &e.gl);
        }
        m_converted.erase(e.key);
        e = Entry{};
        m_free.push_back(static_cast<TextureHandle>(i + 1));
    }
}

std::size_t TexturePool::size() const {
    return static_cast<std::size_t>(
        std::count_if(m_entries.begin(), m_entries.end(), [](const Entry& e) { return e.live; })
    );
}

}  // namespace openrac::renderer
