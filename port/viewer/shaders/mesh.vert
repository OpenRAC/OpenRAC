#version 410 core
invariant gl_Position;
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Extracted level geometry, instanced: one matrix per placed object, in the
// game's axes (Z up). The projection has reversed depth.
//
// A live moby is skinned first: its vertex is the weighted sum of its joints'
// matrices from the moby's joint palette (F_j = P_j S_j, as the game's
// MobyAnimEval builds it), applied to the bind-pose vertex. The palettes are
// a float texture, four texels (the matrix's columns) per joint; `palette` is
// the instance's first matrix, -1 for an object drawn as stored.
//
// A lit tie (`lights` >= 0) colours each vertex with its instance's colour for
// the vertex's light slot, from the light texture: what LightTies computed.

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 colour;
layout(location = 4) in mat4 model;  // locations 4 to 7
layout(location = 8) in vec4 tint;
#if !defined(OPENRAC_STATIC_COLOUR) || defined(OPENRAC_HERO_WETNESS)
layout(location = 9) in float palette;
#endif
#ifndef OPENRAC_STATIC_COLOUR
layout(location = 10) in uvec4 joints;
layout(location = 11) in vec4 weights;
#endif
layout(location = 12) in float light_slot;
layout(location = 13) in float lights;
layout(location = 14) in vec4
    moby_light;  // set 0, set 1, cross-fade; -1 authored light, 1 world, 2 UI
layout(location = 15) in vec4 moby_ambient;  // ambient RGB / 128; w: 1 + glow RGB (mode 0x10), else 0

uniform mat4 view_projection;
uniform mat4 view;
uniform float picture_scale;  // the game's 4:3 picture's width as a part of the target's (1: all of it)
// The game's fog line (LevelScene::set_fog): F = depth x slope + offset, clamped to [z, w].
uniform vec4 fog_params;
#ifndef OPENRAC_STATIC_COLOUR
uniform int skinning;
uniform sampler2D palette_texture;
#endif
uniform sampler2D light_texture;
// The level's 16 directional sets: colour A (w = back factor), direction A, colour B, direction B.
uniform vec4 light_sets[64];
// A point-lit moby's third light (LevelScene::set_moby_point_lights): colour, then the direction
// from the lights to it, for the record moby_light.x / 256 - 1 names. Going Commando's moby
// program gives it no back light.
uniform vec4 moby_point_lights[64];

// MobyProc's scaling of a cross-faded light direction by VRSQRT of its squared length. A zero
// direction (a light set with no lights, as the page menus' set 14 is until a menu fills it) stays
// zero: the VU's reciprocal root of 0 is its largest float, and 0 times it is 0 on the console.
// GLSL's normalize gives NaN there, and min(NaN, 2) lit the whole moby at the clamp.
vec3 vu_unit(vec3 v) {
    float d = dot(v, v);
    return d > 0.0 ? v * inversesqrt(d) : vec3(0.0);
}
// The game's eight point lights as it holds them (colour, falloff; position, reach), added to the
// terrain's colours when terrain_point_lit is set (Going Commando).
// openrac_mode: terrain_point_lit metal_pass probe_mode probe_receiver sun_shadow_enabled
// openrac_mode: hero_shadow_enabled hero_wetness_enabled
uniform int terrain_point_lit;
uniform vec4 terrain_point_lights[16];
// The metal ("shine") pass (MobyProc's metal packets): texture coordinates from the view-space
// normal, a sphere map of the level's chrome or glass picture, the vertex colour 0x70 and the
// alpha the moby's distance gives (ReRAC moby_skinning_lighting.md 6 and 9).
uniform int metal_pass;
// 1: clip-space z goes to the window as it is (renderer/gl/precise_depth.h), 0: GL's (z + w) / 2.
uniform int clip_zero_to_one;
uniform int probe_mode;
uniform int probe_receiver;
uniform int sun_shadow_enabled;
uniform int hero_shadow_enabled;

#ifdef OPENRAC_HERO_WETNESS
layout(std430,binding=9) readonly buffer HeroWetness { float hero_wetness[]; };
uniform int hero_wetness_enabled;
uniform int hero_wetness_first, hero_wetness_count, hero_wetness_palette;
out float v_hero_wetness;
#endif

#ifdef OPENRAC_HERO_FUR
flat out int v_hero_fur;
#endif

out vec2 v_uv;
out vec4 v_colour;
out vec3 v_normal;
out vec3 v_world_position;
out vec4 v_probe_multiplier;
flat out vec4 v_probe_selector;
flat out int v_lit;
flat out int v_glow;  // a glow packet's vertex drawn in the moby's glow colour
out vec3 v_script_emission;
flat out vec3 v_probe_ambient;
out vec3 v_emission_colour;
// GS F / 255, interpolated in screen space as the GS does: F times the clip w and w, both
// interpolated perspective-correctly, whose ratio mesh.frag takes. A noperspective F gave NaN
// where a triangle crossed the near plane (Mesa, lavapipe): black wedges at the screen's edges
// when the camera stood by a wall.
out vec2 v_fog_w;

const int kPaletteWidth = 1024;
const int kLightWidth = 1024;

#ifndef OPENRAC_STATIC_COLOUR
mat4 joint_matrix(int index) {
    int texel = 4 * index;
    ivec2 at = ivec2(texel % kPaletteWidth, texel / kPaletteWidth);
    return mat4(
        texelFetch(palette_texture, at, 0),
        texelFetch(palette_texture, at + ivec2(1, 0), 0),
        texelFetch(palette_texture, at + ivec2(2, 0), 0),
        texelFetch(palette_texture, at + ivec2(3, 0), 0)
    );
}
#endif

void main() {
    vec4 p = vec4(position, 1.0);
    vec3 n = normal;
#ifdef OPENRAC_HERO_FUR
    if (hero_fur_shell_enabled != 0 && hero_fur_shell > 0.) {
        // Displace bind-pose vertices before skinning, so every shell follows the same joints.
        int shell_receiver = hero_fur_receiver(gl_VertexID, palette, moby_light.w);
        float shell_wetness = 0.;
#ifdef OPENRAC_HERO_WETNESS
        int shell_wet_index = gl_VertexID - hero_wetness_first;
        if (hero_wetness_enabled != 0 && int(palette + .5) == hero_wetness_palette
            && shell_wet_index >= 0 && shell_wet_index < hero_wetness_count) {
            shell_wetness = hero_wetness[shell_wet_index];
        }
#endif
        if (shell_receiver != 0 && dot(normal, normal) > 1e-12) {
            p.xyz += normalize(normal) * fur_shell_extent(shell_wetness);
        }
    }
#endif
#ifndef OPENRAC_STATIC_COLOUR
    float total = weights.x + weights.y + weights.z + weights.w;
    if (skinning != 0 && palette >= 0.0 && total > 0.0) {
        int base = int(palette + 0.5);
        mat4 skin = weights.x * joint_matrix(base + int(joints.x))
                  + weights.y * joint_matrix(base + int(joints.y))
                  + weights.z * joint_matrix(base + int(joints.z))
                  + weights.w * joint_matrix(base + int(joints.w));
        p = vec4((skin * p).xyz, 1.0);
        n = mat3(skin) * n;
    }
#endif
    vec4 world = model * p;
#ifdef OPENRAC_WATER_FLOAT
    mat4 water_pose = water_float_pose(model[3].xyz);
    world = water_pose * world;
#endif
    mat3 normal_model = mat3(model);
#ifdef OPENRAC_WATER_FLOAT
    normal_model = mat3(water_pose) * normal_model;
#endif
    v_world_position = world.xyz;
#ifdef OPENRAC_HERO_FUR
    v_hero_fur = hero_fur_receiver(gl_VertexID, palette, moby_light.w);
#endif
#ifdef OPENRAC_HERO_WETNESS
    int wet_index=gl_VertexID-hero_wetness_first;
    bool wet_receiver=hero_wetness_enabled!=0 && palette>=0. && int(palette+.5)==hero_wetness_palette
        && moby_light.w<1.5 && wet_index>=0 && wet_index<hero_wetness_count;
    v_hero_wetness=wet_receiver?hero_wetness[wet_index]:-1.;
#endif
    // Terrain COLOR_0 is already lit. Mobys' COLOR_0 is their authored
    // multiplier. Keep alpha and the original colour path for fallback.
    v_probe_multiplier = (probe_receiver == 1 ? vec4(1.0, 1.0, 1.0, colour.a) : colour) * tint;
    v_probe_selector = moby_light;
    // The moby's ambient colour (0x80 = 1.0), which anchored probe lighting keeps as its GI floor.
    v_probe_ambient = moby_ambient.rgb;
    v_script_emission = moby_light.w < -0.5 ? colour.rgb * tint.rgb * moby_ambient.rgb : vec3(0.0);
    v_emission_colour = colour.rgb * tint.rgb;
    gl_Position = view_projection * world;
    // A moby of the 2D layer (a page menu's frame object, moby_light.w = 2) on a target wider than
    // the game's picture: cut at the picture's sides, where the television's edge cut it.
    bool in_picture = moby_light.w > 1.5 && picture_scale < 1.0;
    gl_ClipDistance[0] = in_picture ? picture_scale * gl_Position.w - gl_Position.x : 1.0;
    gl_ClipDistance[1] = in_picture ? picture_scale * gl_Position.w + gl_Position.x : 1.0;
    // Camera depth in the game's raw units (game units x 1024); the view looks down -Z.
    float depth = -(view * world).z * 1024.0;
    float fog = trunc(clamp(depth * fog_params.x + fog_params.y, fog_params.z, fog_params.w)) / 255.0;
    v_fog_w = vec2(fog, 1.0) * gl_Position.w;
    v_uv = uv;
    v_colour = colour * tint;
    v_lit = 0;
    v_glow = 0;
    // A lit tie's vertex takes its instance's colour for its light slot, as VU1 does
    // (0x80 = 1.0 under the GS's MODULATE).
    if (lights >= 0.0 && light_slot >= 0.0) {
        int texel = int(lights + 0.5) + int(light_slot + 0.5);
        v_colour = texelFetch(light_texture, ivec2(texel % kLightWidth, texel / kLightWidth), 0)
                 * (255.0 / 128.0) * tint;
        // Explicit static emission uses the same authored instance colour as the original surface.
        v_emission_colour = v_colour.rgb;
        v_lit = 1;
    }
    // Going Commando's terrain (PatchTfragVertexLighting, 0x2F0468): each point light in reach
    // adds colour x (1 - d/r) x max(N.(L - P)/d + 1 - d/r, 0) to the colour the level lit, the
    // sum clamped at 255 (0x80 = 1.0). A light with x and y both 0 is free, as MobyProc reads it.
    if (terrain_point_lit != 0) {
        vec3 nw = normalize(normal_model * n);
        vec3 lit = colour.rgb;
        for (int i = 0; i < 8; ++i) {
            vec4 c = terrain_point_lights[2 * i];
            vec4 q = terrain_point_lights[2 * i + 1];
            if (q.w <= 0.0 || (q.x == 0.0 && q.y == 0.0)) {
                continue;
            }
            vec3 d = q.xyz - world.xyz;
            float dist = length(d);
            float fall = 1.0 - dist / q.w;
            if (fall > 0.0) {
                lit += max(c.rgb, vec3(0.0)) * fall * max(dot(nw, d) / max(dist, 1.0e-4) + fall, 0.0);
            }
        }
        v_colour = vec4(min(lit, vec3(255.0 / 128.0)), colour.a) * tint;
    }
    // A live moby, lit as MobyProc and its VU0 pass light it: its set's two directional
    // lights (or two sets cross-faded) on the skinned normal, with the leaky back clamp
    // max(d, -|K| d), plus its ambient colour, times the vertex's multiplier (all 0x80 = 1.0).
    if (abs(moby_light.w) > 0.5) {
        int s0 = (int(moby_light.x + 0.5) & 15) * 4;
        int s1 = (int(moby_light.y + 0.5) & 15) * 4;
        float t = moby_light.z;
        vec4 ca = light_sets[s0], cb = light_sets[s0 + 2];
        vec3 da = light_sets[s0 + 1].xyz, db = light_sets[s0 + 3].xyz;
        if (t > 0.0) {
            ca = mix(ca, light_sets[s1], t);
            cb = mix(cb, light_sets[s1 + 2], t);
            da = vu_unit(mix(da, light_sets[s1 + 1].xyz, t));
            db = vu_unit(mix(db, light_sets[s1 + 3].xyz, t));
        }
        vec3 nw = normalize(normal_model * n);
        float fa = dot(-da, nw);
        float fb = dot(-db, nw);
        fa = max(fa, -abs(ca.w) * fa);
        fb = max(fb, -abs(cb.w) * fb);
        vec3 lit = moby_ambient.rgb + ca.rgb * fa + cb.rgb * fb;
        int point = (int(moby_light.x + 0.5) >> 8) - 1;
        if (point >= 0 && point < 32) {
            vec3 dp = moby_point_lights[2 * point + 1].xyz;
            lit += moby_point_lights[2 * point].rgb * max(dot(-dp, nw), 0.0);
        }
        v_colour = vec4(min(lit * colour.rgb, vec3(255.0 / 128.0)), colour.a) * tint;
        // A glow packet's vertex (slot -2) of a moby whose mode has 0x10: its glow RGB, unlit,
        // with no multiplier (MobyProc's glow list overwrites the lit colours).
        if (light_slot < -1.5 && moby_ambient.w > 0.5) {
            // w holds an exact integer: round it before taking the 1 off. Rounding w - 1 + 0.5
            // instead is not exact near 2^24 (a white glow came out as 0x1000000: black on Vulkan).
            int glow = int(round(moby_ambient.w)) - 1;
            v_colour = vec4(float(glow & 255), float((glow >> 8) & 255), float((glow >> 16) & 255), 128.0)
                     / 128.0 * tint;
            v_glow = 1;
        }
        v_lit = 1;
    }
    v_normal = normal_model * n;
#ifdef OPENRAC_ENHANCED_RENDERING
    if ((probe_mode != 0 || sun_shadow_enabled != 0 || hero_shadow_enabled != 0)
        && probe_receiver != 0) {
        v_normal = transpose(inverse(normal_model)) * n;
    }
#endif
#ifndef OPENRAC_COLOUR_SPECIALISED
    if (metal_pass != 0) {
        // st = (e.xy + 1) / 2, e the normal in camera space (x right, y down: t grows downward).
        vec3 e = normalize(mat3(view) * normalize(normal_model * n));
        v_uv = vec2(e.x * 0.5 + 0.5, 0.5 - e.y * 0.5);
        // The shine alpha: MobyProc's gate min(((distance << 10) - depth) >> 7, 0x80), depth in raw
        // units: full up to 16 game units nearer than the moby's shine distance (+0x73, 0x18 unless the
        // class changes it; the frontend passes it as lights = -1 - distance), none past it.
        float reach = lights < -1.5 ? -lights - 1.0 : 24.0;
        float centre = -(view * model[3]).z;
        float shine = clamp((reach - centre) / 16.0, 0.0, 1.0);
        v_colour = vec4(vec3(112.0 / 128.0), shine);
        // MobyProc's pull toward the camera (GS Z + 1000 Q): a little nearer, so the shine is not
        // lost to the moby's own surface under it (reversed depth: larger is nearer). 0.5e-4 of the
        // window either way: GL's form halves the clip-space z it is given, precise depth does not.
        gl_Position.z += (clip_zero_to_one != 0 ? 0.5e-4 : 1.0e-4) * gl_Position.w;
        v_lit = 1;
        v_glow = 0;
    }
#endif
}
