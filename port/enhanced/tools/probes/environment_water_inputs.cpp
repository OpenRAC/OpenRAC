// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Reuse the prepared collision export's explicit swimmable-surface classification.
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "assets/bytes.h"
#include "common/json.h"
#include "enhanced/tools/probes/environment_stage.h"

namespace openrac::lighting {
namespace {
// Read only the immutable wet/rest-height channel; simulation state and flow are irrelevant here.
struct WetMask {
    std::vector<std::array<float, 20>> bodies;
    std::vector<float> cells;

    bool contains(Float3 p) const {
        for (const auto& body : bodies) {
            const float step = body[18];
            const int x = int(std::floor((p[0] - body[0]) / step));
            const int y = int(std::floor((p[1] - body[1]) / step));

            if (x < 0 || y < 0 || x >= int(body[7]) || y >= int(body[11])) {
                continue;
            }

            const auto index =
                std::size_t(body[15]) + std::size_t(y) * std::size_t(body[7]) + std::size_t(x);
            const float cell = cells[index];

            if (cell > 0 && std::abs((cell - 512.f) - p[2]) < 1) {
                return true;
            }
        }

        return false;
    }
};

// Bounds-check the immutable body grid before any point query converts its coordinates to an index.
std::array<float, 20> read_body(const assets::ByteView& data, unsigned i, unsigned cells) {
    std::array<float, 20> body{};

    for (unsigned j = 0; j < body.size(); ++j) {
        body[j] = data.f32_at(52 + std::size_t(i) * 80 + j * 4);

        if (!std::isfinite(body[j])) {
            throw std::runtime_error("nonfinite gameplay water grid");
        }
    }

    if (std::abs(body[0]) > 100000 || std::abs(body[1]) > 100000 || body[7] < 1 || body[11] < 1
        || body[7] > 4096 || body[11] > 4096 || std::floor(body[7]) != body[7]
        || std::floor(body[11]) != body[11] || body[18] != .5f || body[15] < 0
        || std::floor(body[15]) != body[15]
        || double(body[15]) + double(body[7]) * double(body[11]) > cells) {
        throw std::runtime_error("invalid gameplay water grid bounds");
    }

    return body;
}

// An existing bake refines the authored collision coastline; levels without one need no water tool dependency.
std::function<bool(Float3)> read_wet_mask(const std::filesystem::path& level) {
    const auto path = level / "water.bin";

    if (level.empty() || !std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file || file.tellg() < 56 || file.tellg() > 160 * 1024 * 1024) {
        throw std::runtime_error("invalid gameplay water mask size");
    }

    std::vector<std::uint8_t> bytes(std::size_t(file.tellg()));
    file.seekg(0);

    if (!file.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(bytes.size()))) {
        throw std::runtime_error("incomplete gameplay water mask");
    }

    const assets::ByteView data(bytes);
    const auto bodies = data.u32_at(12), cells = data.u32_at(16);

    if (std::string_view(reinterpret_cast<const char*>(bytes.data()), 8) != "ORWATER1"
        || data.u32_at(8) != 3 || bodies == 0 || bodies > 64 || cells == 0
        || cells > 4 * 1024 * 1024
        || bytes.size() != 52 + std::size_t(bodies) * 80 + std::size_t(cells) * 32) {
        throw std::runtime_error("invalid gameplay water mask header");
    }

    WetMask mask;

    for (unsigned i = 0; i < bodies; ++i) {
        mask.bodies.push_back(read_body(data, i, cells));
    }

    for (unsigned i = 0; i < cells; ++i) {
        const float wet = data.f32_at(52 + std::size_t(bodies) * 80 + std::size_t(i) * 16);

        if (!std::isfinite(wet)) {
            throw std::runtime_error("nonfinite gameplay water cell");
        }

        mask.cells.push_back(wet);
    }

    return [mask = std::move(mask)](Float3 p) {
        return mask.contains(p);
    };
}
}  // namespace

void append_exported_water(
    const std::filesystem::path& output,
    std::vector<EnvironmentFloorSample>& floors,
    const std::filesystem::path& level,
    const BakeTraversalSettings& traversal
) {
    const auto path = output / "interior-input.json";

    if (!std::filesystem::exists(path)) {
        return;
    }

    json::Value document;
    std::string error;

    if (!json::parse_file(path.string(), document, error) || document["format"].number() != 1
        || !document["triangles"].is_array() || document["triangles"].items().size() > 2000000) {
        throw std::runtime_error("invalid prepared gameplay collision");
    }

    std::vector<GameplayTriangle> triangles;

    for (const auto& row : document["triangles"].items()) {
        if (!row.is_array() || row.items().size() != 10) {
            throw std::runtime_error("invalid prepared gameplay triangle");
        }

        GameplayTriangle triangle;

        for (unsigned i = 0; i < 9; ++i) {
            const double value = row[i].number();

            if (!row[i].is_number() || !std::isfinite(value) || std::abs(value) > 1000000) {
                throw std::runtime_error("invalid gameplay collision coordinate");
            }

            triangle.vertices[i / 3][i % 3] = float(value);
        }

        if (!row[9].is_number() || (row[9].number() != 0 && row[9].number() != 1)) {
            throw std::runtime_error("invalid gameplay support classification");
        }

        // prepare_interiors.py marks only authored collision surface zero as non-solid water.
        triangle.swimmable = row[9].number() == 0;
        triangles.push_back(triangle);
    }

    append_swimming_surfaces(
        triangles, floors, 2, read_wet_mask(level), traversal.minimum_swim_depth
    );
}
}  // namespace openrac::lighting
