// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reusable material finishes and display-only defaults for Material Studio's schema controls.
#pragma once

#include <algorithm>
#include <array>

#include "enhanced/renderer/fur.h"
#include "enhanced/renderer/wetness.h"
#include "materials/catalog.h"
#include "scene/level.h"

namespace openrac::frontend::debug_overlay {

// Display inherited controls without making them authored overrides until the user edits one.
inline materials::Properties material_control_properties(
    materials::Properties properties, const scene::Material& inherited = {}
) {
    properties = materials::material_definition(std::move(properties));
    properties.try_emplace("surface.tint", std::array<double, 3>{1, 1, 1});
    properties.try_emplace("surface.diffuse_scale", double(inherited.diffuse_scale));
    const auto emission = *std::max_element(inherited.emission.begin(), inherited.emission.end());
    properties.try_emplace("emission.runtime_intensity", double(emission));
    properties.try_emplace(
        "emission.bake_intensity", double(inherited.bake_intensity.value_or(emission))
    );
    properties.try_emplace("emission.contrast", double(inherited.emission_contrast));
    properties.try_emplace("emission.vertex_colour", inherited.emission_vertex_colour);
    properties
        .try_emplace("emission.use_base_texture", inherited.emission_image == inherited.image);
    return properties;
}

enum class PresetSurface {
    Opaque,
    Metal,
    Glass,
    Water,
    Ocean,
    Foliage,
    DryFur,
    WetFur
};

struct MaterialPreset {
    PresetSurface surface;
    const char* name;
    const char* description;
    double roughness = 0;
    double reflectivity = .04;
    std::array<double, 3> colour{1, 1, 1};  // Metal F0 or glass transmission tint, in linear RGB.
    double transmission = 0;
};

// Roughness is a suggested finish. Metal F0 uses Epic's measured-material reference values.
// clang-format off: one finish per row for comparing library values.
inline constexpr MaterialPreset kMaterialPresets[] = {
    {PresetSurface::Opaque, "Satin plastic", "Soft highlights for robot trim and weapon grips.", .40},
    {PresetSurface::Opaque, "Matte rubber", "Broad highlights for tyres, seals and boot soles.", .78},
    {PresetSurface::Opaque, "Gloss enamel paint", "A smooth painted finish for ship and robot panels.", .24},
    {PresetSurface::Opaque, "Powder-coated paint", "A subdued finish for industrial doors and casings.", .57},
    {PresetSurface::Opaque, "Glazed ceramic", "A smooth glaze for pottery and decorative surfaces.", .18},
    {PresetSurface::Opaque, "Terracotta", "A dry, unglazed finish for clay pots and masonry.", .79},
    {PresetSurface::Opaque, "Dry concrete", "Broad, weak highlights for platforms and bunker walls.", .84},
    {PresetSurface::Opaque, "Polished stone", "A smooth finish for monuments and interior floors.", .29, .045},
    {PresetSurface::Opaque, "Rough sandstone", "A dry, granular finish for cliffs and masonry.", .87},
    {PresetSurface::Opaque, "Sealed wood", "A satin finish for finished timber and props.", .39},
    {PresetSurface::Opaque, "Raw wood", "Broad highlights for crates and unfinished boards.", .73},
    {PresetSurface::Opaque, "Dry sand", "A very matte finish for dry beaches and dunes.", .92},
    {PresetSurface::Opaque, "Worn asphalt", "A coarse finish for roads and landing pads.", .83},
    {PresetSurface::Opaque, "Matte interior paint", "Soft highlights for painted walls and ceilings.", .72},
    {PresetSurface::Opaque, "Painted plaster", "A muted wall finish for interiors such as Novalis.", .78},
    {PresetSurface::Opaque, "Rough stucco", "A coarse finish for exterior walls and columns.", .91},
    {PresetSurface::Opaque, "Fired red brick", "A rough fired-clay finish for masonry and chimneys.", .80},
    {PresetSurface::Opaque, "Chalky mortar", "A very matte finish for seams and exposed mortar.", .94},
    {PresetSurface::Opaque, "Glazed floor tile", "A tight glaze highlight for clean corridors and lobbies.", .20},
    {PresetSurface::Opaque, "Terrazzo", "A lightly polished finish for patterned interior floors.", .34},
    {PresetSurface::Opaque, "Trowelled concrete", "A satin finish for workshops and hangars.", .46},
    {PresetSurface::Opaque, "Vinyl flooring", "A soft sheen for ship interiors and control rooms.", .42},
    {PresetSurface::Opaque, "Honed limestone", "A smooth matte finish for steps and worn monuments.", .57},
    {PresetSurface::Opaque, "Polished granite", "A tight highlight for decorative floors and counters.", .23, .045},
    {PresetSurface::Opaque, "Rough basalt", "A dark-rock finish for volcanic cliffs and rubble.", .89},
    {PresetSurface::Opaque, "Packed dirt", "A broad highlight for trails and exposed soil.", .90},
    {PresetSurface::Opaque, "Damp compacted sand", "A tighter highlight for the swash zone. Tune diffuse brightness separately.", .59},
    {PresetSurface::Opaque, "Coarse gravel", "A very matte finish for road shoulders and riverbeds.", .92},
    {PresetSurface::Opaque, "Compacted snow", "A broad highlight for snow paths and banks. Uses opaque shading.", .84, .035},
    {PresetSurface::Opaque, "Lacquered veneer", "A glossy finish for furniture and decorative panelling.", .24},
    {PresetSurface::Opaque, "Oiled hardwood", "A soft sheen for handrails and worn floors.", .43},
    {PresetSurface::Opaque, "Weathered dock timber", "A rough finish for sun-worn docks and pilings.", .84},
    {PresetSurface::Opaque, "Textured ABS", "A satin moulded finish for gadgets and grips.", .53},
    {PresetSurface::Opaque, "Opaque acrylic", "A smooth finish for coloured buttons and trims.", .21, .039},
    {PresetSurface::Opaque, "Smooth leather", "A soft sheen for seats, straps and gloves.", .46},
    {PresetSurface::Opaque, "Woven canvas", "A matte finish for sacks, awnings and upholstery.", .85, .045},
    {PresetSurface::Opaque, "Oxidised iron crust", "A rough dielectric finish for rusted areas of industrial props.", .88},
    {PresetSurface::Metal, "Machined aluminium", "Bright neutral reflections for ship trim and housings.", .30, 0, {.913, .921, .925}},
    {PresetSurface::Metal, "Bead-blasted aluminium", "Broad neutral reflections for industrial casings.", .56, 0, {.913, .921, .925}},
    {PresetSurface::Metal, "Dark iron", "Broad, darker reflections for railings and machinery.", .52, 0, {.560, .570, .580}},
    {PresetSurface::Metal, "Brushed nickel", "Soft warm reflections for fittings. Uses isotropic highlights.", .36, 0, {.660, .609, .526}},
    {PresetSurface::Metal, "Satin titanium", "Muted warm-grey reflections for panels and barrels.", .33, 0, {.542, .497, .449}},
    {PresetSurface::Metal, "Polished silver", "Bright, near-neutral reflections for trim and bolts.", .17, 0, {.972, .960, .915}},
    {PresetSurface::Metal, "Satin copper", "Warm red-orange reflections for pipes and coils.", .34, 0, {.955, .637, .538}},
    {PresetSurface::Metal, "Polished gold", "Tight golden reflections for trophies and bolts.", .20, 0, {1, .766, .336}},
    {PresetSurface::Metal, "Worn gold", "Broader golden reflections for ornamental trim.", .44, 0, {1, .766, .336}},
    {PresetSurface::Metal, "Cobalt alloy", "Subdued neutral reflections for machine parts.", .39, 0, {.662, .655, .634}},
    {PresetSurface::Glass, "Clear canopy", "High transmission and tight reflections for a clear visor.", .10, .04, {1, 1, 1}, .94},
    {PresetSurface::Glass, "Smoke-tinted canopy", "Lower transmission and a neutral dark tint for a visor.", .16, .04, {.18, .19, .20}, .70},
    {PresetSurface::Glass, "Frosted glass", "Broad reflections and a pale tint. The transmitted scene stays sharp.", .46, .04, {.75, .78, .82}, .55},
    {PresetSurface::Water, "Calm pool", "A smooth local-water finish. Keeps the level's waves and colour.", .10, .020},
    {PresetSurface::Ocean, "Tropical ocean", "A soft FFT ocean highlight. Keeps the level's colour, absorption and waves.", .16, .020},
    {PresetSurface::Foliage, "Waxy tropical leaf", "A satin leaf surface with visible backlighting.", .47, .04, {1, 1, 1}, .45},
    {PresetSurface::Foliage, "Dry leaf", "A rough leaf surface with weaker backlighting.", .72, .04, {1, 1, 1}, .14},
    {PresetSurface::DryFur, "Dry fur", "Directional strand detail and subtle scattering on Ratchet's verified coat."},
    {PresetSurface::WetFur, "Wet fur", "Darkens the verified coat and tightens highlights when wet."},
};
// clang-format on

// Complete optical finishes reset metallic when switching back to a dielectric.
inline materials::Properties preset_properties(const MaterialPreset& preset) {
    using Colour = std::array<double, 3>;
    if (preset.surface == PresetSurface::Ocean) {
        return {
            {"type", std::string("lit")},
            {"shading", std::string("water")},
            {"ocean.roughness", preset.roughness},
            {"ocean.reflectivity", preset.reflectivity}
        };
    }
    if (preset.surface == PresetSurface::DryFur) {
        const enhanced::FurMaterialSettings fur;
        return {
            {"type", std::string("lit-fur")},
            {"fur.enabled", fur.enabled},
            {"fur.strength", double(fur.strength)},
            {"fur.contrast", double(fur.contrast)},
            {"fur.fade_distance", double(fur.fade_distance)},
            {"fur.lighting_enabled", fur.lighting_enabled},
            {"fur.highlight", double(fur.highlight)},
            {"fur.softness", double(fur.softness)},
            {"fur.directionality", double(fur.directionality)},
            {"fur.scattering", double(fur.scattering)},
            {"fur.environment_strength", double(fur.environment_strength)}
        };
    }
    if (preset.surface == PresetSurface::WetFur) {
        const enhanced::WetnessMaterialSettings wet;
        return {
            {"type", std::string("lit-wet-fur")},
            {"wetness.diffuse_scale", double(wet.diffuse_scale)},
            {"wetness.specular_strength", double(wet.specular_strength)},
            {"wetness.dry_roughness", double(wet.dry_roughness)},
            {"wetness.wet_roughness", double(wet.wet_roughness)},
            {"wetness.reflectivity", double(wet.reflectivity)}
        };
    }
    materials::Properties properties{
        {"type",
         std::string(
             preset.surface == PresetSurface::Glass     ? "glass-lit"
             : preset.surface == PresetSurface::Foliage ? "foliage"
                                                        : "lit"
         )},
        {"surface.roughness", preset.roughness},
        {"surface.reflectivity", preset.reflectivity},
        {"reflection.strength", 1.},
        {"reflection.tint", Colour{1, 1, 1}},
    };
    switch (preset.surface) {
        case PresetSurface::Opaque:
            properties["surface.metallic"] = 0.;
            properties["shading"] = std::string("standard");
            break;
        case PresetSurface::Metal: {
            const auto& [r, g, b] = preset.colour;
            const double f0 = std::max({r, g, b});
            properties["shading"] = std::string("standard");
            properties["surface.metallic"] = 1.;
            properties["surface.reflectivity"] = f0;
            properties["reflection.tint"] = Colour{r / f0, g / f0, b / f0};
            break;
        }
        case PresetSurface::Glass:
            properties["shading"] = std::string("standard");
            properties["reflection.opacity"] = 1.;
            properties["glass.transmission"] = preset.transmission;
            properties["glass.tint"] = preset.colour;
            break;
        case PresetSurface::Water:
            properties["shading"] = std::string("water");
            break;
        case PresetSurface::Foliage:
            properties["surface.metallic"] = 0.;
            properties["shading"] = std::string("foliage");
            properties["foliage.transmission"] = preset.transmission;
            properties["foliage.thickness"] = .7;
            properties["foliage.normal_bend"] = .2;
            break;
        default:
            break;
    }
    return properties;
}

}  // namespace openrac::frontend::debug_overlay
