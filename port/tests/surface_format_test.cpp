// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The swapchain's format (renderer/vulkan/presentation/surface_format.h): SDR picks what the backend always
// picked, also when the surface lists HDR colour spaces first; HDR picks the asked kind, else the
// other one, and SDR where the encode pass cannot write either.
#include <vector>

#include "renderer/vulkan/presentation/surface_format.h"
#include "tests/check.h"

namespace {

using openrac::hdr::Output;
using openrac::vkgl::choose_surface_format;
using openrac::vkgl::offers_hdr;

constexpr VkSurfaceFormatKHR kBgra{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
constexpr VkSurfaceFormatKHR kBgraSrgb{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
constexpr VkSurfaceFormatKHR kRgb10Sdr{
    VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
};
constexpr VkSurfaceFormatKHR kHdr10{
    VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT
};
constexpr VkSurfaceFormatKHR kHdr10Bgr{
    VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT
};
constexpr VkSurfaceFormatKHR kScRgb{
    VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT
};

bool same(const VkSurfaceFormatKHR& a, const VkSurfaceFormatKHR& b) {
    return a.format == b.format && a.colorSpace == b.colorSpace;
}

bool store_all(void*, VkFormat) {
    return true;
}

bool store_none(void*, VkFormat) {
    return false;
}

bool store_floats_only(void*, VkFormat f) {
    return f == VK_FORMAT_R16G16B16A16_SFLOAT;
}

// The SDR choice before HDR existed: the first linear 8- or 10-bit format, else the first one.
void test_sdr_as_before() {
    const std::vector<VkSurfaceFormatKHR> desktop{kBgraSrgb, kBgra, kRgb10Sdr};
    auto c = choose_surface_format(desktop, Output::Sdr, store_all, nullptr);
    CHECK(same(c.format, kBgra) && c.output == Output::Sdr);
    const std::vector<VkSurfaceFormatKHR> only_srgb{kBgraSrgb};
    c = choose_surface_format(only_srgb, Output::Sdr, store_all, nullptr);
    CHECK(same(c.format, kBgraSrgb));
    // HDR colour spaces listed first never become the SDR choice.
    const std::vector<VkSurfaceFormatKHR> hdr_first{kHdr10, kScRgb, kBgraSrgb, kBgra};
    c = choose_surface_format(hdr_first, Output::Sdr, store_all, nullptr);
    CHECK(same(c.format, kBgra) && c.output == Output::Sdr);
}

void test_hdr_kinds() {
    const std::vector<VkSurfaceFormatKHR> windows{kBgra, kRgb10Sdr, kHdr10, kScRgb};
    auto c = choose_surface_format(windows, Output::Hdr10, store_all, nullptr);
    CHECK(same(c.format, kHdr10) && c.output == Output::Hdr10);
    c = choose_surface_format(windows, Output::ScRgb, store_all, nullptr);
    CHECK(same(c.format, kScRgb) && c.output == Output::ScRgb);
    // The other kind when the asked one cannot be written.
    c = choose_surface_format(windows, Output::Hdr10, store_floats_only, nullptr);
    CHECK(same(c.format, kScRgb) && c.output == Output::ScRgb);
    // Neither: SDR, as before.
    c = choose_surface_format(windows, Output::Hdr10, store_none, nullptr);
    CHECK(same(c.format, kBgra) && c.output == Output::Sdr);
    c = choose_surface_format(windows, Output::Hdr10, nullptr, nullptr);
    CHECK(c.output == Output::Sdr);
    // An HDR10 format the pass has no GLSL format for (A2R10G10B10) is not taken.
    const std::vector<VkSurfaceFormatKHR> bgr_only{kBgra, kHdr10Bgr};
    c = choose_surface_format(bgr_only, Output::Hdr10, store_all, nullptr);
    CHECK(same(c.format, kBgra) && c.output == Output::Sdr);
}

void test_offers() {
    CHECK(offers_hdr(std::vector<VkSurfaceFormatKHR>{kBgra, kHdr10}));
    CHECK(offers_hdr(std::vector<VkSurfaceFormatKHR>{kScRgb}));
    CHECK(!offers_hdr(std::vector<VkSurfaceFormatKHR>{kBgra, kRgb10Sdr, kHdr10Bgr}));
}

}  // namespace

int main() {
    test_sdr_as_before();
    test_hdr_kinds();
    test_offers();
    return openrac::test::result();
}
