// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private UI seams between Material Studio navigation, property controls and its isolated preview.
#pragma once

#include <span>
#include <string>

#include "materials/catalog.h"
#include "scene/material_overrides.h"

namespace openrac::frontend::debug_overlay {
struct Shown;

// Edits one supported type; the caller applies the returned change to the shared definition.
bool type_widget(std::string& value, const char* label);
// Draws diagnostic controls without writing them into material definitions.
void foliage_diagnostics();
// Assigns a compatible reusable finish to the selected mesh slots.
void library_controls(const scene::MaterialBindings& bindings, std::span<const std::size_t> slots);
// Edits a shared material while leaving inherited values unauthored until changed.
void preset_controls(
    const std::string& material, const materials::Properties& properties, bool heading = true
);
// Draws the currently selected surface in the Studio's private render target.
void material_preview(const scene::MaterialBindings& bindings, const Shown& shown);
}  // namespace openrac::frontend::debug_overlay
