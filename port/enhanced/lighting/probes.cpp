// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Probe storage, runtime sampling and versioned GPU-format serialisation.
#include "enhanced/lighting/probes.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "common/profiler.h"
#include "enhanced/lighting/probe_atlas.h"

namespace openrac::lighting {
namespace {
constexpr std::uint32_t kMagic = 0x5047524f;  // ORGP

[[noreturn]] void invalid(const char* message) {
    throw std::runtime_error(message);
}

float dot(Float3 a, Float3 b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Float3 subtract(Float3 a, Float3 b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Float3 unit(Float3 v) {
    const float length = std::sqrt(dot(v, v));

    if (!(length > 0) || !std::isfinite(length)) {
        invalid("probe direction must be finite and nonzero");
    }

    for (float& x : v) {
        x /= length;
    }

    return v;
}

float sign(float x) {
    return x < 0 ? -1.0f : 1.0f;
}

template <std::size_t N>
bool finite(const std::array<float, N>& v) {
    return std::all_of(v.begin(), v.end(), [](float x) { return std::isfinite(x); });
}

std::size_t product(std::size_t a, std::size_t b) {
    if (b && a > std::numeric_limits<std::size_t>::max() / b) {
        invalid("probe size overflow");
    }

    return a * b;
}

template <std::size_t N, bool EncodeIrradiance = false>
std::array<float, N> map_sample(
    const std::vector<std::array<float, N>>& maps,
    const ProbeGrid& grid,
    std::size_t probe,
    Float3 direction
) {
    const Float2 uv = oct_encode(direction);
    const std::size_t side = grid.map_resolution + 2;
    const float px = uv[0] * static_cast<float>(grid.map_resolution) + 0.5f;
    const float py = uv[1] * static_cast<float>(grid.map_resolution) + 0.5f;
    const auto x = static_cast<std::size_t>(px);
    const auto y = static_cast<std::size_t>(py);
    const float tx = px - static_cast<float>(x), ty = py - static_cast<float>(y);
    std::array<float, N> result{};

    for (std::size_t j = 0; j < 2; ++j) {
        for (std::size_t i = 0; i < 2; ++i) {
            const float w = (i ? tx : 1 - tx) * (j ? ty : 1 - ty);
            const auto& value = maps[probe * side * side + (y + j) * side + x + i];

            for (std::size_t c = 0; c < N; ++c) {
                if constexpr (EncodeIrradiance) {
                    result[c] += encode_probe_irradiance(value[c]) * w;
                } else {
                    result[c] += value[c] * w;
                }
            }
        }
    }

    return result;
}

void write_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
    }
}

void write_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    write_u32(out, static_cast<std::uint32_t>(v));
    write_u32(out, static_cast<std::uint32_t>(v >> 32));
}

void write_float(std::vector<std::uint8_t>& out, float v) {
    write_u32(out, std::bit_cast<std::uint32_t>(v));
}

struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t offset = 0;

    std::uint32_t u32() {
        if (bytes.size() - offset < 4) {
            invalid("truncated probe asset");
        }

        std::uint32_t v = 0;

        for (unsigned i = 0; i < 4; ++i) {
            v |= std::uint32_t{bytes[offset++]} << (8 * i);
        }

        return v;
    }

    std::uint64_t u64() {
        const auto lo = u32();
        return lo | (std::uint64_t{u32()} << 32);
    }

    float f32() { return std::bit_cast<float>(u32()); }
};

// Reads grid geometry and identity once, before packed or expanded payload validation.
ProbeGrid read_probe_grid(Reader& reader) {
    ProbeGrid grid;
    grid.bake_version = reader.u32();
    const auto size = reader.u32();
    // The portable format bounds game identity strings to 64 bytes.
    constexpr std::uint32_t kMaximumIdentityBytes = 64;

    // The identity is bounded independently of the payload size.
    if (!size || size > kMaximumIdentityBytes || size > reader.bytes.size() - reader.offset) {
        invalid("invalid probe identity");
    }

    grid.game_version
        .assign(reinterpret_cast<const char*>(reader.bytes.data() + reader.offset), size);
    reader.offset += size;
    grid.level = reader.u32();
    grid.scene_hash = reader.u64();
    grid.settings_hash = reader.u64();

    // Serialized axes retain their original read and arithmetic order.
    for (float& value : grid.origin) {
        value = reader.f32();
    }

    // The upright frame is stored as its two horizontal axis components.
    for (float& value : grid.frame.axis) {
        value = reader.f32();
    }

    // Spacing and dimensions determine the payload size validated by the caller.
    for (float& value : grid.spacing) {
        value = reader.f32();
    }

    // Read each dimension before validating the complete extent.
    for (auto& value : grid.dimensions) {
        value = reader.u32();
    }

    grid.map_resolution = reader.u32();
    grid.max_distance = reader.f32();
    grid.validate();

    return grid;
}

// Reads only the format contract; callers validate every packed texel before returning a field.
ProbeVolume read_probe_header(std::vector<std::uint8_t> bytes) {
    const profiling::Zone timing("Read probe header");
    Reader reader{bytes};

    // Files from another asset family cannot be interpreted as probes.
    if (reader.u32() != kMagic) {
        invalid("unknown probe asset format");
    }

    const auto format = reader.u32();

    // Only the current GPU plane layout is supported.
    if (format != kProbeFormatVersion) {
        invalid("unsupported probe file version; rebake with this version of the baker");
    }

    const ProbeGrid grid = read_probe_grid(reader);
    ProbeVolume volume;
    volume.grid = grid;

    // Canonical atlas dimensions must agree with the decoded grid.
    if (reader.u32() != kProbeAtlasTilesX || reader.u32() != volume.atlas_rows()) {
        invalid("unsupported probe atlas layout; rebake this level");
    }

    // R11G11B10F irradiance plus RG32F moments and RGBA16F probe metadata.
    constexpr std::size_t kMapTexelBytes = 12;
    constexpr std::size_t kMetadataBytes = 8;
    const auto expected = product(
        volume.atlas_capacity(), product(grid.texels_per_probe(), kMapTexelBytes) + kMetadataBytes
    );

    // Reject truncation and trailing bytes before accessing any packed texel.
    if (bytes.size() - reader.offset != expected) {
        invalid("probe payload size mismatch");
    }

    volume.gpu_offset = reader.offset;
    volume.gpu_bytes = std::move(bytes);

    return volume;
}
}  // namespace

Float3 ProbeFrame::to_local(Float3 p) const {
    return {axis[0] * p[0] + axis[1] * p[1], -axis[1] * p[0] + axis[0] * p[1], p[2]};
}

Float3 ProbeFrame::to_world(Float3 p) const {
    return {axis[0] * p[0] - axis[1] * p[1], axis[1] * p[0] + axis[0] * p[1], p[2]};
}

bool ProbeFrame::valid() const {
    return std::isfinite(axis[0]) && std::isfinite(axis[1])
           && std::abs(axis[0] * axis[0] + axis[1] * axis[1] - 1) < .0001f;
}

float encode_probe_irradiance(float linear) {
    return std::pow(std::max(linear, 0.0f), 1.0f / kProbeIrradianceGamma);
}

std::size_t ProbeGrid::probe_count() const {
    return product(product(dimensions[0], dimensions[1]), dimensions[2]);
}

std::size_t ProbeGrid::texels_per_probe() const {
    return product(std::size_t{map_resolution} + 2, std::size_t{map_resolution} + 2);
}

void ProbeGrid::validate() const {
    if (bake_version != kProbeBakeVersion) {
        invalid("unsupported probe bake revision; rebake with this version of the baker");
    }

    if (!frame.valid() || !finite(origin) || !finite(spacing) || !std::isfinite(max_distance)
        || max_distance <= 0
        || max_distance > std::sqrt(std::numeric_limits<float>::max()) * 0.5f) {
        invalid("invalid probe world units or distance");
    }

    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (dimensions[axis] < 2 || !(spacing[axis] > 0)
            || !std::isfinite(
                origin[axis] + spacing[axis] * static_cast<float>(dimensions[axis] - 1)
            )) {
            invalid("invalid probe grid extent");
        }
    }

    if (map_resolution < 2 || map_resolution > 256 || game_version.empty()
        || game_version.size() > 64) {
        invalid("invalid probe map resolution or game version");
    }

    // GPU atlases and index fields use 32-bit offsets. Validate before allocating.
    if (product(probe_count(), texels_per_probe()) > std::numeric_limits<std::uint32_t>::max()) {
        invalid("probe volume exceeds 32-bit map indexing");
    }
}

ProbeVolume ProbeVolume::allocate(const ProbeGrid& grid) {
    grid.validate();
    ProbeVolume out;
    out.grid = grid;
    out.probes.resize(grid.probe_count());
    const auto count = product(grid.probe_count(), grid.texels_per_probe());
    out.irradiance.resize(count);
    out.distance_moments.resize(count, {grid.max_distance, grid.max_distance * grid.max_distance});
    return out;
}

Float3 ProbeVolume::position(std::size_t probe) const {
    const auto& d = grid.dimensions;
    const std::array<std::size_t, 3> index{
        probe % d[0], (probe / d[0]) % d[1], probe / (std::size_t{d[0]} * d[1])
    };
    Float3 out{};

    for (std::size_t i = 0; i < 3; ++i) {
        out[i] = grid.spacing[i] * static_cast<float>(index[i]);
    }

    out = grid.frame.to_world(out);

    for (unsigned a = 0; a < 3; ++a) {
        out[a] += grid.origin[a] + probes.at(probe).offset[a];
    }

    return out;
}

void ProbeVolume::validate() const {
    const profiling::Zone timing("Validate probe volume");

    grid.validate();
    const auto count = product(grid.probe_count(), grid.texels_per_probe());

    if (probes.size() != grid.probe_count()
        || (gpu_bytes.empty() ? (irradiance.size() != count || distance_moments.size() != count)
                              : (!irradiance.empty() || !distance_moments.empty()))) {
        invalid("probe payload size disagrees with grid");
    }

    for (const auto& p : probes) {
        if (!finite(p.offset)) {
            invalid("nonfinite probe relocation");
        }

        for (std::size_t i = 0; i < 3; ++i) {
            if (std::abs(grid.frame.to_local(p.offset)[i]) > grid.spacing[i] * 0.45f) {
                invalid("probe relocation exceeds cell limit");
            }
        }
    }

    for (const auto& e : irradiance) {
        if (!finite(e) || *std::min_element(e.begin(), e.end()) < 0) {
            invalid("invalid probe irradiance");
        }
    }

    for (const auto& m : distance_moments) {
        if (!finite(m) || m[0] < 0 || m[0] > grid.max_distance || m[1] < 0
            || m[1] > grid.max_distance * grid.max_distance * 1.001f) {
            invalid("invalid probe distance moments");
        }
    }
}

Float2 oct_encode(Float3 direction) {
    auto n = unit(direction);
    const float scale = std::abs(n[0]) + std::abs(n[1]) + std::abs(n[2]);
    float x = n[0] / scale, y = n[1] / scale;

    if (n[2] < 0) {
        const float old = x;
        x = (1 - std::abs(y)) * sign(old);
        y = (1 - std::abs(old)) * sign(y);
    }

    return {x * 0.5f + 0.5f, y * 0.5f + 0.5f};
}

Float3 oct_decode(Float2 uv) {
    const float x = uv[0] * 2 - 1, y = uv[1] * 2 - 1;
    Float3 n{x, y, 1 - std::abs(x) - std::abs(y)};
    const float t = std::max(-n[2], 0.0f);
    n[0] -= sign(x) * t;
    n[1] -= sign(y) * t;
    return unit(n);
}

void update_probe_gutters(ProbeVolume& v) {
    const auto n = static_cast<std::size_t>(v.grid.map_resolution), side = n + 2;

    for (std::size_t p = 0; p < v.probes.size(); ++p) {
        for (std::size_t y = 0; y < side; ++y) {
            for (std::size_t x = 0; x < side; ++x) {
                if (x != 0 && y != 0 && x != side - 1 && y != side - 1) {
                    continue;
                }

                std::size_t sx = x, sy = y;

                if ((x == 0 || x == n + 1) && (y == 0 || y == n + 1)) {
                    sx = x == 0 ? n : 1;
                    sy = y == 0 ? n : 1;
                } else if (x == 0 || x == n + 1) {
                    sx = x == 0 ? 1 : n;
                    sy = n + 1 - y;
                } else {
                    sx = n + 1 - x;
                    sy = y == 0 ? 1 : n;
                }

                const auto dst = p * side * side + y * side + x,
                           src = p * side * side + sy * side + sx;
                v.irradiance[dst] = v.irradiance[src];
                v.distance_moments[dst] = v.distance_moments[src];
            }
        }
    }
}

static std::optional<ProbeSample> sample_probe_grid(
    const ProbeVolume& v,
    Float3 p,
    Float3 n,
    float bias,
    bool conservative_visibility,
    Float3 view_offset = {}
) {
    if (v.irradiance.empty()) {
        invalid("CPU probe sampling requires expanded offline maps");
    }

    if (!finite(p) || !finite(n) || !finite(view_offset) || !std::isfinite(bias) || bias < 0
        || !(dot(n, n) > 0)) {
        return {};
    }

    n = unit(n);
    std::array<std::size_t, 3> cell{};
    Float3 fraction{}, receiver = p;

    for (unsigned a = 0; a < 3; ++a) {
        receiver[a] += n[a] * bias + view_offset[a];
    }

    const auto local = v.grid.frame.to_local(subtract(p, v.grid.origin));
    const auto local_receiver = v.grid.frame.to_local(subtract(receiver, v.grid.origin));

    for (std::size_t axis = 0; axis < 3; ++axis) {
        const float coordinate = local[axis] / v.grid.spacing[axis];

        if (coordinate < 0 || coordinate > static_cast<float>(v.grid.dimensions[axis] - 1)) {
            return {};
        }

        const float biased_coordinate = std::clamp(
            local_receiver[axis] / v.grid.spacing[axis],
            0.0f,
            static_cast<float>(v.grid.dimensions[axis] - 1)
        );
        cell[axis] = std::min(
            static_cast<std::size_t>(biased_coordinate), std::size_t{v.grid.dimensions[axis] - 2}
        );
        fraction[axis] = biased_coordinate - static_cast<float>(cell[axis]);
    }

    ProbeSample out;
    float support = 0;
    float valid_support = 0;
    float facing_support = 0;

    for (unsigned corner = 0; corner < 8; ++corner) {
        std::array<std::size_t, 3> at = cell;
        float weight = 1;

        for (unsigned axis = 0; axis < 3; ++axis) {
            const bool high = (corner & (1u << axis)) != 0;
            at[axis] += high ? 1 : 0;
            weight *= high ? fraction[axis] : 1 - fraction[axis];
        }

        const auto index = at[0] + v.grid.dimensions[0] * (at[1] + v.grid.dimensions[1] * at[2]);

        if (weight <= 0) {
            continue;
        }

        const auto delta = subtract(receiver, v.position(index));
        const float distance = std::sqrt(dot(delta, delta));

        if (distance > 1e-6f) {
            const auto direction = unit(delta);
            const float wrap = std::max(0.05f, (1 - dot(direction, n)) * 0.5f);
            weight *= wrap * wrap;
        }

        support += weight;

        if (!v.probes[index].valid) {
            continue;
        }

        valid_support += weight;
        // Filtered distance moments can mix floor hits with sky. Keep probes
        // behind the receiver plane from lighting its front, with a small
        // smooth tolerance for coplanar probes and floating-point error.
        const float side = dot(subtract(v.position(index), p), n);
        float facing = std::clamp((side + 0.1f) / 0.1f, 0.0f, 1.0f);
        facing = facing * facing * (3 - 2 * facing);
        weight *= facing;
        facing_support += weight;

        if (distance > 1e-6f) {
            const auto moments = map_sample(v.distance_moments, v.grid, index, unit(delta));

            if (distance > moments[0]) {
                // Sparse variance is reduced only beyond the mean blocker distance, preserving unobstructed fill.
                constexpr float kSparseVarianceScale = .5f;
                const float variance = std::max(0.0f, moments[1] - moments[0] * moments[0])
                                       * (conservative_visibility ? kSparseVarianceScale : 1.f);
                const float difference = distance - moments[0];
                const float visibility = variance / (variance + difference * difference);
                weight *= visibility * visibility * visibility;
            }
        }

        const auto e = map_sample<3, true>(v.irradiance, v.grid, index, n);

        for (std::size_t c = 0; c < 3; ++c) {
            out.irradiance[c] += std::pow(e[c], kProbeIrradianceGamma * 0.5f) * weight;
        }

        out.weight += weight;
    }

    if (!(valid_support > 0)) {
        return {};
    }

    out.coverage = std::clamp(valid_support / support, 0.0f, 1.0f);
    out.confidence = std::clamp(out.weight / support, 0.0f, 1.0f);
    out.visibility = facing_support > 0 ? std::clamp(out.weight / facing_support, 0.f, 1.f) : 0;

    if (!(out.weight > 1e-8f)) {
        out.irradiance = {};
        return out;
    }

    // A valid local grid's occlusion is not missing data. Keep its irradiance
    // normalized normally, but smoothly darken the last 10% of visible support
    // so normalization cannot amplify a vanishing contribution into a hard edge.
    float visibility = std::clamp(out.weight / facing_support / 0.1f, 0.0f, 1.0f);
    visibility = visibility * visibility * (3 - 2 * visibility);

    for (float& c : out.irradiance) {
        const float root = c / out.weight;
        c = root * root * visibility;
    }

    return out;
}

std::optional<ProbeSample> sample_probes(
    const ProbeVolume& v, Float3 p, Float3 n, float bias, Float3 view_offset
) {
    // Sparse support can also lie across walls, especially with large cells.
    // Occluded valid probes are darkness, not missing data or full-bright GI.
    return sample_probe_grid(v, p, n, bias, true, view_offset);
}

void validate_probe_pair(const ProbeVolume& coarse, const ProbeVolume& dense) {
    coarse.validate();
    dense.validate();
    const auto& a = coarse.grid;
    const auto& b = dense.grid;

    if (a.game_version != b.game_version || a.level != b.level || a.scene_hash != b.scene_hash
        || a.map_resolution != b.map_resolution) {
        invalid("local probe identity or map layout differs from level volume");
    }

    for (unsigned axis = 0; axis < 3; ++axis) {
        if (b.spacing[axis] >= a.spacing[axis]) {
            invalid("local probe volume must be finer");
        }
    }

    for (unsigned corner = 0; corner < 8; ++corner) {
        Float3 local{};

        for (unsigned axis = 0; axis < 3; ++axis) {
            local[axis] =
                (corner & (1u << axis)) ? b.spacing[axis] * float(b.dimensions[axis] - 1) : 0;
        }

        auto point = b.frame.to_world(local);

        for (unsigned axis = 0; axis < 3; ++axis) {
            point[axis] += b.origin[axis] - a.origin[axis];
        }

        point = a.frame.to_local(point);

        for (unsigned axis = 0; axis < 3; ++axis) {
            if (point[axis] < -.001f
                || point[axis] > a.spacing[axis] * float(a.dimensions[axis] - 1) + .001f) {
                invalid("local probe volume must be contained in level volume");
            }
        }
    }
}

std::optional<ProbeSample> sample_local_probes(
    const ProbeVolume& volume, Float3 p, Float3 n, float normal_bias, Float3 view_offset
) {
    return sample_probe_grid(volume, p, n, normal_bias, false, view_offset);
}

float probe_boundary_weight(const ProbeGrid& grid, Float3 p) {
    float edge = 1;
    p = grid.frame.to_local(subtract(p, grid.origin));

    for (unsigned axis = 0; axis < 3; ++axis) {
        const float coordinate = p[axis] / grid.spacing[axis];
        edge = std::min(
            edge, std::min(coordinate, static_cast<float>(grid.dimensions[axis] - 1) - coordinate)
        );
    }

    edge = std::clamp(edge, 0.0f, 1.0f);
    return edge * edge * (3 - 2 * edge);
}

std::optional<ProbeSample> sample_probe_volumes(
    const ProbeVolume& coarse,
    const ProbeVolume* dense,
    Float3 p,
    Float3 n,
    float normal_bias,
    Float3 view_offset
) {
    const auto local =
        dense ? sample_probe_grid(*dense, p, n, normal_bias, false, view_offset) : std::nullopt;
    // Missing local probes fade to coarse; blocked local visibility does not.
    const float confidence = local ? local->coverage : 0;
    const float blend = local ? probe_boundary_weight(dense->grid, p) * confidence * confidence
                                    * (3 - 2 * confidence)
                              : 0;

    if (local && blend >= 1) {
        return local;
    }

    const auto base = sample_probes(coarse, p, n, normal_bias, view_offset);

    if (!base) {
        return local;
    }

    if (!local) {
        return base;
    }

    ProbeSample result = *base;

    for (unsigned c = 0; c < 3; ++c) {
        result.irradiance[c] = base->irradiance[c] * (1 - blend) + local->irradiance[c] * blend;
    }

    result.weight = base->weight * (1 - blend) + local->weight * blend;
    result.confidence = base->confidence * (1 - blend) + local->confidence * blend;
    result.coverage = base->coverage * (1 - blend) + local->coverage * blend;
    return result;
}

std::vector<std::uint8_t> encode_probes(const ProbeVolume& v) {
    const profiling::Zone timing("Encode probe asset");

    v.validate();
    std::vector<std::uint8_t> out;
    write_u32(out, kMagic);
    write_u32(out, kProbeFormatVersion);
    write_u32(out, v.grid.bake_version);
    write_u32(out, static_cast<std::uint32_t>(v.grid.game_version.size()));

    for (char c : v.grid.game_version) {
        out.push_back(static_cast<std::uint8_t>(c));
    }

    write_u32(out, v.grid.level);
    write_u64(out, v.grid.scene_hash);
    write_u64(out, v.grid.settings_hash);

    for (float x : v.grid.origin) {
        write_float(out, x);
    }

    for (float x : v.grid.frame.axis) {
        write_float(out, x);
    }

    for (float x : v.grid.spacing) {
        write_float(out, x);
    }

    for (auto x : v.grid.dimensions) {
        write_u32(out, x);
    }

    write_u32(out, v.grid.map_resolution);
    write_float(out, v.grid.max_distance);
    write_u32(out, kProbeAtlasTilesX);
    write_u32(out, static_cast<std::uint32_t>(v.atlas_rows()));

    if (v.gpu_bytes.empty()) {
        const auto planes = pack_probe_planes(v);
        out.insert(out.end(), planes.begin(), planes.end());
    } else {
        for (unsigned i = 0; i < 3; ++i) {
            const auto plane = v.gpu_plane(i);
            out.insert(out.end(), plane.begin(), plane.end());
        }
    }

    return out;
}

ProbeVolume decode_gpu_probes(std::vector<std::uint8_t> bytes) {
    const profiling::Zone timing("Decode GPU probe asset");
    auto volume = read_probe_header(std::move(bytes));
    read_probe_planes(volume, false);
    volume.validate();

    return volume;
}

ProbeVolume decode_probes(std::span<const std::uint8_t> bytes) {
    const profiling::Zone timing("Expand CPU probe asset");
    auto volume = read_probe_header({bytes.begin(), bytes.end()});

    // Expansion validates the same packed channels without an earlier duplicate plane scan.
    read_probe_planes(volume, true);
    volume.gpu_bytes = std::vector<std::uint8_t>{};
    volume.gpu_offset = 0;
    volume.validate();

    return volume;
}

bool probe_identity_matches(
    const ProbeVolume& v,
    const std::string& version,
    std::uint32_t level,
    std::uint64_t scene,
    std::uint64_t settings
) {
    return v.grid.game_version == version && v.grid.level == level && v.grid.scene_hash == scene
           && v.grid.settings_hash == settings;
}
}  // namespace openrac::lighting
