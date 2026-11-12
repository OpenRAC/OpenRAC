// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/viewer/probe_lighting.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <format>
#include <future>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include "common/env.h"
#include "common/json.h"
#include "common/log.h"
#include "common/stats.h"
#include "enhanced/lighting/probe_depth.h"
#include "enhanced/renderer/local_lights.h"
#include "enhanced/tools/baker/sources/activation.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"
#include "enhanced/viewer/early_lighting.h"
#include "enhanced/viewer/probe_texture_upload.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/gl.h"

#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif
#include "renderer_shaders.h"

namespace openrac::lighting {
using namespace openrac::gl;

namespace {

struct ProbeTextureFormat {
    GLenum internal, source, type;
};

constexpr ProbeTextureFormat kProbeFormats[] = {
    {GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV},
    {GL_RG32F, GL_RG, GL_FLOAT},
    {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT},
    {GL_RG32F, GL_RG, GL_FLOAT},
    {GL_RGBA32F, GL_RGBA, GL_FLOAT},
};
// Caps dense probe textures independently of the device's maximum atlas dimensions.
constexpr std::size_t kDenseResidencyBudget = 256ull * 1024 * 1024;
constexpr unsigned kFirstUnit = 3;
// Stored layout and device capability select one of these upload/sampling modes.
constexpr std::array<const char*, 5> kProbeDepthNames{
    "RG32F moments",
    "decoded ASTC moments",
    "ASTC 4x4 mean/spread",
    "RG8 mean/spread",
    "RG16 UNORM mean/spread"
};
// The final two units are reserved for compressed depth spread and FP32 ranges.
constexpr std::array<unsigned, 5> kProbeUnits{3, 4, 5, 30, 31};

// Shader::uniform caches by name address. Keep each array element's name stable across draws.
template <std::size_t N>
constexpr auto dense_uniform_names(const char (&base)[N]) {
    static_assert(kDenseProbeSlots <= 10);
    std::array<std::array<char, N + 3>, kDenseProbeSlots> names{};
    for (unsigned slot = 0; slot < names.size(); ++slot) {
        auto& name = names[slot];
        std::copy_n(base, N - 1, name.begin());
        name[N - 1] = '[';
        name[N] = static_cast<char>('0' + slot);
        name[N + 1] = ']';
    }
    return names;
}

constexpr auto kDenseWeight = dense_uniform_names("probe_dense_weight");
constexpr auto kDenseFrame = dense_uniform_names("probe_dense_frame");
constexpr auto kDenseOrigin = dense_uniform_names("probe_dense_origin");
constexpr auto kDenseSpacing = dense_uniform_names("probe_dense_spacing");
constexpr auto kDenseDimensions = dense_uniform_names("probe_dense_dimensions");
constexpr auto kDenseOffset = dense_uniform_names("probe_dense_offset");

// Selects the native scalar width while preserving legacy moment and irradiance uploads.
ProbeTextureFormat upload_format(unsigned plane, int depth_mode) {
    // Only interleaved native depth formats change the original upload contract.
    if (plane != 1 || depth_mode < 3) {
        return kProbeFormats[plane];
    }

    // Sixteen-bit channels use the same gather path as legacy eight-bit channels.
    if (depth_mode == 4) {
        return {GL_RG16, GL_RG, GL_UNSIGNED_SHORT};
    }

    return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
}

std::span<const std::uint8_t> probe_plane(
    const ProbeVolume& volume, unsigned map, int depth_mode, std::vector<Float2>& decoded
) {
    if (map == 1 && depth_mode == 1) {
        decoded = decode_probe_depth_moment_atlas(volume);
        return {
            reinterpret_cast<const std::uint8_t*>(decoded.data()), decoded.size() * sizeof(Float2)
        };
    }
    return volume.gpu_plane(map);
}

// A baked tile row can be one of several horizontal segments in the resident atlas.
void pack_probe_plane(
    std::span<std::uint8_t> atlas,
    std::span<const std::uint8_t> plane,
    std::size_t offset,
    std::size_t baked_rows,
    std::size_t rows_per_tile,
    std::size_t atlas_tiles_x
) {
    const auto row_bytes = plane.size() / (baked_rows * rows_per_tile);
    const auto tile_bytes = row_bytes / kProbeAtlasTilesX;
    const auto pitch = atlas_tiles_x * tile_bytes;
    for (std::size_t row = 0; row < baked_rows; ++row) {
        const auto tile = offset + row * kProbeAtlasTilesX;
        const auto y = tile / atlas_tiles_x * rows_per_tile;
        const auto x = tile % atlas_tiles_x * tile_bytes;
        for (std::size_t line = 0; line < rows_per_tile; ++line) {
            const auto src = (row * rows_per_tile + line) * row_bytes;
            const auto dst = (y + line) * pitch + x;
            std::memcpy(atlas.data() + dst, plane.data() + src, row_bytes);
        }
    }
}

// Returns the value `var` was set to, or the empty string when it is unset: the probe GI's reads
// take the two alike.
std::string environment(env::Var var) {
    return std::string(env::value(var).value_or(std::string_view{}));
}

unsigned rays_from_environment() {
    const auto value = environment(env::Var::ProbeRays);
    if (value.empty()) {
        return 1024;
    }
    std::string_view text(value);
    unsigned result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result < 16) {
        throw std::runtime_error("invalid OPENRAC_PROBE_RAYS");
    }
    return result;
}
}  // namespace

std::string probe_shader_source(std::string source) {
    if (!enhanced_rendering_available()) {
        return source;
    }
    source.insert(source.find('\n') + 1, enhanced::local_light_shader_source());
    source.insert(source.find('\n') + 1, shaders::shading_brdf_glsl);
    source.insert(source.find('\n') + 1, renderer::shaders::shading_normal_glsl);
    static const bool& gather_off = stats::probe(
        "OPENRAC_SHADOW_GATHER_OFF", "Reference: 16 individual shadow texel fetches (reload level)"
    );
    if (gl::is_vulkan() && !gather_off) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_SHADOW_GATHER 1\n");
    }
    static const bool& reference = stats::probe(
        "OPENRAC_SHADOW_FILTER_REFERENCE",
        "Profiling reference: nine bilinear shadow taps instead of unique texels"
    );
    if (reference) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_SHADOW_FILTER_REFERENCE 1\n");
    }
    static const bool& zero_weight_reference = stats::probe(
        "OPENRAC_PROBE_ZERO_WEIGHT_REFERENCE",
        "Profiling reference: fetch probe atlases for zero-weight valid corners"
    );
    if (zero_weight_reference) {
        source.insert(source.find('\n') + 1, "#define OPENRAC_PROBE_ZERO_WEIGHT_REFERENCE 1\n");
    }
    return source;
}

std::string probe_mesh_fragment(std::string source) {
    if (!enhanced_rendering_available()) {
        return source;
    }
    source.insert(
        source.find('\n') + 1,
        std::string(
            "#if (!defined(OPENRAC_SOLID) && (!defined(OPENRAC_COLOUR_SPECIALISED) || "
            "(defined(OPENRAC_HERO_WETNESS) && !defined(OPENRAC_STATIC_COLOUR)))) || "
            "defined(OPENRAC_HERO_FUR)\n"
        ) + std::string(shaders::environment_glsl)
            + "\n#endif\n"
    );
    source += std::string("\n#if !defined(OPENRAC_SOLID) && !defined(OPENRAC_COLOUR_SPECIALISED)\n")
              + std::string(shaders::chrome_glsl) + "\n#endif\n";
    return probe_shader_source(std::move(source));
}

std::string probe_early_fragment(std::string source) {
    if (!enhanced_rendering_available()) {
        return probe_mesh_fragment(std::move(source));
    }
    source += shaders::early_lighting_glsl;
    source.insert(
        source.find('\n') + 1,
        std::string(
            "#if (!defined(OPENRAC_SOLID) && (!defined(OPENRAC_COLOUR_SPECIALISED) || "
            "(defined(OPENRAC_HERO_WETNESS) && !defined(OPENRAC_STATIC_COLOUR)))) || "
            "defined(OPENRAC_HERO_FUR)\n"
        ) + std::string(shaders::environment_glsl)
            + "\n#endif\n"
    );
    source += std::string("\n#if !defined(OPENRAC_SOLID) && !defined(OPENRAC_COLOUR_SPECIALISED)\n")
              + std::string(shaders::chrome_glsl) + "\n#endif\n";
    source.insert(source.find('\n') + 1, "#define OPENRAC_EARLY_LIGHTING 1\n");

    return probe_shader_source(std::move(source));
}

bool ProbeLighting::upload(
    const lighting::ProbeVolume& volume, std::string& error, const lighting::ProbeVolume* dense
) {
    std::vector<lighting::ProbeVolume> regions;
    if (dense) {
        regions.push_back(*dense);
    }
    if (!upload_regions(volume, std::move(regions), error)) {
        return false;
    }
    // An explicit dense pair starts fully active until the first camera update.
    if (dense) {
        m_selector.slots[0] = {0, 1};
        upload_region(0, 0);
    }
    return true;
}

bool ProbeLighting::upload_regions(
    const lighting::ProbeVolume& volume,
    std::vector<lighting::ProbeVolume> regions,
    std::string& error
) {
    release();
    if (!enhanced_rendering_available()) {
        error = "Probe lighting requires an enhanced rendering build and Vulkan";
        return false;
    }
    try {
        volume.validate();
        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
        if (limit <= 0) {
            throw std::runtime_error("no probe texture capacity");
        }
        const auto colour_internal = lighting::probe_texture_internal(volume.colour_format);
        // Unsupported devices reconstruct moments once on load, retaining the original hardware filter.
        m_depth_mode = 0;
        if (volume.depth_format == ProbeDepthFormat::Astc4x4) {
            m_depth_mode = gl::has_extension("GL_KHR_texture_compression_astc_ldr") ? 2 : 1;
        } else if (volume.depth_format == ProbeDepthFormat::Rg8) {
            m_depth_mode = 3;
        }
        // Native RG16 shares the interleaved gather path with higher component precision.
        if (volume.depth_format == ProbeDepthFormat::Rg16) {
            m_depth_mode = 4;
        }
        for (const auto& region : regions) {
            if (region.colour_format != volume.colour_format
                || region.depth_format != volume.depth_format) {
                throw std::runtime_error(
                    "probe volumes use different texture formats; rebake lighting"
                );
            }
            lighting::validate_probe_pair(volume, region);
            m_region_capacity = std::max(m_region_capacity, region.atlas_capacity());
        }
        m_regions = std::move(regions);
        std::size_t total_regions = 0;
        for (const auto& region : m_regions) {
            m_region_grids.push_back(region.grid);
            m_region_offsets.push_back(
                static_cast<unsigned>(volume.atlas_capacity() + total_regions)
            );
            total_regions += region.atlas_capacity();
        }
        std::string compute_error;
        const auto side = volume.grid.map_resolution + 2;
        // Interleaved RG16 doubles the two-byte RG8 texel without changing range records.
        const std::size_t scalar_bytes = m_depth_mode == 4 ? 4 : 2;
        const auto probe_bytes =
            lighting::probe_texture_bytes(volume.colour_format, side, side)
            + (m_depth_mode >= 2 ? scalar_bytes * side * side + 16 : 8ull * side * side) + 8;
        const auto resident_bytes = total_regions * probe_bytes;
        if (resident_bytes <= kDenseResidencyBudget) {
            m_full_residency = m_selection.prepare(m_region_grids, m_region_offsets, compute_error);
        } else {
            m_selection.prepare({}, {}, compute_error);
            log::warn("probe GPU selection fallback: dense residency exceeds 256 MiB budget");
        }
        if (!compute_error.empty()) {
            log::warn("probe GPU selection fallback: {}", compute_error);
        }
        if (volume.gpu_bytes.empty()) {
            throw std::runtime_error("probe upload requires GPU-ready baked data");
        }
        for (const auto& region : m_regions) {
            if (region.gpu_bytes.empty()) {
                throw std::runtime_error("dense probe upload requires GPU-ready baked data");
            }
        }
        const auto rows_for = [&](bool resident) {
            return (volume.atlas_capacity()
                    + (resident ? total_regions : m_region_capacity * lighting::kDenseProbeSlots)
                    + m_atlas_tiles_x - 1)
                   / m_atlas_tiles_x;
        };
        // Widen by complete baked rows until the resident atlas fits the device's height limit.
        while (rows_for(m_full_residency) * side > static_cast<unsigned>(limit)
               && m_atlas_tiles_x * side <= static_cast<unsigned>(limit) / 2) {
            m_atlas_tiles_x *= 2;
        }
        const unsigned width = m_atlas_tiles_x * side;
        if (width > static_cast<unsigned>(limit)) {
            throw std::runtime_error("probe atlas width exceeds GPU capacity");
        }
        if (rows_for(m_full_residency) * side > static_cast<unsigned>(limit) && m_full_residency) {
            m_full_residency = false;
            m_selection.release();
            log::warn("probe GPU selection fallback: full resident atlas exceeds capacity");
        }
        const auto rows = rows_for(m_full_residency);
        if (rows * side > static_cast<unsigned>(limit)) {
            throw std::runtime_error("probe atlas exceeds GPU capacity");
        }
        const unsigned maps = m_depth_mode >= 2 ? 5 : 3;
        glGenTextures(static_cast<GLsizei>(maps), m_textures.data());
        std::vector<std::uint8_t> pixels;
        std::vector<Float2> decoded;
        for (unsigned i = 0; i < maps; ++i) {
            // RG8 interleaves spread with mean; the spare texture name needs no backing image.
            if (m_depth_mode >= 3 && i == 3) {
                continue;
            }
            glActiveTexture(GL_TEXTURE0 + kProbeUnits[i]);
            glBindTexture(GL_TEXTURE_2D, m_textures[i]);
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (i == 2 || i == 4) ? GL_NEAREST : GL_LINEAR
            );
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (i == 2 || i == 4) ? GL_NEAREST : GL_LINEAR
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            const bool compressed_depth = m_depth_mode == 2 && (i == 1 || i == 3);
            const bool compressed_colour =
                i == 0 && volume.colour_format != lighting::ProbeTextureFormat::R11G11B10;
            const unsigned block = compressed_depth ? 4
                                   : compressed_colour ? probe_texture_block(volume.colour_format)
                                                       : 1;
            const unsigned scale = i == 2 || i == 4 ? 1 : side;
            const auto base_plane = probe_plane(volume, i, m_depth_mode, decoded);
            const auto tile_bytes = base_plane.size() / volume.atlas_capacity();
            pixels.resize(m_atlas_tiles_x * rows * tile_bytes);
            if (block == 1) {
                std::ranges::fill(pixels, 0);
            } else {
                // Sparse slots and padding need valid compressed blocks before their first upload.
                constexpr std::size_t block_bytes = 16;
                for (std::size_t at = 0; at < pixels.size(); at += block_bytes) {
                    std::memcpy(pixels.data() + at, base_plane.data(), block_bytes);
                }
            }
            pack_probe_plane(pixels, base_plane, 0, volume.atlas_rows(), scale / block, m_atlas_tiles_x);
            if (m_full_residency) {
                for (std::size_t region = 0; region < m_regions.size(); ++region) {
                    const auto& source = m_regions[region];
                    const auto plane = probe_plane(source, i, m_depth_mode, decoded);
                    pack_probe_plane(pixels, plane, m_region_offsets[region], source.atlas_rows(),
                                     scale / block, m_atlas_tiles_x);
                }
            }

            if (block != 1) {
                glCompressedTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    compressed_depth ? GL_COMPRESSED_RGBA_ASTC_4x4_KHR : colour_internal,
                    static_cast<GLsizei>(width),
                    static_cast<GLsizei>(rows * side),
                    0,
                    static_cast<GLsizei>(pixels.size()),
                    pixels.data()
                );
            } else {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    upload_format(i, m_depth_mode).internal,
                    static_cast<GLsizei>(m_atlas_tiles_x * scale),
                    static_cast<GLsizei>(rows * scale),
                    0,
                    kProbeFormats[i].source,
                    upload_format(i, m_depth_mode).type,
                    pixels.data()
                );
            }
        }
        glActiveTexture(GL_TEXTURE0);
        if (glGetError() != GL_NO_ERROR) {
            throw std::runtime_error("probe texture upload failed");
        }
        m_grid = volume.grid;
        m_sparse_capacity = volume.atlas_capacity();
        log::info(
            "probe textures: {} irradiance, {} depth, RGBA16F metadata; {}x{}, {} "
            "bytes",
            lighting::probe_texture_name(volume.colour_format),
            kProbeDepthNames[static_cast<std::size_t>(m_depth_mode)],
            width,
            rows * side,
            (m_atlas_tiles_x * rows) * probe_bytes
        );
        if (m_full_residency) {
            log::info(
                "tiled probe selection: {} resident regions, 16x16 tiles, three volumes per "
                "fragment",
                m_regions.size()
            );
        }
        const auto append_spheres = [&](const lighting::ProbeVolume& source, int region) {
            const float radius =
                0.025f * *std::min_element(source.grid.spacing.begin(), source.grid.spacing.end());
            for (std::size_t i = 0; i < source.probes.size(); ++i) {
                const auto position = source.position(i);
                const auto& probe = source.probes[i];
                const bool relocated = probe.relocated;
                m_spheres.push_back(
                    {{position[0], position[1], position[2], radius},
                     !probe.valid  ? lighting::Float3{1, 0.05f, 0.7f}
                     : relocated   ? lighting::Float3{1, 0.6f, 0.05f}
                     : region >= 0 ? lighting::Float3{0.15f, 1.0f, 0.3f}
                                   : lighting::Float3{0.05f, 0.85f, 1},
                     region}
                );
            }
        };
        append_spheres(volume, -1);
        for (std::size_t i = 0; i < m_regions.size(); ++i) {
            append_spheres(m_regions[i], static_cast<int>(i));
        }
        return true;
#if defined(OPENRAC_VULKAN)
    } catch (const vkgl::RecordingFailure&) {
        throw;
#endif
    } catch (const std::exception& why) {
        error = why.what();
        release();
        return false;
    }
}

void ProbeLighting::upload_volume(const lighting::ProbeVolume& volume, std::size_t offset) {
    const auto side = volume.grid.map_resolution + 2;
    // Place complete baked rows side by side without decoding or rearranging their bytes.
    std::vector<Float2> expanded;
    for (unsigned map = 0; map < (m_depth_mode >= 2 ? 5u : 3u); ++map) {
        glActiveTexture(GL_TEXTURE0 + kProbeUnits[map]);
        glBindTexture(GL_TEXTURE_2D, m_textures[map]);
        const unsigned scale = map == 2 || map == 4 ? 1 : side;
        const auto plane = probe_plane(volume, map, m_depth_mode, expanded);
        // Interleaved RG8 has no separate spread plane.
        if (plane.empty()) {
            continue;
        }
        const std::size_t row_bytes = plane.size() / volume.atlas_rows();
        // Each source row contains complete probe tiles, including their gutters and padding.
        for (std::size_t row = 0; row < volume.atlas_rows(); ++row) {
            const std::size_t tile = offset + row * lighting::kProbeAtlasTilesX;
            if ((map == 0 && volume.colour_format != lighting::ProbeTextureFormat::R11G11B10)
                || (m_depth_mode == 2 && (map == 1 || map == 3))) {
                glCompressedTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    static_cast<GLint>(tile % m_atlas_tiles_x * scale),
                    static_cast<GLint>(tile / m_atlas_tiles_x * scale),
                    static_cast<GLsizei>(lighting::kProbeAtlasTilesX * scale),
                    static_cast<GLsizei>(scale),
                    map == 0 ? lighting::probe_texture_internal(volume.colour_format)
                             : GL_COMPRESSED_RGBA_ASTC_4x4_KHR,
                    static_cast<GLsizei>(row_bytes),
                    plane.data() + row * row_bytes
                );
            } else {
                glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    static_cast<GLint>(tile % m_atlas_tiles_x * scale),
                    static_cast<GLint>(tile / m_atlas_tiles_x * scale),
                    static_cast<GLsizei>(lighting::kProbeAtlasTilesX * scale),
                    static_cast<GLsizei>(scale),
                    kProbeFormats[map].source,
                    upload_format(map, m_depth_mode).type,
                    plane.data() + row * row_bytes
                );
            }
        }
    }
    glActiveTexture(GL_TEXTURE0);
    if (glGetError() != GL_NO_ERROR) {
        throw std::runtime_error("packed probe texture upload failed");
    }
}

std::size_t ProbeLighting::region_offset(unsigned slot, int region) const {
    return m_full_residency ? m_region_offsets.at(region)
                            : m_sparse_capacity + slot * m_region_capacity;
}

void ProbeLighting::upload_region(unsigned slot, int region) {
    upload_volume(m_regions.at(region), region_offset(slot, region));
    if (!m_full_residency) {
        m_gpu_regions[slot] = region;
    }
}

void ProbeLighting::update_regions(
    lighting::Float3 camera, float seconds, const renderer::Mat4* world_to_clip
) {
    const auto now = std::chrono::steady_clock::now();
    if (seconds < 0) {
        seconds = m_camera_started
                      ? std::min(0.1f, std::chrono::duration<float>(now - m_region_time).count())
                      : m_selector.fade_seconds;
    }
    // A bake read in the background (load) is uploaded here, on the rendering thread.
    poll_load();
    if (!m_camera_started) {
        m_selector.slots = {};
    }
    m_camera_started = true;
    m_region_time = now;
    if (!active()) {
        return;
    }
    m_selector.update(m_region_grids, camera, seconds, world_to_clip);
    if (m_full_residency) {
        return;
    }
    for (unsigned slot = 0; slot < lighting::kDenseProbeSlots; ++slot) {
        const int region = m_selector.slots[slot].region;
        if (region >= 0 && region != m_gpu_regions[slot]) {
            upload_region(slot, region);
        }
    }
}

namespace {
std::filesystem::path& probe_root_storage() {
    static std::filesystem::path root;
    return root;
}

bool& probe_background_load() {
    static bool background = false;
    return background;
}

// The level's bake under the probe root: <root>/level_NN/sparse.orpg, NN its folder's name.
std::string level_bake(const scene::LevelData& level, const std::filesystem::path& root) {
    if (root.empty()) {
        return {};
    }
    auto name = level.dir.filename();
    if (name.empty()) {
        name = level.dir.parent_path().filename();
    }
    if (name.empty() && level.level >= 0) {
        name = std::format("level_{:02}", level.level);
    }
    auto cache = root;
    if (!level.overrides.empty()) {
        try {
            cache /=
                std::filesystem::path("variants") / lighting::probe_variant_key(level.overrides);
        } catch (const std::exception& why) {
            log::warn("cannot identify lighting variant for {}: {}", name.string(), why.what());
            return {};
        }
    }
    const auto sparse = cache / name / "sparse.orpg";
    std::error_code error;
    if (name.empty() || !std::filesystem::is_regular_file(sparse, error)) {
        log::info("probe GI: no bake for {} yet; original lighting", name.string());
        return {};
    }
    return sparse.string();
}
}  // namespace

void set_probe_root(std::filesystem::path root) {
    probe_root_storage() = std::move(root);
}

const std::filesystem::path& probe_root() {
    return probe_root_storage();
}

void set_probe_background_load(bool background) {
    probe_background_load() = background;
}

namespace {
// The parts of `level` that its probe scene fingerprint reads (export_probe_scene's, through
// probe_activation_hash): its folders, number, light sets, images count, sky, materials and the
// mobys that place proxy emitters. A copy, so that the reading thread never shares the level the
// game keeps using. Resolved material bindings are immutable and shared with the reader.
std::shared_ptr<const scene::LevelData> fingerprint_inputs(const scene::LevelData& level) {
    auto inputs = std::make_shared<scene::LevelData>();
    inputs->dir = level.dir;
    inputs->overrides = level.overrides;
    inputs->game = level.game;
    inputs->level = level.level;
    inputs->light_sets = level.light_sets;
    inputs->missing_images = level.missing_images;
    inputs->sky_panorama = level.sky_panorama;
    inputs->materials = level.materials;
    inputs->material_bindings = level.material_bindings;
    inputs->moby_classes = level.moby_classes;
    const auto mobys = static_cast<std::size_t>(scene::Layer::Mobys);
    inputs->instances[mobys] = level.instances[mobys];
    return inputs;
}

// Reads and verifies the bake at `path` for `level` (the profile, the scene and settings
// fingerprints, the sparse grid and its dense regions): everything of a load but the uploads, which
// need the rendering thread. Throws on a bake that does not match; a region that does not is left
// out with a warning, as before. Stops between regions once `cancelled` is set.
ProbeBakeRead read_bake(
    const scene::LevelData& level, const std::string& path, const std::atomic<bool>& cancelled
) {
    const auto start = std::chrono::steady_clock::now();
    ProbeBakeRead read;
    read.path = path;
    auto& expected = read.expected;
    auto profile = lighting::load_level_bake_profile(level.dir, level.overrides);
    expected = profile.activation;
    read.look_regions = std::move(profile.look_regions);
    if (!environment(env::Var::ProbeRays).empty()) {
        expected.bake.rays_per_probe = rays_from_environment();
        expected.bake.adaptive_rays = false;
        profile.explicit_ray_policy = true;
    }
    const auto sky = environment(env::Var::ProbeSky);
    if (!sky.empty()) {
        if (sky != "level" && sky != "uniform") {
            throw std::runtime_error("invalid OPENRAC_PROBE_SKY");
        }
        expected.bake.use_level_sky = sky == "level";
    }
    const auto intensity = environment(env::Var::ProbeSkyIntensity);
    if (!intensity.empty()) {
        std::size_t consumed = 0;
        expected.bake.sky_intensity = std::stof(intensity, &consumed);
        if (consumed != intensity.size() || !std::isfinite(expected.bake.sky_intensity)
            || expected.bake.sky_intensity < 0) {
            throw std::runtime_error("invalid OPENRAC_PROBE_SKY_INTENSITY");
        }
    }
    lighting::ProbeAssetLoader loader(level, expected);
    read.volume = loader.load_with_previous_defaults(path, profile);
    const auto directory = std::filesystem::path(path).parent_path();
    std::error_code environment_error;
    const bool has_environment =
        std::filesystem::is_regular_file(directory / "environment.orpe", environment_error);
    lighting::EnvironmentGiPayloads loaded;
    if (has_environment) {
        loaded.emplace(std::filesystem::absolute(path).lexically_normal(), read.volume.gpu_bytes);
    }
    const auto add_region = [&](const std::filesystem::path& file) {
        if (cancelled.load(std::memory_order_relaxed)) {
            return;
        }
        try {
            auto region = loader.load(file);
            lighting::validate_probe_pair(read.volume, region);
            read.regions.push_back(std::move(region));
            if (has_environment) {
                loaded.emplace(
                    std::filesystem::absolute(file).lexically_normal(),
                    read.regions.back().gpu_bytes
                );
            }
        } catch (const std::exception& why) {
            log::warn("local probe region {} inactive: {}", file.string(), why.what());
        }
    };
    const auto dense_path = environment(env::Var::ProbeDense);
    if (!dense_path.empty() && dense_path != "0") {
        add_region(dense_path);
    }
    auto manifest_path = environment(env::Var::ProbeRegions);
    if (manifest_path.empty()) {
        const auto sidecar = path + ".regions.json";
        if (std::filesystem::is_regular_file(sidecar)) {
            manifest_path = sidecar;
        }
    }
    if (!manifest_path.empty() && manifest_path != "0") {
        json::Value manifest;
        std::string why;
        if (!json::parse_file(manifest_path, manifest, why) || !manifest["regions"].is_array()) {
            log::warn(
                "probe region manifest inactive: {}", why.empty() ? "regions must be an array" : why
            );
        } else {
            for (const auto& entry : manifest["regions"].items()) {
                if (!entry.is_string() || entry.string().empty()) {
                    log::warn("probe region manifest entry must be an asset path");
                    continue;
                }
                add_region(std::filesystem::path(manifest_path).parent_path() / entry.string());
            }
        }
    }
    if (has_environment && !cancelled.load(std::memory_order_relaxed)) {
        try {
            read.environment_gi_hash = lighting::environment_gi_hash(directory, loaded);
        } catch (const std::exception& why) {
            log::warn("environment GI hash unavailable: {}", why.what());
        }
    }
    read.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return read;
}
}  // namespace

void ProbeLighting::load(const scene::LevelData& level) {
    cancel_load();
    m_load_error.clear();
    if (!enhanced_rendering_available()) {
        return;
    }
    auto path = environment(env::Var::ProbeGi);
    if (path == "0") {
        return;
    }
    const bool background = probe_background_load();
    const auto root = probe_root();
    const bool locate_in_background =
        path.empty() && background && !level.overrides.empty() && !root.empty();
    if (path.empty() && !locate_in_background) {
        path = level_bake(level, root);
        if (path.empty()) {
            return;
        }
    }
    m_pending_level = level.level;
    if (!background) {
        try {
            const std::atomic<bool> never{false};
            finish_load(read_bake(level, path, never));
#if defined(OPENRAC_VULKAN)
        } catch (const vkgl::RecordingFailure&) {
            throw;
#endif
        } catch (const std::exception& why) {
            fail_load(why.what());
        }
        return;
    }
    // The bake is read and verified on another thread while the level is drawn with its original
    // lighting: reading it here held the level's first picture back (seconds with large levels).
    // update_regions, on the rendering thread, uploads it once it is ready.
    m_cancel_read = std::make_shared<std::atomic<bool>>(false);
    m_pending_read = std::async(
        std::launch::async,
        [inputs = fingerprint_inputs(level), path, root, cancelled = m_cancel_read] {
            try {
                const auto selected = path.empty() ? level_bake(*inputs, root) : path;
                return selected.empty() ? ProbeBakeRead{}
                                        : read_bake(*inputs, selected, *cancelled);
            } catch (const std::exception& why) {
                ProbeBakeRead failed;
                failed.error = why.what();
                return failed;
            }
        }
    );
    log::info(
        "probe GI: reading level {}'s bake; original lighting until it is ready", level.level
    );
}

void ProbeLighting::poll_load() {
    if (!m_pending_read.valid()
        || m_pending_read.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return;
    }
    auto read = m_pending_read.get();
    m_cancel_read.reset();
    if (!read.error.empty()) {
        fail_load(read.error);
        return;
    }
    if (read.path.empty()) {
        return;
    }
    try {
        finish_load(std::move(read));
#if defined(OPENRAC_VULKAN)
    } catch (const vkgl::RecordingFailure&) {
        throw;
#endif
    } catch (const std::exception& why) {
        fail_load(why.what());
    }
}

void ProbeLighting::cancel_load() {
    if (m_cancel_read) {
        m_cancel_read->store(true, std::memory_order_relaxed);
    }
    // The read owns its inputs, so wait for it to finish before discarding the result.
    if (m_pending_read.valid()) {
        m_pending_read.wait();
        m_pending_read = {};
    }
    m_cancel_read.reset();
}

void ProbeLighting::fail_load(std::string why) {
    release();
    m_load_error = std::move(why);
    log::warn("probe GI inactive for level {}: {}", m_pending_level, m_load_error);
}

void ProbeLighting::finish_load(ProbeBakeRead read) {
    const auto start = std::chrono::steady_clock::now();
    const auto& expected = read.expected;
    const auto& volume = read.volume;
    const auto& path = read.path;
    {
#if defined(OPENRAC_VULKAN)
        const auto bytes = std::accumulate(
            read.regions.begin(),
            read.regions.end(),
            volume.gpu_bytes.size(),
            [](std::size_t sum, const auto& region) { return sum + region.gpu_bytes.size(); }
        );
        vkgl::BulkTextureUpload staging(bytes);
#endif
        std::string error;
        if (!upload_regions(volume, std::move(read.regions), error)) {
            log::warn("probe region allocation failed: {}; using level grid", error);
            if (!upload(volume, error)) {
                throw std::runtime_error(error);
            }
        }
        if (read.environment_gi_hash) {
            m_environment.load(
                std::filesystem::path(path).parent_path(), volume.grid, read.environment_gi_hash
            );
        } else {
            m_environment.release();
        }
#if defined(OPENRAC_VULKAN)
        staging.finish();
#endif
    }
    log::info(
        "probe GI: {} dense regions registered; at most three dense samples per fragment",
        m_regions.size()
    );
    m_sunlight_scale = expected.bake.sunlight_scale;
    // Optional markers are loaded only after the corresponding probe bake passed validation.
    const auto medial =
        m_medial_axis.load(std::filesystem::path(path).parent_path() / "medial-axis.json");
    if (!medial) {
        log::warn("medial axis overlay unavailable: {}", medial.error().message);
    }
    const auto markers = m_area_lights.load(
        std::filesystem::path(path).parent_path() / "interior-lights-report.json"
    );
    if (!markers) {
        log::warn("ceiling area light overlay unavailable: {}", markers.error().message);
    } else if (*markers != 0) {
        log::info("ceiling area light overlay: {} baked sources loaded", *markers);
    }
    m_sunlight_direction = expected.bake.sunlight_direction;
    // Report overrides after verification so a stale bake cannot claim the new direction.
    if (m_sunlight_direction) {
        const auto& direction = *m_sunlight_direction;
        log::info(
            "enhanced sunlight travel direction: {} {} {}", direction[0], direction[1], direction[2]
        );
    }

    m_look_uniforms.clear();
    m_look_uniforms.reserve(read.look_regions.size() * 8);
    for (const auto& region : read.look_regions) {
        m_look_uniforms.insert(
            m_look_uniforms.end(),
            {region.minimum[0],
             region.minimum[1],
             region.minimum[2],
             region.feather,
             region.maximum[0],
             region.maximum[1],
             region.maximum[2],
             region.diffuse_gain}
        );
    }
    m_terrain_vertex_albedo =
        expected.bake.terrain_vertex_albedo ? expected.bake.terrain_vertex_albedo_strength : 0;
    log::info(
        "enhanced sunlight RGB scale: {} {} {}",
        m_sunlight_scale[0],
        m_sunlight_scale[1],
        m_sunlight_scale[2]
    );
    const auto view = environment(env::Var::ProbeView);
    if (!view.empty()) {
        const std::string_view name(view);
        if (name == "original") {
            mode = 0;
        } else if (name == "combined") {
            mode = 1;
        } else if (name == "indirect") {
            mode = 2;
        } else if (name == "direct") {
            mode = 3;
        } else if (name == "coverage") {
            mode = 4;
        } else {
            throw std::runtime_error("unknown OPENRAC_PROBE_VIEW");
        }
    }
    backlight = env::flag(env::Var::ProbeBacklight);
    const auto sphere_view = environment(env::Var::ProbeSpheres);
    sphere_xray = sphere_view == "xray";
    all_volume_bounds = sphere_view == "all";
    show_spheres = sphere_view == "1" || sphere_xray || all_volume_bounds;
    environment_spheres = show_spheres;
    if (show_spheres) {
        log::info(
            "probe spheres: actual positions within {} grid spacings of camera; cyan "
            "sparse, green dense, gold relocated, magenta invalid",
            sphere_xray ? 10 : 5
        );
    }
    log::info(
        "probe GI: {} probes for {} level {}, scene/settings verified; view {}",
        volume.probes.size(),
        m_grid.game_version,
        m_grid.level,
        mode
    );
    log::info(
        "probe GI: read in {:.0f} ms, uploaded in {:.0f} ms",
        read.seconds * 1000.0,
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
    );
    log::info(
        "probe GI: original ambient replaced on terrain and lit mobys; GTAO follows the "
        "user setting"
    );
}

void ProbeLighting::bind(
    const renderer::Shader& shader,
    scene::Layer layer,
    bool all_static_receivers,
    const renderer::Camera* camera,
    bool depth_matched
) const {
    if (!enhanced_rendering_available()) {
        return;
    }
    m_selection.bind(shader, camera, depth_matched);
    m_environment.bind(shader, active(), camera);
    const bool static_receiver =
        layer == scene::Layer::Terrain
        || ((mode == 2 || all_static_receivers)
            && (layer == scene::Layer::Ties || layer == scene::Layer::Shrubs));
    const int receiver = static_receiver ? 1 : (layer == scene::Layer::Mobys ? 2 : 0);
    glUniform1i(shader.uniform("probe_receiver"), receiver);
    glUniform1i(shader.uniform("probe_mode"), active() && receiver ? mode : 0);
    const int look_count =
        active() && receiver && mode == 1 ? static_cast<int>(m_look_uniforms.size() / 8) : 0;
    glUniform1i(shader.uniform("probe_look_count"), look_count);
    if (look_count) {
        glUniform4fv(shader.uniform("probe_look[0]"), look_count * 2, m_look_uniforms.data());
    }
    glUniform1f(shader.uniform("probe_gi_multiplier"), gi_multiplier);
    glUniform1f(
        shader.uniform("probe_terrain_albedo"),
        layer == scene::Layer::Terrain ? m_terrain_vertex_albedo : 0
    );
    // Going Commando's original lighting is the anchor (README, Retail anchoring): its moby ambient
    // and terrain vertex lighting carry more of each level's look than its dimmer skies give the
    // bake. Ratchet & Clank keeps the tuned look it has.
    glUniform1i(
        shader.uniform("probe_retail_anchor"), active() && m_grid.game_version.starts_with("rac2-")
    );
    glUniform3f(
        shader.uniform("probe_sunlight_scale"),
        m_sunlight_scale[0],
        m_sunlight_scale[1],
        m_sunlight_scale[2]
    );
    glUniform1f(shader.uniform("probe_normal_bias"), normal_bias);
    const auto direction_override =
        runtime_sunlight_direction ? runtime_sunlight_direction : m_sunlight_direction;
    const auto direction = direction_override.value_or(lighting::Float3{});
    glUniform4f(
        shader.uniform("probe_sunlight_direction"),
        direction[0],
        direction[1],
        direction[2],
        direction_override.has_value() ? 1.0f : 0.0f
    );
    glUniform1f(shader.uniform("probe_view_bias"), view_bias);
    glUniform1i(shader.uniform("probe_effect"), 0);
    glUniform1i(shader.uniform("probe_backlight"), backlight ? 1 : 0);
    glUniform1i(shader.uniform("probe_irradiance"), kFirstUnit);
    glUniform1i(shader.uniform("probe_moments"), kFirstUnit + 1);
    glUniform1i(shader.uniform("probe_metadata"), kFirstUnit + 2);
    glUniform1i(shader.uniform("probe_depth_mode"), std::max(0, m_depth_mode - 1));
    glUniform1i(shader.uniform("probe_spread"), kProbeUnits[3]);
    glUniform1i(shader.uniform("probe_depth_ranges"), kProbeUnits[4]);
    if (!active() || !receiver) {
        return;
    }
    glUniform2f(shader.uniform("probe_frame"), m_grid.frame.axis[0], m_grid.frame.axis[1]);
    glUniform3f(
        shader.uniform("probe_origin"), m_grid.origin[0], m_grid.origin[1], m_grid.origin[2]
    );
    glUniform3f(
        shader.uniform("probe_spacing"), m_grid.spacing[0], m_grid.spacing[1], m_grid.spacing[2]
    );
    glUniform3f(
        shader.uniform("probe_dimensions"),
        static_cast<float>(m_grid.dimensions[0]),
        static_cast<float>(m_grid.dimensions[1]),
        static_cast<float>(m_grid.dimensions[2])
    );
    glUniform1f(shader.uniform("probe_resolution"), static_cast<float>(m_grid.map_resolution));
    glUniform1i(shader.uniform("probe_tiles_x"), static_cast<GLint>(m_atlas_tiles_x));
    for (unsigned slot = 0; slot < lighting::kDenseProbeSlots; ++slot) {
        const auto& selected = m_selector.slots[slot];
        glUniform1f(
            shader.uniform(kDenseWeight[slot].data()),
            selected.region >= 0 ? selected.weight : 0.0f
        );
        if (selected.region < 0) {
            continue;
        }
        const auto& g = m_region_grids[selected.region];
        glUniform2f(
            shader.uniform(kDenseFrame[slot].data()), g.frame.axis[0], g.frame.axis[1]
        );
        glUniform3f(
            shader.uniform(kDenseOrigin[slot].data()),
            g.origin[0],
            g.origin[1],
            g.origin[2]
        );
        glUniform3f(
            shader.uniform(kDenseSpacing[slot].data()),
            g.spacing[0],
            g.spacing[1],
            g.spacing[2]
        );
        glUniform3f(
            shader.uniform(kDenseDimensions[slot].data()),
            float(g.dimensions[0]),
            float(g.dimensions[1]),
            float(g.dimensions[2])
        );
        glUniform1i(
            shader.uniform(kDenseOffset[slot].data()),
            static_cast<int>(region_offset(slot, selected.region))
        );
    }
    for (unsigned i = 0; i < m_textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + kProbeUnits[i]);
        // An unused spread sampler aliases valid RG8 data without another image allocation.
        glBindTexture(GL_TEXTURE_2D, m_depth_mode >= 3 && i == 3 ? m_textures[1] : m_textures[i]);
    }
    glActiveTexture(GL_TEXTURE0);
}

void ProbeLighting::draw_spheres(
    const renderer::FrameInput& input,
    renderer::RenderState& state,
    std::optional<lighting::Float3> player
) {
    if (active()) {
        m_environment.draw_debug(input, state, show_environment_probes);
    }
    if (active() && show_medial_axis) {
        std::string error;
        if (!m_medial_axis.draw(input, state, error)) {
            log::warn("medial axis overlay disabled: {}", error);
            show_medial_axis = false;
        }
    }
    // Source markers share the existing placement-overlay pass and its displayed world camera.
    if (active() && show_area_lights) {
        std::string error;
        if (!m_area_lights.draw(input, state, error)) {
            log::warn("ceiling area light overlay disabled: {}", error);
            show_area_lights = false;
        }
    }
    if (!active() || !show_spheres) {
        return;
    }
    draw_volume_bounds(input, state, player);

    if (!m_sphere_shader.id()) {
        std::string error;
        if (!m_sphere_shader.build(
                "probe placement spheres",
                shaders::probe_spheres_vert,
                shaders::probe_spheres_frag,
                error
            )) {
            log::warn("probe spheres disabled: {}", error);
            show_spheres = false;
            return;
        }
        glGenVertexArrays(1, &m_sphere_vao);
        glGenBuffers(1, &m_sphere_buffer);
        glGenBuffers(1, &m_sphere_indices);
        glBindVertexArray(m_sphere_vao);
        std::array<unsigned, 12 * 8 * 6> indices{};
        std::iota(indices.begin(), indices.end(), 0u);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphere_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);
    }
    const float range = (sphere_xray ? 10.0f : 5.0f)
                        * *std::max_element(m_grid.spacing.begin(), m_grid.spacing.end());
    m_sphere_indirect = prepare_gpu_spheres(input, range);

    // The reference also handles devices where compute preparation failed.
    if (!m_sphere_indirect) {
        prepare_cpu_spheres(input, range);

        if (m_visible_spheres.empty()) {
            glBindVertexArray(0);
            return;
        }
    }
    glBindVertexArray(m_sphere_vao);
    bind_sphere_attributes(m_sphere_indirect ? m_sphere_culling.output() : m_sphere_buffer);
    m_sphere_shader.use();
    m_selection.bind(m_sphere_shader, &input.camera);
    glUniform1i(m_sphere_shader.uniform("probe_compacted"), m_sphere_indirect ? 1 : 0);
    if (all_volume_bounds) {
        glUniform1i(m_sphere_shader.uniform("probe_gpu_selection"), 0);
    }
    const auto matrix = renderer::multiply(input.camera.projection, input.camera.view);
    glUniformMatrix4fv(m_sphere_shader.uniform("view_projection"), 1, GL_FALSE, matrix.data());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_FALSE);  // diagnostic spheres never hide the scene or later markers
    // The latitude/longitude triangles have inward winding. Draw only the
    // outward-visible hemisphere to avoid alpha self-overdraw during fades.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    constexpr int vertices = 12 * 8 * 6;
    const auto draw = [&]() {
#if defined(OPENRAC_VULKAN)
        // The device-generated count remains on the GPU, including empty draws.
        if (m_sphere_indirect) {
            if (vkgl::draw_indexed_indirect(m_sphere_culling.arguments(), kProbeSphereVertices)) {
                ++state.stats.draw_calls;
            }
            return;
        }
#endif
        glDrawElementsInstanced(
            GL_TRIANGLES,
            vertices,
            GL_UNSIGNED_INT,
            nullptr,
            static_cast<GLsizei>(m_visible_spheres.size())
        );
        ++state.stats.draw_calls;
        state.stats.triangles += m_visible_spheres.size() * (vertices / 3);
    };
    if (sphere_xray) {
        // Hidden portions are translucent and never modify scene depth.
        // Both visible and hidden portions follow the dense region's fade.
        glDepthFunc(GL_LESS);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
        glUniform1f(m_sphere_shader.uniform("debug_alpha"), 0.18f);
        draw();
        glDepthFunc(GL_GEQUAL);
    }
    glUniform1f(m_sphere_shader.uniform("debug_alpha"), 1.0f);
    draw();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ProbeLighting::release() {
    cancel_load();
    m_environment.release();
    m_load_error.clear();
    m_volume_bounds.release();
    m_containing_regions.clear();
    m_volume_bounds_failed = false;
    m_medial_axis.release();
    show_medial_axis = false;
    m_area_lights.release();
    show_area_lights = false;
    show_environment_probes = false;
    m_sunlight_scale = {1, 1, 1};
    m_sunlight_direction.reset();
    runtime_sunlight_direction.reset();
    m_terrain_vertex_albedo = 0;
    m_look_uniforms.clear();
    m_selection.release();
    m_full_residency = false;
    m_region_offsets.clear();
    m_sphere_culling.release();
    m_sphere_gpu_attempted = false;
    m_sphere_indirect = false;
    m_sphere_shader.release();
    if (m_sphere_vao) {
        glDeleteVertexArrays(1, &m_sphere_vao);
    }
    if (m_sphere_buffer) {
        glDeleteBuffers(1, &m_sphere_buffer);
    }
    if (m_sphere_indices) {
        glDeleteBuffers(1, &m_sphere_indices);
    }
    m_sphere_vao = m_sphere_buffer = m_sphere_indices = 0;
    m_spheres.clear();
    m_visible_spheres.clear();
    show_spheres = false;
    environment_spheres = false;
    sphere_xray = false;
    all_volume_bounds = false;
    if (m_textures[0]) {
        glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
    }
    m_textures = {};
    m_depth_mode = 0;
    m_sparse_capacity = 0;
    m_atlas_tiles_x = kMinimumAtlasTilesX;
    m_regions.clear();
    m_region_grids.clear();
    m_selector = {};
    m_gpu_regions = {-1, -1, -1};
    m_region_capacity = 0;
    m_camera_started = false;
    m_region_time = {};
}
}  // namespace openrac::lighting
