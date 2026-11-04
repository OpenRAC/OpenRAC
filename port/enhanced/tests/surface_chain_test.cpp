// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synthetic geometry drives the real GTAO, probe selection and early GI through Renderer::render.
#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "enhanced/renderer/gtao.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/viewer/early_lighting.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced/viewer/probe_selection.h"
#include "renderer/colour_graph.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/renderer.h"
#include "renderer/surface_chain.h"
#include "renderer/vulkan/frame_capture.h"
#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/world/gl_world.h"
#include "renderer/world/gpu.h"
#include "scene/shadow_graph.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
namespace opt = renderer::render_opt;
// Fullscreen synthetic geometry, with a depth discontinuity and valid encoded smooth normals.
constexpr const char* kVertex = R"(#version 430 core
uniform bool edge_coverage;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));vec2 xy=p*2.-1.;
gl_Position=vec4(edge_coverage?xy*.75:xy,0,1);})";
constexpr const char* kDepth = R"(#version 430 core
uniform mat4 projection;out vec4 colour;
void main(){float d=gl_FragCoord.x<11.?4.:8.;vec4 p=projection*vec4(0,0,-d,1);
gl_FragDepth=p.z/p.w*.5+.5;colour=vec4(.5,.5,1,1);})";
constexpr const char* kColour = R"(#version 430 core
uniform samplerBuffer early_values;uniform usamplerBuffer early_guides;uniform mat4 projection;uniform int cells_x;out vec4 colour;
void main(){ivec2 p=ivec2(gl_FragCoord.xy);ivec2 c=p/2;
int i=c.y*cells_x+c.x;vec4 light=texelFetch(early_values,i);
uint guide=texelFetch(early_guides,i*2+1).r;
colour=vec4(light.rgb*(.25+.75*light.a),float((guide>>20u)&255u)/255.);
float d=gl_FragCoord.x<11.?4.:8.;vec4 clip=projection*vec4(0,0,-d,1);
gl_FragDepth=clip.z/clip.w*.5+.5;})";
struct Fixture;

// A minimal terrain provider exercises production eligibility, prepass and colour-depth reuse.
class Terrain final : public renderer::BucketRenderer {
public:
    // Borrows the fixture through every prepared and executed callback.
    explicit Terrain(Fixture& fixture)
        : BucketRenderer("surface fixture", renderer::Bucket::Terrain),
          m_fixture(fixture) {}

    bool init(renderer::RenderState&, std::string&) override { return true; }

    bool enhanced_lighting_available() const override { return true; }

    renderer::AoLighting ao_lighting() const override { return renderer::AoLighting::Indirect; }

    bool needs_early_lighting(const renderer::FrameInput&) const override { return true; }

    bool batches_opaque_colour() const override { return true; }

    bool opaque_colour_barrier() const override;
    bool prepares_colour_chain() const override;
    std::span<const unsigned> opaque_colour_buffers(
        const renderer::FrameInput&, const renderer::RenderState&
    ) const override;
    void render_opaque_colour(
        unsigned, const renderer::FrameInput&, renderer::RenderState&
    ) override;

    bool prepares_surface_chain(
        const renderer::FrameInput&, const renderer::RenderState&
    ) const override {
        return true;
    }

    void render_depth(const renderer::FrameInput&, renderer::RenderState&) override;
    void render_early_lighting(
        unsigned, const renderer::FrameInput&, renderer::RenderState&
    ) override;
    void render(const renderer::FrameInput&, renderer::RenderState&) override;

private:
    // Draws the depth-equal cache consumer in the existing opaque callback.
    void draw_colour(const renderer::FrameInput&, renderer::RenderState&);
    Fixture& m_fixture;  // Fixture owns the renderer and outlives all scoped callbacks.
};

// Providers and allocations remain owned throughout the synchronous prepare/execute interval.
struct Fixture {
    renderer::Renderer renderer;         // Owns the minimal terrain and actual GTAO implementation.
    lighting::ProbeLighting probes;      // GPU-quantized constant irradiance, with no retail data.
    lighting::ProbeSelection selection;  // Real shortlist and tile-mask kernels.
    lighting::EarlyLighting early;       // Real fused GI/AO kernel.
    renderer::FrameBuffer shadow_cache;  // Static depth producer for native combination.
    renderer::FrameBuffer shadow_atlas;  // Copy plus dynamic shadow overlay.
    renderer::FrameBuffer hero_map;      // Independent hero depth sampled by the early kernel.
    unsigned combined_shadow = 0;        // Complete native R32F composition output.
    bool edge_coverage =
        false;  // A three-quarter-scale triangle creates partially covered MSAA edge pixels.
    bool shadow_inputs = true;  // Disabling isolates whether early lighting consumes both maps.
    bool expose_colour_buffers =
        true;                        // Unknown providers must keep colour outside the shared plan.
    bool blend_boundary = true;      // Blended work must split otherwise compatible opaque layers.
    bool join_world_colour = false;  // Existing prepass-only cases keep their precise chain counts.
    std::vector<unsigned> order;     // Records actual callbacks, rather than graph preparation.
    renderer::Shader validation;     // Tiny writer for atomic preparation-failure checks.
    renderer::world::GpuBuffer validation_buffer;  // Owned storage for the isolated writer.
    renderer::Shader depth;
    renderer::Shader colour;       // Synthetic geometry and cache display.
    renderer::FrameBuffer target;  // Shared across warmed reference/prepared comparisons.
    renderer::FrameInput input;    // Lives through all callbacks and later colour draws.
    unsigned vao = 0;  // Attribute-free fullscreen geometry.
    // Initializes shaders, constant probes and provider registration before graph construction.
    Fixture();
    // Releases providers while the graphics context is current.
    ~Fixture();
    // Uploads constant GI and selection metadata before constructing any frame graph.
    void prepare_probes();
    // Allocates tiny shadow resources before any chain imports.
    void prepare_shadow_targets();
    // Declares real static/copy/dynamic/hero phases and native composition within the lighting chain.
    void prepare_shadows();
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
    // Resizes targets before any graph import.
    void resize(int width, int height, bool half, int samples = 1);
    // Binds the actual early kernel's GI, selection and raw AO inputs.
    void bind(const renderer::Shader&, const renderer::FrameInput&, const renderer::RenderState&);
};

// A second opaque layer records ordering without obscuring the fixture's cache-sampling triangle.
class FollowingLayer final : public renderer::BucketRenderer {
public:
    // Borrows the fixture until its renderer releases all registered layers.
    explicit FollowingLayer(Fixture& fixture)
        : BucketRenderer("following layer", renderer::Bucket::Ties),
          m_fixture(fixture) {}

    bool batches_opaque_colour() const override { return true; }

    bool prepares_colour_chain() const override { return true; }

    std::span<const unsigned> opaque_colour_buffers(
        const renderer::FrameInput& input, const renderer::RenderState& state
    ) const override {
        return m_fixture.early.buffers(input, state);
    }

    void render_opaque_colour(
        unsigned phase, const renderer::FrameInput&, renderer::RenderState&
    ) override {
        m_fixture.order.push_back(3 + phase);
    }

    void render(const renderer::FrameInput&, renderer::RenderState& state) override {
        CHECK(state.opaque_colour_batched);
        m_fixture.order.push_back(5);
    }

private:
    Fixture& m_fixture;  // Fixture owns this layer through the synchronous frame.
};

// ---- Fixture resources ----

Fixture::Fixture() {
    std::string error;
    CHECK(depth.build("surface depth", kVertex, kDepth, error));
    CHECK(colour.build("surface colour", kVertex, kColour, error));
    glGenVertexArrays(1, &vao);
    prepare_probes();
    prepare_shadow_targets();
    CHECK(validation.build_compute(
        "surface preparation validator",
        R"(#version 430 core
layout(local_size_x=1) in;layout(std430,binding=0) buffer Data{uint value;};
void main(){value=123u;})",
        error
    ));
    const unsigned zero = 0;
    validation_buffer.upload(GL_SHADER_STORAGE_BUFFER, &zero, sizeof(zero), true);
    renderer.set_ambient_occlusion(std::make_unique<renderer::Gtao>());
    CHECK(renderer.add(std::make_unique<Terrain>(*this), error));
    input.gtao = true;
    input.prepares_world_depth = true;
    input.after_world_depth = [this](unsigned source, int width, int height) {
        selection.dispatch(source, input.camera, width, height);
    };
}

void Fixture::prepare_probes() {
    std::string error;
    lighting::ProbeGrid grid;
    grid.origin = {-64, -64, -64};
    grid.spacing = {64, 64, 64};
    grid.dimensions = {3, 3, 3};
    auto volume = lighting::ProbeVolume::allocate(grid);
    // Every synthetic lattice point is usable so visibility cannot hide a scheduling error.
    for (auto& probe : volume.probes) {
        probe.valid = true;
    }
    // Unequal channels expose missing GI bindings in exact image comparisons.
    for (auto& texel : volume.irradiance) {
        texel = {.25f, .5f, .75f};
    }
    // Distant synthetic occluders leave the test geometry unobstructed.
    for (auto& moments : volume.distance_moments) {
        moments = {1024, 1024 * 1024};
    }
    lighting::update_probe_gutters(volume);
    CHECK(probes.upload(test::gpu_probes(volume), error));
    probes.normal_bias = 0;
    probes.view_bias = 0;
    const std::array grids{grid};
    const std::array offsets{0u};
    CHECK(selection.prepare(grids, offsets, error));
}

// Clears binary-exact synthetic depth while matching the production shadow attachment state.
void clear_shadow(unsigned target, double depth) {
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glViewport(0, 0, 8, 8);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Fixture::prepare_shadow_targets() {
    std::string error;
    // Eight texels cover copied depth, a dynamic patch and wrapped static sampling.
    CHECK(shadow_cache.create_depth(8, 8, error));
    CHECK(shadow_atlas.create_depth(8, 8, error));
    CHECK(hero_map.create_depth(8, 8, error));
    glGenTextures(1, &combined_shadow);
    CHECK(vkgl::allocate_storage_texture(combined_shadow, 0x822E, 8, 8));
}

// Turns a failed shadow declaration into the existing frame error boundary.
void require_shadow(Result<void> result) {
    // Incomplete producers must never reach an image-equivalence assertion as black fallback lighting.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

void Fixture::prepare_shadows() {
    require_shadow(
        scene::shadow_depth_pass(
            "surface static shadows",
            shadow_cache.id(),
            scene::ShadowDepthContents::Clear,
            [this] { clear_shadow(shadow_cache.id(), .25); },
            scene::ShadowDepthConsumer::Copy
        )
    );
    require_shadow(scene::shadow_depth_copy(shadow_cache.id(), shadow_atlas.id(), [this] {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, shadow_cache.id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_atlas.id());
        glBlitFramebuffer(0, 0, 8, 8, 0, 0, 8, 8, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }));
    require_shadow(
        scene::shadow_depth_pass(
            "surface dynamic shadows",
            shadow_atlas.id(),
            scene::ShadowDepthContents::Preserve,
            [this] {
                glBindFramebuffer(GL_FRAMEBUFFER, shadow_atlas.id());
                glDepthMask(GL_TRUE);
                glEnable(GL_SCISSOR_TEST);
                glScissor(3, 3, 2, 2);
                glClearDepth(.75);
                glClear(GL_DEPTH_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);
            }
        )
    );
    require_shadow(
        scene::shadow_depth_pass(
            "surface hero shadows", hero_map.id(), scene::ShadowDepthContents::Clear, [this] {
                clear_shadow(hero_map.id(), .5);
            }
        )
    );
    CHECK(
        vkgl::combine_shadow_depth(
            shadow_cache.depth_texture(),
            shadow_atlas.depth_texture(),
            combined_shadow,
            {2, -1},
            {0, 0, 8, 8}
        )
    );
}

// Binds both native composition and hero depth to the production early-lighting shader.
void bind_shadows(
    const Fixture& fixture, const renderer::Shader& shader, const renderer::Camera& camera
) {
    glUniform1i(shader.uniform("sun_shadow_enabled"), fixture.shadow_inputs ? 1 : 0);
    glUniform1i(shader.uniform("hero_shadow_enabled"), fixture.shadow_inputs ? 1 : 0);
    glUniform1i(shader.uniform("sun_shadow_separate_maps"), 1);
    glUniformMatrix4fv(shader.uniform("sun_shadow_view"), 1, GL_FALSE, camera.view.data());
    glUniform4f(shader.uniform("sun_shadow_ends"), 16, 32, 64, 128);
    glUniform1f(shader.uniform("sun_shadow_near"), .1f);
    const auto matrix = renderer::scale(.01f);
    glUniformMatrix4fv(shader.uniform("sun_shadow_matrix[0]"), 1, GL_FALSE, matrix.data());
    glUniformMatrix4fv(shader.uniform("hero_shadow_matrix"), 1, GL_FALSE, matrix.data());
    glUniform1i(shader.uniform("sun_shadow_map0"), 7);
    glUniform1i(shader.uniform("hero_shadow_map"), 6);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, fixture.combined_shadow);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, fixture.hero_map.depth_texture());
    glActiveTexture(GL_TEXTURE0);
}

Fixture::~Fixture() {
    glDeleteTextures(1, &combined_shadow);
    validation_buffer.release();
    renderer.release();
    early.release();
    selection.release();
    probes.release();
    glDeleteVertexArrays(1, &vao);
}

void Fixture::resize(int width, int height, bool half, int samples) {
    std::string error;
    CHECK(target.create(width, height, error, samples, samples == 1));
    input.camera.projection = renderer::perspective_reversed(
        std::numbers::pi_v<float> / 2,
        static_cast<float>(width) / static_cast<float>(height),
        .1f,
        128
    );
    input.camera.view = renderer::identity();
    input.gtao_settings.half_resolution = half;
    input.gtao_settings.hi_z = false;
}

void Fixture::bind(
    const renderer::Shader& shader,
    const renderer::FrameInput& frame,
    const renderer::RenderState& state
) {
    probes.bind(shader, scene::Layer::Terrain, true, &frame.camera);
    selection.bind(shader, &frame.camera, true);
    glUniform1i(shader.uniform("early_ao_enabled"), state.gtao_raw != 0 ? 1 : 0);
    glUniform1i(shader.uniform("early_ao"), 18);
    glUniform1i(shader.uniform("early_ao_scale"), state.gtao_scale);
    glUniform1f(shader.uniform("early_ao_strength"), state.gtao_strength);
    glActiveTexture(GL_TEXTURE0 + 18);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, state.gtao_raw);
    glActiveTexture(GL_TEXTURE0);
    bind_shadows(*this, shader, frame.camera);
}

void Terrain::render_depth(const renderer::FrameInput& input, renderer::RenderState&) {
    m_fixture.depth.use();
    glUniform1i(m_fixture.depth.uniform("edge_coverage"), m_fixture.edge_coverage);
    glUniformMatrix4fv(
        m_fixture.depth.uniform("projection"), 1, GL_FALSE, input.camera.projection.data()
    );
    glBindVertexArray(m_fixture.vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Terrain::render_early_lighting(
    unsigned source, const renderer::FrameInput& input, renderer::RenderState& state
) {
    // Real shadow producers precede their reflected early-lighting consumers in the shared plan.
    if (m_fixture.shadow_inputs) {
        m_fixture.prepare_shadows();
    }
    std::string error;
    state.early_lighting = m_fixture.early.render(
        source,
        input,
        state.target,
        [&](const renderer::Shader& shader) { m_fixture.bind(shader, input, state); },
        error
    );
    // Shader failures need their diagnostic before the image assertions.
    if (!state.early_lighting) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    CHECK(state.early_lighting);
}

void Terrain::render(const renderer::FrameInput& input, renderer::RenderState& state) {
    m_fixture.order.push_back(2);
    // A queued opaque batch must not draw again through the ordinary bucket entry point.
    if (!state.opaque_colour_batched) {
        draw_colour(input, state);
    }
}

bool Terrain::opaque_colour_barrier() const {
    return m_fixture.blend_boundary;
}

bool Terrain::prepares_colour_chain() const {
    return m_fixture.join_world_colour;
}

std::span<const unsigned> Terrain::opaque_colour_buffers(
    const renderer::FrameInput& input, const renderer::RenderState& state
) const {
    // Unknown providers deliberately retain the separately executed colour phase.
    if (!m_fixture.expose_colour_buffers) {
        return {};
    }
    return m_fixture.early.buffers(input, state);
}

void Terrain::render_opaque_colour(
    unsigned phase, const renderer::FrameInput& input, renderer::RenderState& state
) {
    m_fixture.order.push_back(phase);
    // This fixture has one solid triangle and no cutout geometry.
    if (phase == 0) {
        draw_colour(input, state);
    }
}

void Terrain::draw_colour(const renderer::FrameInput& input, renderer::RenderState& state) {
    m_fixture.colour.use();
    glUniform1i(m_fixture.colour.uniform("edge_coverage"), m_fixture.edge_coverage);
    m_fixture.early.bind(m_fixture.colour, input, state);
    glUniform1i(m_fixture.colour.uniform("cells_x"), (state.target.width + 1) / 2);
    glUniformMatrix4fv(
        m_fixture.colour.uniform("projection"), 1, GL_FALSE, input.camera.projection.data()
    );
    // Match the prepass depth without sampling the attachment bound for opaque rendering.
    glDepthFunc(GL_EQUAL);
    glBindVertexArray(m_fixture.vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthFunc(GL_GEQUAL);
}

// ---- Comparisons and rejection ----

// Complete displayed bytes and command counts from one synchronous synthetic frame.
struct RecordedFrame {
    std::vector<std::uint8_t> pixels;  // Full colour output, including alpha.
    std::uint64_t barriers = 0;  // Native barrier calls, without readback's later transitions.
    std::uint64_t chains = 0;    // Successfully executed shared plans.
    std::uint64_t draws = 0;     // Actual draw commands detect accidental batch replay.
};

// Runs one frame and reads its result after all declared work has executed.
RecordedFrame render_frame(Fixture& fixture, bool expose_depth = true) {
    fixture.order.clear();
    const auto before = vkgl::submission_counters();
    ++fixture.input.frame;
    renderer::RenderTarget target{
        fixture.target.id(), fixture.target.width(), fixture.target.height()
    };
    if (expose_depth) {
        target.depth_texture = fixture.target.depth_texture();
    }
    fixture.renderer.render(fixture.input, target);
    const auto after = vkgl::submission_counters();
    return {
        fixture.target.read_rgba(),
        after.barriers - before.barriers,
        after.surface_chains - before.surface_chains,
        after.commands[static_cast<unsigned>(vkgl::Command::Draw)]
            - before.commands[static_cast<unsigned>(vkgl::Command::Draw)]
    };
}

// Full pixels and native counters compare warmed execution, without timing assumptions.
void compare(Fixture& fixture, bool eligible = true) {
    // Warmup normalizes initial layouts and shader/resource preparation before both measured paths.
    render_frame(fixture);
    const auto reference = render_frame(fixture);
    const auto shared = render_frame(fixture);
    CHECK(reference.pixels == shared.pixels);
    CHECK(reference.draws == shared.draws);
    CHECK(shared.chains == (eligible ? 1u : 0u));
    // The supported chain must remove boundaries rather than add broad barriers.
    if (eligible) {
        CHECK(shared.barriers <= reference.barriers);
    }
    const bool lit = std::any_of(shared.pixels.begin(), shared.pixels.end(), [](auto value) {
        return value > 0 && value < 255;
    });
    CHECK(lit);
    std::printf(
        "surface chain barriers: %llu -> %llu\n",
        static_cast<unsigned long long>(reference.barriers),
        static_cast<unsigned long long>(shared.barriers)
    );
}

// Prepares a writer whose declared first access can deliberately require undefined prior contents.
Result<void> validation_dispatch(
    Fixture& fixture, renderer::graph::Access access, unsigned& prepared
) {
    namespace graph = renderer::graph;
    return renderer::run_surface_chain([&] {
        vkgl::GraphExecution execution;
        const auto output = execution.import_buffer(
            "validation output", fixture.validation_buffer.id(), vkgl::InitialContents::Discard
        );
        CHECK(output);
        execution.add_pass(
            {"validation dispatch",
             {},
             {{*output,
               {0, sizeof(unsigned)},
               {graph::Stage::ComputeShader, access, graph::Layout::Buffer}}}}
        );
        const auto queued = execution.run([&](unsigned) {
            ++prepared;
            fixture.validation.use();
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fixture.validation_buffer.id());
            glDispatchCompute(1, 1, 1);
        });
        CHECK(queued);
    });
}

// Compilation failure must occur after CPU preparation but before any GPU dispatch, then permit a
// fresh chain.
void test_failed_preparation(Fixture& fixture) {
    unsigned prepared = 0;
    const auto before = vkgl::submission_counters();
    const auto failed = validation_dispatch(fixture, renderer::graph::Access::ReadWrite, prepared);
    CHECK(!failed);
    CHECK(prepared == 1);
    const auto rejected = vkgl::submission_counters();
    CHECK(
        rejected.commands[static_cast<unsigned>(vkgl::Command::Dispatch)]
        == before.commands[static_cast<unsigned>(vkgl::Command::Dispatch)]
    );
    CHECK(validation_dispatch(fixture, renderer::graph::Access::Write, prepared));
    CHECK(prepared == 2);
    const auto recovered = vkgl::submission_counters();
    CHECK(recovered.surface_chains == before.surface_chains + 1);
    const auto unrepresented = renderer::run_surface_chain([] { glClear(GL_COLOR_BUFFER_BIT); });
    CHECK(!unrepresented);
}

// Synchronous material setup flushes producers and restores collection after success or a setup exception.
void test_setup_boundary() {
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(2, 2, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    bool produced = false;
    const auto recorded = renderer::run_surface_chain([&] {
        CHECK(
            renderer::colour_pass(
                "before material setup",
                {target.id(), 2, 2},
                [&] { produced = true; },
                renderer::ColourAttachments::Colour
            )
        );
        CHECK(!produced);
        CHECK(renderer::run_surface_setup([&] {
            CHECK(produced);
            CHECK(!renderer::collecting_surface_chain());
        }));
        CHECK(renderer::collecting_surface_chain());
        CHECK(!renderer::run_surface_setup([] { throw std::runtime_error("setup rejected"); }));
        CHECK(renderer::collecting_surface_chain());
    });
    CHECK(recorded);
    CHECK(!renderer::collecting_surface_chain());
    CHECK(!renderer::run_surface_setup({}));
}

// Checks colour formats and depth/stencil limits because supported MSAA counts need not be contiguous.
VkSampleCountFlags surface_sample_counts() {
    vkgl::NativeDevice device;
    const bool available = vkgl::native_device(device);
    CHECK(available);

    // Failure remains a test error, without passing invalid handles to the driver.
    if (!available) {
        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkPhysicalDeviceProperties limits{};
    vkGetPhysicalDeviceProperties(device.physical, &limits);
    VkSampleCountFlags counts =
        limits.limits.framebufferDepthSampleCounts & limits.limits.framebufferStencilSampleCounts;
    // The world and normal targets share the renderer's depth format but use different colours.
    for (const auto format : {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT}) {
        VkImageFormatProperties properties{};
        const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const auto result = vkGetPhysicalDeviceImageFormatProperties(
            device.physical,
            format,
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,
            usage,
            0,
            &properties
        );
        CHECK(result == VK_SUCCESS);

        // Unsupported attachment formats cannot provide a valid multisample fixture.
        if (result != VK_SUCCESS) {
            return VK_SAMPLE_COUNT_1_BIT;
        }
        counts &= properties.sampleCounts;
    }
    return counts;
}

// Chooses a supported MSAA tier for ordering and rejection checks, or one without MSAA.
int surface_multisamples() {
    const auto counts = surface_sample_counts();
    // Both tiers still run separately in the complete coverage regression below.
    for (const int samples : {2, 4}) {
        if ((counts & static_cast<VkSampleCountFlags>(samples)) != 0) {
            return samples;
        }
    }
    return 1;
}

// Odd sizes, full/half AO and recreated resources keep exact output; unknown hooks use separate scopes.
void test_chain() {
    Fixture fixture;
    // Odd extents expose incorrect rounded buffer ranges across target recreation.
    for (const auto size : {std::array{37, 23}, std::array{19, 17}}) {
        // Both AO scales feed the same fused early-lighting consumer.
        for (bool half : {false, true}) {
            fixture.resize(size[0], size[1], half);
            compare(fixture);
        }
    }
    fixture.shadow_inputs = false;
    const auto unshadowed = render_frame(fixture);
    fixture.shadow_inputs = true;
    const auto shadowed = render_frame(fixture);
    CHECK(unshadowed.pixels != shadowed.pixels);
    fixture.expose_colour_buffers = false;
    compare(fixture);
    fixture.expose_colour_buffers = true;

    compare(fixture);

    fixture.input.prepares_world_depth = false;
    compare(fixture, false);
    fixture.input.prepares_world_depth = true;
    fixture.input.gtao_settings.hi_z = true;
    compare(fixture);
    fixture.resize(19, 17, true, surface_multisamples());
    compare(fixture);
    fixture.input.gtao_settings.hi_z = false;
    compare(fixture);
    test_failed_preparation(fixture);
}

// Every supported sample count preserves depth-equal coverage, resolved normals and both AO scales.
void test_multisample_chain() {
    Fixture fixture;
    fixture.edge_coverage = true;
    const auto counts = surface_sample_counts();
    // Tiny odd extents exercise resize and row rounding without full-scene compilation cost.
    for (int samples : {2, 4}) {
        // Some drivers support 4x MSAA without supporting 2x for these attachment formats.
        if ((counts & static_cast<VkSampleCountFlags>(samples)) == 0) {
            continue;
        }
        for (bool half : {false, true}) {
            fixture.resize(23, 17, half, samples);
            compare(fixture);
            fixture.resize(19, 13, half, samples);
            compare(fixture);
        }
    }
    // Returning to a single-sample depth texture must discard the old resolve allocations and imports.
    fixture.resize(21, 15, false);
    compare(fixture);
}

// Shared depth skips the copy; multisample resolves still precede their sampled consumers.
void test_depth_order(test::GlContext& context, int samples) {
    Fixture fixture;
    fixture.edge_coverage = true;
    fixture.resize(23, 17, true, samples);
    render_frame(fixture, false);
    const auto reference = render_frame(fixture, false);
    render_frame(fixture);
    CHECK(vkgl::capture_frame_graph());
    const auto rendered = render_frame(fixture);
    CHECK(rendered.pixels == reference.pixels);
    fixture.target.blit_to(0, 64, 64);
    context.window().swap();
    const auto description = vkgl::frame_graph_description();
    CHECK(description);
    // A missing capture cannot prove producer/consumer order.
    if (!description) {
        return;
    }
    const auto prepass = description->find(" work depth/normal prepass\n");
    const auto reuse = description->find(" work surface depth reuse copy\n");
    const auto normals = description->find(" work surface normal resolve\n");
    const auto depth = description->find(" work surface depth resolve\n");
    const auto ao = description->find(" work GTAO horizons\n");
    const auto early = description->find(" work early GI and cascades\n");
    const auto colour = description->find(" work world opaque colour\n");
    if (samples == 1) {
        CHECK(prepass != std::string::npos && colour != std::string::npos);
        CHECK(reuse == std::string::npos && depth == std::string::npos);
        CHECK(prepass < ao && ao < early && early < colour);
        return;
    }
    CHECK(prepass != std::string::npos && reuse != std::string::npos);
    CHECK(normals != std::string::npos && depth != std::string::npos);
    CHECK(ao != std::string::npos && early != std::string::npos && colour != std::string::npos);
    CHECK(prepass < reuse && reuse < normals && normals < depth);
    CHECK(depth < ao && ao < early && early < colour);
}

// Confirms a rejected copy leaves GPU command counters and the completed-section count unchanged.
void check_rejected_copy(unsigned source, unsigned destination, renderer::SurfaceCopy aspect) {
    const auto before = vkgl::submission_counters();
    const auto result = renderer::run_surface_chain([&] {
        const auto queued = renderer::queue_surface_copy(source, destination, {4, 4}, aspect);
        // Propagate the rejected declaration exactly as the production surface path does.
        if (!queued) {
            throw std::runtime_error(queued.error().message);
        }
    });
    CHECK(!result);
    const auto after = vkgl::submission_counters();
    CHECK(before.commands == after.commands);
    CHECK(before.surface_chains == after.surface_chains);
}

// Bad extents, aliased attachments and unsupported sample conversions reject before GPU recording.
void test_surface_copy_contracts() {
    renderer::FrameBuffer source;
    renderer::FrameBuffer resized;
    renderer::FrameBuffer multisample;
    std::string error;
    CHECK(source.create(4, 4, error));
    CHECK(resized.create(5, 4, error));
    CHECK(multisample.create(4, 4, error, surface_multisamples()));
    // Both attachment types promise complete overwrites and must enforce the same coverage contract.
    for (auto aspect : {renderer::SurfaceCopy::DepthStencil, renderer::SurfaceCopy::Colour}) {
        // The first aliases, the second is partial, and the third would broadcast into MSAA.
        for (unsigned destination : {source.id(), resized.id(), multisample.id()}) {
            // A single-sample-only device cannot exercise unsupported multisample conversion.
            if (destination == multisample.id() && multisample.samples() == 1) {
                continue;
            }
            check_rejected_copy(source.id(), destination, aspect);
        }
    }
    source.release();
    resized.release();
    multisample.release();
}

void test_surface_depth_scratch() {
    renderer::FrameBuffer source;
    std::string error;
    const auto lease = [&](std::array<int, 2> size) {
        renderer::SurfaceDepthScratch scratch;
        CHECK(!renderer::lease_surface_depth(source.id(), size));
        CHECK(renderer::run_surface_chain([&] {
            scratch = renderer::lease_surface_depth(source.id(), size);
            CHECK(scratch && scratch.texture);
            CHECK(!renderer::lease_surface_depth(source.id(), size));
            CHECK(renderer::run_surface_setup([&] {
                CHECK(renderer::run_surface_chain([&] {
                    CHECK(!renderer::lease_surface_depth(source.id(), size));
                }));
            }));
        }));
        return scratch.texture;
    };
    CHECK(source.create(8, 6, error));
    const auto first = lease({8, 6});
    CHECK(lease({8, 6}) == first);
    const auto refraction = [&](std::array<int, 2> size) {
        renderer::SurfaceRefractionScratch scratch;
        CHECK(renderer::run_surface_chain([&] {
            scratch = renderer::lease_surface_refraction(source.id(), size);
            CHECK(scratch && scratch.textures[0] != scratch.textures[1]);
            CHECK(!renderer::lease_surface_depth(source.id(), size));
        }));
        return scratch.textures;
    };
    const auto first_snapshot = refraction({8, 6});
    CHECK(first_snapshot[1] == first);
    CHECK(refraction({8, 6}) == first_snapshot);
    CHECK(source.create(9, 7, error));
    const auto resized = lease({9, 7});
    CHECK(resized != first);
    CHECK(lease({9, 7}) == resized);
    const auto resized_snapshot = refraction({9, 7});
    CHECK(resized_snapshot[0] != first_snapshot[0] && resized_snapshot[1] == resized);
    source.release();
}

// Cache exports follow the actual opaque consumer only when that consumer supplied matching reads.
void test_colour_order(test::GlContext& context) {
    Fixture fixture;
    fixture.resize(23, 17, true);
    // Missing provider metadata keeps the same pixels while retaining the original export boundary.
    for (bool expose : {true, false}) {
        fixture.expose_colour_buffers = expose;
        render_frame(fixture);
        CHECK(vkgl::capture_frame_graph());
        render_frame(fixture);
        fixture.target.blit_to(0, 64, 64);
        context.window().swap();
        const auto description = vkgl::frame_graph_description();
        CHECK(description);
        // Missing captures must not hide an ordering failure.
        if (!description) {
            continue;
        }
        const auto producer = description->find(" work early GI and cascades\n");
        const auto consumer = description->find(" work world opaque colour\n");
        const auto exported = description->find(" export early GI and cascades consumers\n");
        CHECK(producer != std::string::npos);
        CHECK(consumer != std::string::npos);
        CHECK(exported != std::string::npos);
        CHECK(producer < consumer);
        CHECK(expose ? consumer < exported : exported < consumer);
    }
}

// A blend boundary splits batches; compatible adjacent layers retain solid-before-cutout ordering.
void test_blend_boundaries() {
    Fixture fixture;
    fixture.resize(23, 17, true);
    std::string error;
    CHECK(fixture.renderer.add(std::make_unique<FollowingLayer>(fixture), error));
    // Shared and immediate paths must traverse the identical sequence of actual draw callbacks.
    {
        fixture.blend_boundary = true;
        render_frame(fixture);
        CHECK(fixture.order == std::vector<unsigned>({0, 1, 2, 3, 4, 5}));
        fixture.blend_boundary = false;
        render_frame(fixture);
        CHECK(fixture.order == std::vector<unsigned>({0, 3, 1, 4, 2, 5}));
    }
}

// World-colour composition preserves both batch orders and each deferred layer's own frame flags.
void test_world_colour() {
    Fixture fixture;
    fixture.resize(23, 17, true);
    fixture.join_world_colour = true;
    std::string error;
    CHECK(fixture.renderer.add(std::make_unique<FollowingLayer>(fixture), error));
    // A blend boundary splits opaque rounds inside the shared sequence, without splitting draw order.
    for (bool boundary : {false, true}) {
        fixture.blend_boundary = boundary;
        const auto reference = render_frame(fixture);
        const auto expected_order = fixture.order;
        const auto shared = render_frame(fixture);
        CHECK(shared.pixels == reference.pixels);
        CHECK(shared.draws == reference.draws);
        CHECK(fixture.order == expected_order);
        CHECK(shared.chains == 2);
    }
}

// Unknown external hooks observe completed world colour before enabled or disabled particle buckets.
void test_world_hook_boundary() {
    Fixture fixture;
    fixture.resize(23, 17, true);
    fixture.join_world_colour = true;
    std::string error;
    CHECK(fixture.renderer.add(std::make_unique<FollowingLayer>(fixture), error));

    // The local class borrows this fixture only through the synchronous frame.
    class Particles final : public renderer::BucketRenderer {
    public:
        // Borrows the fixture through the synchronous test frame.
        explicit Particles(Fixture& fixture)
            : BucketRenderer("boundary particles", renderer::Bucket::Particles),
              m_fixture(fixture) {}

        void render(const renderer::FrameInput&, renderer::RenderState&) override {
            m_fixture.order.push_back(7);
        }

    private:
        Fixture& m_fixture;  // Fixture owns this bucket until the test ends.
    };

    auto* particles = fixture.renderer.add(std::make_unique<Particles>(fixture), error);
    CHECK(particles);
    // Initialization failure must retain its original diagnostic instead of dereferencing null.
    if (!particles) {
        return;
    }
    // The renderer invokes the hook after all preceding colour callbacks have completed.
    fixture.input.on_before_particles = [&fixture](const renderer::RenderTarget&) {
        CHECK(fixture.order == std::vector<unsigned>({0, 1, 2, 3, 4, 5}));
        fixture.order.push_back(6);
    };
    // Disabled particle renderers must not suppress the earlier opaque-colour hook.
    for (bool enabled : {true, false}) {
        particles->enabled = enabled;
        const auto frame = render_frame(fixture);
        CHECK(frame.chains == 2);
        CHECK(fixture.order.size() == (enabled ? 8u : 7u));
        CHECK(fixture.order[6] == 6);
    }
}

// Invalid explicit colour inputs reject preparation without invoking the deferred draw.
void test_invalid_colour_read() {
    Fixture fixture;
    fixture.resize(23, 17, true);

    bool called = false;
    const auto before = vkgl::submission_counters();
    const auto result = renderer::run_surface_chain([&] {
        const std::array invalid{0u};
        const auto recorded = renderer::colour_pass(
            "invalid colour input",
            {fixture.target.id(), 23, 17},
            [&called] { called = true; },
            renderer::ColourAttachments::ColourDepth,
            {invalid, {}}
        );
        CHECK(!recorded);
        // Propagate import errors through the enclosing chain's Result boundary.
        if (!recorded) {
            throw std::runtime_error(recorded.error().message);
        }
    });
    CHECK(!result && !called);
    CHECK(vkgl::submission_counters().surface_chains == before.surface_chains);
}
}  // namespace

// Runs synthetic GPU work only; unavailable Vulkan is the sole skip condition.
int main() {
    // Keep graphics setup failures visible to the test runner.
    try {
        test::GlContext context;
        std::string error;
        // Hosts without Vulkan cannot exercise native packet recording.
        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }
        test_chain();
        test_multisample_chain();
        test_depth_order(context, 1);
        if (const int samples = surface_multisamples(); samples > 1) {
            test_depth_order(context, samples);
        }
        test_surface_copy_contracts();
        test_surface_depth_scratch();
        test_colour_order(context);
        test_blend_boundaries();
        test_world_colour();
        test_world_hook_boundary();
        test_invalid_colour_read();
        test_setup_boundary();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        opt::reset();
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
