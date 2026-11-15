// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

#include "debug/menu_ui.h"
#include "enhanced/renderer/material_runtime.h"
#include "frontend.h"
#include "renderer/gl/gl.h"
#include "scene/object_picker.h"

namespace openrac::frontend::debug_overlay {
namespace {
std::atomic<bool> enabled{false};
// The JSON snapshot is local diagnostics, never game assets or a network service.
std::string report_path = "moby-picker.json";

void write_report(bool active) {
    const auto& hit = s.picker_result.hit;
    auto report = scene::picker_report_json(
        s.picker_result,
        active,
        s.game,
        hit && hit->class_id >= 0 ? class_name(hit->class_id) : std::string{}
    );
    std::ofstream file(report_path.c_str(), std::ios::trunc);
    file << report;
    file.flush();
    s.picker_error = file ? std::string{} : "Could not write moby-picker.json";
}

struct Picture {
    renderer::Mat4 matrix;
    ImVec2 low, high;

    std::array<float, 2> ndc(ImVec2 at) const {
        return {
            2 * (at.x - low.x) / (high.x - low.x) - 1, 1 - 2 * (at.y - low.y) / (high.y - low.y)
        };
    }

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
        const auto editor = frontend::material_editor();
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
                 picture.ndc(io.MousePos),
                 s.material_settings,
                 editor ? editor->bindings.get() : nullptr
             )};
        s.material_pick_pending = s.material_settings;
        if (const auto& hit = s.picker_result.hit;
            hit && hit->layer == scene::Layer::Mobys && hit->identity) {
            s.selected = std::uint32_t(hit->identity);
            s.follow_hero = false;
            s.inspector = !s.object_viewer.open;
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
    const float width = std::clamp(picture.high.x - picture.low.x - dp(16), dp(100), dp(430));
    ImGui::SetNextWindowSizeConstraints({width, 0}, {width, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {dp(16), dp(12)});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, dp(14));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ui_colors::mix(ui().panel_hi, ui().ink_deep, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_Border, ui().line);
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Moby picker selection", nullptr, flags)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ui().gold);
        ImGui::TextUnformatted("MOBY PICKER");
        ImGui::PopStyleColor();
        ImGui::Separator();
        if (const auto& hit = s.picker_result.hit) {
            const auto name = hit->class_id >= 0 ? class_name(hit->class_id) : std::string{};
            const auto layer = scene::layer_name(hit->layer);
            ImGui::TextWrapped("%s", name.empty() ? hit->name.c_str() : name.c_str());
            ImGui::TextDisabled(
                "%s  /  Model %u  /  Material %d", layer, hit->model, hit->material
            );
            if (!hit->texture.empty()) {
                ImGui::TextWrapped("Texture %s", hit->texture.c_str());
            }
            if (ImGui::TreeNode("Selection details")) {
                ImGui::TextWrapped("%s", hit->name.c_str());
                text(
                    "Instance {} | Primitive {} | Triangle {}",
                    hit->instance,
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
                ImGui::TreePop();
            }
            ImGui::Spacing();
            if (s.developer) {
                if (ImGui::Button("Edit material")) {
                    const auto& level = *shown.picker_level;
                    const auto* material =
                        hit->material >= 0 && std::size_t(hit->material) < level.materials.size()
                            ? &level.materials[hit->material]
                            : nullptr;
                    if (material && scene::is_retail_chrome(material->metal, material->name)) {
                        const auto editor = frontend::material_editor();
                        ImVec2 at;
                        auto visible = picture.project(hit->position, at)
                                           ? shown.picker->cast(
                                                 level,
                                                 shown.picker_live,
                                                 shown.picker_palette,
                                                 *shown.camera,
                                                 1u << unsigned(hit->layer),
                                                 picture.ndc(at),
                                                 true,
                                                 editor ? editor->bindings.get() : nullptr
                                             )
                                           : std::nullopt;
                        if (visible && visible->model == hit->model
                            && visible->instance == hit->instance) {
                            s.picker_result.hit = std::move(visible);
                        } else {
                            s.picker_result.hit.reset();
                        }
                        changed = true;
                    }
                    s.material_settings = true;
                    s.material_pick_pending = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Inspect in isolation")) {
                    select_object_viewer(shown);
                }
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
            ImGui::TextWrapped("Click a moby, tie, shrub or terrain surface to select it.");
        }
        ImGui::Separator();
        if (ImGui::Button("Stop picking")) {
            set_picker_enabled(false);
        }
        ImGui::TextDisabled("Selection report: %s", report_path.c_str());
        if (!s.picker_error.empty()) {
            ImGui::TextUnformatted(s.picker_error.c_str());
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    if (picker_enabled() && (changed || ImGui::GetTime() - s.picker_export_at > .5)) {
        s.picker_result.tick = shown.picker_tick;
        write_report(true);
        s.picker_export_at = ImGui::GetTime();
    }
}

void set_picker_report_folder(const std::string& folder) {
    report_path = (std::filesystem::path(folder) / "moby-picker.json").string();
}

}  // namespace openrac::frontend::debug_overlay
