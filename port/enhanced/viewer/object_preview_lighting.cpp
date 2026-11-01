// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Production Forward+ table, attenuation, spot cones and GGX in the private object view.
#include "enhanced/viewer/object_preview_lighting.h"

#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"

namespace openrac::enhanced {
Result<void> ObjectPreviewLighting::set_lights(std::span<const LocalLight> lights) {
    auto result = m_lights.set_lights(lights);

    if (result) {
        m_count = lights.size();
    }

    return result;
}

std::string ObjectPreviewLighting::source() const {
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

void ObjectPreviewLighting::prepare(std::uint64_t frame) {
    m_lights.reference = reference;
    m_lights.prepare(frame, {}, true);
}

void ObjectPreviewLighting::cull(
    unsigned depth, const renderer::Camera& camera, int width, int height
) {
    m_lights.dispatch(depth, camera, width, height);
}

void ObjectPreviewLighting::bind(
    const renderer::Shader& shader, const renderer::Camera& camera, bool opaque
) const {
    m_lights.bind(shader, &camera, opaque);
    gl::glUniform1i(shader.uniform("preview_enhanced"), enabled);
}

void ObjectPreviewLighting::release() {
    m_lights.release();
}
}  // namespace openrac::enhanced
