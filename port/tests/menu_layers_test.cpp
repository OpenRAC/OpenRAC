// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests game/common/frontend/menu_layers.h: where the mobys a menu draws with a camera of its own
// go, alone (its picture of the frame under them) and over a level's world drawn live under it
// (the pause menu while a party plays), and where the 2D path is split for the world.

#include <array>
#include <cstddef>

#include "game/common/frontend/menu_layers.h"
#include "tests/check.h"

namespace {

using openrac::frontend::bytes_before_world;
using openrac::frontend::frame_zoom;
using openrac::frontend::FrameZoom;
using openrac::frontend::own_camera_layer;
using openrac::frontend::OwnCameraLayer;

// The frame's horizontal tangent in play, a level's camera zoomed in, and the zoom of 1.0 a page
// menu sets for the widgets it renders to textures.
constexpr float kFrameTan = 0.63f;
constexpr float kZoomedTan = 0.45f;
constexpr float kWidgetTan = 1.0f;
// A 2D list with a page menu's read-back picture of the frame before its first render to a texture.
constexpr std::size_t kMenuBeforeWorld = 0x41000;

// The pause menu alone: no world ran; its frame objects are the frame's (drawn in the world pass
// with their camera, over its picture of the frame), its widgets go into their textures.
void test_menu_alone_keeps_its_layers() {
    const std::array<float, 3> own{kFrameTan, kFrameTan, kWidgetTan};
    const FrameZoom zoom = frame_zoom(false, 0.0f, own);
    CHECK(zoom.camera == 0);
    CHECK(zoom.tan == kFrameTan);
    CHECK(own_camera_layer(kFrameTan, zoom, true, false) == OwnCameraLayer::World);
    CHECK(own_camera_layer(kWidgetTan, zoom, true, false) == OwnCameraLayer::Texture);
    CHECK(bytes_before_world(kMenuBeforeWorld, false) == kMenuBeforeWorld);
}

// In play the world's camera is the frame's: a vendor's screens, drawn wider, go into textures.
void test_world_camera_is_the_frame_in_play() {
    const std::array<float, 1> own{kWidgetTan};
    const FrameZoom zoom = frame_zoom(true, kFrameTan, own);
    CHECK(zoom.camera == FrameZoom::kWorld);
    CHECK(own_camera_layer(kWidgetTan, zoom, true, false) == OwnCameraLayer::Texture);
    // Focal lengths unknown: nothing can be fitted into a texture, it stays in the world.
    CHECK(own_camera_layer(kWidgetTan, zoom, false, false) == OwnCameraLayer::World);
}

// A level's world under the menu: its camera does not count. Counted, a level camera narrower than
// the menu's sent the menu's frame objects into textures that are never drawn (buttons without
// their frames); not counted, they go on the screen over the world, the widgets into textures.
void test_world_under_menu_puts_the_menu_on_the_screen() {
    const std::array<float, 3> own{kFrameTan, kFrameTan, kWidgetTan};
    const FrameZoom counted = frame_zoom(true, kZoomedTan, own);
    CHECK(own_camera_layer(kFrameTan, counted, true, true) == OwnCameraLayer::Texture);

    const FrameZoom zoom = frame_zoom(false, kZoomedTan, own);
    CHECK(zoom.camera == 0);
    CHECK(own_camera_layer(kFrameTan, zoom, true, true) == OwnCameraLayer::Screen);
    CHECK(own_camera_layer(kWidgetTan, zoom, true, true) == OwnCameraLayer::Texture);

    // All of the menu's 2D goes over the world, wherever the terrain put its marker.
    CHECK(bytes_before_world(kMenuBeforeWorld, true) == 0);
    CHECK(bytes_before_world(0, true) == 0);
}

// No camera of its own drawn: the zoom is the world's, or none.
void test_zoom_without_own_cameras() {
    CHECK(frame_zoom(true, kFrameTan, {}).camera == FrameZoom::kWorld);
    CHECK(frame_zoom(false, kFrameTan, {}).camera == FrameZoom::kNone);
    // The narrowest of several, the first of equals.
    const std::array<float, 3> own{kWidgetTan, kFrameTan, kFrameTan};
    CHECK(frame_zoom(false, 0.0f, own).camera == 1);
}

}  // namespace

int main() {
    test_menu_alone_keeps_its_layers();
    test_world_camera_is_the_frame_in_play();
    test_world_under_menu_puts_the_menu_on_the_screen();
    test_zoom_without_own_cameras();
    return openrac::test::result();
}
