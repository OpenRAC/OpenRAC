// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material Studio's isolated preview, texture cache and shape selection.
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include "debug/material_widgets.h"
#include "debug/menu_ui.h"
#include "enhanced/renderer/material_runtime.h"
#include "renderer/gl/gl.h"

namespace openrac::frontend::debug_overlay {
namespace {
// The window thread owns this renderer and cache while the game's render worker is idle.
scene::ObjectPreview studio_preview;
std::weak_ptr<const scene::MaterialAssets> preview_assets;
std::optional<std::size_t> preview_selection;
bool preview_sphere = false;
bool uploaded_sphere = false;
std::vector<bool> preview_visibility;

// Refreshes optical controls while retaining preview textures and geometry.
bool update_preview_materials(const scene::MaterialBindings& bindings, std::uint32_t model) {
    const auto& parts = studio_preview.mesh().parts;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto found = bindings.lookup.find({model, parts[index].source_material});
        if (found == bindings.lookup.end()) {
            continue;
        }
        const auto& binding = bindings.bindings[found->second];
        const auto& source = bindings.assets->sources[found->second];
        if (scene::is_retail_chrome(source.identity.metal, source.identity.material_name)) {
            continue;
        }
        const auto response = enhanced::material_response(source, binding.values.properties);
        const auto& optics = response.reflection;
        const float metallic = std::max(response.metallic, 0.f);
        if (!studio_preview.update_material(
                index,
                binding.surface(),
                {optics.tint, optics.roughness, optics.reflectivity, optics.strength, metallic}
            )) {
            return false;
        }
    }
    return true;
}

// Rebuilds only when the selected geometry, coverage or texture identity changes.
Result<void> prepare_preview(
    const scene::MaterialBindings& bindings,
    const scene::LevelData& level,
    std::size_t selection,
    bool sphere
) {
    const auto& source = bindings.assets->sources[selection];
    std::vector<bool> visibility;
    if (!sphere) {
        for (const auto& primitive : level.models[source.model].primitives) {
            visibility.push_back(
                scene::material_pass_visible(
                    primitive,
                    level.materials[primitive.material],
                    true,
                    bindings.original(source.model, primitive.appearance_slot())
                )
            );
        }
    }
    if (preview_assets.lock() != bindings.assets || preview_selection != selection
        || preview_visibility != visibility || uploaded_sphere != sphere
        || !update_preview_materials(bindings, source.model)) {
        preview_selection.reset();
        scene::Instance instance;
        instance.model = source.model;
        scene::PickHit hit{};
        hit.model = source.model;
        hit.layer = scene::Layer::Mobys;
        auto mesh = sphere ? Result<scene::PreviewMesh>(scene::preview_uv_sphere(
                                 level, source.material, bindings.bindings[selection].surface()
                             ))
                           : scene::preview_mesh(level, hit, instance, {}, &bindings);
        if (!mesh) {
            studio_preview.release();
            return unexpected(mesh.error());
        }
        const auto loaded = studio_preview.select(std::move(*mesh));
        if (!loaded) {
            return unexpected(loaded.error());
        }
        update_preview_materials(bindings, source.model);
        preview_assets = bindings.assets;
        preview_visibility = std::move(visibility);
        preview_selection = selection;
        uploaded_sphere = sphere;
    }

    return {};
}
}  // namespace

// Draws the selected surface as an object or sphere using the current immutable bindings.
void material_preview(const scene::MaterialBindings& bindings, const Shown& shown) {
    const auto selection = *s.material_selection;
    const auto& source = bindings.assets->sources[selection];
    const bool has_mesh = source.identity.layer == "moby";
    const float size = dp(132.f);
    const auto top = ImGui::GetCursorScreenPos();
    if (ImGui::InvisibleButton("Material preview", {size, size}, ImGuiButtonFlags_MouseButtonRight)
        && has_mesh) {
        preview_sphere = !preview_sphere;
    }
    if (has_mesh && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Right-click to switch between the object and UV sphere.");
    }
    const bool sphere = !has_mesh || preview_sphere;
    const auto high = ImVec2(top.x + size, top.y + size);
    const auto* level = shown.picker_level;
    if (!ImGui::IsItemVisible() || !level) {
        return;
    }

    const auto show_error = [&](const Error& error) {
        ImGui::GetWindowDrawList()->AddRectFilled(top, high, ui().ink_deep);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", error.message.c_str());
        }
    };
    const auto prepared = prepare_preview(bindings, *level, selection, sphere);
    if (!prepared) {
        show_error(prepared.error());
        return;
    }

    scene::PreviewOrbit orbit;
    orbit.distance = sphere ? 2.5f : 2.25f;
    const auto result = studio_preview.draw(
        int(size * ImGui::GetIO().DisplayFramebufferScale.x),
        int(size * ImGui::GetIO().DisplayFramebufferScale.y),
        orbit
    );
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
    gl::glViewport(0, 0, shown.window_width, shown.window_height);
    if (!result) {
        show_error(result.error());
        return;
    }
    ImGui::GetWindowDrawList()
        ->AddImage(ImTextureID(studio_preview.texture()), top, high, {0, 1}, {1, 0});
}

void release_material_preview() {
    studio_preview.release();
    preview_assets.reset();
    preview_selection.reset();
}

}  // namespace openrac::frontend::debug_overlay
