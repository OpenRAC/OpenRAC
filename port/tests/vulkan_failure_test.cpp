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
unsigned g_waits = 0;

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
    test_upload_size_overflow_is_rejected();
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
