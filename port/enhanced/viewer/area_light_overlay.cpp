// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// AreaLightOverlay: optional bake-report loading, source geometry and reversed-depth wireframe drawing.

#include "enhanced/viewer/area_light_overlay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "common/json.h"
#include "enhanced/lighting/interior_lights.h"

namespace openrac::lighting {
namespace {
// Each source contributes four frame edges, an arrow shaft and four arrowhead edges.
constexpr std::size_t kVerticesPerLight = 18;
constexpr std::uintmax_t kMaximumReportBytes = 4ull * 1024 * 1024;
constexpr float kArrowLength = 3;
constexpr float kArrowHeadLength = .7f;
constexpr float kArrowHeadWidth = .4f;
constexpr renderer::Vec3 kArrowColour{.1f, 1, 1};

// Adds a scaled world-space edge without a translation matrix.
renderer::Vec3 offset(renderer::Vec3 position, renderer::Vec3 direction, float distance) {
    // All marker construction uses the baked world axes.
    for (std::size_t channel = 0; channel < position.size(); ++channel) {
        position[channel] += direction[channel] * distance;
    }

    return position;
}

// Reads finite vectors before any position or colour reaches the GPU.
renderer::Vec3 read_vector(const json::Value& value) {
    // Missing fields must not turn into zero-length emitting axes.
    if (!value.is_array() || value.items().size() != renderer::Vec3{}.size()) {
        throw std::runtime_error("area light report requires three-component vectors");
    }

    renderer::Vec3 result{};

    // Reject magnitudes that would overflow cross products or GPU clip transforms.
    for (std::size_t channel = 0; channel < result.size(); ++channel) {
        constexpr float kMaximumCoordinate = 1'000'000;

        if (!value[channel].is_number() || !std::isfinite(value[channel].number())
            || std::abs(value[channel].number()) > kMaximumCoordinate) {
            throw std::runtime_error("invalid area light report vector");
        }

        result[channel] = static_cast<float>(value[channel].number());
    }

    return result;
}

// Appends one GL_LINES segment while retaining its diagnostic colour.
void line(
    std::vector<AreaLightVertex>& vertices,
    renderer::Vec3 start,
    renderer::Vec3 end,
    renderer::Vec3 colour
) {
    vertices.push_back({start, colour});
    vertices.push_back({end, colour});
}

// Builds a source frame at its exact baked extent and an arrow following cross(axis_u, axis_v).
void append_light(std::vector<AreaLightVertex>& vertices, const json::Value& light) {
    const auto position = read_vector(light["position"]);
    const auto axis_u = read_vector(light["axis_u"]);
    const auto axis_v = read_vector(light["axis_v"]);
    const auto radiance = read_vector(light["radiance"]);
    const auto crossed = renderer::cross(axis_u, axis_v);

    // A missing emitting hemisphere cannot be represented by a direction arrow.
    if (renderer::dot(crossed, crossed) <= 0
        || *std::min_element(radiance.begin(), radiance.end()) < 0) {
        throw std::runtime_error("degenerate area light report source");
    }

    const auto normal = renderer::normalize(crossed);
    const auto tangent = renderer::normalize(axis_u);
    const auto bitangent = renderer::normalize(axis_v);
    const float peak = *std::max_element(radiance.begin(), radiance.end());
    renderer::Vec3 colour{.3f, .3f, .3f};

    // Source hue is visible regardless of absolute emitted energy; inactive sources stay grey.
    if (peak > 0) {
        for (std::size_t channel = 0; channel < colour.size(); ++channel) {
            colour[channel] = radiance[channel] / peak;
        }
    }

    const std::array corners{
        offset(offset(position, axis_u, -1), axis_v, -1),
        offset(offset(position, axis_u, 1), axis_v, -1),
        offset(offset(position, axis_u, 1), axis_v, 1),
        offset(offset(position, axis_u, -1), axis_v, 1)
    };

    // Frame edges use source colour; arrows use cyan to distinguish their meaning.
    for (std::size_t n = 0; n < corners.size(); ++n) {
        line(vertices, corners[n], corners[(n + 1) % corners.size()], colour);
    }

    const auto tip = offset(position, normal, kArrowLength);
    const auto base = offset(tip, normal, -kArrowHeadLength);
    line(vertices, position, tip, kArrowColour);

    // Four wings keep the arrow readable when either local source axis is edge-on.
    for (const auto& axis : {tangent, bitangent}) {
        for (const float sign : {-1.f, 1.f}) {
            line(vertices, tip, offset(base, axis, sign * kArrowHeadWidth), kArrowColour);
        }
    }
}
}  // namespace

Result<std::vector<AreaLightVertex>> area_light_debug_vertices(std::string_view report) {
    // Bad optional diagnostics must not leave partial source geometry active.
    try {
        json::Value data;
        std::string error;

        if (!json::parse(report, data, error)) {
            return unexpected(Error{error});
        }

        if (data["version"].number() != 1 || !data["lights"].is_array()
            || data["lights"].items().size() > kMaximumInteriorLightsPerLevel) {
            return unexpected(Error{"invalid area light report"});
        }

        std::vector<AreaLightVertex> vertices;
        vertices.reserve(data["lights"].items().size() * kVerticesPerLight);

        // Geometry comes from the completed bake, not a second run of source placement.
        for (const auto& light : data["lights"].items()) {
            append_light(vertices, light);
        }

        return vertices;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

AreaLightOverlay::~AreaLightOverlay() {
    release();
}

Result<std::size_t> AreaLightOverlay::load(const std::filesystem::path& path) {
    release();

    // Old bakes have no debug report and retain their ordinary lighting.
    try {
        std::error_code error;

        if (!std::filesystem::exists(path, error) && !error) {
            return std::size_t{0};
        }

        const auto size = std::filesystem::file_size(path, error);

        if (error || size > kMaximumReportBytes) {
            return unexpected(Error{"cannot read bounded area light report"});
        }

        std::ifstream input(path, std::ios::binary);
        std::string text(static_cast<std::size_t>(size), '\0');

        if (!input.read(text.data(), static_cast<std::streamsize>(text.size()))) {
            return unexpected(Error{"cannot read area light report"});
        }

        auto vertices = area_light_debug_vertices(text);

        if (!vertices) {
            return unexpected(vertices.error());
        }

        const std::size_t count = vertices->size() / kVerticesPerLight;
        m_lines.set_vertices(std::move(*vertices));
        return count;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

bool AreaLightOverlay::draw(
    const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
) {
    return m_lines.draw(input, state, error);
}

void AreaLightOverlay::release() {
    m_lines.release();
}
}  // namespace openrac::lighting
