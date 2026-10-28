// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Where the mobys a game draws with a camera of its own go (frontend/frame.cpp's pose_mobys), and
// where a tick's 2D path is split for its world. A moby the game drew with a wider tangent than the
// frame's went into a texture (the vendor's screens, the page menus' widgets); the others were the
// frame's, drawn in the world with the frame's camera (a menu's frame objects over the picture it
// opened on). A level's world drawn live under a menu (openrac_game_world_under_menu: the pause
// menu while a party plays) is the level's, with the level's camera: the menu's frame objects are
// not its. They are drawn over it on the screen, at their place in the 2D path, with the menu's
// camera and after the world's depth is cleared; the world's camera does not take part in the
// zoom, and all of the 2D path goes over the world.

#pragma once

#include <cstddef>
#include <span>

namespace openrac::frontend {

// Where a moby drawn with a camera of its own goes.
enum class OwnCameraLayer {
    World,    // in the world pass, placed so the world's camera sees it where its own camera did
    Texture,  // into the texture the game rendered it to, at its marker in the 2D path
    Screen,   // on the screen over the world, at its marker in the 2D path, with its own camera
};

// The frame's zoom: the narrowest tangent among the cameras that count, and whose it is.
struct FrameZoom {
    static constexpr int kNone = -2;   // no camera counts
    static constexpr int kWorld = -1;  // the world's camera
    float tan = 0.0f;
    int camera = kNone;  // kWorld, kNone or an index into the own cameras' tangents
};

// The frame's zoom from the world's camera (when `world_counts`: it ran, and is not a level's under
// a menu) and the mobys' own cameras' horizontal tangents, in the order they were drawn.
[[nodiscard]] inline FrameZoom frame_zoom(
    bool world_counts, float world_tan, std::span<const float> own_tans
) {
    FrameZoom zoom;
    if (world_counts) {
        zoom.tan = world_tan;
        zoom.camera = FrameZoom::kWorld;
    }
    for (std::size_t i = 0; i < own_tans.size(); ++i) {
        if (zoom.tan == 0.0f || own_tans[i] < zoom.tan) {
            zoom.tan = own_tans[i];
            zoom.camera = static_cast<int>(i);
        }
    }
    return zoom;
}

// Where a moby drawn with a camera whose horizontal tangent is `tan` goes: wider than the frame's
// zoom (whose focal lengths are known, `focal_known`), into a texture; else the frame's, in the
// world, or over a world drawn under a menu on the screen.
[[nodiscard]] inline OwnCameraLayer own_camera_layer(
    float tan, const FrameZoom& zoom, bool focal_known, bool world_under_menu
) {
    if (tan > zoom.tan + 1.0e-3f && focal_known) {
        return OwnCameraLayer::Texture;
    }
    return world_under_menu ? OwnCameraLayer::Screen : OwnCameraLayer::World;
}

// The bytes of the 2D path drawn before the world: those before its split (the world's marker, the
// first VU1 program or the first render to a texture), or none under a menu, whose 2D (its picture
// of the frame, its panels, wherever the level's terrain put its marker among them) all goes over
// the world drawn under it.
[[nodiscard]] inline std::size_t bytes_before_world(
    std::size_t before_world, bool world_under_menu
) {
    return world_under_menu ? 0 : before_world;
}

}  // namespace openrac::frontend
