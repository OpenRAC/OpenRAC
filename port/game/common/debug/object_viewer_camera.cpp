// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Unity-style scene controls scoped to the preview image, never the gameplay camera.
#include <algorithm>
#include <cmath>

#include "debug/overlay.h"
#include "renderer/gl.h"

namespace openrac::frontend::debug_overlay {
namespace {
// Alt+left orbits, middle pans, Alt+right dollies; right-drag enables free look and fly keys.
void camera_controls() {
    auto& p = s.object_viewer;
    const auto& io = ImGui::GetIO();

    if (!ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
        return;
    }

    const bool right = ImGui::IsMouseDown(ImGuiMouseButton_Right),
               middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (io.KeyAlt && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        p.orbit.navigate(io.MouseDelta.x, io.MouseDelta.y, false);
    } else if (right && io.KeyAlt) {
        p.orbit.move(0, 0, (io.MouseDelta.x - io.MouseDelta.y) * .1f);
    } else if (right) {
        p.orbit.navigate(io.MouseDelta.x, io.MouseDelta.y, true);
        renderer::Vec3 move{
            float(ImGui::IsKeyDown(ImGuiKey_D)) - float(ImGui::IsKeyDown(ImGuiKey_A)),
            float(ImGui::IsKeyDown(ImGuiKey_E)) - float(ImGui::IsKeyDown(ImGuiKey_Q)),
            float(ImGui::IsKeyDown(ImGuiKey_S)) - float(ImGui::IsKeyDown(ImGuiKey_W))
        };
        const float factor = p.orbit.fly_speed * io.DeltaTime * (io.KeyShift ? 4.f : 1.f);
        move = renderer::normalize(move);

        for (float& c : move) {
            c *= factor;
        }

        p.orbit.travel(move);
        p.orbit.fly_speed =
            std::clamp(p.orbit.fly_speed * std::exp(io.MouseWheel * .2f), .05f, 100.f);
    } else if (middle) {
        const float scale = p.orbit.distance / std::max(p.image_size.y, 64.f);
        p.orbit.travel({-io.MouseDelta.x * scale, io.MouseDelta.y * scale, 0});
    }

    if (!right && io.MouseWheel != 0) {
        p.orbit.move(0, 0, io.MouseWheel);
    }

    if (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F)) {
        p.orbit = {};
    }

#ifdef OPENRAC_LIGHT_AUTHORING
    if (!right && !io.WantTextInput && p.lighting) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) {
            p.gizmo_mode = 0;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_E)) {
            p.gizmo_mode = 1;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            p.gizmo_mode = 2;
        }
    }
#endif
}
}  // namespace

void object_viewer_picture(const Shown& shown) {
    auto& p = s.object_viewer;
    const auto& io = ImGui::GetIO();
    p.image_size.x = std::max(64.f, ImGui::GetContentRegionAvail().x);
    p.image_size.y = std::max(140.f, std::min(600.f, ImGui::GetContentRegionAvail().y - dp(55)));
    p.image_low = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(
        "Object scene view",
        p.image_size,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight
            | ImGuiButtonFlags_MouseButtonMiddle
    );
    const float aspect = p.image_size.x / p.image_size.y;
    bool manipulated = false;
#ifdef OPENRAC_LIGHT_AUTHORING
    manipulated = object_light_gizmos(p.orbit.camera(aspect), true);
#endif

    if (!manipulated) {
        camera_controls();
    }

#ifdef OPENRAC_LIGHT_AUTHORING
    update_object_preview_lights();
#endif
    const auto result = p.renderer.draw(
        int(p.image_size.x * io.DisplayFramebufferScale.x),
        int(p.image_size.y * io.DisplayFramebufferScale.y),
        p.orbit
    );
    p.draw_error = result ? std::string{} : result.error().message;
    const ImVec2 high{p.image_low.x + p.image_size.x, p.image_low.y + p.image_size.y};
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddImage(ImTextureID(p.renderer.texture()), p.image_low, high, {0, 1}, {1, 0});
    draw->PushClipRect(p.image_low, high, true);
    object_viewer_bones(p.orbit.camera(aspect));
#ifdef OPENRAC_LIGHT_AUTHORING
    object_light_gizmos(p.orbit.camera(aspect), false);
#endif
    draw->PopClipRect();
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
    gl::glViewport(0, 0, shown.window_width, shown.window_height);
    ImGui::TextWrapped(
        "Alt+LMB orbit | MMB pan | RMB look + WASD/QE fly | Shift faster | Wheel zoom | F frame"
    );
}
}  // namespace openrac::frontend::debug_overlay
