// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Numeric viewer state for agent diagnostics; no extracted mesh or texture assets are written.
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>

#include "debug/overlay.h"

namespace openrac::frontend::debug_overlay {
namespace {
std::string quote(std::string_view text) {
    std::string out = "\"";
    constexpr char kHex[] = "0123456789abcdef";

    for (unsigned char c : text) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += char(c);
        } else if (c < 32) {
            out += "\\u00";
            out += kHex[c >> 4];
            out += kHex[c & 15];
        } else {
            out += char(c);
        }
    }

    return out + '"';
}

void vector(std::ostream& out, const renderer::Vec3& v) {
    out << '[' << v[0] << ',' << v[1] << ',' << v[2] << ']';
}

#ifdef OPENRAC_LIGHT_AUTHORING
// Local authored values and their current bone-resolved preview positions are both inspectable.
void light_report(std::ostream& out) {
    const auto& p = s.object_viewer;
    out << "[";

    for (std::size_t i = 0; i < p.lights.size(); ++i) {
        const auto& attached = p.lights[i];
        const auto& light = attached.light;
        out << (i ? "," : "") << "{\"name\":" << quote(attached.name)
            << ",\"bone\":" << quote(attached.bone) << ",\"enabled\":" << attached.enabled
            << ",\"type\":" << quote(light.spot ? "spot" : "point") << ",\"position\":";
        vector(out, light.position);
        out << ",\"direction\":";
        vector(out, light.direction);
        out << ",\"colour\":";
        vector(out, light.colour);
        out << ",\"intensity\":" << light.intensity << ",\"range\":" << light.range;
        const int joint = enhanced::attached_light_joint(attached, p.renderer.mesh().bones);
        const auto frame = !p.selected || joint == -2
                               ? std::nullopt
                               : scene::preview_attachment(p.renderer.mesh(), joint);

        if (frame) {
            out << ",\"preview_position\":";
            vector(out, renderer::transform_point(*frame, light.position));
        }

        out << '}';
    }

    out << ']';
}
#endif
// Prefer the primary authoring error, then attachment errors, then a rendering failure.
std::string_view viewer_error() {
    const auto& p = s.object_viewer;

    if (!p.error.empty()) {
        return p.error;
    }

    if (!p.light_error.empty()) {
        return p.light_error;
    }

    return p.draw_error;
}
}  // namespace

// Only visible controls are exported; stale or clipped widget positions cannot be clicked accidentally.
void remember_object_control(std::string_view key) {
    if (!ImGui::IsItemVisible()) {
        return;
    }

    const auto low = ImGui::GetItemRectMin(), high = ImGui::GetItemRectMax();
    s.object_viewer.controls[std::string(key)] = {low.x, low.y, high.x - low.x, high.y - low.y};
}

bool object_viewer_button(const char* label, const char* key) {
    const bool pressed = ImGui::Button(label);
    remember_object_control(key);
    return pressed;
}

bool object_viewer_combo(const char* label, int* value, const char* options, const char* key) {
    const bool changed = ImGui::Combo(label, value, options);
    remember_object_control(key);
    return changed;
}

void write_object_viewer_report() {
    auto& p = s.object_viewer;
    std::ostringstream out;
#ifdef OPENRAC_LIGHT_AUTHORING
    std::size_t dirty = 0;

    for (const auto& [path, catalog] : p.drafts) {
        dirty += catalog.objects.size();
    }
#endif
    out.imbue(std::locale::classic());
    out << std::setprecision(9) << std::boolalpha << "{\"schema\":2,\"tick\":" << s.tick
        << ",\"open\":" << p.open << ",\"selected\":" << p.selected << ",\"frozen\":" << p.frozen
        << ",\"pose_tick\":" << p.pose_tick << ",\"pose_mode\":" << p.pose_mode
        << ",\"selection\":" << scene::picker_report_json(p.selection, p.selected, s.game);
#ifdef OPENRAC_LIGHT_AUTHORING
    out << ",\"object_key\":" << quote(p.key)
        << ",\"profile\":" << quote(p.profile_path.generic_string())
        << ",\"dirty_objects\":" << dirty << ",\"selected_light\":" << p.selected_light
        << ",\"lights\":";
    light_report(out);
#endif
    out << ",\"orbit\":{\"yaw\":" << p.orbit.yaw << ",\"pitch\":" << p.orbit.pitch
        << ",\"distance\":" << p.orbit.distance << ",\"focus\":";
    vector(out, p.orbit.focus);
    out << "},\"image\":[" << p.image_low.x << ',' << p.image_low.y << ',' << p.image_size.x << ','
        << p.image_size.y << "]"
        << ",\"controls\":{";
    bool first = true;

    for (const auto& [key, rect] : p.controls) {
        out << (first ? "" : ",") << quote(key) << ":[" << rect[0] << ',' << rect[1] << ','
            << rect[2] << ',' << rect[3] << ']';
        first = false;
    }

    out << "},\"error\":" << quote(viewer_error()) << "}\n";
    std::ofstream file("object-viewer.json", std::ios::trunc);
    file << out.str();
    file.flush();

    if (!file) {
        p.error = "Could not write object-viewer.json in the game's working folder.";
    }

    p.report_at = ImGui::GetTime();
}
}  // namespace openrac::frontend::debug_overlay
