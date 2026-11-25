#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture times vertex colour, alpha-tested for cutout materials, with a
// fixed sun so shapes read where the export has no lighting. The editor's PNGs
// carry alpha from 0 to 255 (it doubles the chip's 0..0x80), so the cutout
// threshold is plain 0.5. A vertex colour of 1.0 is the GS's 0x80: the product
// is the GS's MODULATE, (texel x colour) >> 7, clamped like the chip's.

in vec2 v_uv;
in vec4 v_colour;
in vec3 v_normal;
in vec3 v_world_position;
in vec4 v_probe_multiplier;
flat in vec4 v_probe_selector;
flat in int v_lit;
flat in int v_glow;
flat in int v_mod_material;  // a mod's material on the moby (mesh.vert): 2 platinum, 3 obsidian, 4 energy
flat in vec3 v_mod_rgb;      // the moby's tint (mesh.vert), the mod's rgb: obsidian's glow colour
in vec3 v_script_emission;
flat in vec3 v_probe_ambient;
in vec3 v_emission_colour;
// GS F / 255 times the vertex's clip w, and that w (mesh.vert): their ratio is F interpolated in
// screen space, as the GS does.
in vec2 v_fog_w;
float v_fog;

uniform sampler2D tex_T0;
uniform vec4 material_colour;
uniform int cutout;
uniform int lighting;
// The pass and setting switches every Vulkan pipeline fixes as constants (pipeline variants).
// openrac_mode: glow_pass glow_split opacity_split translucent_pass effect_pass effect_additive
// openrac_mode: metal_pass gtao_enabled hero_wetness_debug object_reflection_enabled
// openrac_mode: gameplay_rim_enabled
uniform int glow_pass;  // mobys: 0 the solid faces, 1 the glow faces (blended)
// The glow pass in two draws, as the GS's TEST 0x5360b splits it (alpha GEQUAL 0x60: Z written, else
// colour only): 1 the texels at or above the reference, with depth; 2 the others, without; 0 all.
uniform int glow_split;
uniform int opacity_split;
const float kWorldAlphaReference = 96.0 / 128.0;
// Cutout materials in two passes, as the GS draws them (alpha test GEQUAL 0x80, failing pixels
// written as colour only): 0 the opaque texels, with depth; 1 the translucent ones, blended.
uniform int translucent_pass;
// Mobys MobyProc blends (an explosion's flash, a fading pickup, one at its draw distance): every
// face and texel, its alpha the texel's times the moby's (the tint), blended by the pass. Split by
// glow_split at the moby's alpha reference (TEST_1 AREF: 0x60, or 0x08 fading), as the glow pass.
uniform int effect_pass;
uniform float effect_reference;
// The additive ones (mode bit 0x200, Cs * As + Cd): the shader weighs Cs by As itself, as the GS
// reaches about 2.0 (0xFF) where GL's SRC_ALPHA stops at 1; drawn with ONE, ONE.
uniform int effect_additive;
// The metal pass: the chrome or glass texel times the vertex colour, its alpha the texel's times the
// shine, blended over the moby (mesh.vert; ALPHA 0x44 as the moby's own).
uniform int metal_pass;
uniform int object_thin_glass;
uniform vec4 fog_colour;  // FOGCOL; w = 1: fog on
uniform mat4 view;
#ifdef OPENRAC_ENHANCED_RENDERING
uniform int gtao_enabled;
uniform vec3 material_emission;
uniform sampler2D emission_texture;
uniform int emission_vertex_colour;
uniform float material_emission_contrast;
uniform float material_diffuse_scale;
uniform int material_fallback;
uniform sampler2D gtao_visibility;
#endif

#ifdef OPENRAC_HERO_WETNESS
in float v_hero_wetness;
// openrac_mode: hero_wetness_enabled
uniform int hero_wetness_enabled; // Zero for every draw but the hero's: no wet coat code there.
uniform int hero_wetness_debug;
uniform vec4 hero_wetness_material; // wet diffuse, specular strength, dry/wet roughness
uniform float hero_wetness_f0;
#endif

layout(location = 0) out vec4 out_colour;

// From above and to one side, in game axes (Z up).
const vec3 kSun = vec3(0.36, 0.48, 0.80);

// A mod's platinum (openrac_api.moby_material, OPENRAC_MOBY_MATERIAL_PLATINUM): render-only, a
// cool white metal. Its texel is made grey (kPlatinumColourKept of its colour left), brightened and
// turned blue; so is its colour after the vertex colour (platinum_colour), which is where a
// coloured moby's colour is: Going Commando's bolt coins are a grey texture lit gold by light set
// 13 (moby_spawn_bolt_pickup; their trail is the game's particles, rac2/host/platinum_trail.c).
// After the lighting it gets a tight highlight from the fixed sun and a bright rim. A glow packet's
// colour is made silver the same way: its texel is not brightened and it gets no highlight.
const int kModMaterialPlatinum = 2;
const float kPlatinumColourKept = 0.0;
const float kPlatinumGain = 1.3;
const float kPlatinumLift = 0.1;
const vec3 kPlatinumTint = vec3(0.86, 0.95, 1.16);
const float kPlatinumGloss = 40.0;     // the highlight's exponent: higher is tighter
const float kPlatinumHighlight = 0.9;  // the highlight's strength at its peak
const float kPlatinumRim = 0.55;       // the rim's strength at a grazing angle
const vec3 kPlatinumShine = vec3(0.92, 0.96, 1.0);
const float kPlatinumColourGain = 1.35; // platinum_colour's brightening

bool platinum() {
    return v_mod_material == kModMaterialPlatinum;
}

// Its surface, not a glow packet's vertex: the one brightened and shone.
bool platinum_surface() {
    return platinum() && v_glow == 0;
}

vec4 platinum_texel(vec4 texel) {
    if (!platinum_surface()) {
        return texel;
    }
    float grey = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    vec3 metal = mix(vec3(grey), texel.rgb, kPlatinumColourKept) * kPlatinumGain + kPlatinumLift;
    return vec4(clamp(metal * kPlatinumTint, 0.0, 1.0), texel.a);
}

// The texel times its vertex colour (lit, or a glow packet's), made grey (with kPlatinumColourKept
// of its colour left), turned blue and brightened; alpha as it is.
vec4 platinum_colour(vec4 c) {
    if (!platinum()) {
        return c;
    }
    float grey = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    vec3 metal = mix(vec3(grey), c.rgb, kPlatinumColourKept) * kPlatinumTint * kPlatinumColourGain;
    return vec4(metal, c.a);
}

vec3 platinum_shine(vec3 rgb) {
    if (!platinum_surface() || dot(v_normal, v_normal) < 1e-12) {
        return rgb;
    }
    vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
    vec3 to_eye = normalize(-eye_position);
    vec3 n = normalize(mat3(view) * v_normal);
    // Double-sided, as the game draws mobys: a face seen from behind shines as its front.
    n = dot(n, to_eye) < 0.0 ? -n : n;
    vec3 half_way = normalize(normalize(mat3(view) * kSun) + to_eye);
    float highlight = pow(max(dot(n, half_way), 0.0), kPlatinumGloss);
    float rim = pow(1.0 - max(dot(n, to_eye), 0.0), 3.0);
    vec3 shine = kPlatinumShine * (kPlatinumHighlight * highlight + kPlatinumRim * rim);
    return min(rgb + shine, vec3(1.0));
}

// A mod's obsidian (OPENRAC_MOBY_MATERIAL_OBSIDIAN, interface 1.24): render-only, glossy black
// with a glow from within. Its texel is made grey and dark (kObsidianAlbedo), so the enhanced
// lighting sees a black albedo; its colour after the vertex colour is made grey at its luminance
// (the mod's tint, which multiplied it, taken out again), turned the body's near black, and given
// a little of the glow (obsidian_colour). After the lighting the glow from within, strongest where
// the surface faces the eye, a dim ember red at its rim and a sharp white highlight from the fixed
// sun (obsidian_shine). A glow packet's colour becomes the glow's at its luminance. The glow's
// colour is the mod's rgb (v_mod_rgb), white for kObsidianGlow: a coin's trail takes the same
// colour (rac2/host/platinum_trail.c).
const int kModMaterialObsidian = 3;
const float kObsidianAlbedo = 0.25;          // the texel's grey kept
const vec3 kObsidianBody = vec3(0.85, 0.70, 1.0); // the lit body's colour, times its luminance
const vec3 kObsidianGlow = vec3(0.55, 0.15, 0.95); // the default glow: violet
const float kObsidianGlowBase = 0.6;         // the glow in the body, times its luminance
const float kObsidianGlowPacket = 1.6;       // a glow packet's brightening
const float kObsidianInner = 0.5;            // the glow from within, facing the eye
const float kObsidianInnerFalloff = 2.0;     // how fast it fades towards the edge
const vec3 kObsidianEmber = vec3(0.85, 0.10, 0.12); // the rim: ember red
const float kObsidianRim = 0.4;              // the rim's strength at a grazing angle
const float kObsidianGloss = 90.0;           // the highlight's exponent: glassy
const float kObsidianHighlight = 1.0;        // the highlight's strength at its peak

bool obsidian() {
    return v_mod_material == kModMaterialObsidian;
}

bool obsidian_surface() {
    return obsidian() && v_glow == 0;
}

// The glow's colour: the mod's rgb, or kObsidianGlow for white (no colour given).
vec3 obsidian_glow() {
    return all(greaterThan(v_mod_rgb, vec3(0.999))) ? kObsidianGlow : v_mod_rgb;
}

vec4 obsidian_texel(vec4 texel) {
    if (!obsidian_surface()) {
        return texel;
    }
    float grey = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    return vec4(vec3(grey * kObsidianAlbedo), texel.a);
}

vec4 obsidian_colour(vec4 c) {
    if (!obsidian()) {
        return c;
    }
    // The tint is the glow's colour, not the moby's: its luminance is taken out of the colour's.
    float tint = max(dot(v_mod_rgb, vec3(0.299, 0.587, 0.114)), 0.05);
    float grey = dot(c.rgb, vec3(0.299, 0.587, 0.114)) / tint;
    if (v_glow != 0) {
        return vec4(obsidian_glow() * grey * kObsidianGlowPacket, c.a);
    }
    return vec4(grey * (kObsidianBody + kObsidianGlowBase * obsidian_glow()), c.a);
}

vec3 obsidian_shine(vec3 rgb) {
    if (!obsidian_surface() || dot(v_normal, v_normal) < 1e-12) {
        return rgb;
    }
    vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
    vec3 to_eye = normalize(-eye_position);
    vec3 n = normalize(mat3(view) * v_normal);
    // Double-sided, as the game draws mobys.
    n = dot(n, to_eye) < 0.0 ? -n : n;
    float facing = max(dot(n, to_eye), 0.0);
    vec3 half_way = normalize(normalize(mat3(view) * kSun) + to_eye);
    float highlight = pow(max(dot(n, half_way), 0.0), kObsidianGloss);
    vec3 inner = obsidian_glow() * (kObsidianInner * pow(facing, kObsidianInnerFalloff));
    vec3 rim = kObsidianEmber * (kObsidianRim * pow(1.0 - facing, 3.0));
    return min(rgb + inner + rim + vec3(kObsidianHighlight * highlight), vec3(1.0));
}

// A mod's energy (OPENRAC_MOBY_MATERIAL_ENERGY, interface 1.25): render-only, the moby charged
// with the mod's colour. Its body keeps its own colours with kEnergyBody of the colour (the
// instance tint holds 1 - kEnergyBody + kEnergyBody x rgb: frame.cpp's apply_mod_material), and
// after the lighting a rim glows in the colour where the surface turns from the eye
// (energy_shine). A glow packet's colour becomes the energy's at its luminance, brightened
// (energy_colour). The effects the moby makes take the colour on the CPU (energy_effects.h).
const int kModMaterialEnergy = 4;
const float kEnergyBody = 0.35;        // OPENRAC_MOBY_MATERIAL_ENERGY_BODY
const float kEnergyGlowPacket = 2.0;   // a glow packet's brightening
const float kEnergyRim = 0.65;         // the rim's strength at a grazing angle
const float kEnergyRimFalloff = 2.0;   // how fast it fades towards the middle

bool energy() {
    return v_mod_material == kModMaterialEnergy;
}

// The energy's colour, taken back out of the tint's share of it.
vec3 energy_rgb() {
    return clamp((v_mod_rgb - vec3(1.0 - kEnergyBody)) / kEnergyBody, 0.0, 1.0);
}

vec4 energy_colour(vec4 c) {
    if (!energy() || v_glow == 0) {
        return c;
    }
    // The tint's share is taken out of the glow packet's luminance first.
    float tint = max(dot(v_mod_rgb, vec3(0.299, 0.587, 0.114)), 0.05);
    float grey = dot(c.rgb, vec3(0.299, 0.587, 0.114)) / tint;
    return vec4(energy_rgb() * grey * kEnergyGlowPacket, c.a);
}

vec3 energy_shine(vec3 rgb) {
    if (!energy() || v_glow != 0 || dot(v_normal, v_normal) < 1e-12) {
        return rgb;
    }
    vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
    vec3 to_eye = normalize(-eye_position);
    vec3 n = normalize(mat3(view) * v_normal);
    // Double-sided, as the game draws mobys.
    n = dot(n, to_eye) < 0.0 ? -n : n;
    float rim = pow(1.0 - max(dot(n, to_eye), 0.0), kEnergyRimFalloff);
    return min(rgb + energy_rgb() * (kEnergyRim * rim), vec3(1.0));
}

// A mod's paint (OPENRAC_MOBY_MATERIAL_PAINT, interface 1.34): render-only, the moby repainted in
// the mod's colour. The texel becomes its own grey, its contrast raised (paint_texel), and the
// instance tint, which holds all of rgb (frame.cpp's apply_mod_material), colours it as the vertex
// colour multiplies it: the same model and texture, shaded as before, in the paint's colour. After
// the lighting a white highlight follows the sun and a rim glows in the colour by the paint's
// level (paint_shine); a glow packet takes the colour at its luminance (paint_colour).
// v_mod_material is kModMaterialPaint plus the level (scene::kModMaterialPaint), plus 16 x (1 + the
// accent's colour, 3 bits a channel) for a paint with an accent (interface 1.38,
// scene::paint_material): the model's strongly saturated texels (paint_accent_mask) are drawn in the
// accent instead of the body's colour, brighter.
const int kModMaterialPaint = 16;
const float kPaintContrast = 1.25;     // the grey's contrast around kPaintPivot
const float kPaintPivot = 0.35;
const float kPaintGain = 1.6;          // the grey's brightening before the colour multiplies it
const float kPaintGloss = 24.0;        // the highlight's exponent
const float kPaintHighlight = 0.35;    // the highlight's strength at its peak
const float kPaintRim = 0.9;           // the rim's strength at a grazing angle, at the top level
const float kPaintRimFalloff = 2.5;    // how fast it fades towards the middle
const float kPaintGlowPacket = 2.0;    // a glow packet's brightening
// The accent's mask: a texel is accent where its saturation passes kPaintAccentSat and it is not
// nearly black (kPaintAccentValue). Carbonox's grey metal has none; its visor's reds and its teal
// lamps have 0.9 and more.
const vec2 kPaintAccentSat = vec2(0.45, 0.70);
const vec2 kPaintAccentValue = vec2(0.10, 0.22);
const float kPaintAccentDim = 0.55;    // the accent's brightness at the darkest accent texel
const float kPaintAccentGlow = 1.4;    // and at the brightest: a lit glass, a little over white
const float kPaintAccentRange = 1.8;   // the texel's value that reaches kPaintAccentGlow is 1 / this
const float kPaintAccentTint = 0.05;   // the lowest body colour the accent divides by

bool paint() {
    return v_mod_material >= kModMaterialPaint;
}

float paint_level() {
    return float((v_mod_material - kModMaterialPaint) & 15) / 15.0;
}

// The accent's code of the paint: 0 for none, else 1 + r << 6 | g << 3 | b (3 bits a channel).
int paint_accent_code() {
    return (v_mod_material - kModMaterialPaint) >> 4;
}

// Returns how far `rgb` is an accent texel, 0 (grey metal, cloth) to 1 (a strongly saturated one).
float paint_accent_mask(vec3 rgb) {
    float high = max(rgb.r, max(rgb.g, rgb.b));
    float low = min(rgb.r, min(rgb.g, rgb.b));
    float saturation = (high - low) / max(high, 1e-4);
    return smoothstep(kPaintAccentSat.x, kPaintAccentSat.y, saturation)
           * smoothstep(kPaintAccentValue.x, kPaintAccentValue.y, high);
}

// Returns the paint's accent colour, 0 to 1 a channel.
vec3 paint_accent_rgb() {
    int code = paint_accent_code() - 1;
    return vec3(float((code >> 6) & 7), float((code >> 3) & 7), float(code & 7)) / 7.0;
}

vec4 paint_texel(vec4 texel) {
    if (!paint() || v_glow != 0) {
        return texel;
    }
    float grey = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    grey = clamp((grey - kPaintPivot) * kPaintContrast + kPaintPivot, 0.0, 1.0) * kPaintGain;
    vec3 body = vec3(min(grey, 1.0));
    if (paint_accent_code() > 0) {
        // The accent texel keeps its own brightness (its value), lifted; the tint colours the
        // vertex colour that multiplies it, so divide it out to leave the accent itself.
        float value = max(texel.r, max(texel.g, texel.b));
        float shine = mix(kPaintAccentDim, kPaintAccentGlow, clamp(value * kPaintAccentRange, 0.0, 1.0));
        vec3 accent = paint_accent_rgb() * shine / max(v_mod_rgb, vec3(kPaintAccentTint));
        body = mix(body, accent, paint_accent_mask(texel.rgb));
    }
    return vec4(body, texel.a);
}

vec4 paint_colour(vec4 c) {
    if (!paint() || v_glow == 0) {
        return c;
    }
    // The tint is the paint's colour: a glow packet's grey is its colour's luminance, coloured.
    float tint = max(dot(v_mod_rgb, vec3(0.299, 0.587, 0.114)), 0.05);
    float grey = dot(c.rgb, vec3(0.299, 0.587, 0.114)) / tint;
    return vec4(v_mod_rgb * grey * kPaintGlowPacket, c.a);
}

vec3 paint_shine(vec3 rgb) {
    if (!paint() || v_glow != 0 || dot(v_normal, v_normal) < 1e-12) {
        return rgb;
    }
    vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
    vec3 to_eye = normalize(-eye_position);
    vec3 n = normalize(mat3(view) * v_normal);
    // Double-sided, as the game draws mobys.
    n = dot(n, to_eye) < 0.0 ? -n : n;
    vec3 half_way = normalize(normalize(mat3(view) * kSun) + to_eye);
    float highlight = pow(max(dot(n, half_way), 0.0), kPaintGloss);
    float rim = pow(1.0 - max(dot(n, to_eye), 0.0), kPaintRimFalloff);
    vec3 glow = v_mod_rgb * (kPaintRim * paint_level() * rim);
    return min(rgb + glow + vec3(kPaintHighlight * highlight), vec3(1.0));
}

// A mod's material on the texel, the colour and the lit colour: platinum's, obsidian's, energy's
// or paint's, the value as it is for a moby with none.
vec4 mod_texel(vec4 texel) {
    return paint_texel(obsidian_texel(platinum_texel(texel)));
}

vec4 mod_colour(vec4 c) {
    return paint_colour(energy_colour(obsidian_colour(platinum_colour(c))));
}

vec3 mod_shine(vec3 rgb) {
    return paint_shine(energy_shine(obsidian_shine(platinum_shine(rgb))));
}

#ifdef OPENRAC_ENHANCED_RENDERING
// Match bake_material: repeat wrapping, decode sRGB before bilinear filtering.
// These level textures are stored as RGBA8 for the original GS colour path.
vec3 emission_texel(bool authored_tint) {
    ivec2 size = textureSize(emission_texture, 0);
    vec2 p = fract(v_uv) * vec2(size) - 0.5;
    ivec2 at = ivec2(floor(p));
    vec2 f = fract(p);
    vec3 result = vec3(0.0);
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            ivec2 q = ((at + ivec2(x, y)) % size + size) % size;
            float weight = (x == 0 ? 1.0 - f.x : f.x) * (y == 0 ? 1.0 - f.y : f.y);
            vec3 srgb = texelFetch(emission_texture, q, 0).rgb;
            if (authored_tint) {
                srgb = clamp(srgb * v_emission_colour, vec3(0.0), vec3(1.0));
            }
            result += probe_linear(srgb) * weight;
        }
    }
    return result;
}

vec3 authored_emission() {
    vec3 emitted = vec3(0.0);
    if (any(greaterThan(material_emission, vec3(0.0)))) {
        vec3 radiance = emission_texel(emission_vertex_colour != 0);
        if (material_emission_contrast > 1.) {
            float luminance = dot(radiance, vec3(.2126, .7152, .0722));
            radiance *= pow(max(luminance, 0.) / .18, material_emission_contrast - 1.);
        }
        emitted += material_emission * radiance;
    }
    return emitted;
}

vec3 surface_emission(vec4 texel) {
    // Scripted brightness follows the original encoded GS modulation, then decodes once.
    return probe_linear(clamp(texel.rgb * v_script_emission, vec3(0.0), vec3(1.0)))
        + authored_emission();
}

uniform int probe_look_count;
uniform vec4 probe_look[24]; // Twelve boxes: minimum/feather and maximum/diffuse gain.

vec3 apply_probe_look(vec3 diffuse, vec3 emitted) {
    float gain = 1.0;
    for (int i = 0; i < probe_look_count; ++i) {
        vec4 low = probe_look[i * 2];
        vec4 high = probe_look[i * 2 + 1];
        vec3 edge = min(v_world_position - low.xyz, high.xyz - v_world_position);
        float weight = smoothstep(0.0, low.w, min(edge.x, min(edge.y, edge.z)));
        gain *= mix(1.0, high.w, weight);
    }
    return diffuse * gain + emitted;
}

uniform int object_reflection_enabled;
uniform vec3 object_reflection_material; // Perceptual roughness, F0, reflection strength.
uniform vec4 object_reflection_tint; // Linear tint and authored-opacity multiplier.
uniform vec3 material_dielectric; // Roughness (zero disables), F0, specular strength.
uniform vec3 material_dielectric_tint;
uniform float material_metallic; // Zero inherits original optics; authored metallic is stored plus one.
vec4 thin_glass_surface();
float thin_glass_coverage();

// probes.glsl is inserted after the version line by probe_mesh_fragment().
#ifdef OPENRAC_EARLY_LIGHTING
uniform int gameplay_rim_enabled;
vec3 gameplay_rim(vec3 indirect, vec3 normal_view, vec3 toward_camera);
bool early_surface(
    vec3 eye_position, vec3 normal_view, out vec4 result, out bool covered,
    out float hero_visibility, out float cloudy_sun
);
#endif
// This replaces RGB only. Alpha tests, fading and blend equations stay in
// the existing path, as do explicitly unlit, additive, metal and glow passes.
#if defined(OPENRAC_HERO_WETNESS) && !defined(OPENRAC_STATIC_COLOUR)
// A thin wet coat: diffuse darkening, direct GGX highlights and local environment reflections.
vec3 wet_coat(vec3 linear_colour, vec3 n, vec3 toward_camera, float visibility) {
    float wet = clamp(v_hero_wetness, 0., 1.);
    if (hero_wetness_enabled == 0 || wet <= 0.) return linear_colour;
    int a = (int(v_probe_selector.x + .5) & 15) * 4;
    int b = (int(v_probe_selector.y + .5) & 15) * 4;
    float blend = clamp(v_probe_selector.z, 0., 1.);
    vec3 travel = probe_sunlight_direction.w != 0. ? probe_sunlight_direction.xyz
        : mix(light_sets[a + 1].xyz, light_sets[b + 1].xyz, blend);
    vec3 light = probe_unit(-travel), eye = probe_unit(toward_camera);
    float roughness = mix(hero_wetness_material.z, hero_wetness_material.w, wet);
    vec3 radiance = max(mix(light_sets[a].rgb, light_sets[b].rgb, blend), vec3(0.))
        * probe_sunlight_scale;
#ifdef OPENRAC_CLOUD_SHADOWS
    radiance *= cloud_sun_attenuation;
#endif
    vec3 shine = radiance * ggx_specular(n, eye, light, roughness, hero_wetness_f0) * visibility;
#ifdef OPENRAC_FORWARD_PLUS
    // Local lights are unshadowed; a directional shadow must not erase wet point/spot highlights.
    LOCAL_LIGHT_LOOP(i) {
        vec4 position = LOCAL_POSITION(i);
        vec3 delta = position.xyz - v_world_position;
        float distance = length(delta);
        if (position.w <= 0. || distance >= position.w || distance <= 1e-6) continue;
        vec3 local_radiance = max(LOCAL_COLOUR(i).rgb, vec3(0.))
            * LOCAL_ATTENUATION(distance,position.w) * LOCAL_CONE(i, delta / distance);
        shine += local_radiance * ggx_specular(n, eye, delta / distance, roughness, hero_wetness_f0);
    }
#endif
#if !defined(OPENRAC_SOLID) && !defined(OPENRAC_STATIC_COLOUR)
    // Captures contain their own lighting; the current sun shadow must not erase them.
    if (dot(n, eye) > 0.) {
        shine += environment_specular(v_world_position, n, eye, roughness, hero_wetness_f0, vec3(0.));
    }
#endif
    shine *= hero_wetness_material.y;
    return linear_colour * mix(1., hero_wetness_material.x, wet) + wet * shine;
}
#else
vec3 wet_coat(vec3 linear_colour,vec3 n,vec3 toward_camera,float visibility) {return linear_colour;}
#endif

// The original ambient as the floor of a moby's diffuse GI (indirect / pi). The ambient is a
// multiplier of gamma-encoded texels, so its linear equivalent is its 2.2 power; the GI's missing
// luminance is added in the ambient's hue, so brighter GI (sky, bounces, emitters) is kept as baked.
vec3 retail_ambient_floor(vec3 indirect, vec3 ambient) {
    vec3 floor_linear = pow(clamp(ambient, vec3(0.0), vec3(2.0)), vec3(2.2));
    const vec3 luma = vec3(0.2126, 0.7152, 0.0722);
    float wanted = dot(floor_linear, luma);
    float deficit = wanted - dot(indirect, luma) / 3.14159265359;
    if (wanted <= 1e-4 || deficit <= 0.0) {
        return indirect;
    }
    return indirect + floor_linear * (deficit / wanted) * 3.14159265359;
}

vec3 standard_surface(vec3 diffuse, vec3 albedo, vec3 indirect, vec3 n, vec3 toward_camera,
                      vec4 selector, float visibility, vec3 receiver_normal) {
    bool metal_packet = metal_pass != 0;
    vec3 optics = metal_packet ? object_reflection_material : material_dielectric;
    if ((!metal_packet && probe_mode != 1) || translucent_pass != 0 || effect_pass != 0
        || optics.x <= 0.) {
        return diffuse;
    }
    vec3 eye = probe_unit(toward_camera);
    float metallic = max(material_metallic - 1., 0.);
    // Reflectivity supplies the measured F0 magnitude; artwork modulates the metal's hue.
    vec3 f0 = optics.y * mix(vec3(1.), clamp(albedo, 0., 1.), metallic);
    float roughness = optics.x;
    float nv = dot(n, eye);
    vec3 base = diffuse * (1. - metallic) * (vec3(1.) - surface_fresnel(nv, f0));
    if (optics.z <= 0.) {
        return base;
    }
    vec3 broad = max(indirect, vec3(0.)) / 3.14159265359 * surface_fresnel(nv, f0);
    vec3 specular = environment_specular_surface_frame(v_world_position, v_world_position + receiver_normal * .5, receiver_normal, n, eye, nv, roughness, f0, broad)
        + surface_specular_direct(v_world_position, n, eye, selector, visibility, roughness, f0);
    float grazing_tint = .5 * dielectric_fresnel(nv, 0.)
        * smoothstep(.35, .65, roughness) * (1. - metallic);
    vec3 reflection_colour = mix(vec3(1.), clamp(albedo, 0., 1.), grazing_tint);
    vec3 tint = metal_packet ? object_reflection_tint.rgb : material_dielectric_tint;
    return base + specular * reflection_colour * tint * optics.z;
}

vec3 probe_surface(vec4 texel, vec3 original) {
#ifdef OPENRAC_CLOUD_SHADOWS
    cloud_sun_attenuation = 1.0;
#endif
#ifdef OPENRAC_HERO_FUR
    if (hero_fur_debug > 0 && hero_fur_debug < 3 && v_hero_fur != 0 && v_glow == 0 && probe_effect == 0 && metal_pass == 0) {
        return fur_diagnostic(v_uv);
    }
#endif
    if (v_glow != 0 || probe_effect != 0
        || (probe_receiver == 2 && v_probe_selector.w > 1.5)) {
        return original;
    }
    // Apply authored appearance over retail lighting when there is no probe receiver.
    if ((material_fallback & 4) != 0) return original;
    if ((probe_mode == 0 || probe_receiver == 0) && material_fallback != 0) {
        vec3 diffuse = probe_linear(original);
        if ((material_fallback & 1) != 0) diffuse *= material_diffuse_scale;
        original = probe_display(diffuse + ((material_fallback & 2) != 0 ? authored_emission() : vec3(0.)));
    }
#ifdef OPENRAC_FOLIAGE
    vec4 leaf = foliage_material();
    bool foliage = leaf.w > 0.;
#else
    const bool foliage = false;
#endif
    if (probe_receiver == 0 && !foliage) return original;
#if defined(OPENRAC_HERO_WETNESS) && !defined(OPENRAC_STATIC_COLOUR)
    if (hero_wetness_enabled != 0 && hero_wetness_debug != 0 && v_hero_wetness >= 0.) {
        return mix(vec3(.12), vec3(0., .65, 1.), clamp(v_hero_wetness, 0., 1.));
    }
#endif
    if (probe_mode == 0 && metal_pass == 0 && !foliage && sun_shadow_enabled == 0 && hero_shadow_enabled == 0
#ifdef OPENRAC_HERO_FUR
        && !(v_hero_fur != 0 && (hero_fur_debug == 3 || hero_fur_lighting_enabled != 0))
#endif
    ) {
        return original;
    }
    vec3 geometric = cross(dFdx(v_world_position), dFdy(v_world_position));
    vec3 n = probe_unit(dot(v_normal, v_normal) > 1e-12 ? v_normal : geometric);
    if (dot(n, n) < 0.5) {
        return original;
    }
    // Match the prepass's two-sided smooth normal, including its grazing-angle constraint.
    vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
    vec3 eye_normal = orient_shading_normal(mat3(view) * n, mat3(view) * geometric, -eye_position);
    n = transpose(mat3(view)) * eye_normal;
#ifdef OPENRAC_PROBE_COVERAGE
    // Only the coverage view's shaders carry this second probe lookup and its registers.
    if (probe_mode == 4) {
        vec3 ignored;
        float dense_weight;
        vec3 bias = probe_surface_bias(n, transpose(mat3(view)) * (-eye_position));
        bool covered = sample_probe_irradiance(v_world_position, n, bias, ignored, dense_weight);
        return covered ? mix(vec3(0.0, 0.5, 1.0), vec3(0.0, 1.0, 0.0), dense_weight)
                       : vec3(1.0, 0.0, 1.0);
    }
#endif
    vec3 surface_n = n;
    float fur_grain = 1.;
#ifdef OPENRAC_HERO_FUR
    FurSurfaceData fur_surface = FurSurfaceData(n, vec3(0.), transpose(mat3(view)) * (-eye_position), 0., 1.);
    if (v_hero_fur != 0 && (hero_fur_detail_enabled != 0 || hero_fur_lighting_enabled != 0)) {
        float wetness = 0.;
#ifdef OPENRAC_HERO_WETNESS
        wetness = v_hero_wetness;
#endif
        fur_surface = fur_surface_data(n, v_world_position, v_uv, transpose(mat3(view)) * (-eye_position), wetness);
        surface_n = fur_surface.normal;
        fur_grain = fur_surface.grain;
    }
    if (hero_fur_debug == 3 && v_hero_fur != 0) {
        return surface_n * .5 + .5;
    }
#endif
    bool cached_surface = false;
    float cloudy_sun = 1.0;
#ifdef OPENRAC_EARLY_LIGHTING
    bool cached_coverage;
    vec4 cached_lighting;
    float hero_visibility;
    cached_surface = early_surface(
        eye_position, mat3(view) * n, cached_lighting, cached_coverage, hero_visibility, cloudy_sun
    );
#ifdef OPENRAC_HERO_FUR
    // Coat slices lie above the body prepass and require lighting at their displaced position.
    if (hero_fur_shell_enabled != 0 && hero_fur_shell > 0.) {
        cached_surface = false;
    }
#endif
    if (!cached_surface) {
        cached_lighting.a = sun_shadow_visibility(v_world_position, n, n);
        hero_visibility = hero_shadow_visibility(v_world_position, n);
    }
    if (!hero_shadow_applies(n)) {
        hero_visibility = 1.0;
    }
    float sun_visibility = cached_lighting.a;
#else
    float sun_visibility = sun_shadow_visibility(v_world_position, n, n);
    float hero_visibility = hero_shadow_visibility(v_world_position, n);
#endif
#ifdef OPENRAC_CLOUD_SHADOWS
    if (cloud_shadow_shape.w > 0.0 && effect_pass == 0) {
        if (!cached_surface) {
            cloud_sun_attenuation = cloud_shadow_visibility(v_world_position, sun_visibility);
        } else if (sun_visibility > 1.0 / 255.0) {
            cloud_sun_attenuation = clamp(cloudy_sun / sun_visibility, 0.0, 1.0);
        }
    }
#endif
    // Either projection casts a normal shadow in sunlight, including beyond cascade coverage.
    float visibility = min(sun_visibility, hero_visibility);
    // Only the airborne footprint adds a platforming cue inside existing cascade shade.
    float hero_cue = 1.0;
    if (hero_shadow_receiver == 0) {
        hero_cue = 1.0 - 0.45 * hero_shadow_air_blend * (1.0 - sun_visibility) * (1.0 - hero_visibility);
    }
    vec3 albedo = probe_linear(texel.rgb) * material_colour.rgb * v_probe_multiplier.rgb;
    if (probe_terrain_albedo > 0.0) {
        // Preserve the GS modulation as an artistic material tint, then decode once.
        vec3 tint = mix(vec3(1.0), v_colour.rgb, probe_terrain_albedo);
        albedo = probe_linear(clamp(texel.rgb * tint * material_colour.rgb, 0.0, 1.0));
    }
    vec3 emitted = surface_emission(texel);
    vec3 fur_direct = vec3(0.);
#ifdef OPENRAC_HERO_FUR
    if (v_hero_fur != 0 && hero_fur_lighting_enabled != 0 && probe_mode <= 3 && probe_mode != 2) {
        vec4 fur_selector = probe_receiver == 1 ? vec4(0.) : v_probe_selector;
        fur_direct = fur_direct_lighting(fur_surface, v_world_position, fur_selector, visibility, albedo);
    }
#endif
    // Without a probe asset, retain the original authored lighting and apply
    // the same shadow floor. This also covers holes in probe coverage.
    // Legacy prelighting may already contain authored brightness. Preserve that
    // component while shadowing the remaining diffuse, without adding it twice.
    vec3 fallback_diffuse = max(probe_linear(original) - emitted, vec3(0.0))
        * fur_grain * probe_fallback_diffuse_shadow(visibility) * hero_cue;
    vec3 metal_ambient = probe_linear(clamp(v_probe_ambient, 0., 1.));
    vec3 toward_camera = transpose(mat3(view)) * (-eye_position);
    vec3 receiver_normal = dot(geometric, geometric) > 0. ? normalize(geometric) : n;
    if (dot(receiver_normal, toward_camera) < 0.) receiver_normal = -receiver_normal;
    vec4 selector = probe_receiver == 1 ? vec4(0.) : v_probe_selector;
    if (probe_mode == 0) {
        if (metal_pass != 0) {
            return probe_display(standard_surface(
                fallback_diffuse, albedo, metal_ambient * 3.14159265359,
                surface_n, toward_camera, selector, visibility, receiver_normal) + emitted);
        }
        vec3 fallback = wet_coat(fallback_diffuse, surface_n, toward_camera, visibility)
            + fur_direct + emitted;
#ifdef OPENRAC_FOLIAGE
        if (foliage) {
            vec4 foliage_selector = probe_receiver == 2 ? v_probe_selector : vec4(0.);
            vec3 direct = foliage_surface(v_world_position, n, toward_camera,
                albedo, vec3(0.), foliage_selector, visibility, leaf, v_uv);
            return probe_display(foliage_debug != 0 ? direct : max(fallback, direct + emitted));
        }
#endif
#ifdef OPENRAC_HERO_FUR
        if (v_hero_fur != 0 && hero_fur_lighting_enabled != 0) {
            fallback += fur_environment_lighting(fur_surface, v_world_position, vec3(0.), albedo);
        }
#endif
        return probe_display(fallback);
    }
    vec3 indirect;
    // Use the same smooth receiver normal as the depth/normal prepass and early lighting.
    vec3 surface_bias = probe_surface_bias(n, toward_camera);
    bool covered;
#ifdef OPENRAC_EARLY_LIGHTING
    if (cached_surface) {
        indirect = cached_lighting.rgb;
        covered = cached_coverage;
    } else
#endif
    {
        covered = sample_probe_irradiance(v_world_position, n, surface_bias, indirect);
    }
    // Diagnostic views must not silently substitute original lighting:
    // indirect is black without coverage, while direct is independent of GI.
    if (!covered && probe_mode == 2) {
        return vec3(0.0);
    }
    if (!covered && probe_mode != 3) {
        // Direct lights and reflection captures have their own coverage. Only the broad
        // irradiance-based reflection fallback is unavailable here.
        vec3 surface = standard_surface(fallback_diffuse, albedo, vec3(0.), surface_n, toward_camera,
                                        selector, visibility, receiver_normal);
        vec3 fallback = wet_coat(surface, surface_n, toward_camera, visibility) + fur_direct + emitted;
#ifdef OPENRAC_HERO_FUR
        // A local capture can illuminate the fur even where diffuse probe coverage is absent.
        if (v_hero_fur != 0 && hero_fur_lighting_enabled != 0) {
            fallback += fur_environment_lighting(fur_surface, v_world_position, vec3(0.), albedo);
        }
#endif
        return probe_display(fallback);
    }
    // Artistic GI exposure; keep the bake and direct-light energy unchanged.
    indirect *= probe_gi_multiplier;
    if (probe_retail_anchor != 0 && probe_receiver == 2 && probe_mode == 1) {
        indirect = retail_ambient_floor(indirect, v_probe_ambient);
    }
    // Before occlusion: anchored terrain compares with the same point unshadowed and unoccluded.
    vec3 open_indirect = indirect;
    // Only opaque diffuse GI receives AO. Blended/effect surfaces do not have
    // a matching prepass depth; direct light, emission and metal stay intact.
#ifndef OPENRAC_EARLY_LIGHTING
    if (gtao_enabled != 0 && translucent_pass == 0 && effect_pass == 0) {
        ivec2 pixel = clamp(ivec2(gl_FragCoord.xy), ivec2(0), textureSize(gtao_visibility, 0) - 1);
        indirect *= texelFetch(gtao_visibility, pixel, 0).r;
    }
#endif
#ifdef OPENRAC_FOLIAGE
    if (foliage) {
        vec3 colour = foliage_surface(v_world_position, n, toward_camera, albedo, indirect,
            selector, visibility, leaf, v_uv);
        return probe_display(probe_mode == 1 && foliage_debug == 0
                                 ? apply_probe_look(colour, emitted) : colour);
    }
#endif
    // Fine fur changes material response; shadow tests, probe lookup bias and cached GI keep smooth normals.
    vec3 directional, local, cloud_loss;
    probe_direct_parts(v_world_position, surface_n, selector, directional, local, cloud_loss);
    vec3 direct = probe_shadowed_directional(directional, cloud_loss, visibility, 0.1) + local;
    vec3 illumination = probe_mode == 2
                            ? indirect / 3.14159265359
                            : (probe_mode == 3 ? direct : direct + indirect / 3.14159265359);
    // Anchored terrain: its vertex colours already hold the original lighting, sunlight included, so
    // it takes the enhanced lighting as a ratio to the same point in unshadowed sunlight. Sunlit
    // ground keeps its original brightness instead of taking the sun twice; shade, GI and occlusion
    // darken or tint it from there. The original lighting has no cast shadows, so cast shade keeps
    // about a third of the sun (0.1 + 0.9 x 0.28) instead of a tenth: the shapes stay, the level's
    // brightness stays near the original's.
    if (probe_retail_anchor != 0 && probe_receiver == 1 && probe_terrain_albedo > 0.0
        && probe_mode == 1) {
        // The same lights at two visibilities, from the one loop above.
        vec3 open = directional + local + open_indirect / 3.14159265359;
        vec3 shaded = probe_shadowed_directional(
            directional, cloud_loss, visibility, mix(0.1, 1.0, 0.28)
        ) + local + indirect / 3.14159265359;
        illumination = (shaded + 0.02) / (open + 0.02);
    }
    if (probe_mode != 2) {
        illumination *= hero_cue;
    }
    albedo *= fur_grain;
    vec3 fur_indirect = vec3(0.);
#ifdef OPENRAC_HERO_FUR
    if (v_hero_fur != 0 && hero_fur_lighting_enabled != 0 && (probe_mode == 1 || probe_mode == 2)) {
        fur_indirect = fur_environment_lighting(fur_surface, v_world_position, indirect, albedo);
    }
#endif
    vec3 rim = vec3(0.0);
#ifdef OPENRAC_EARLY_LIGHTING
    // Essential opaque world objects pick up their surroundings at grazing angles.
    // Diagnostic views, UI, effects and receivers without early-lit GI keep their existing result.
    if (gameplay_rim_enabled != 0 && probe_receiver == 2 && probe_mode == 1 && cached_surface
        && covered && translucent_pass == 0 && effect_pass == 0) {
        rim = gameplay_rim(indirect, eye_normal, -eye_position);
    }
#endif
    vec3 diffuse = albedo * illumination * material_diffuse_scale;
    vec3 surface = standard_surface(diffuse, albedo, indirect, surface_n, toward_camera, selector, visibility, receiver_normal);
    // Emission is independent of GI exposure, AO and shadow visibility.
    vec3 lit = wet_coat(surface + rim, surface_n, toward_camera, visibility)
        + fur_direct + fur_indirect;
    return probe_display(probe_mode == 1 ? apply_probe_look(lit, emitted) : lit);
}

#else
vec3 probe_surface(vec4 texel, vec3 original) {
    return original;
}
#endif

// OPENRAC_UPSCALED_TEXTURE_COLOURS_SOURCE

void main() {
#if defined(OPENRAC_HERO_FUR) && !defined(OPENRAC_SOLID)
    if (fur_shell_reject(hero_fur_map, v_uv, v_hero_fur)) discard;
#endif
    v_fog = v_fog_w.x / v_fog_w.y;
#ifdef OPENRAC_SOLID
    // The static solid variant contains no discard. The Vulkan colour batch draws its solid
    // geometry with it too, with reused EQUAL depth and depth writes disabled.
    vec4 texel = mod_texel(sample_upscaled_texture_colours(v_uv));
    // FP32 inputs and only two multiplies: narrowing adds packing cost on the measured GPU.
    vec4 c = mod_colour(texel * v_colour * material_colour);
    float light = 1.0;
    if (lighting != 0 && v_lit == 0) {
        light = float(mp_float(0.55) + mp_float(0.45) * abs(dot(mp_vec3(normalize(v_normal)), mp_vec3(kSun))));
    }
    vec3 rgb = min(c.rgb * light, vec3(1.0));
    rgb = mod_shine(probe_surface(texel, rgb));
#ifdef OPENRAC_ATMOSPHERE
    if (atmosphere_enabled != 0 && v_probe_selector.w < 1.5) {
        rgb = atmosphere_apply(rgb, v_world_position, true);
    } else
#endif
    if (fog_colour.w > 0.5) {
        rgb = vec3(mix(mp_vec3(fog_colour.rgb), mp_vec3(rgb), mp_float(v_fog)));
    }
    out_colour = vec4(rgb, 1.0);
#endif
#ifndef OPENRAC_SOLID
#ifdef OPENRAC_ENHANCED_RENDERING
    if (object_thin_glass == 1 && metal_pass == 0) {
        out_colour = thin_glass_surface();
        out_colour.a *= thin_glass_coverage();
        return;
    }
#endif
#ifndef OPENRAC_COLOUR_SPECIALISED
    if (metal_pass != 0) {
        vec4 m;
        vec3 mrgb;
#ifdef OPENRAC_ENHANCED_RENDERING
        if (object_thin_glass == 2) discard;
        if (object_thin_glass == 1) {
            m = thin_glass_surface();
            mrgb = m.rgb;
            m.a *= thin_glass_coverage();
        } else
#endif
        {
            vec4 metal = sample_upscaled_texture_colours(v_uv);
            m = vec4(vec3(mp_vec3(metal.rgb) * mp_vec3(v_colour.rgb)), metal.a * v_colour.a);
            mrgb = min(m.rgb, vec3(1.0));
#if defined(OPENRAC_ENHANCED_RENDERING) && !defined(OPENRAC_COLOUR_SPECIALISED)
            bool standard_metal = probe_receiver == 2 && v_probe_selector.w < 1.5
                && object_reflection_enabled != 0
                && (probe_mode == 1 || (probe_mode == 0 && object_reflection_enabled == 2));
            if (standard_metal) {
                mrgb = probe_surface(metal, mrgb);
            }
#endif
#ifdef OPENRAC_EARLY_LIGHTING
            // Chrome blends over the diffuse pass: give both layers the edge reflection so the
            // overlay's opacity cannot erase it. Use cached GI without another probe lookup.
            if (!standard_metal && gameplay_rim_enabled != 0 && probe_receiver == 2 && probe_mode == 1
                && v_probe_selector.w < 1.5) {
                vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
                vec3 geometric = cross(dFdx(eye_position), dFdy(eye_position));
                vec3 normal_view = orient_shading_normal(mat3(view) * v_normal, geometric, -eye_position);
                vec4 cached;
                bool covered;
                float hero_visibility;
                float cloudy_sun;
                if (early_surface(eye_position, normal_view, cached, covered, hero_visibility, cloudy_sun)
                    && covered) {
                    vec3 rim = gameplay_rim(cached.rgb * probe_gi_multiplier, normal_view, -eye_position);
                    mrgb = probe_display(probe_linear(mrgb) + rim);
                }
            }
#endif
        }
#ifdef OPENRAC_ATMOSPHERE
        if (atmosphere_enabled != 0 && v_probe_selector.w < 1.5) {
            mrgb = atmosphere_apply(mrgb, v_world_position, true);
        } else
#endif
        if (fog_colour.w > 0.5) {
            mrgb = vec3(mix(mp_vec3(fog_colour.rgb), mp_vec3(mrgb), mp_float(v_fog)));
        }
        out_colour = vec4(mrgb, clamp(m.a, 0.0, 1.0));
#ifdef OPENRAC_ENHANCED_RENDERING
        if (object_thin_glass == 1) out_colour.rgb *= out_colour.a;
#endif
        return;
    }
#endif
    if (effect_pass == 0 && v_glow != glow_pass) {
        discard;
    }
    vec4 texel = mod_texel(sample_upscaled_texture_colours(v_uv));
    // FP32 inputs and only two multiplies: narrowing adds packing cost on the measured GPU.
    vec4 c = mod_colour(texel * v_colour * material_colour);
#ifndef OPENRAC_COLOUR_SPECIALISED
    if (opacity_split != 0 && (c.a >= 1.0) != (opacity_split > 0)) {
        discard;
    }
    if (effect_pass != 0) {
        // An added moby skips the first half (it writes no depth), so the second draws all of it.
        if (glow_split != 0 && effect_additive == 0
            && (c.a >= effect_reference) != (glow_split == 1)) {
            discard;
        }
        vec3 e = min(c.rgb, vec3(1.0));
        e = mod_shine(probe_surface(texel, e));
    #ifdef OPENRAC_ATMOSPHERE
        if (atmosphere_enabled != 0 && v_probe_selector.w < 1.5) {
            e = atmosphere_apply(e, v_world_position, true);
        } else
#endif
        if (fog_colour.w > 0.5) {
            e = vec3(mix(mp_vec3(fog_colour.rgb), mp_vec3(e), mp_float(v_fog)));
        }
        if (effect_additive != 0) {
            out_colour = vec4(min(e * clamp(c.a, 0.0, 255.0 / 128.0), vec3(1.0)), 0.0);
            return;
        }
        out_colour = vec4(e, clamp(c.a, 0.0, 1.0));
        return;
    }
    // The glow pass blends every texel at its alpha (the Gadgetron hologram's lettering is mostly
    // part-transparent texels); only the solid pass leaves a cutout's part-transparent texels out.
    if (glow_pass != 0 && glow_split != 0 && (c.a >= kWorldAlphaReference) != (glow_split == 1)) {
        discard;
    }
#endif
    if (cutout != 0 && translucent_pass == 0 && glow_pass == 0 && c.a < 0.99) {
        discard;
    }
    if (translucent_pass != 0 && (cutout == 0 || c.a >= 0.99 || c.a < 0.02)) {
        discard;
    }
    float light = 1.0;
    if (lighting != 0 && v_lit == 0) {
        // Double-sided: a face lit from behind is lit too.
        light = float(mp_float(0.55) + mp_float(0.45) * abs(dot(mp_vec3(normalize(v_normal)), mp_vec3(kSun))));
    }
    vec3 rgb = min(c.rgb * light, vec3(1.0));
    rgb = mod_shine(probe_surface(texel, rgb));
    // The GS's fog, after the texture: RGB only.
#ifdef OPENRAC_ATMOSPHERE
    if (atmosphere_enabled != 0 && v_probe_selector.w < 1.5) {
        rgb = atmosphere_apply(rgb, v_world_position, true);
    } else
#endif
    if (fog_colour.w > 0.5) {
        rgb = vec3(mix(mp_vec3(fog_colour.rgb), mp_vec3(rgb), mp_float(v_fog)));
    }
    out_colour = vec4(rgb, v_glow != 0 || translucent_pass != 0 ? clamp(c.a, 0.0, 1.0) : 1.0);
#endif
}
