// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Injects driver failures into a tiny offscreen Vulkan context; no retail assets are needed.
// Failed submissions must never wait, allocation retries must be bounded, and invalid sizes
// must leave existing resources intact. Vulkan entry points are restored before teardown.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/shader.h"
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl_internal.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"

namespace {

namespace vkgl = openrac::vkgl;
namespace detail = openrac::vkgl::detail;
namespace render_opt = openrac::renderer::render_opt;
using namespace openrac::gl;

// Restores the real driver before any resource owner tears down or an assertion fails.
template <typename Function>
class FunctionOverride {
public:
    FunctionOverride(Function& target, Function replacement)
        : m_target(target),
          m_original(std::exchange(target, replacement)) {}

    ~FunctionOverride() { m_target = m_original; }

    FunctionOverride(const FunctionOverride&) = delete;
    FunctionOverride& operator=(const FunctionOverride&) = delete;

private:
    Function& m_target;   // The process-global entry point, used only on this test's thread.
    Function m_original;  // The driver function to restore before teardown.
};

// Injection counters are accessed only by the test thread during synchronous Vulkan calls.
unsigned g_allocations = 0;
unsigned g_waits = 0;
unsigned g_layouts = 0;
PFN_vkAllocateDescriptorSets g_allocate = nullptr;

// Fails once so a regression that retries incorrectly fails a count check instead of hanging CI.
VKAPI_ATTR VkResult VKAPI_CALL fail_descriptor_allocation(
    VkDevice device, const VkDescriptorSetAllocateInfo* info, VkDescriptorSet* sets
) {
    ++g_allocations;
    return g_allocations == 1 ? VK_ERROR_OUT_OF_DEVICE_MEMORY : g_allocate(device, info, sets);
}

// A rejected submit has no work or signalled fence for the host to wait on.
VKAPI_ATTR VkResult VKAPI_CALL fail_submit(VkQueue, std::uint32_t, const VkSubmitInfo*, VkFence) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

// A bad implementation must fail promptly rather than wait forever on the rejected submit.
VKAPI_ATTR VkResult VKAPI_CALL
count_waits(VkDevice, std::uint32_t, const VkFence*, VkBool32, std::uint64_t) {
    ++g_waits;
    return VK_TIMEOUT;
}

// Stops shader module creation before any dependent layout/pipeline can be created.
VKAPI_ATTR VkResult VKAPI_CALL fail_module(
    VkDevice, const VkShaderModuleCreateInfo*, const VkAllocationCallbacks*, VkShaderModule*
) {
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

// Descriptor layout failure must stop linking before the pipeline layout is attempted.
VKAPI_ATTR VkResult VKAPI_CALL fail_set_layout(
    VkDevice,
    const VkDescriptorSetLayoutCreateInfo*,
    const VkAllocationCallbacks*,
    VkDescriptorSetLayout*
) {
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

// Counts an illegal continuation without handing an invalid descriptor layout to the real driver.
VKAPI_ATTR VkResult VKAPI_CALL count_pipeline_layouts(
    VkDevice, const VkPipelineLayoutCreateInfo*, const VkAllocationCallbacks*, VkPipelineLayout*
) {
    ++g_layouts;
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

// Uses one workgroup and no textures, so descriptor fault coverage needs no rendered scene.
bool build_compute(openrac::renderer::Shader& shader, std::string& error) {
    return shader.build_compute(
        "Vulkan failure fixture",
        "#version 430 core\nlayout(local_size_x=1) in; void main() {}",
        error
    );
}

// Invalid upload ranges and mip levels must not wrap arithmetic or discard the previous texture.
void test_rejects_invalid_resource_ranges() {
    constexpr std::array<std::uint8_t, 4> kBytes{1, 2, 3, 4};
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, kBytes.size(), kBytes.data(), GL_STATIC_DRAW);
    const auto maximum = std::numeric_limits<GLintptr>::max();
    glBufferSubData(GL_ARRAY_BUFFER, maximum, maximum, kBytes.data());
    CHECK(glGetError() == detail::kGlInvalidValue);
    CHECK(detail::g->buffers.find(buffer)->data.size() == kBytes.size());
    CHECK(std::equal(kBytes.begin(), kBytes.end(), detail::g->buffers.find(buffer)->data.begin()));
    glBufferSubData(GL_ARRAY_BUFFER, 0, kBytes.size(), nullptr);
    CHECK(glGetError() == detail::kGlInvalidValue);
    glDeleteBuffers(1, &buffer);

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, kBytes.data());
    const auto image = detail::g->textures.find(texture)->image;
    glTexImage2D(
        GL_TEXTURE_2D,
        std::numeric_limits<unsigned>::digits,
        GL_RGBA8,
        1,
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    CHECK(glGetError() == detail::kGlInvalidValue);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        std::numeric_limits<int>::max(),
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    CHECK(glGetError() == detail::kGlInvalidValue);
    CHECK(detail::g->textures.find(texture)->image == image);
    glDeleteTextures(1, &texture);
}

// Shader module allocation errors surface through the normal build result and diagnostic log.
void test_shader_module_failure_is_reported() {
    openrac::renderer::Shader shader;
    std::string error;
    const FunctionOverride injected(vkCreateShaderModule, fail_module);
    CHECK(!build_compute(shader, error));
    CHECK(error.find("shader module") != std::string::npos);
    CHECK(error.find(std::to_string(VK_ERROR_OUT_OF_DEVICE_MEMORY)) != std::string::npos);
}

// Failed descriptor layouts neither leak null handles downstream nor lose their driver diagnostic.
void test_shader_layout_failure_stops_linking() {
    openrac::renderer::Shader shader;
    std::string error;
    g_layouts = 0;
    const FunctionOverride descriptor(vkCreateDescriptorSetLayout, fail_set_layout);
    const FunctionOverride pipeline(vkCreatePipelineLayout, count_pipeline_layouts);
    CHECK(!build_compute(shader, error));
    CHECK(error.find("descriptor set layout") != std::string::npos);
    CHECK(g_layouts == 0);
}

// Impossible upload sizes fail before staging allocation or pointer arithmetic.
void test_upload_size_overflow_is_rejected() {
    bool rejected = false;
    try {
        (void)detail::ring(std::numeric_limits<VkDeviceSize>::max());
    } catch (const std::length_error&) {
        rejected = true;
    }
    CHECK(rejected);
}

// Enhanced passes cannot dispatch outside the workgroup counts reported by the device.
void test_rejects_oversized_dispatch() {
    const auto before = vkgl::submission_counters().commands;
    auto& maximum = detail::g->properties.limits.maxComputeWorkGroupCount[0];
    const auto original = maximum;
    // Some drivers allow UINT32_MAX groups; use a smaller cached limit to avoid a wrapping test input.
    maximum = 1;
    openrac::renderer::world::gl::glDispatchCompute(2, 1, 1);
    maximum = original;
    CHECK(glGetError() == detail::kGlInvalidValue);
    CHECK(vkgl::submission_counters().commands == before);
}

// Descriptor OOM is fatal for this recording rather than an instruction to allocate more pools.
void test_descriptor_failure_is_not_retried() {
    openrac::renderer::Shader shader;
    std::string error;
    CHECK(build_compute(shader, error));
    shader.use();
    render_opt::set(render_opt::Opt::VkPersistentDescriptors, false);
    g_allocate = vkAllocateDescriptorSets;
    g_allocations = 0;
    const FunctionOverride injected(vkAllocateDescriptorSets, fail_descriptor_allocation);
    bool rejected = false;
    try {
        openrac::renderer::world::gl::glDispatchCompute(1, 1, 1);
    } catch (const std::runtime_error& failure) {
        rejected =
            std::string(failure.what()).find("allocate frame descriptors") != std::string::npos;
    }
    CHECK(rejected);
    CHECK(g_allocations == 1);
    render_opt::reset();
}

// A failed queue submit leaves the slot unsubmitted and does not enter its fence wait.
void test_failed_submit_never_waits() {
    g_waits = 0;
    const FunctionOverride submit(vkQueueSubmit, fail_submit);
    const FunctionOverride wait(vkWaitForFences, count_waits);
    bool rejected = false;
    try {
        detail::flush_and_wait();
    } catch (const std::runtime_error& failure) {
        rejected = std::string(failure.what()).find("submit frame") != std::string::npos;
    }
    CHECK(rejected);
    CHECK(g_waits == 0);
    CHECK(!detail::frame().submitted);
}

// Runs every check in one tiny context; injected entry points are restored before shutdown.
int run_tests() {
    vkgl::Config config;
    config.width = 8;  // A tiny attachment suffices; these tests do not rasterise scenes.
    config.height = 8;
    config.validation = true;
    std::string error;

    // Hosts without Vulkan report a CTest skip rather than passing without exercising the backend.
    if (!vkgl::open(config, error)) {
        std::printf("SKIP: Vulkan unavailable: %s\n", error.c_str());
        return 77;
    }

    CHECK(openrac::gl::load(vkgl::get_proc, error));
    test_rejects_invalid_resource_ranges();
    test_shader_module_failure_is_reported();
    test_shader_layout_failure_stops_linking();
    test_upload_size_overflow_is_rejected();
    test_rejects_oversized_dispatch();
    test_descriptor_failure_is_not_retried();
    test_failed_submit_never_waits();
    CHECK(vkgl::submission_counters().validation_errors == 0);
    vkgl::close();
    return openrac::test::result();
}

}  // namespace

int main() {
    try {
        return run_tests();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "Vulkan failure regression: %s\n", failure.what());
    } catch (...) {
        std::fprintf(stderr, "Vulkan failure regression: unexpected exception\n");
    }
    return 1;
}
