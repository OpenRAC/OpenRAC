// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <cmath>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string_view>

#include "common/log.h"
#include "enhanced/renderer/water_internal.h"

namespace openrac::enhanced::water_detail {

Water::Water(scene::LevelScene& scene, const std::filesystem::path& path) : m_scene(scene) {
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
    log::info("enhanced Novalis water: {} bodies, {} simulation cells", bodies, cells);
}
}  // namespace openrac::enhanced::water_detail
