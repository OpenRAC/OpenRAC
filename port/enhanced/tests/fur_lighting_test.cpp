// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Production fur-light responses under known directions, visibility, attenuation and HDR irradiance.
#include <bit>
#include <cmath>
#include <cstdio>
#include <exception>

#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/shader.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#endif

using namespace openrac;
using namespace openrac::gl;

namespace {
// The lighting interface and cases contain no implementation of the production BRDF.
const char* const kLightInterface = R"(
vec4 light_sets[64];
vec3 probe_sunlight_scale=vec3(1);
vec4 probe_sunlight_direction=vec4(0);
vec4 probe_point_position[8],probe_point_colour[8];
vec3 probe_unit(vec3 v){return v*inversesqrt(max(dot(v,v),1e-12));}
)";

const char* const kCases = R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x);
    vec3 n=vec3(0,0,1),eye=n,light=normalize(vec3(.6,0,1));
    FurSurfaceData surface=FurSurfaceData(n,vec3(1,0,0),eye,1.,1.);
    light_sets[0]=light_sets[2]=vec4(0);light_sets[1]=light_sets[3]=vec4(0);
    for(int i=0;i<8;++i){probe_point_position[i]=vec4(0);probe_point_colour[i]=vec4(0);}
    float value=0.;
    if(test<4) {
        if((test&1)!=0) surface.tangent=vec3(0,1,0);
        value=fur_highlight(surface,eye,light);
    } else if(test>=4 && test<=12) {
        if(test!=4) {light_sets[0]=vec4(1,.5,.25,0);light_sets[1]=vec4(-light,0);}
        if(test>=8 && test<=10) {
            light_sets[0]=vec4(0);
            float z=test==8?2.:(test==9?4.:0.);
            probe_point_position[0]=vec4(0,0,z,4);probe_point_colour[0]=vec4(1,0,0,0);
        }
        if(test==11) surface.coverage=0.;
        if(test==12) surface.eye=vec3(0);
        value=fur_direct_lighting(surface,vec3(0),vec4(0),test==5?0.:(test==7?.5:1.),vec3(.8,.4,.1)).r;
    } else if(test==13) {
        value=fur_scattering(n,eye,-light);
    } else if(test>=14 && test<=16) {
        eye=normalize(vec3(.98,0,.2));light=normalize(vec3(-.99,0,-.1));
        if(test==14) value=fur_scattering(n,eye,light);
        else {
            vec3 response=fur_light_response(surface,eye,light,vec3(.8,.4,.1));
            value=test==15?response.r:response.b;
        }
    } else if(test>=17 && test<=20) {
        if(test>=19) surface.eye=normalize(vec3(.98,0,.2));
        vec3 response=fur_indirect_lighting(surface,test==17?vec3(0):vec3(1,2,3),vec3(.5));
        value=test==20?response.g:response.r;
    } else if(test==21) {
        value=fur_highlight(surface,eye,-eye);
    } else if(test==22) {
        eye=normalize(vec3(1,0,1e-6));light=normalize(vec3(-1,0,1e-6));
        value=fur_highlight(surface,eye,light);
    } else if(test>=23 && test<=25) {
        vec4 derivatives=test==24?vec4(-1,0,0,1):vec4(1,0,0,1);
        if(test==25) derivatives=vec4(0);
        vec3 t=fur_world_groom(n,vec3(1,0,0),vec3(0,2,0),derivatives,test==23?vec2(0,1):vec2(1,0));
        value=test==23?t.y:(test==24?-t.x:dot(t,t));
    }
    if(test==26) {
        surface.eye=normalize(vec3(.98,0,.2));
        value=fur_indirect_lighting(surface,vec3(1e6),vec3(.5)).r;
    } else if(test==27 || test==28) {
        light_sets[0]=vec4(1e6);light_sets[1]=vec4(-light,0);
        value=fur_direct_lighting(surface,vec3(0),vec4(0),test==28?0.:1.,vec3(.8,.4,.1)).r;
    }
    if(test==29 || test==30) {
        vec3 response=fur_indirect_lighting(surface,vec3(1),vec3(.8,.4,.1));
        value=test==29?response.r:response.b;
    } else if(test==31) {
        test_environment_available=true;
        environment_count=1;
        value=fur_environment_lighting(surface,vec3(0),vec3(0),vec3(.5)).r;
    }
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";

// Compiles the same material functions as the production mesh shader.
std::string fragment_source() {
    return std::string("#version 430 core\n") + lighting::shaders::fur_glsl
           + lighting::shaders::fur_detail_glsl + kLightInterface
           + lighting::shaders::fur_lighting_glsl + R"(
bool test_environment_available=false;
int environment_count=0;
vec3 environment_specular_frame(vec3 p,vec3 n,vec3 view,float nv,float roughness,float f0,vec3 fallback) {
    return test_environment_available?vec3(.25):fallback;
}
)" + lighting::shaders::fur_environment_glsl
           + kCases;
}

// Decodes the shader's float bits, retaining precision for small, physically restrained lobes.
float value_at(const std::vector<std::uint8_t>& pixels, unsigned x) {
    const auto at = std::size_t(x) * 4;
    const std::uint32_t bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                               | (std::uint32_t(pixels[at + 2]) << 16)
                               | (std::uint32_t(pixels[at + 3]) << 24);
    return std::bit_cast<float>(bits);
}

// Checks each physical dependency independently, including the exact absence of unlit/shadowed sheen.
void test_response(const std::vector<std::uint8_t>& pixels) {
    const auto at = [&](unsigned x) {
        return value_at(pixels, x);
    };
    for (unsigned x = 0; x < 32; ++x) {
        CHECK(std::isfinite(at(x)) && at(x) >= 0);
    }
    CHECK(at(1) > at(0) * 2);  // The broad lobe follows the groom rather than a screen-space rim.
    CHECK(at(4) == 0 && at(5) == 0 && at(6) > 0);
    CHECK(std::abs(at(7) * 2 - at(6)) < 1e-6f);
    CHECK(at(8) > 0 && at(9) == 0 && at(10) == 0);
    CHECK(at(11) == 0 && at(12) == 0);
    CHECK(at(13) == 0 && at(14) > 0);
    CHECK(
        at(15) > at(16) && at(16) > 0
    );  // Absorbed/scattered light retains the fur's warm colour.
    CHECK(at(17) == 0 && at(18) > 0 && at(19) > at(18));
    CHECK(std::abs(at(20) - at(19) * 2) < 1e-6f);
    CHECK(at(21) == 0);
    CHECK(at(23) == 1 && at(24) == 1 && at(25) == 0);
    CHECK(at(26) > 100 && at(27) > 100 && at(28) == 0);
    CHECK(std::abs(at(27) - at(6) * 1e6f) < at(27) * 1e-5f);
    CHECK(at(29) > at(30) && at(30) > 0);  // Indirect scattering keeps the fur's colour in shade.
    CHECK(
        at(31) > 0
    );  // A local reflection capture can illuminate fur with no direct sun or diffuse GI.
}

// The compiler's audit counts typed arithmetic in SPIR-V, including programs loaded from its cache.
void test_precision(std::uint64_t before) {
#ifdef OPENRAC_VULKAN
    vkgl::NativeDevice device;
    CHECK(vkgl::native_device(device));
    const auto operations = vkgl::submission_counters().shader_fp16_ops - before;
    CHECK(device.float16 ? operations > 0 : operations == 0);
    std::printf(
        "Fur lighting: %llu typed FP16 operations; enabled: %d\n",
        static_cast<unsigned long long>(operations),
        int(device.float16)
    );
#else
    (void)before;
#endif
}

// Directionality can be disabled independently; zero material strengths produce exactly zero added light.
void test_controls(const renderer::Shader& shader, renderer::FrameBuffer& target) {
    const auto draw = [&] {
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return target.read_rgba();
    };
    test_response(draw());
    glUniform4f(shader.uniform("hero_fur_lighting_parameters"), .35f, .7f, 0, .12f);
    const auto isotropic = draw();
    CHECK(std::abs(value_at(isotropic, 0) - value_at(isotropic, 1)) < 1e-6f);
    glUniform1i(shader.uniform("hero_fur_lighting_enabled"), 0);
    const auto disabled = draw();
    CHECK(value_at(disabled, 6) == 0 && value_at(disabled, 8) == 0 && value_at(disabled, 19) == 0);
    glUniform1i(shader.uniform("hero_fur_lighting_enabled"), 1);
    glUniform1f(shader.uniform("hero_fur_environment_strength"), 1);
    glUniform4f(shader.uniform("hero_fur_lighting_parameters"), 0, .7f, .65f, 0);
    const auto zero = draw();
    CHECK(value_at(zero, 6) == 0 && value_at(zero, 15) == 0 && value_at(zero, 19) == 0);
    glUniform4f(shader.uniform("hero_fur_lighting_parameters"), 0, .7f, .65f, .12f);
    CHECK(value_at(draw(), 18) > 0);  // Scattering alone must remain visible under indirect light.
    glUniform4f(shader.uniform("hero_fur_lighting_parameters"), .35f, .7f, .65f, .12f);
    glUniform1f(shader.uniform("hero_fur_environment_strength"), 0);
    const auto no_environment = draw();
    CHECK(value_at(no_environment, 18) == 0 && value_at(no_environment, 31) == 0);
    CHECK(value_at(no_environment, 6) > 0);  // Ambient tuning must not weaken direct lighting.
}

int test_gpu() {
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    renderer::Shader shader;
    std::uint64_t before = 0;
#ifdef OPENRAC_VULKAN
    before = vkgl::submission_counters().shader_fp16_ops;
#endif
    const bool built = shader.build(
        "fur directional lighting reference",
        R"(#version 430 core
flat out int v_hero_fur;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);v_hero_fur=1;})",
        fragment_source(),
        error
    );
    if (!built) {
        std::printf("%s\n", error.c_str());
    }
    CHECK(built);
    if (!built) {
        return test::result();
    }
    test_precision(before);
    renderer::FrameBuffer target;
    CHECK(target.create(32, 1, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 32, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    shader.use();
    glUniform1i(shader.uniform("hero_fur_lighting_enabled"), 1);
    glUniform1f(shader.uniform("hero_fur_environment_strength"), 1);
    glUniform4f(shader.uniform("hero_fur_lighting_parameters"), .35f, .7f, .65f, .12f);
    test_controls(shader, target);
    glDeleteVertexArrays(1, &vao);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
}  // namespace

int main() {
    try {
        return test_gpu();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Fur lighting test: %s\n", error.what());
        return 1;
    }
}
