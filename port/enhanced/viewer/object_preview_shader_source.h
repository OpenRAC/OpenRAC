// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Neutral and enhanced object-preview lighting source shared by the viewer and build recipes.

#pragma once
#include <string>

#include "enhanced_viewer_shaders.h"

namespace openrac::enhanced {
inline std::string object_preview_lighting_source() {
    return std::string("uniform vec4 probe_point_position[8],probe_point_colour[8];\n")
           + lighting::shaders::local_lights_glsl + lighting::shaders::shading_brdf_glsl + R"(



uniform int preview_enhanced;



vec3 preview_illumination(vec3 albedo,vec3 n,vec3 p,vec3 eye,float diffuse) {



    if(preview_enhanced==0) return preview_neutral(albedo,n);



    vec3 lit=albedo*.06;



    LOCAL_LIGHT_LOOP(i) {



        vec4 position=LOCAL_POSITION(i),colour=LOCAL_COLOUR(i);



        vec3 delta=position.xyz-p;float distance=length(delta);



        if(position.w<=0. || distance>=position.w || distance<1e-6) continue;



        vec3 direction=delta/distance;



        vec3 radiance=max(colour.rgb,vec3(0))*LOCAL_ATTENUATION(distance,position.w)*LOCAL_CONE(i,direction);



        lit+=radiance*(albedo*max(dot(n,direction),0.)*diffuse+ggx_specular(n,eye,direction,.55,.04));



    }



    return lit;



})";
}
}  // namespace openrac::enhanced
