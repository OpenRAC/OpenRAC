#version 410 core
// Copyright (C) 2021 The Android Open Source Project
// SPDX-License-Identifier: Apache-2.0
// AgX matrices adapted from Filament be858f42ab95cf0ba6e224ac2dc3950bdb172080.
// Exposure uses an analytical sigmoid and a 40% display-hue blend.
// See ../lighting/README.md and agx-LICENSE.txt for attribution and SDR limitations.
uniform sampler2D tex_T0;
uniform float exposure_ev;
out vec4 colour;
float AgXSigmoid(float x)
{
    const float pivotX = 10.0/16.5;
    const float pivotY = 0.4894370896; // pow(0.18, 1/2.4)
    float scale = x < pivotX ? 0.5656756067 : 0.7151982863;
    float t = 2.4*abs(x-pivotX)/scale;
    return pivotY + sign(x-pivotX)*scale*t/pow(1+pow(t,1.5),1/1.5);
}
vec3 AgXHSV(vec3 rgb)
{
    float hi = max(rgb.r,max(rgb.g,rgb.b)), lo = min(rgb.r,min(rgb.g,rgb.b));
    float delta = hi-lo;
    float h = 0;
    if(delta > 1e-10)
    {
        if(hi == rgb.r) { h = (rgb.g-rgb.b)/delta; }
        else if(hi == rgb.g) { h = 2+(rgb.b-rgb.r)/delta; }
        else { h = 4+(rgb.r-rgb.g)/delta; }
        h = fract(h/6);
    }
    return vec3(h,hi > 1e-10 ? delta/hi : 0,hi);
}
vec3 AgXRGB(vec3 hsv)
{
    vec3 hueWeights = clamp(abs(fract(hsv.x+vec3(0,2.0/3,1.0/3))*6-3)-1,0.0,1.0);
    return hsv.z*mix(vec3(1),hueWeights,hsv.y);
}
vec3 AgXDisplay(vec3 scene)
{
    // Linear Rec.709/D65 -> linear Rec.2020/D65.
    const mat3 to2020 = mat3(
        0.627403896,0.329283038,0.043313066,
        0.069097289,0.919540395,0.011362316,
        0.016391439,0.088013308,0.895595253);
    const mat3 inset = mat3(
        0.856627153,0.095121241,0.048251606,
        0.137318973,0.761241991,0.101439036,
        0.111898213,0.076799419,0.811302368);
    const mat3 outset = mat3(
        1.127100582,-0.110606643,-0.016493939,
        -0.141329763,1.157823703,-0.016493939,
        -0.141329763,-0.110606643,1.251936406);
    const mat3 to709 = mat3(
        1.660491002,-0.587641139,-0.072849863,
        -0.124550475,1.132899897,-0.008349423,
        -0.018150763,-0.100578898,1.118729661);
    vec3 value = (max(scene,vec3(0))*to2020)*inset;
    float hue = AgXHSV(value).x;
    value = clamp((log2(max(value,vec3(1e-10)))+12.47393)/16.5,0.0,1.0);
    value = pow(max(vec3(AgXSigmoid(value.r),AgXSigmoid(value.g),AgXSigmoid(value.b)),vec3(0)),vec3(2.4));
    vec3 hsv = AgXHSV(value);
    hsv.x = fract(hue+0.4*(fract(hsv.x-hue+0.5)-0.5));
    value = clamp((AgXRGB(hsv)*outset)*to709,0.0,1.0);
    // Encode once. Both native swapchains use UNORM, not sRGB render targets.
    return vec3(value.r <= 0.0031308 ? 12.92*value.r : 1.055*pow(value.r,1/2.4)-0.055,
                  value.g <= 0.0031308 ? 12.92*value.g : 1.055*pow(value.g,1/2.4)-0.055,
                  value.b <= 0.0031308 ? 12.92*value.b : 1.055*pow(value.b,1/2.4)-0.055);
}

void main() {
    vec4 source = texelFetch(tex_T0, ivec2(gl_FragCoord.xy), 0);
    // Existing world storage is display-referred UNORM, not scene-linear HDR.
    vec3 linear = mix(source.rgb / 12.92, pow((source.rgb + 0.055) / 1.055, vec3(2.4)),
                      greaterThan(source.rgb, vec3(0.04045)));
    colour = vec4(AgXDisplay(linear * exp2(exposure_ev)), source.a);
}
