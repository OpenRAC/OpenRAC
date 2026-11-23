// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Ratchet & Clank's own menus under the mouse, apart from the game so they are tested alone: the
// map screen and the vendor's monitors. The parts every game shares are in
// game/common/keyboard_mouse/kbm.hpp.
#ifndef KBM_RAC1_MENUS_HPP
#define KBM_RAC1_MENUS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "game/common/keyboard_mouse/kbm.hpp"

namespace kbm {

// ---- rac1's map screen ----
//
// The mouse drags the map and the wheel zooms it, as the sticks do (update_map_zoom_and_pan): the
// pan is 16.15 fixed point over 0..0x10000000, the zoom 0.65..4.

// The pan for one pixel of the game's screen at zoom 1, across and down (measured: the map
// follows the mouse).
inline constexpr float kMapPanPerPixelX = 522000.0f;
inline constexpr float kMapPanPerPixelY = 436000.0f;
inline constexpr float kMapZoomMin = 0.65f;
inline constexpr float kMapZoomMax = 4.0f;
inline constexpr float kMapZoomPerNotch = 1.15f;

// A map's pan dragged `pixels` along one axis at `zoom`, within the game's bounds for that axis
// (y keeps 1280 / zoom of its range at each end, x none).
inline std::int32_t map_pan(std::int32_t pan, float pixels, float zoom, bool y_axis) {
    const float z = std::clamp(zoom, kMapZoomMin, kMapZoomMax);
    const std::int64_t moved =
        static_cast<std::int64_t>(pan)
        - static_cast<std::int64_t>(pixels * (y_axis ? kMapPanPerPixelY : kMapPanPerPixelX) / z);
    const std::int64_t lo =
        static_cast<std::int64_t>(static_cast<int>((y_axis ? 1280.0f : 0.0f) / z)) << 15;
    const std::int64_t hi = 0x10000000 - lo;
    return static_cast<std::int32_t>(std::clamp(moved, lo, std::max(lo, hi)));
}

// A map's zoom after the wheel's `notches` (in for more), within the game's bounds.
inline float map_zoom(float zoom, float notches) {
    return std::clamp(zoom * std::pow(kMapZoomPerNotch, notches), kMapZoomMin, kMapZoomMax);
}

// ---- rac1's vendor ----
//
// The vendor draws its screens into textures and shows them on monitors in the scene
// (render_vendor_capture_pass_sequence); the camera there is the vendor's own, so the monitors are
// always at the same place in the picture. Places are fractions of the 4:3 picture (pointer_x/y),
// measured in a run (assumed the same on every vendor and disc).
struct FracRect {
    float x0, y0, x1, y1;

    bool contains(float x, float y) const { return x >= x0 && x <= x1 && y >= y0 && y <= y1; }
};

inline constexpr FracRect kVendorBuy{47.0f / 640, 270.0f / 480, 183.0f / 640, 352.0f / 480};
inline constexpr FracRect kVendorExit{457.0f / 640, 268.0f / 480, 597.0f / 640, 350.0f / 480};
// The icon strip's monitor: its height, and its width, which the vendor narrows for fewer than 7
// rows (open_vendor_menu moves its corners in by 7 - rows steps). Its texture shows one texel a
// pixel of the game's 512 wide picture from its left edge: an icon each 56 texels from 8, the
// monitor 16 texels wider than seven of them, about the middle of the picture (measured: with 2
// rows its left edge at 192.8, and from 8 rows the selected slot in the middle).
inline constexpr float kVendorStripY0 = 372.0f / 480;
inline constexpr float kVendorStripY1 = 458.0f / 480;
inline constexpr float kVendorStripStep = 56.0f;
inline constexpr int kVendorStripMostShown = 7;
// From 8 rows the strip scrolls: eleven icons from -100 texels, the selected one the sixth.
inline constexpr int kVendorScrollRows = 8;

// The strip monitor's width in texels, for `count` rows.
inline float vendor_strip_width(int count) {
    return 16.0f + kVendorStripStep * static_cast<float>(std::min(count, kVendorStripMostShown));
}

enum class VendorSpot {
    None,
    Buy,   // the left monitor: Cross
    Exit,  // the right monitor: Triangle
    Item,  // an icon of the strip
};

struct VendorHit {
    VendorSpot spot = VendorSpot::None;
    int steps = 0;  // for an icon: the d-pad's steps to it, right for more
};

// What is under the pointer (x, y, fractions of the picture) on the vendor's screen, with `count`
// rows, `cursor` the selected one and `scroll` the strip's slide.
inline VendorHit vendor_hit(float x, float y, int count, int cursor, int scroll) {
    VendorHit h;

    // The two monitors with the buttons.
    if (kVendorBuy.contains(x, y)) {
        h.spot = VendorSpot::Buy;
        return h;
    }

    if (kVendorExit.contains(x, y)) {
        h.spot = VendorSpot::Exit;
        return h;
    }

    // No rows, or off the strip's monitor: nothing.
    if (count <= 0 || cursor < 0 || cursor >= count || y < kVendorStripY0 || y > kVendorStripY1) {
        return h;
    }

    const float width = vendor_strip_width(count);
    const float tx = x * kScreenWidth - (kScreenWidth - width) * 0.5f;

    // Left or right of the monitor.
    if (tx < 0.0f || tx > width) {
        return h;
    }

    // Few rows: one icon each from 8 texels, the strip still.
    if (count < kVendorScrollRows) {
        const float at = (tx - 8.0f) / kVendorStripStep;
        const int i = static_cast<int>(std::floor(at));

        // Only on an icon there is.
        if (at >= 0.0f && i < count) {
            h.spot = VendorSpot::Item;
            h.steps = i - cursor;
        }

        return h;
    }

    // Many rows: the slot under the pointer, the selected one at slot 5.
    const float at = (tx - static_cast<float>(scroll) + 100.0f - 4.0f) / kVendorStripStep;
    const int k = static_cast<int>(std::floor(at));

    // Only on one of the eleven slots.
    if (at >= 0.0f && k <= 10) {
        h.spot = VendorSpot::Item;
        h.steps = k - 5;
    }

    return h;
}

// The d-pad's step from row `cursor` toward row `target` of `count`: kLeft, kRight, or 0 there.
// From 8 rows the strip wraps round, so the shorter way.
inline std::uint16_t vendor_step(int cursor, int target, int count) {
    // There already, or rows that make no sense.
    if (cursor == target || count <= 0) {
        return 0;
    }

    // Few rows: no wrapping.
    if (count < kVendorScrollRows) {
        return target > cursor ? kRight : kLeft;
    }

    const int right = ((target - cursor) % count + count) % count;
    return right <= count - right ? kRight : kLeft;
}

}  // namespace kbm

#endif  // KBM_RAC1_MENUS_HPP
