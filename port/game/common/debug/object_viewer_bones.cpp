// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// General skeleton overlay and studio projection, independent of enhanced light authoring.
#include <cmath>

#include "debug/overlay.h"

namespace openrac::frontend::debug_overlay {
// Project normalized studio coordinates into this image; objects behind the eye have no handle.
std::optional<ImVec2> object_viewer_project(const renderer::Camera& camera, renderer::Vec3 p) {
    const auto& view = s.object_viewer;
    const auto clip = renderer::multiply(camera.projection, camera.view);
    const float w = clip[3] * p[0] + clip[7] * p[1] + clip[11] * p[2] + clip[15];

    if (w <= .001f) {
        return std::nullopt;
    }

    const auto h = renderer::transform_point(clip, p);
    return ImVec2{
        view.image_low.x + (h[0] / w * .5f + .5f) * view.image_size.x,
        view.image_low.y + (.5f - h[1] / w * .5f) * view.image_size.y
    };
}

// Skeleton glyphs are a general inspection aid, independent of enhanced graphics.
void object_viewer_bones(const renderer::Camera& camera) {
    const auto& p = s.object_viewer;

    if (!p.show_bones) {
        return;
    }

    const auto& mesh = p.renderer.mesh();
    auto* draw = ImGui::GetWindowDrawList();

    for (std::size_t j = 0; j < mesh.bones.size(); ++j) {
        const auto frame = scene::preview_attachment(mesh, int(j));

        if (!frame) {
            continue;
        }

        const auto point =
            object_viewer_project(camera, renderer::transform_point(*frame, {0, 0, 0}));

        if (!point) {
            continue;
        }

        draw->AddCircleFilled(*point, 3, IM_COL32(230, 230, 160, 230));
        const int parent = mesh.bones[j].parent;

        if (parent >= 0) {
            const auto bone = scene::preview_attachment(mesh, parent);
            const auto at =
                bone ? object_viewer_project(camera, renderer::transform_point(*bone, {0, 0, 0}))
                     : std::nullopt;

            if (at) {
                draw->AddLine(*point, *at, IM_COL32(220, 220, 150, 160));
            }
        }

        const auto mouse = ImGui::GetIO().MousePos;
        const float dx = mouse.x - point->x, dy = mouse.y - point->y;
        if (std::sqrt(dx * dx + dy * dy) < 6) {
            ImGui::SetTooltip("%s", mesh.bones[j].name.c_str());
        }
    }
}
}  // namespace openrac::frontend::debug_overlay
