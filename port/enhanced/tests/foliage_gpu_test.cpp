// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Production thin-leaf functions checked against Lambert, Beer-Lambert and GGX arithmetic.
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "enhanced/renderer/foliage.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;
// Synthetic lighting has no assets; the same directional/local loops run as in production.
constexpr const char* kInterface = R"(
#define OPENRAC_PRECISION_DECLARED 1
#define mp_float float
#define mp_vec2 vec2
#define mp_vec3 vec3
vec4 light_sets[64];
vec4 probe_point_position[8],probe_point_colour[8];
vec4 probe_sunlight_direction=vec4(0);
vec3 probe_sunlight_scale=vec3(1);
int probe_mode=1;
float test_shadow=1.;
uniform float material_diffuse_scale;
uniform vec4 material_colour;
float material_metallic=0.;
vec3 material_dielectric=vec3(0.),material_dielectric_tint=vec3(1.);
float sun_shadow_visibility(vec3 p,vec3 n,vec3 g){return test_shadow;}
float hero_shadow_visibility(vec3 p,vec3 n){return 1.;}
vec3 probe_unit(vec3 v){return v*inversesqrt(max(dot(v,v),1e-12));}
#define LOCAL_LIGHT_LOOP(i) for(int i=0;i<8;++i)
#define LOCAL_POSITION(i) probe_point_position[i]
#define LOCAL_COLOUR(i) probe_point_colour[i]
#define LOCAL_ATTENUATION(d,r) (1.-(d)/(r))
#define LOCAL_CONE(i,l) 1.
)";

// Each output pixel carries one exact FP32 scalar, encoded as four UNORM bytes.
constexpr const char* kCases = R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x);
    vec3 n=vec3(0,0,1),eye=n;
    vec4 material=vec4(1,1,0,1);
    for(int i=0;i<64;++i) light_sets[i]=vec4(0);
    for(int i=0;i<8;++i){probe_point_position[i]=vec4(0);probe_point_colour[i]=vec4(0);}
    vec3 shaped=foliage_normal(n,vec3(gl_FragCoord.xy*.1,0),gl_FragCoord.xy*.01,.2);
    vec3 albedo=vec3(1);
    float value=0.;
    if(test<4) {
        if(test==0) {material.x=0.;albedo=vec3(.5);}
        if(test==3) material.x=.5;
        FoliageSurfaceData surface=prepare_foliage(n,eye,albedo,material);
        value=foliage_response(surface,test<2?n:-n).r;
    } else if(test<=8) {
        albedo=test==8?vec3(0):vec3(.1,.8,.2);
        material.y=test==4?0.:(test==6?2.:1.);
        vec3 response=foliage_response(prepare_foliage(n,eye,albedo,material),-n);
        value=test==7?response.g:response.r;
    } else if(test<=14 || test>=19 && test<=22) {
        if(test!=9) {light_sets[0]=vec4(vec3(1),0);light_sets[1]=vec4(n,0);}
        if(test==10 || test==20) test_shadow=0.;
        if(test==12) light_sets[1]=vec4(-n,0);
        if(test==13 || test==14) {
            light_sets[0]=vec4(vec3(0),0);probe_point_colour[0]=vec4(1);
            probe_point_position[0]=vec4(0,0,test==13?-2.:2.,4);
        }
        if(test==19) light_sets[0]=vec4(vec3(0),0);
        if(test==20 || test==21) light_sets[0]=vec4(vec3(1e6),0);
        if(test==22) {light_sets[0]=vec4(vec3(0),0);light_sets[2]=vec4(1,1,1,0);light_sets[3]=vec4(n,0);test_shadow=0.;}
        value=foliage_direct(vec3(0),n,vec4(0),test==12?0.:1.,prepare_foliage(n,eye,albedo,material)).r;
    } else if(test==15 || test==16) {
        light_sets[0]=vec4(vec3(1),0);light_sets[1]=vec4(n,0);
        value=foliage_surface(vec3(0),n,eye,albedo,vec3(test==15?0.:1.),vec4(0),1.,material,vec2(.5)).r;
    } else if(test==17 || test==18) {
        vec3 l=normalize(vec3(-1,0,.00001));
        eye=test==17?normalize(vec3(1,0,.00001)):-l;
        value=foliage_response(prepare_foliage(n,eye,albedo,material),l).r;
    } else if(test==23 || test==24) {
        vec4 decoded=foliage_material();
        value=test==23?decoded.x:decoded.w;
    } else if(test==25) value=shaped.z;
    else if(test==26) value=dot(shaped,shaped);
    else if(test==27) value=foliage_normal(n,vec3(gl_FragCoord.xy*.1,0),gl_FragCoord.xy*.01,0.).z;
    else if(test>=28 && test<=39) {
        material.x=test==29?1.:0.;
        albedo=vec3(.5,.25,.125);
        material_metallic=test==34?1.5:2.;
        material_dielectric=vec3(1.,.04,test==30 || test==37?0.:(test==33 || test==38?2.:1.));
        if(test==31 || test==39) material_dielectric_tint=vec3(.5,1.,1.);
        FoliageSurfaceData surface=prepare_foliage(n,eye,albedo,material);
        vec3 response=foliage_response(surface,test==29?-n:n);
        if(test==32 || test>=37) response=foliage_surface(vec3(0),n,eye,albedo,vec3(1),vec4(0),1.,material,vec2(.5));
        value=test==35?response.g:(test==36?response.b:response.r);
    }
    uint bits=floatBitsToUint(value*material_colour.r);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";

// Reads little-endian float bits without depending on alignment or byte order.
float scalar(const std::vector<std::uint8_t>& pixels, unsigned x) {
    std::uint32_t word = 0;
    // Four bytes encode one independent IEEE FP32 result.
    for (unsigned byte = 0; byte < sizeof(float); ++byte) {
        word |= static_cast<std::uint32_t>(pixels[static_cast<std::size_t>(x) * 4 + byte])
                << (byte * 8);
    }
    return std::bit_cast<float>(word);
}

// Uses independently derived energy, absorption and half-distance falloff values.
void test_lighting(const std::vector<std::uint8_t>& pixels) {
    constexpr float kTolerance = .00001f;
    const auto near = [&](unsigned pixel, float expected) {
        CHECK(std::abs(scalar(pixels, pixel) - expected) < kTolerance);
    };
    // Every diagnostic must remain finite and nonnegative, including opposing and grazing directions.
    for (unsigned x = 0; x < 40; ++x) {
        CHECK(std::isfinite(scalar(pixels, x)) && scalar(pixels, x) >= 0);
    }
    near(0, .5f * .99f + .0025f);
    near(1, .0025f);
    near(2, .99f);
    near(3, .495f);
    near(4, .99f);
    near(5, .99f * .125f);
    near(6, .99f * .125f * .125f);
    near(7, .99f);
    near(8, 0);
    near(9, 0);
    near(10, 0);
    near(11, .99f);
    near(12, 0);
    near(13, .495f);
    near(14, .00125f);
    // Front GI adds only its own broad reflection; it never modulates the direct transmitted lobe.
    near(15, .99f);
    near(16, .99f + .01f / 3.14159265f);
    near(19, 0);
    near(20, 0);
    CHECK(std::abs(scalar(pixels, 21) - 990'000.f) < 1);
    near(22, .99f);
    near(23, 1);
    near(24, 1);
    CHECK(scalar(pixels, 25) > .5f && scalar(pixels, 25) < 1);
    near(26, 1);
    near(27, 1);
    // Measured F0 modulates the artwork, metallic removes transmission/diffuse,
    // and tint/strength affect both directional reflection and broad GI reflection.
    near(28, .00125f);
    near(29, 0);
    near(30, 0);
    near(31, .000625f);
    near(32, .005f / 3.14159265f);
    near(33, .0025f);
    near(34, .25f);
    near(35, .000625f);
    near(36, .0003125f);
    near(37, 0);
    near(38, .01f / 3.14159265f);
    near(39, .0025f / 3.14159265f);
}

// Owns the fullscreen fixture while its graphics context remains current.
struct Fixture {
    renderer::Shader shader;      // Production leaf functions compiled for this device.
    renderer::FrameBuffer frame;  // Float-bit readback target.
    unsigned buffer = 0;          // Owned synthetic matrix vertex input.
    unsigned vao = 0;             // Owned fullscreen vertex array.
    Fixture() = default;
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;

    // Releases fixture inputs while the context is current.
    ~Fixture() {
        glDeleteBuffers(1, &buffer);
        glDeleteVertexArrays(1, &vao);
    }
};

// Supplies every matrix column required by the native material-program vertex interface.
void bind_inputs(Fixture& fixture) {
    constexpr std::array<float, 48> kMatrices{};
    glGenVertexArrays(1, &fixture.vao);
    glBindVertexArray(fixture.vao);
    glGenBuffers(1, &fixture.buffer);
    glBindBuffer(GL_ARRAY_BUFFER, fixture.buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kMatrices), kMatrices.data(), GL_STATIC_DRAW);
    // Four vec4 columns occupy consecutive vertex locations.
    for (unsigned column = 0; column < 4; ++column) {
        glEnableVertexAttribArray(column);
        // All unused columns may read the same synthetic zero value.
        glVertexAttribPointer(column, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), nullptr);
    }
}

// Builds production leaf functions against a synthetic asset-free lighting bank.
bool build_shader(Fixture& fixture, std::string& error) {
    const std::string fragment = std::string("#version 430 core\n") + kInterface
                                 + lighting::shaders::shading_brdf_glsl
                                 + lighting::shaders::foliage_glsl + kCases;
    const bool built = fixture.shader.build(
        "foliage reference",
        R"(#version 430 core
in mat4 model;
void main() {
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0,1)+model[0];
})",
        fragment,
        error
    );
    CHECK(built);
    if (!built) {
        std::printf("%s\n", error.c_str());
        return false;
    }
    return true;
}

// Exercises packed material words through both the normal and bindless Vulkan program paths.
int test_gpu() {
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::printf("SKIP: %s\n", error.c_str());
        return test::kSkip;
    }
    Fixture fixture;
    // Shader failure must not continue into readback or mask its diagnostics.
    if (!build_shader(fixture, error)) {
        return test::result();
    }
    const bool created = fixture.frame.create(64, 1, error);
    CHECK(created);
    if (!created) {
        std::printf("%s\n", error.c_str());
        return test::result();
    }
    bind_inputs(fixture);
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.frame.id());
    glViewport(0, 0, 64, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    fixture.shader.use();
    glUniform4f(fixture.shader.uniform("material_colour"), 1, 1, 1, 1);
    glUniform1f(fixture.shader.uniform("material_diffuse_scale"), 1);
    glUniform1f(fixture.shader.uniform("foliage_reflectivity"), .04f);
    const auto words = enhanced::pack_foliage_material({1, 1, 1, 1});
    glUniform1i(
        fixture.shader.uniform("foliage_parameters0"), std::bit_cast<std::int32_t>(words[0])
    );
    glUniform1i(
        fixture.shader.uniform("foliage_parameters1"), std::bit_cast<std::int32_t>(words[1])
    );
    glDrawArrays(GL_TRIANGLES, 0, 3);
    const auto pixels = fixture.frame.read_rgba();
    CHECK(pixels.size() == 256u);
    // A failed readback is reported without accessing an incomplete image.
    if (pixels.size() == 256u) {
        test_lighting(pixels);
    }
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    return test::result();
}
}  // namespace

// Synthetic optical tests never need a level, a disc or audio.
int main() {
    try {
        return test_gpu();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
