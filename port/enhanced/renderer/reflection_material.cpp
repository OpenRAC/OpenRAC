// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/renderer/reflection_material.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <mutex>

namespace openrac::enhanced {
namespace {
std::mutex material_mutex;
std::array<ReflectionMaterialSettings, static_cast<unsigned>(ReflectionSurface::Count)> materials{
    default_reflection_material(ReflectionSurface::Water),
    default_reflection_material(ReflectionSurface::Clank),
    default_reflection_material(ReflectionSurface::Wrench),
    default_reflection_material(ReflectionSurface::Chrome),
    default_reflection_material(ReflectionSurface::Glass),
    default_reflection_material(ReflectionSurface::Stylised)
};
std::vector<ReflectionSubject> subjects;
std::map<int, ReflectionSurface> overrides;

// Authored glass wins over object defaults; unusual chrome effects require an explicit override.
ReflectionSurface automatic_surface(std::string_view name, int class_id) {
    if (name == "metal_glass") {
        return ReflectionSurface::Glass;
    }
    if (name == "metal_stylised" || name == "metal_stylized") {
        return ReflectionSurface::Stylised;
    }
    if (class_id == 87) {
        return ReflectionSurface::Clank;
    }
    if (class_id == 71) {
        return ReflectionSurface::Wrench;
    }
    return ReflectionSurface::Chrome;
}
}  // namespace

ReflectionMaterialSettings default_reflection_material(ReflectionSurface surface) {
    switch (surface) {
        case ReflectionSurface::Clank:
            return {.26f, .65f, 1};
        case ReflectionSurface::Wrench:
            return {.18f, .75f, 1};
        case ReflectionSurface::Chrome:
            return {.25f, .65f, 1};
        case ReflectionSurface::Glass:
            return {.08f, .04f, 1};
        case ReflectionSurface::Stylised:
            return {.35f, .12f, .65f};
        default:
            return {};
    }
}

ReflectionMaterialSettings reflection_material_settings(ReflectionSurface surface) {
    const std::lock_guard lock(material_mutex);
    return materials.at(static_cast<unsigned>(surface));
}

void set_reflection_material_settings(
    ReflectionSurface surface, ReflectionMaterialSettings settings
) {
    const auto defaults = default_reflection_material(surface);
    const auto bounded = [](float value, float lo, float hi, float fallback) {
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    };
    settings.roughness = bounded(settings.roughness, .04f, 1, defaults.roughness);
    settings.reflectivity = bounded(settings.reflectivity, 0, 1, defaults.reflectivity);
    settings.strength = bounded(settings.strength, 0, 4, defaults.strength);
    settings.opacity = bounded(settings.opacity, 0, 1, defaults.opacity);
    for (unsigned channel = 0; channel < 3; ++channel) {
        settings.tint[channel] = bounded(settings.tint[channel], 0, 1, defaults.tint[channel]);
    }
    const std::lock_guard lock(material_mutex);
    materials.at(static_cast<unsigned>(surface)) = settings;
}

void set_reflection_subjects(std::vector<ReflectionSubject> available) {
    std::sort(available.begin(), available.end(), [](const auto& a, const auto& b) {
        return a.class_id < b.class_id;
    });
    const std::lock_guard lock(material_mutex);
    subjects = std::move(available);
}

std::vector<ReflectionSubject> reflection_subjects() {
    const std::lock_guard lock(material_mutex);
    auto result = subjects;
    for (auto& subject : result) {
        if (auto found = overrides.find(subject.class_id); found != overrides.end()) {
            subject.override = found->second;
        }
    }
    return result;
}

void set_reflection_override(int class_id, std::optional<ReflectionSurface> surface) {
    if (class_id < 0
        || (surface
            && (*surface == ReflectionSurface::Water || *surface >= ReflectionSurface::Count))) {
        return;
    }
    const std::lock_guard lock(material_mutex);
    if (surface) {
        overrides[class_id] = *surface;
    } else {
        overrides.erase(class_id);
    }
}

void clear_reflection_overrides() {
    const std::lock_guard lock(material_mutex);
    overrides.clear();
}

ReflectionMaterialSettings reflection_material_for(std::string_view name, int class_id) {
    const std::lock_guard lock(material_mutex);
    const auto found = overrides.find(class_id);
    const auto surface =
        found != overrides.end() ? found->second : automatic_surface(name, class_id);
    return materials.at(static_cast<unsigned>(surface));
}

}  // namespace openrac::enhanced
