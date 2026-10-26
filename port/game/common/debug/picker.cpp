// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

#include "debug/overlay.h"
#include "scene/object_picker.h"

namespace openrac::frontend::debug_overlay {
namespace {
std::atomic<bool> enabled{false};
// The JSON snapshot is local diagnostics, never game assets or a network service.
const char* report_path = "moby-picker.json";

void write_report(bool active) {
    const auto& hit = s.picker_result.hit;
    auto report = scene::picker_report_json(
        s.picker_result,
        active,
        s.game,
        hit && hit->class_id >= 0 ? class_name(hit->class_id) : std::string{}
    );
    std::ofstream file(report_path, std::ios::trunc);
    file << report;
    file.flush();
    s.picker_error = file ? std::string{} : "Could not write moby-picker.json";
}

struct Picture {
    renderer::Mat4 matrix;
    ImVec2 low, high;

    bool project(renderer::Vec3 p, ImVec2& at) const {
        float x = matrix[0] * p[0] + matrix[4] * p[1] + matrix[8] * p[2] + matrix[12];
        float y = matrix[1] * p[0] + matrix[5] * p[1] + matrix[9] * p[2] + matrix[13];
        float w = matrix[3] * p[0] + matrix[7] * p[1] + matrix[11] * p[2] + matrix[15];
        if (w <= .001f) {
            return false;
        }
        at =
            {low.x + (x / w * .5f + .5f) * (high.x - low.x),
             low.y + (.5f - y / w * .5f) * (high.y - low.y)};
        return std::isfinite(at.x) && std::isfinite(at.y);
    }
};
}  // namespace

bool picker_enabled() {
    return enabled.load();
}

void set_picker_enabled(bool value) {
    const bool old = enabled.exchange(value);
    if (old != value) {
        s.picker_result = {};
        s.picker_export_at = -1;
        write_report(value);
    }
}

void picker_overlay(const Shown& shown) {
    if (!picker_enabled()) {
        return;
    }
    const bool ready = shown.camera && shown.picker && shown.picker_level
                       && shown.picker_level_number >= 0 && shown.box_width > 0
                       && shown.box_height > 0;
    if (!ready) {
        if (s.picker_result.hit) {
            s.picker_result = {};
            write_report(true);
        }
        return;
    }
    const auto& io = ImGui::GetIO();
    const float scale = io.DisplayFramebufferScale.x > 0 ? io.DisplayFramebufferScale.x : 1;
    Picture picture{
        renderer::multiply(shown.camera->projection, shown.camera->view),
        {float(shown.box_x) / scale,
         float(shown.window_height - shown.box_y - shown.box_height) / scale},
        {float(shown.box_x + shown.box_width) / scale,
         float(shown.window_height - shown.box_y) / scale}
    };
    bool changed = false;
    if (s.picker_result.level != shown.picker_level_number) {
        s.picker_result = {true, shown.picker_level_number, shown.picker_tick, {}};
        changed = true;
    }
    if (s.picker_result.hit
        && !shown.picker->refresh(
            *shown.picker_level,
            shown.picker_live,
            shown.picker_palette,
            *shown.camera,
            *s.picker_result.hit
        )) {
        s.picker_result.hit.reset();
        s.selected = 0;
        s.inspector = false;
        changed = true;
    }
    const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse
                         && io.MousePos.x >= picture.low.x && io.MousePos.x < picture.high.x
                         && io.MousePos.y >= picture.low.y && io.MousePos.y < picture.high.y;
    if (clicked) {
        std::array<float, 2> ndc{
            2 * (io.MousePos.x - picture.low.x) / (picture.high.x - picture.low.x) - 1,
            1 - 2 * (io.MousePos.y - picture.low.y) / (picture.high.y - picture.low.y)
        };
        s.picker_result =
            {true,
             shown.picker_level_number,
             shown.picker_tick,
             shown.picker->cast(
                 *shown.picker_level,
                 shown.picker_live,
                 shown.picker_palette,
                 *shown.camera,
                 shown.picker_layers,
                 ndc
             )};
        if (const auto& hit = s.picker_result.hit;
            hit && hit->layer == scene::Layer::Mobys && hit->identity) {
            s.selected = std::uint32_t(hit->identity);
            s.follow_hero = false;
            s.inspector = true;
        } else {
            s.selected = 0;
            s.inspector = false;
        }
        changed = true;
    }
    if (const auto& hit = s.picker_result.hit) {
        auto* draw = ImGui::GetBackgroundDrawList();
        draw->PushClipRect(picture.low, picture.high, true);
        constexpr auto colour = IM_COL32(255, 220, 50, 255);
        for (unsigned a = 0; a < 8; ++a) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                if (!(a & (1u << axis))) {
                    ImVec2 from, to;
                    if (picture.project(hit->bounds[a], from)
                        && picture.project(hit->bounds[a | (1u << axis)], to)) {
                        draw->AddLine(from, to, colour, 2);
                    }
                }
            }
        }
        ImVec2 a, b, c;
        if (picture.project(hit->face[0], a) && picture.project(hit->face[1], b)
            && picture.project(hit->face[2], c)) {
            draw->AddTriangleFilled(a, b, c, IM_COL32(255, 220, 50, 70));
            draw->AddTriangle(a, b, c, colour, 2);
        }
        ImVec2 mark;
        if (picture.project(hit->position, mark)) {
            draw->AddCircle(mark, 5, colour, 16, 2);
        }
        draw->PopClipRect();
    }
    ImGui::SetNextWindowPos(
        {picture.low.x + dp(8), picture.high.y - dp(8)}, ImGuiCond_Always, {0, 1}
    );
    ImGui::SetNextWindowSizeConstraints(
        {0, 0}, {std::max(dp(100), picture.high.x - picture.low.x - dp(16)), FLT_MAX}
    );
    ImGui::SetNextWindowBgAlpha(.88f);
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Moby picker selection", nullptr, flags)) {
        if (const auto& hit = s.picker_result.hit) {
            const auto name = hit->class_id >= 0 ? class_name(hit->class_id) : std::string{};
            text_coloured(
                kYellow,
                "{}: {}{}{}",
                scene::layer_name(hit->layer),
                hit->name,
                name.empty() ? "" : " / ",
                name
            );
            text(
                "Instance {} | Model {} | Material {} | Primitive {} | Triangle {}",
                hit->instance,
                hit->model,
                hit->material,
                hit->primitive,
                hit->triangle
            );
            if (hit->identity) {
                text("Moby address {:08X} | Class {}", hit->identity, hit->class_id);
            }
            text(
                "Hit {:.2f}, {:.2f}, {:.2f} | Distance {:.2f}",
                hit->position[0],
                hit->position[1],
                hit->position[2],
                hit->distance
            );
            if (!hit->texture.empty()) {
                text("Texture: {}", hit->texture);
            }
            if (ImGui::Button("Copy details")) {
                const auto report = scene::picker_report_json(s.picker_result, true, s.game, name);
                ImGui::SetClipboardText(report.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear selection")) {
                s.picker_result.hit.reset();
                s.selected = 0;
                s.inspector = false;
                changed = true;
            }
        } else {
            ImGui::TextUnformatted("Click a moby, tie, shrub or terrain surface to select it.");
        }
        if (ImGui::Button("Turn off click picker")) {
            set_picker_enabled(false);
        }
        ImGui::TextDisabled("Selection report: moby-picker.json (game working folder)");
        if (!s.picker_error.empty()) {
            ImGui::TextUnformatted(s.picker_error.c_str());
        }
    }
    ImGui::End();
    if (picker_enabled() && (changed || ImGui::GetTime() - s.picker_export_at > .5)) {
        s.picker_result.tick = shown.picker_tick;
        write_report(true);
        s.picker_export_at = ImGui::GetTime();
    }
}
}  // namespace openrac::frontend::debug_overlay
