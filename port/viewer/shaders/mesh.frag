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
in vec3 v_script_emission;
in vec3 v_emission_colour;
// GS F / 255 times the vertex's clip w, and that w (mesh.vert): their ratio is F interpolated in
// screen space, as the GS does.
in vec2 v_fog_w;
float v_fog;

uniform sampler2D tex_T0;
uniform vec4 material_colour;
uniform int cutout;
uniform int lighting;
uniform int glow_pass;  // mobys: 0 the solid faces, 1 the glow faces (blended)
// The glow pass in two draws, as the GS's TEST 0x5360b splits it (alpha GEQUAL 0x60: Z written, else
// colour only): 1 the texels at or above the reference, with depth; 2 the others, without; 0 all.
uniform int glow_split;
const float kWorldAlphaReference = 96.0 / 128.0;
// Cutout materials in two passes, as the GS draws them (alpha test GEQUAL 0x80, failing pixels
// written as colour only): 0 the opaque texels, with depth; 1 the translucent ones, blended.
uniform int translucent_pass;
// Mobys MobyProc blends (an explosion's flash, a fading pickup): every face and texel, its alpha
// the texel's times the moby's (the tint), blended by the pass.
uniform int effect_pass;
// The additive ones (mode bit 0x200, Cs * As + Cd): the shader weighs Cs by As itself, as the GS
// reaches about 2.0 (0xFF) where GL's SRC_ALPHA stops at 1; drawn with ONE, ONE.
uniform int effect_additive;
// The metal pass: the chrome or glass texel times the vertex colour, its alpha the texel's times the
// shine, blended over the moby (mesh.vert; ALPHA 0x44 as the moby's own).
uniform int metal_pass;
uniform vec4 fog_colour;  // FOGCOL; w = 1: fog on
uniform mat4 view;
#ifdef OPENRAC_ENHANCED_RENDERING
uniform int gtao_enabled;
uniform vec3 material_emission;
uniform sampler2D emission_texture;
uniform int emission_vertex_colour;
uniform float material_emission_contrast;
uniform float material_diffuse_scale;
uniform sampler2D gtao_visibility;
#endif

#ifdef OPENRAC_HERO_WETNESS
in float v_hero_wetness;
uniform int hero_wetness_debug;
uniform vec4 hero_wetness_material; // wet diffuse, specular strength, dry/wet roughness
uniform float hero_wetness_f0;
#endif

layout(location = 0) out vec4 out_colour;

// From above and to one side, in game axes (Z up).
const vec3 kSun = vec3(0.36, 0.48, 0.80);

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

vec3 surface_emission(vec4 texel) {
    // Scripted brightness follows the original encoded GS modulation, then decodes once.
    vec3 emitted = probe_linear(clamp(texel.rgb * v_script_emission, vec3(0.0), vec3(1.0)));
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

uniform int object_reflection_enabled;
uniform vec3 object_reflection_material; // Perceptual roughness, F0, reflection strength.
uniform vec4 object_reflection_tint; // Linear tint and authored-opacity multiplier.
vec4 chrome_surface(vec3 original);

// probes.glsl is inserted after the version line by probe_mesh_fragment().
#ifdef OPENRAC_EARLY_LIGHTING
uniform int gameplay_rim_enabled;
vec3 gameplay_rim(vec3 indirect, vec3 normal_view, vec3 toward_camera);
bool early_surface(
    vec3 eye_position,
    vec3 normal_view,
    out vec4 result,
    out bool covered,
    out float hero_visibility
);
#endif
// This replaces RGB only. Alpha tests, fading and blend equations stay in
// the existing path, as do explicitly unlit, additive, metal and glow passes.
#ifdef OPENRAC_HERO_WETNESS
// A thin wet coat: diffuse darkening, direct GGX highlights and local environment reflections.
vec3 wet_coat(vec3 linear_colour, vec3 n, vec3 toward_camera, float visibility) {
    float wet = clamp(v_hero_wetness, 0., 1.);
    if (wet <= 0.) return linear_colour;
    int a = (int(v_probe_selector.x + .5) & 15) * 4;
    int b = (int(v_probe_selector.y + .5) & 15) * 4;
    float blend = clamp(v_probe_selector.z, 0., 1.);
    vec3 travel = probe_sunlight_direction.w != 0. ? probe_sunlight_direction.xyz
        : mix(light_sets[a + 1].xyz, light_sets[b + 1].xyz, blend);
    vec3 light = probe_unit(-travel), eye = probe_unit(toward_camera);
    float roughness = mix(hero_wetness_material.z, hero_wetness_material.w, wet);
    vec3 radiance = max(mix(light_sets[a].rgb, light_sets[b].rgb, blend), vec3(0.))
        * probe_sunlight_scale;
    vec3 shine = radiance * ggx_specular(n, eye, light, roughness, hero_wetness_f0) * visibility;
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

vec3 probe_surface(vec4 texel, vec3 original) {
#ifdef OPENRAC_HERO_FUR
    if (hero_fur_debug > 0 && hero_fur_debug < 3 && v_hero_fur != 0 && v_glow == 0 && probe_effect == 0 && metal_pass == 0) {
        return fur_diagnostic(v_uv);
    }
#endif
    if (probe_receiver == 0 || v_glow != 0 || probe_effect != 0 || metal_pass != 0
        || (probe_receiver == 2 && v_probe_selector.w > 1.5)) {
        return original;
    }
#ifdef OPENRAC_HERO_WETNESS
    if (hero_wetness_debug != 0 && v_hero_wetness >= 0.) {
        return mix(vec3(.12), vec3(0., .65, 1.), clamp(v_hero_wetness, 0., 1.));
    }
#endif
    if (probe_mode == 0 && sun_shadow_enabled == 0 && hero_shadow_enabled == 0
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
#ifdef OPENRAC_EARLY_LIGHTING
    bool cached_coverage;
    vec4 cached_lighting;
    float hero_visibility;
    bool cached_surface = early_surface(
        eye_position, mat3(view) * n, cached_lighting, cached_coverage, hero_visibility
    );
    if (!hero_shadow_applies(n)) {
        hero_visibility = 1.0;
    }
    float sun_visibility = cached_lighting.a;
#else
    float sun_visibility = sun_shadow_visibility(v_world_position, n, n);
    float hero_visibility = hero_shadow_visibility(v_world_position, n);
#endif
    // Either projection casts a normal shadow in sunlight, including beyond cascade coverage.
    float visibility = min(sun_visibility, hero_visibility);
    // Only the airborne footprint adds a platforming cue inside existing cascade shade.
    float hero_cue = 1.0;
    if (hero_shadow_receiver == 0) {
        hero_cue = 1.0 - 0.45 * hero_shadow_air_blend * (1.0 - sun_visibility) * (1.0 - hero_visibility);
    }
    vec3 albedo = probe_linear(texel.rgb) * material_colour.rgb * v_probe_multiplier.rgb;
    if (probe_terrain_albedo != 0) {
        // Preserve the GS modulation as an artistic material tint, then decode once.
        albedo = probe_linear(clamp(texel.rgb * v_colour.rgb * material_colour.rgb, 0.0, 1.0));
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
    vec3 fallback = wet_coat(
        max(probe_linear(original) - emitted, vec3(0.0)) * fur_grain * mix(0.1, 1.0, visibility) * hero_cue,
        surface_n, transpose(mat3(view)) * (-eye_position), visibility
    ) + fur_direct + emitted;
    if (probe_mode == 0) {
#ifdef OPENRAC_HERO_FUR
        if (v_hero_fur != 0 && hero_fur_lighting_enabled != 0) {
            fallback += fur_environment_lighting(fur_surface, v_world_position, vec3(0.), albedo);
        }
#endif
        return probe_display(fallback);
    }
    vec3 indirect;
    // Use the same smooth receiver normal as the depth/normal prepass and early lighting.
    vec3 toward_camera = transpose(mat3(view)) * (-eye_position);
    vec3 surface_bias = probe_surface_bias(n, toward_camera);
#ifdef OPENRAC_EARLY_LIGHTING
    indirect = cached_lighting.rgb;
    bool covered = cached_coverage;
#else
    bool covered = sample_probe_irradiance(v_world_position, n, surface_bias, indirect);
#endif
    if (probe_mode == 4) {
        return covered ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 1.0);
    }
    // Diagnostic views must not silently substitute original lighting:
    // indirect is black without coverage, while direct is independent of GI.
    if (!covered && probe_mode == 2) {
        return vec3(0.0);
    }
    if (!covered && probe_mode != 3) {
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
    // Only opaque diffuse GI receives AO. Blended/effect surfaces do not have
    // a matching prepass depth; direct light, emission and metal stay intact.
#ifndef OPENRAC_EARLY_LIGHTING
    if (gtao_enabled != 0 && translucent_pass == 0 && effect_pass == 0) {
        ivec2 pixel = clamp(ivec2(gl_FragCoord.xy), ivec2(0), textureSize(gtao_visibility, 0) - 1);
        indirect *= texelFetch(gtao_visibility, pixel, 0).r;
    }
#endif
    vec4 selector = probe_receiver == 1 ? vec4(0.0) : v_probe_selector;
    // Fine fur changes material response; shadow tests, probe lookup bias and cached GI keep smooth normals.
    vec3 direct = probe_direct(v_world_position, surface_n, selector, visibility);
    vec3 illumination = probe_mode == 2
                            ? indirect / 3.14159265359
                            : (probe_mode == 3 ? direct : direct + indirect / 3.14159265359);
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
    // Emission is independent of GI exposure, AO and shadow visibility.
    return probe_display(
        wet_coat(albedo * illumination * material_diffuse_scale + rim, surface_n, toward_camera, visibility)
        + fur_direct + fur_indirect
        + (probe_mode == 1 ? emitted : vec3(0.0))
    );
}

#else
vec3 probe_surface(vec4 texel, vec3 original) {
    return original;
}
#endif

// OPENRAC_UPSCALED_TEXTURE_COLOURS_SOURCE

void main() {
    v_fog = v_fog_w.x / v_fog_w.y;
#ifdef OPENRAC_OPAQUE_COLOUR
    if (opaque_solid != 0) {
#endif
#if defined(OPENRAC_SOLID) || defined(OPENRAC_OPAQUE_COLOUR)
    // The static solid variant contains no discard. The Vulkan colour-batch
    // variant selects this original solid path or the original general path
    // per draw, preserving their texture/derivative and alpha-test ordering.
    // It only runs with reused EQUAL depth and depth writes disabled.
    vec4 texel = sample_upscaled_texture_colours(v_uv);
    // FP32 inputs and only two multiplies: narrowing adds packing cost on the measured GPU.
    vec4 c = texel * v_colour * material_colour;
    float light = 1.0;
    if (lighting != 0 && v_lit == 0) {
        light = float(mp_float(0.55) + mp_float(0.45) * abs(dot(mp_vec3(normalize(v_normal)), mp_vec3(kSun))));
    }
    vec3 rgb = min(c.rgb * light, vec3(1.0));
    rgb = probe_surface(texel, rgb);
#ifdef OPENRAC_ATMOSPHERE
    if (atmosphere_enabled != 0 && v_probe_selector.w < 1.5) {
        rgb = atmosphere_apply(rgb, v_world_position, true);
    } else
#endif
    if (fog_colour.w > 0.5) {
        rgb = vec3(mix(mp_vec3(fog_colour.rgb), mp_vec3(rgb), mp_float(v_fog)));
    }
    out_colour = vec4(rgb, 1.0);
#ifdef OPENRAC_OPAQUE_COLOUR
    return;
    }
#endif
#endif
#ifndef OPENRAC_SOLID
#ifndef OPENRAC_COLOUR_SPECIALISED
    if (metal_pass != 0) {
        vec4 metal = sample_upscaled_texture_colours(v_uv);
        vec4 m = vec4(vec3(mp_vec3(metal.rgb) * mp_vec3(v_colour.rgb)), metal.a * v_colour.a);
        vec3 mrgb = min(m.rgb, vec3(1.0));
#if defined(OPENRAC_ENHANCED_RENDERING) && !defined(OPENRAC_COLOUR_SPECIALISED)
        vec4 reflected = chrome_surface(mrgb);
        mrgb = reflected.rgb;
        m.a *= reflected.a;
#endif
#ifdef OPENRAC_EARLY_LIGHTING
        // Chrome blends over the diffuse pass: give both layers the edge reflection so the
        // overlay's opacity cannot erase it. Use cached GI without another probe lookup.
        if (gameplay_rim_enabled != 0 && probe_receiver == 2 && probe_mode == 1
            && v_probe_selector.w < 1.5) {
            vec3 eye_position = (view * vec4(v_world_position, 1.0)).xyz;
            vec3 geometric = cross(dFdx(eye_position), dFdy(eye_position));
            vec3 normal_view = orient_shading_normal(mat3(view) * v_normal, geometric, -eye_position);
            vec4 cached;
            bool covered;
            float hero_visibility;
            if (early_surface(eye_position, normal_view, cached, covered, hero_visibility) && covered) {
                vec3 rim = gameplay_rim(cached.rgb * probe_gi_multiplier, normal_view, -eye_position);
                mrgb = probe_display(probe_linear(mrgb) + rim);
            }
        }
#endif
    #ifdef OPENRAC_ATMOSPHERE
        if (atmosphere_enabled != 0 && v_probe_selector.w < 1.5) {
            mrgb = atmosphere_apply(mrgb, v_world_position, true);
        } else
#endif
        if (fog_colour.w > 0.5) {
            mrgb = vec3(mix(mp_vec3(fog_colour.rgb), mp_vec3(mrgb), mp_float(v_fog)));
        }
        out_colour = vec4(mrgb, clamp(m.a, 0.0, 1.0));
        return;
    }
#endif
    if (effect_pass == 0 && v_glow != glow_pass) {
        discard;
    }
    vec4 texel = sample_upscaled_texture_colours(v_uv);
    // FP32 inputs and only two multiplies: narrowing adds packing cost on the measured GPU.
    vec4 c = texel * v_colour * material_colour;
#ifndef OPENRAC_COLOUR_SPECIALISED
    if (effect_pass != 0) {
        vec3 e = min(c.rgb, vec3(1.0));
        e = probe_surface(texel, e);
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
    rgb = probe_surface(texel, rgb);
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
