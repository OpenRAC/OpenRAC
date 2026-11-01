// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Move, rotate and range handles for the optional isolated light authoring tools.
#include <algorithm>
#include <cmath>

#include "debug/overlay.h"

#ifdef OPENRAC_LIGHT_AUTHORING
namespace openrac::frontend::debug_overlay {
namespace {
constexpr std::array<ImU32, 3> kAxes{
    IM_COL32(240, 85, 85, 255), IM_COL32(85, 225, 120, 255), IM_COL32(90, 145, 255, 255)
};

// Distance to the exact visible segment; endpoint handling remains stable on short or foreshortened axes.
float distance(ImVec2 p, ImVec2 a, ImVec2 b) {
    const float x = b.x - a.x, y = b.y - a.y, denominator = x * x + y * y;
    const float t = denominator > 1
                        ? std::clamp(((p.x - a.x) * x + (p.y - a.y) * y) / denominator, 0.f, 1.f)
                        : 0;
    const float dx = p.x - a.x - x * t, dy = p.y - a.y - y * t;
    return std::sqrt(dx * dx + dy * dy);
}

// Studio-space attachment plus selected axes; translations convert back through the full inverse frame.
struct Shape {
    renderer::Mat4 frame;
    renderer::Vec3 position{}, direction{};
    std::array<renderer::Vec3, 3> axes{};
    float length = 1, range = 1;
};

std::optional<Shape> shape(const renderer::Camera& camera, const enhanced::AttachedLight& light) {
    const auto& p = s.object_viewer;
    const auto& mesh = p.renderer.mesh();
    const int joint = enhanced::attached_light_joint(light, mesh.bones);

    if (joint == -2) {
        return std::nullopt;
    }

    const auto frame = scene::preview_attachment(mesh, joint);

    if (!frame) {
        return std::nullopt;
    }

    Shape out;
    out.frame = *frame;
    out.position = renderer::transform_point(*frame, light.light.position);
    out.direction = renderer::normalize(renderer::transform_vector(*frame, light.light.direction));
    out.length = std::max(.15f, p.orbit.distance * .16f);
    out.range = light.light.range / mesh.radius;

    for (unsigned c = 0; c < 3; ++c) {
        renderer::Vec3 axis{};
        axis[c] = 1;
        out.axes[c] =
            p.local_axes ? renderer::normalize(renderer::transform_vector(*frame, axis)) : axis;
    }

    (void)camera;
    return out;
}

renderer::Vec3 offset(renderer::Vec3 p, const renderer::Vec3& v, float scale) {
    for (unsigned c = 0; c < 3; ++c) {
        p[c] += v[c] * scale;
    }

    return p;
}

// A short segment is shared by drawing and picking, keeping the visual and interactive handles identical.
struct Segment {
    renderer::Vec3 a{}, b{};
    int axis = 0;
};

struct Segments {
    std::array<Segment, 96> lines{};
    unsigned count = 0;
};

Segments segments(const Shape& item, const renderer::Camera& camera) {
    const auto& p = s.object_viewer;
    Segments out;

    if (p.gizmo_mode == 2) {
        const renderer::Vec3 right{camera.view[0], camera.view[4], camera.view[8]};
        out.lines[0] = {item.position, offset(item.position, right, item.range), 0};
        out.count = 1;
        return out;
    }

    for (unsigned axis = 0; axis < 3; ++axis) {
        if (p.gizmo_mode == 0) {
            out.lines[out.count++] =
                {item.position, offset(item.position, item.axes[axis], item.length), int(axis)};
            continue;
        }

        constexpr unsigned kRingSegments = 32;

        for (unsigned i = 0; i < kRingSegments; ++i) {
            const float a = float(i) * 6.2831853f / float(kRingSegments),
                        b = float(i + 1) * 6.2831853f / float(kRingSegments);
            const auto u = item.axes[(axis + 1) % 3], v = item.axes[(axis + 2) % 3];
            out.lines[out.count++] =
                {offset(
                     offset(item.position, u, std::cos(a) * item.length),
                     v,
                     std::sin(a) * item.length
                 ),
                 offset(
                     offset(item.position, u, std::cos(b) * item.length),
                     v,
                     std::sin(b) * item.length
                 ),
                 int(axis)};
        }
    }

    return out;
}

// Selected handles take precedence over light icons; Alt gestures always belong to the scene camera.
int pick_axis(const Shape& item, const renderer::Camera& camera) {
    const auto lines = segments(item, camera);
    float nearest = 7;
    int axis = -1;
    const auto mouse = ImGui::GetIO().MousePos;

    for (unsigned i = 0; i < lines.count; ++i) {
        const auto a = object_viewer_project(camera, lines.lines[i].a),
                   b = object_viewer_project(camera, lines.lines[i].b);

        if (!a || !b) {
            continue;
        }

        const float d = distance(mouse, *a, *b);

        if (d < nearest) {
            nearest = d;
            axis = lines.lines[i].axis;
        }
    }

    const auto center = object_viewer_project(camera, item.position);

    if (s.object_viewer.gizmo_mode == 0 && center && distance(mouse, *center, *center) < 7) {
        return 3;
    }

    return axis;
}

// Dragging updates authored local data, rather than the original object's transform or its bone pose.
// Rotation rings operate on the spot axis in the chosen parent frame.
void rotate_light(
    const Shape& item, const renderer::Camera& camera, enhanced::AttachedLight& light
) {
    auto& p = s.object_viewer;
    const auto& io = ImGui::GetIO();
    const auto origin = object_viewer_project(camera, item.position);
    const auto inverse = renderer::inverse_affine(item.frame);

    if (!origin || !inverse) {
        return;
    }

    const float angle = std::atan2(io.MousePos.y - origin->y, io.MousePos.x - origin->x);
    float delta = std::remainder(angle - p.rotate_angle, 6.2831853f);
    p.rotate_angle = angle;
    auto axis = item.axes[std::size_t(std::clamp(p.drag_axis, 0, 2))];
    const renderer::Vec3 forward{-camera.view[2], -camera.view[6], -camera.view[10]};

    if (renderer::dot(axis, forward) > 0) {
        delta = -delta;
    }

    const auto cross = renderer::cross(axis, item.direction);
    renderer::Vec3 rotated{};

    for (unsigned c = 0; c < 3; ++c) {
        rotated[c] = item.direction[c] * std::cos(delta) + cross[c] * std::sin(delta)
                     + axis[c] * renderer::dot(axis, item.direction) * (1 - std::cos(delta));
    }

    light.light.direction = renderer::normalize(renderer::transform_vector(*inverse, rotated));
    object_lights_changed();
}

// Linear handles convert screen motion back into the authored parent frame.
void drag_light(const Shape& item, const renderer::Camera& camera, enhanced::AttachedLight& light) {
    auto& p = s.object_viewer;
    const auto& io = ImGui::GetIO();

    if (p.gizmo_mode == 1) {
        rotate_light(item, camera, light);
        return;
    }

    const auto origin = object_viewer_project(camera, item.position);

    if (!origin) {
        return;
    }

    const auto inverse = renderer::inverse_affine(item.frame);

    if (!inverse) {
        return;
    }

    const renderer::Vec3 right{camera.view[0], camera.view[4], camera.view[8]},
        up{camera.view[1], camera.view[5], camera.view[9]};

    const auto direction =
        p.gizmo_mode == 2 || p.drag_axis == 3 ? right : item.axes[std::size_t(p.drag_axis)];
    const auto end = object_viewer_project(camera, offset(item.position, direction, 1));

    if (!end) {
        return;
    }

    const float x = end->x - origin->x, y = end->y - origin->y, pixels = x * x + y * y;

    if (pixels < 4) {
        return;
    }

    const float scalar = (io.MouseDelta.x * x + io.MouseDelta.y * y) / pixels;

    if (p.gizmo_mode == 2) {
        light.light.range =
            std::clamp(light.light.range + scalar * p.renderer.mesh().radius, .01f, 10000.f);
    } else {
        auto movement = offset(renderer::Vec3{}, direction, scalar);

        if (p.drag_axis == 3) {
            movement = offset(movement, up, -io.MouseDelta.y / std::sqrt(pixels));
        }

        movement = renderer::transform_vector(*inverse, movement);

        for (unsigned c = 0; c < 3; ++c) {
            light.light.position[c] += movement[c];
        }
    }

    object_lights_changed();
}

// Visible cone/range wires have no bearing on shading or the world object's mesh.
void draw_range(ImDrawList& draw, const Shape& item, const renderer::Camera& camera, bool spot) {
    const renderer::Vec3 seed =
        std::abs(item.direction[2]) < .9f ? renderer::Vec3{0, 0, 1} : renderer::Vec3{0, 1, 0};
    const auto u = renderer::normalize(renderer::cross(item.direction, seed)),
               v = renderer::cross(item.direction, u);
    float radius = item.range;
    auto center = item.position;

    if (spot) {
        const auto& light =
            s.object_viewer.lights[std::size_t(s.object_viewer.selected_light)].light;
        center = offset(center, item.direction, item.range * std::cos(light.outer_angle));
        radius *= std::sin(light.outer_angle);
    }

    constexpr unsigned kSteps = 32;

    for (unsigned i = 0; i < kSteps; ++i) {
        const float a = float(i) * 6.2831853f / float(kSteps),
                    b = float(i + 1) * 6.2831853f / float(kSteps);
        const auto first = offset(offset(center, u, std::cos(a) * radius), v, std::sin(a) * radius);
        const auto second =
            offset(offset(center, u, std::cos(b) * radius), v, std::sin(b) * radius);
        const auto p = object_viewer_project(camera, first),
                   q = object_viewer_project(camera, second);

        if (p && q) {
            draw.AddLine(*p, *q, IM_COL32(250, 215, 90, 95));
        }

        if (spot && i % 8 == 0) {
            const auto origin = object_viewer_project(camera, item.position);

            if (p && origin) {
                draw.AddLine(*origin, *p, IM_COL32(250, 215, 90, 95));
            }
        }
    }
}

// Pointer interaction happens before camera movement; drawing uses the resulting final camera.
bool interact_lights(const renderer::Camera& camera) {
    auto& p = s.object_viewer;
    const auto& io = ImGui::GetIO();

    if (io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        p.drag_axis = -1;
        return false;
    }

    const bool valid = p.selected_light >= 0 && std::size_t(p.selected_light) < p.lights.size();
    const auto selected =
        valid ? shape(camera, p.lights[std::size_t(p.selected_light)]) : std::nullopt;

    if (selected && p.drag_axis >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        drag_light(*selected, camera, p.lights[std::size_t(p.selected_light)]);
        return true;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        p.drag_axis = -1;
    }

    if (!ImGui::IsItemHovered() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return false;
    }

    if (selected) {
        p.drag_axis = pick_axis(*selected, camera);
        const auto at = object_viewer_project(camera, selected->position);

        if (at) {
            p.rotate_angle = std::atan2(io.MousePos.y - at->y, io.MousePos.x - at->x);
        }

        if (p.drag_axis >= 0) {
            return true;
        }
    }

    for (std::size_t i = 0; i < p.lights.size(); ++i) {
        const auto item = shape(camera, p.lights[i]);

        if (!item) {
            continue;
        }

        const auto at = object_viewer_project(camera, item->position);

        if (!at) {
            continue;
        }

        if (distance(io.MousePos, *at, *at) < 9) {
            p.selected_light = int(i);
            p.drag_axis = -1;
            return true;
        }
    }

    return false;
}

// Selected axes and influence wires use exactly the same segments as hit testing.
void draw_selected_light(const Shape& item, const renderer::Camera& camera) {
    auto* draw = ImGui::GetWindowDrawList();
    const auto lines = segments(item, camera);

    for (unsigned n = 0; n < lines.count; ++n) {
        const auto a = object_viewer_project(camera, lines.lines[n].a),
                   b = object_viewer_project(camera, lines.lines[n].b);

        if (a && b) {
            draw->AddLine(*a, *b, kAxes[std::size_t(lines.lines[n].axis)], 2);
        }
    }

    draw_range(
        *draw,
        item,
        camera,
        s.object_viewer.lights[std::size_t(s.object_viewer.selected_light)].light.spot
    );
}

// Every light stays selectable; selected light wires add detail without obscuring the whole scene.
void draw_lights(const renderer::Camera& camera) {
    auto& p = s.object_viewer;
    auto* draw = ImGui::GetWindowDrawList();

    for (std::size_t i = 0; i < p.lights.size(); ++i) {
        const auto item = shape(camera, p.lights[i]);

        if (!item) {
            continue;
        }

        const auto at = object_viewer_project(camera, item->position);

        if (!at) {
            continue;
        }

        const auto colour =
            p.selected_light == int(i) ? IM_COL32(255, 215, 80, 255) : IM_COL32(120, 215, 250, 220);
        draw->AddCircle(*at, 6, colour, 16, 2);

        if (p.selected_light == int(i)) {
            draw_selected_light(*item, camera);
        }
    }
}
}  // namespace

// Light handles consume pointer input before the general scene camera.
bool object_light_gizmos(const renderer::Camera& camera, bool interact) {
    auto& p = s.object_viewer;
    if (!p.lighting || !p.light_gizmos) {
        return false;
    }
    if (interact) {
        return interact_lights(camera);
    }
    draw_lights(camera);
    return false;
}
}  // namespace openrac::frontend::debug_overlay
#endif
