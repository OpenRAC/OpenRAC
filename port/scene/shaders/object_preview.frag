#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
vec3 preview_neutral(vec3 albedo,vec3 n) {return albedo*(.35+.65*abs(dot(n,normalize(vec3(.4,-.5,.8)))));}
// PREVIEW_LIGHTING
in vec3 v_position,v_normal;
in vec2 v_uv;
in vec4 v_colour;
uniform sampler2D tex_T0,tex_T1;
uniform vec4 material_colour;
uniform vec3 material_emission,eye;
uniform float material_diffuse_scale,material_emission_contrast;
uniform int preview_depth_only;
uniform int material_cutout,material_emission_vertex_colour,material_transparent;
out vec4 out_colour;
// The studio decodes the original artwork, shades in linear space, then displays SDR sRGB.
vec3 linear_colour(vec3 c) {return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
vec3 display_colour(vec3 c) {c=max(c,vec3(0));return mix(c*12.92,1.055*pow(c,vec3(1./2.4))-.055,step(vec3(.0031308),c));}
void main() {
    vec4 texel=texture(tex_T0,v_uv)*material_colour;
    float alpha=texel.a*v_colour.a;
    // Match the world shader's opaque/partial-alpha split without changing texture alpha.
    if(material_cutout!=0 && material_transparent==0 && alpha<.99) discard;
    if(material_cutout!=0 && material_transparent!=0 && (alpha>=.99 || alpha<.02)) discard;
    if(preview_depth_only!=0) {out_colour=vec4(0);return;}
    vec3 albedo=linear_colour(clamp(texel.rgb*v_colour.rgb,vec3(0),vec3(1)));
    vec3 n=normalize(v_normal),view_direction=normalize(eye-v_position);
    if(!gl_FrontFacing) n=-n;
    vec3 lit=preview_illumination(albedo,n,v_position,view_direction,material_diffuse_scale);
    vec3 emission_texel=texture(tex_T1,v_uv).rgb;
    if(material_emission_vertex_colour!=0) emission_texel=clamp(emission_texel*v_colour.rgb,vec3(0),vec3(1));
    vec3 emission=linear_colour(emission_texel);
    float luminance=dot(emission,vec3(.2126,.7152,.0722));
    if(material_emission_contrast>1.) emission*=pow(max(luminance,0.)/.18,material_emission_contrast-1.);
    lit+=emission*material_emission;
    out_colour=vec4(display_colour(lit),material_transparent!=0?alpha:1.);
}
