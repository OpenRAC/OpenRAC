// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/viewer/medial_axis_overlay.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "common/json.h"

namespace openrac::lighting {
namespace {
// Orange keeps floor connectivity distinct from reflection coverage and room markers.
constexpr renderer::Vec3 kAxisColour{1, .45f, .05f};
// World-space dash lengths remain stable as the camera moves closer to the graph.
constexpr double kDashLength = .45;
constexpr double kDashPeriod = .75;
// Baked jump arcs are subdivided; reject malformed segments before expanding them into dashes.
constexpr float kMaximumJumpSegmentLength = 128;

// Returns an endpoint on the original line, keeping dashed geometry on the baked trajectory.
renderer::Vec3 line_point(renderer::Vec3 start, renderer::Vec3 delta, float fraction) {
    return {
        start[0] + delta[0] * fraction,
        start[1] + delta[1] * fraction,
        start[2] + delta[2] * fraction
    };
}

// Splits a checked jump arc segment into visible dashes without changing its trajectory.
void jump_segment(
    std::vector<DebugLineVertex>& vertices, renderer::Vec3 start, renderer::Vec3 end, double& phase
) {
    const renderer::Vec3 delta{end[0] - start[0], end[1] - start[1], end[2] - start[2]};
    const double length = std::sqrt(renderer::dot(delta, delta));

    // Repeated endpoints have no visible segment and cannot define a direction.
    if (length <= 0) {
        return;
    }

    for (double distance = -phase; distance < length; distance += kDashPeriod) {
        const double first = std::max(0., distance);
        const double last = std::min(length, distance + kDashLength);

        // Gaps are omitted geometry, so hidden-line rendering preserves the dashed pattern too.
        if (last > first) {
            vertices.push_back(
                {line_point(start, delta, static_cast<float>(first / length)), kAxisColour}
            );
            vertices.push_back(
                {line_point(start, delta, static_cast<float>(last / length)), kAxisColour}
            );
        }
    }

    phase = std::fmod(phase + length, kDashPeriod);
}
}  // namespace

Result<std::vector<DebugLineVertex>> medial_axis_debug_vertices(std::string_view report) {
    json::Value document;
    std::string error;

    if (!json::parse(report, document, error) || document["version"].number() != 1) {
        return unexpected(Error{"invalid medial axis report"});
    }

    std::vector<DebugLineVertex> vertices;
    double phase = 0;

    for (const char* name : {"paths", "jump_paths", "junctions", "centres"}) {
        const auto& rows = document[name];
        const bool jumps = std::string_view(name) == "jump_paths";
        const unsigned width = jumps || std::string_view(name) == "paths" ? 6 : 3;

        // Reports baked before typed jump paths contain only solid floor paths.
        if (jumps && rows.is_null()) {
            continue;
        }

        if (!rows.is_array() || rows.items().size() > 100000) {
            return unexpected(Error{"invalid medial axis array"});
        }

        for (const auto& row : rows.items()) {
            if (!row.is_array() || row.items().size() != width) {
                return unexpected(Error{"invalid medial axis point"});
            }

            for (const auto& v : row.items()) {
                if (!v.is_number() || !std::isfinite(v.number()) || std::abs(v.number()) > 1e7) {
                    return unexpected(Error{"invalid medial axis coordinate"});
                }
            }

            renderer::Vec3 p{
                float(row[0].number()), float(row[1].number()), float(row[2].number())
            };

            if (width == 6) {
                const renderer::Vec3 end{
                    float(row[3].number()), float(row[4].number()), float(row[5].number())
                };

                // Airborne traversal is dashed; the supported medial axis remains continuous.
                if (jumps) {
                    const renderer::Vec3 delta{end[0] - p[0], end[1] - p[1], end[2] - p[2]};

                    // Report coordinates can be valid while a corrupt link spans the entire level.
                    if (renderer::dot(delta, delta)
                        > kMaximumJumpSegmentLength * kMaximumJumpSegmentLength) {
                        return unexpected(Error{"medial jump segment too long"});
                    }

                    jump_segment(vertices, p, end, phase);
                } else {
                    vertices.push_back({p, kAxisColour});
                    vertices.push_back({end, kAxisColour});
                }
            } else {
                const renderer::Vec3 colour = std::string_view(name) == "junctions"
                                                  ? renderer::Vec3{1, .2f, .8f}
                                                  : renderer::Vec3{1, 1, 1};

                for (unsigned axis = 0; axis < 3; ++axis) {
                    for (float sign : {-1.f, 1.f}) {
                        auto q = p;
                        q[axis] += .4f * sign;
                        vertices.push_back({q, colour});
                    }
                }
            }
        }
    }

    return vertices;
}

Result<std::size_t> MedialAxisOverlay::load(const std::filesystem::path& path) {
    release();

    try {
        if (!std::filesystem::exists(path)) {
            return std::size_t{0};
        }

        const auto bytes = std::filesystem::file_size(path);

        if (bytes > 16 * 1024 * 1024) {
            return unexpected(Error{"medial axis report too large"});
        }

        std::ifstream file(path, std::ios::binary);
        std::string text(static_cast<std::size_t>(bytes), '\0');

        if (!file.read(text.data(), static_cast<std::streamsize>(bytes))) {
            return unexpected(Error{"cannot read medial axis report"});
        }

        auto vertices = medial_axis_debug_vertices(text);

        if (!vertices) {
            return unexpected(vertices.error());
        }

        const auto count = vertices->size() / 2;
        m_lines.set_vertices(std::move(*vertices));
        return count;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

bool MedialAxisOverlay::draw(
    const renderer::FrameInput& input, renderer::RenderState& state, std::string& error
) {
    return m_lines.draw(input, state, error);
}

void MedialAxisOverlay::release() {
    m_lines.release();
}
}  // namespace openrac::lighting
