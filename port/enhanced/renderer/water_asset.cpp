// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <fstream>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string_view>

#include "common/json.h"
#include "common/log.h"
#include "enhanced/renderer/water_internal.h"

namespace openrac::enhanced::water_detail {
namespace {
constexpr double kTau = 2 * std::numbers::pi;

// Inverse radix-two transform of one complex positive-frequency bin.
std::array<std::complex<double>, kOceanSide> ocean_basis() {
    std::array<std::complex<double>, kOceanSide> basis{};
    // Bin one in bit-reversed order occupies the middle of the array.
    basis[kOceanSide / 2] = 1.;
    for (unsigned span = 2; span <= kOceanSide; span *= 2) {
        for (unsigned first = 0; first < kOceanSide; first += span) {
            for (unsigned j = 0; j < span / 2; ++j) {
                const auto even = basis[first + j];
                const auto odd = basis[first + j + span / 2] * std::polar(1., kTau * j / span);
                basis[first + j] = even + odd;
                basis[first + j + span / 2] = even - odd;
            }
        }
    }
    return basis;  // Unit bin amplitude uses the unnormalized inverse convention.
}

// Returns a bounded number before converting authored data to GPU floats.
float ocean_number(const json::Value& value, double low, double high) {
    const double number = value.number(std::numeric_limits<double>::quiet_NaN());
    if (!std::isfinite(number) || number < low || number > high) {
        throw std::runtime_error("invalid ocean response value");
    }
    return static_cast<float>(number);
}
}  // namespace

OceanSpectrum::OceanSpectrum(
    double depth, bool fine, double tile_override, double rms_override, double heading
)
    : tile(
          tile_override > 0. ? tile_override
          : fine             ? 17.
                             : 96.
      ) {
    // Deterministic, separate bands for broad swell and wind-driven detail.
    std::uint32_t state = fine ? 923819u : 385961u;
    const double low = fine ? 1.65 : .24;
    const double high = fine ? 9.5 : 1.05;
    const double wind_x = std::cos(heading * kTau / 360.);
    const double wind_y = std::sin(heading * kTau / 360.);
    const auto random = [&] {
        state = state * 1664525u + 1013904223u;
        return (state + .5) / 4294967296.;
    };
    double energy = 0;
    for (int y = 0; y < kOceanSide; ++y) {
        for (int x = 0; x < kOceanSide; ++x) {
            if ((x == 0 && y == 0) || x == kOceanSide / 2 || y == kOceanSide / 2) {
                continue;
            }
            const double kx = kTau * (x < kOceanSide / 2 ? x : x - kOceanSide) / tile;
            const double ky = kTau * (y < kOceanSide / 2 ? y : y - kOceanSide) / tile;
            const double k = std::hypot(kx, ky);
            const double alignment = std::max(0., (wind_x * kx + wind_y * ky) / k);
            const double directional = .14 + .86 * std::pow(alignment, 4);
            const double power =
                directional * std::exp(-std::pow(low / k, 4) - std::pow(k / high, 2)) / (k * k * k);
            const double radius = std::sqrt(-power * std::log(random()));
            const int i = y * kOceanSide + x;
            seed[i] = std::polar(radius, kTau * random());
            frequency[i] = std::sqrt(9.81 * k * std::tanh(k * depth));
            energy += radius * radius;
            slope_variance += radius * radius * k * k;
        }
    }
    // Normalize the independent cascades to different RMS heights.
    const double rms = rms_override >= 0. ? rms_override : fine ? .006 : .085;
    gain = rms / std::sqrt(2 * energy);
    slope_variance *= 2 * gain * gain;
}

void OceanSpectrum::evaluate(double clock) {
    for (int y = 0; y < kOceanSide; ++y) {
        for (int x = 0; x < kOceanSide; ++x) {
            const int i = y * kOceanSide + x;
            const int opposite =
                ((kOceanSide - y) % kOceanSide) * kOceanSide + (kOceanSide - x) % kOceanSide;
            const auto phase = std::polar(1., -frequency[i] * clock);
            field[i] = seed[i] * phase + std::conj(seed[opposite] * phase);
        }
    }
    // Transform rows and columns separately to recover a real-valued height field.
    const auto transform = [&](int start, int stride) {
        for (int i = 1, j = 0; i < kOceanSide; ++i) {
            int bit = kOceanSide / 2;
            for (; j & bit; bit /= 2) {
                j ^= bit;
            }
            j ^= bit;
            if (i < j) {
                std::swap(field[start + i * stride], field[start + j * stride]);
            }
        }
        for (int span = 2; span <= kOceanSide; span *= 2) {
            for (int j = 0; j < span / 2; ++j) {
                const auto phase = std::polar(1., kTau * j / span);
                for (int i = j; i < kOceanSide; i += span) {
                    auto& even = field[start + i * stride];
                    auto& odd = field[start + (i + span / 2) * stride];
                    const auto rotated = odd * phase;
                    odd = even - rotated;
                    even += rotated;
                }
            }
        }
    };
    for (int i = 0; i < kOceanSide; ++i) {
        transform(i * kOceanSide, 1);
    }
    for (int i = 0; i < kOceanSide; ++i) {
        transform(i, kOceanSide);
    }
    const auto height = [&](int x, int y) {
        return float(gain * field[(y % kOceanSide) * kOceanSide + x % kOceanSide].real());
    };
    const float inverse_baseline = float(kOceanSide / (2 * tile));
    for (int y = 0; y < kOceanSide; ++y) {
        for (int x = 0; x < kOceanSide; ++x) {
            pixels[y * kOceanSide + x] =
                {height(x, y),
                 (height(x + 1, y) - height(x + kOceanSide - 1, y)) * inverse_baseline,
                 (height(x, y + 1) - height(x, y + kOceanSide - 1)) * inverse_baseline,
                 0};
        }
    }
    time = clock;
}

void Water::load_ocean(const std::filesystem::path& path) {
    json::Value wave;
    std::string error;
    if (!json::parse_file(path.string(), wave, error)) {
        throw std::runtime_error("invalid Pokitaru wave response");
    }
    const auto version = wave["version"].number();
    if (version != 1 && version != 2 && version != 3) {
        throw std::runtime_error("unsupported Pokitaru wave response version");
    }
    const auto& patch = wave["patch"];
    const auto& origin = patch["origin"].items();
    const auto& banks = patch["bank"].items();
    const auto& depths = patch["depth"].items();
    const bool has_patch = !patch.is_null();
    json::Value external_profile;
    const auto profile_path = path.parent_path() / "ocean-profile.json";
    if (version == 3 && std::filesystem::exists(profile_path)
        && (!json::parse_file(profile_path.string(), external_profile, error)
            || external_profile.type() != json::Value::Type::Object)) {
        throw std::runtime_error("invalid ocean-profile.json");
    }
    const auto& profile = external_profile.is_null() ? wave["ocean"] : external_profile;
    constexpr auto kCells = kOceanSide * kOceanSide;
    if ((!has_patch && version != 3)
        || (has_patch
            && (patch["size"].number() != kOceanSide || patch["cell"].number() != .5
                || origin.size() != 2 || banks.size() != kCells || depths.size() != kCells))) {
        throw std::runtime_error("unsupported ocean response grid");
    }
    const auto x = has_patch ? ocean_number(origin[0], -100000, 100000) : 0.f;
    const auto y = has_patch ? ocean_number(origin[1], -100000, 100000) : 0.f;
    const auto z =
        ocean_number(has_patch ? patch["mean_height"] : profile["mean_height"], -510, 100000);
    const auto depth = ocean_number(wave["offshore_depth"], .1, 10000);
    m_ocean_depth = depth;
    const auto& modes = wave["modes"].items();
    const auto mode_count = version == 1 ? std::size_t{1} : modes.size();
    if ((version != 3 && mode_count == 0) || mode_count > m_ocean_modes.size()
        || (version == 3 && (mode_count > 2 || (!has_patch && mode_count != 0)))) {
        throw std::runtime_error("unsupported Pokitaru spectrum size");
    }
    if (version == 3) {
        const auto option = [&](const json::Value& value, float fallback, float lo, float hi) {
            return value.is_null() ? fallback : ocean_number(value, lo, hi);
        };
        m_ocean_horizon = option(profile["horizon"], 4096, 512, 16384);
        m_ocean_near = option(profile["near_radius"], 32, 16, 128);
        m_ocean_choppiness = option(profile["choppiness"], .85f, 0, 2);
        const auto& material = profile["material"];
        if (!material.is_null() && material.type() != json::Value::Type::Object) {
            throw std::runtime_error("invalid ocean material");
        }
        const auto read_rgb = [](const json::Value& value, auto& rgb, float max) {
            // Missing material properties retain the existing ocean appearance.
            if (value.is_null()) {
                return;
            }
            const auto& channels = value.items();
            if (channels.size() != rgb.size()) {
                throw std::runtime_error("expected three ocean material channels");
            }
            for (std::size_t i = 0; i < rgb.size(); ++i) {
                rgb[i] = ocean_number(channels[i], 0, max);
            }
        };
        read_rgb(material["colour"], m_ocean_material.colour, 1);
        read_rgb(material["absorption"], m_ocean_material.absorption, 4);
        m_ocean_material.roughness =
            option(material["roughness"], m_ocean_material.roughness, .04f, 1);
        m_ocean_material.reflectivity =
            option(material["reflectivity"], m_ocean_material.reflectivity, 0, 1);
        m_ocean_material.reflection_strength =
            option(material["reflection_strength"], m_ocean_material.reflection_strength, 0, 4);
        const float heading = option(profile["wind_heading"], 163.739795f, -360, 360);
        const auto& dry = profile["dry_rects"].items();
        if (dry.size() > m_ocean_dry_regions.size()) {
            throw std::runtime_error("too many ocean dry rectangles");
        }
        for (std::size_t i = 0; i < dry.size(); ++i) {
            const auto& bounds = dry[i].items();
            if (bounds.size() != 4) {
                throw std::runtime_error("invalid ocean dry rectangle");
            }
            auto& rect = m_ocean_dry_regions[i];
            for (std::size_t j = 0; j < rect.size(); ++j) {
                rect[j] = ocean_number(bounds[j], -100000, 100000);
            }
            if (rect[0] >= rect[2] || rect[1] >= rect[3]) {
                throw std::runtime_error("inverted ocean dry rectangle");
            }
        }
        m_ocean_dry_count = int(dry.size());
        for (std::size_t i = 0; i < m_ocean_spectra.size(); ++i) {
            const bool fine = i != 0;
            const auto& cascade = profile[fine ? "fine" : "chop"];
            const float tile = option(cascade["tile"], fine ? 17.f : 96.f, 4, 256);
            const float rms = option(cascade["rms"], fine ? .006f : .085f, 0, .25f);
            m_ocean_spectra[i] = std::make_unique<OceanSpectrum>(depth, fine, tile, rms, heading);
        }
    }
    std::array<const json::Value*, 2> corrections{};
    unsigned correction_count = 0;
    for (std::size_t i = 0; i < mode_count; ++i) {
        const auto& mode = version == 1 ? wave : modes[i];
        const auto heading = ocean_number(mode["heading"], -360, 360) * kTau / 360.;
        const auto k = ocean_number(mode["wavenumber"], .001, 3);
        const auto period = ocean_number(mode["period"], .25, 60);
        m_ocean_modes[i] =
            {float(std::cos(heading)), float(std::sin(heading)), k, float(kTau / period)};
        m_ocean_amplitudes[i / 4][i % 4] =
            version == 1 ? .09f : ocean_number(mode["amplitude"], .001, .25);
        const auto& field = mode["correction"];
        if (version == 1 || !field.is_null()) {
            if (i >= corrections.size() || field.items().size() != kCells) {
                throw std::runtime_error("unsupported Pokitaru coastal correction grid");
            }
            corrections[i] = &field;
            ++correction_count;
        }
    }
    m_ocean_mode_count = int(mode_count);
    m_ocean_grid = {x, y, .5f, float(kOceanSide)};
    m_bodies =
        {{x, y, 0, 1}, {1, 0, 0, kOceanSide}, {0, 1, 0, kOceanSide}, {0, 0, 1, 0}, {0, 0, .5f, 0}};
    m_ocean_pixels.reserve(kCells + kOceanSide);
    for (unsigned i = 0; i < kCells; ++i) {
        if (has_patch && banks[i].type() != json::Value::Type::Bool) {
            throw std::runtime_error("invalid Pokitaru coastal response cell");
        }
        const bool bank = has_patch && banks[i].boolean();
        const auto floor =
            !has_patch || depths[i].is_null() ? depth : ocean_number(depths[i], .001, 10000);
        // Coast channels supply the local solver and the existing optical shore fade.
        const int cx = int(i % kOceanSide), cy = int(i / kOceanSide);
        float distance = 2.f;
        for (int dy = -3; dy <= 3; ++dy) {
            for (int dx = -3; dx <= 3; ++dx) {
                const int px = cx + dx, py = cy + dy;
                if (has_patch && px >= 0 && py >= 0 && px < int(kOceanSide) && py < int(kOceanSide)
                    && banks[unsigned(py) * kOceanSide + unsigned(px)].boolean()) {
                    distance = std::min(distance, .5f * std::hypot(float(dx), float(dy)));
                }
            }
        }
        m_coast.push_back({(bank ? -1.f : 1.f) * (512.f + z), distance, 0, 0});
        m_hydraulics.push_back({std::clamp(floor, .25f, 1.5f), 0, 0, floor});
        Texel pixel{};
        for (std::size_t mode = 0; mode < corrections.size(); ++mode) {
            if (corrections[mode] == nullptr) {
                continue;
            }
            const auto& value = corrections[mode]->items()[i];
            if (version == 3 && value.is_null()) {
                continue;
            }
            const auto& pair = value.items();
            if (pair.size() != 2) {
                throw std::runtime_error("invalid Pokitaru coastal response cell");
            }
            pixel[2 * mode] = ocean_number(pair[0], -16, 16);
            pixel[2 * mode + 1] = ocean_number(pair[1], -16, 16);
        }
        m_ocean_pixels.push_back(pixel);
    }
    for (const auto value : ocean_basis()) {
        m_ocean_pixels.push_back({float(value.real()), float(value.imag()), 0, 0});
    }
    m_mesh = std::make_unique<WaterMesh>();
    log::info(
        "Pokitaru ocean: {} authored modes, {} coastal responses, {} FFT cascades, patch at {}, {}",
        mode_count,
        correction_count,
        m_ocean_spectra[0] ? m_ocean_spectra.size() : 0,
        x,
        y
    );
}

Water::Water(scene::LevelScene& scene, const std::filesystem::path& path) : m_scene(scene) {
    // The response is an explicit opt-in asset beside the level manifest.
    if (path.filename() == "ocean.wave") {
        load_ocean(path);
        return;
    }
    std::ifstream file(path, std::ios::binary);
    std::array<char, 8> magic{};
    unsigned version = 0, bodies = 0, cells = 0;
    std::array<char, 32> fingerprint{};
    file.read(magic.data(), 8);
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&bodies), 4);
    file.read(reinterpret_cast<char*>(&cells), 4);
    file.read(fingerprint.data(), 32);
    if (!file || std::string_view(magic.data(), 8) != "ORWATER1" || version != 3 || bodies == 0
        || bodies > 64 || cells == 0 || cells > 4 * 1024 * 1024) {
        throw std::runtime_error("invalid water bake header");
    }
    m_bodies.resize(static_cast<std::size_t>(bodies) * 5);
    m_coast.resize(cells);
    m_hydraulics.resize(cells);
    file.read(
        reinterpret_cast<char*>(m_bodies.data()), std::streamsize(m_bodies.size() * sizeof(Texel))
    );
    file.read(
        reinterpret_cast<char*>(m_coast.data()), std::streamsize(m_coast.size() * sizeof(Texel))
    );
    file.read(reinterpret_cast<char*>(m_hydraulics.data()), std::streamsize(cells * sizeof(Texel)));
    for (const auto& h : m_hydraulics) {
        if (!std::isfinite(h[0]) || h[0] < .25f || h[0] > 1.5f || !std::isfinite(h[1])
            || !std::isfinite(h[2]) || !std::isfinite(h[3]) || h[3] < 0.f || std::abs(h[1]) > 1.f
            || std::abs(h[2]) > 1.f) {
            throw std::runtime_error("invalid water hydraulics");
        }
    }
    if (!file || file.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("invalid water bake size");
    }
    for (const auto& values : {std::span<const Texel>(m_bodies), std::span<const Texel>(m_coast)}) {
        for (const auto& texel : values) {
            for (float x : texel) {
                if (!std::isfinite(x)) {
                    throw std::runtime_error("nonfinite water bake");
                }
            }
        }
    }
    std::size_t expected = 0;
    for (unsigned i = 0; i < bodies; ++i) {
        const auto* b = m_bodies.data() + static_cast<std::size_t>(i) * 5;
        const float width = b[1][3], height = b[2][3];
        if (width < 1 || height < 1 || width > 4096 || height > 4096 || std::floor(width) != width
            || std::floor(height) != height || b[3][3] != float(expected) || b[4][2] != .5f
            || b[0][3] < 1 || b[0][3] > 7 || std::floor(b[0][3]) != b[0][3]
            || std::abs(b[0][0]) > 100000 || std::abs(b[0][1]) > 100000 || b[0][2] != 0
            || b[1][0] != 1 || b[1][1] != 0 || b[1][2] != 0 || b[2][0] != 0 || b[2][1] != 1
            || b[2][2] != 0 || b[3][0] != 0 || b[3][1] != 0 || b[3][2] != 1) {
            throw std::runtime_error("invalid water grid");
        }
        const auto first = expected;
        expected += std::size_t(width) * std::size_t(height);
        if (expected > cells) {
            throw std::runtime_error("water grid outside atlas");
        }
        for (auto cell = first; cell < expected; ++cell) {
            if (m_coast[cell][3] != float(i)) {
                throw std::runtime_error("water boundary belongs to another grid");
            }
        }
    }
    if (expected != cells) {
        throw std::runtime_error("incomplete water atlas");
    }
    for (const auto& cell : m_coast) {
        if (cell[0] < -2048 || cell[0] > 2048 || cell[1] < 0 || cell[2] < 0 || cell[2] > 1
            || cell[3] < 0 || cell[3] >= float(bodies) || std::floor(cell[3]) != cell[3]) {
            throw std::runtime_error("invalid water boundary");
        }
    }
    m_mesh = std::make_unique<WaterMesh>(path, fingerprint);
    log::info("enhanced pool water: {} bodies, {} simulation cells", bodies, cells);
}
}  // namespace openrac::enhanced::water_detail
