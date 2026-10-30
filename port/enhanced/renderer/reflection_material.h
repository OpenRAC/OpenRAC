// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openrac::enhanced {
enum class ReflectionSurface {
    Water,
    Clank,
    Wrench,
    Chrome,
    Glass,
    Stylised,
    Count
};

// Session-only controls, copied safely between the debug UI and render worker.
struct ReflectionMaterialSettings {
    float roughness = .22f;
    float reflectivity = .02037f;  // Normal-incidence Fresnel reflectance (F0).
    float strength = 1;
    std::array<float, 3> tint{1, 1, 1};  // Linear reflected-light tint; white is neutral.
    float opacity = 1;                   // Multiplies authored packet alpha, never replaces it.
};

struct ReflectionSubject {
    int class_id = -1;
    std::string name;
    std::optional<ReflectionSurface> override;
};

// Explicit session overrides apply to an object's chrome/glass packets, never its painted faces.
void set_reflection_subjects(std::vector<ReflectionSubject> subjects);
std::vector<ReflectionSubject> reflection_subjects();
void set_reflection_override(int class_id, std::optional<ReflectionSurface> surface);
void clear_reflection_overrides();
ReflectionMaterialSettings reflection_material_for(std::string_view authored_name, int class_id);

ReflectionMaterialSettings default_reflection_material(ReflectionSurface surface);
ReflectionMaterialSettings reflection_material_settings(ReflectionSurface surface);
void set_reflection_material_settings(
    ReflectionSurface surface, ReflectionMaterialSettings settings
);
}  // namespace openrac::enhanced
