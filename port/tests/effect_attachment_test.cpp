// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Effect anchors and the established quad-history rules from scene_hud.cpp, using synthetic
// captures without renderer instances. Expected positions come from a two-unit interpolated step.
#include <array>
#include <cmath>

#include "game/common/frontend/effect_attachment.h"
#include "game/common/frontend/geometry.h"
#include "game/common/profile.h"
#include "renderer/renderer.h"
#include "tests/check.h"

namespace {

// Resolves a callback-only object and aligns geometry produced on different ticks.
void test_meshless_anchor_aligns_body_and_outline() {
    openrac::host::LiveMoby moby;
    moby.address = 1;
    moby.class_id = 1;
    const std::array previous{moby};
    moby.matrix[12] = 2;
    const std::array current{moby};
    const auto anchor = openrac::frontend::captured_effect_parent(previous, current, 1, 0.25f);
    CHECK(anchor.has_value());

    // Keep a missing result from being dereferenced after the failed check.
    if (!anchor) {
        return;
    }

    float body[3] = {0.5f, 0, 0};
    float outline[3] = {2.5f, 0, 0};
    openrac::frontend::follow_moby(anchor->previous, anchor->drawn, body);
    openrac::frontend::follow_moby(anchor->current, anchor->drawn, outline);
    const float expected = 2.0f * 0.25f + 0.5f;
    CHECK(std::fabs(body[0] - expected) < 0.0001f);
    CHECK(std::fabs(outline[0] - expected) < 0.0001f);
}

// Rejects missing captures and addresses absent from the captured moby lists.
void test_missing_anchors_have_no_transform() {
    openrac::host::LiveMoby moby;
    moby.address = 1;
    const std::array captured{moby};
    CHECK(!openrac::frontend::captured_effect_parent({}, captured, 1, 0.5f));
    CHECK(!openrac::frontend::captured_effect_parent(captured, {}, 1, 0.5f));
    CHECK(!openrac::frontend::captured_effect_parent(captured, captured, 0, 0.5f));
}

// Rejects slot reuse and teleports rather than moving old geometry with an unrelated anchor.
void test_discontinuous_anchors_are_not_interpolated() {
    openrac::host::LiveMoby moby;
    moby.address = 1;
    moby.class_id = 1;
    const std::array previous{moby};
    moby.class_id = 2;
    std::array current{moby};
    CHECK(!openrac::frontend::captured_effect_parent(previous, current, 1, 0.5f));
    current[0].class_id = 1;
    current[0].matrix[12] = 100;
    CHECK(!openrac::frontend::captured_effect_parent(previous, current, 1, 0.5f));
}

// Places a newborn sprite with its displayed anchor without requiring previous particle corners.
void test_newborn_sprites_follow_the_anchor_position() {
    openrac::frontend::EffectParent parent{
        openrac::renderer::identity(),
        openrac::renderer::translation({2, 0, 0}),
        openrac::renderer::translation({0.5f, 0, 0})
    };
    openrac::renderer::EffectQuad sprite{};
    sprite.rigid = true;
    sprite.corner[0][0] = 2.25f;
    sprite.corner[3][0] = 2.75f;
    openrac::frontend::follow_effect_parent(sprite, parent);
    CHECK(sprite.id == 0);
    CHECK(sprite.corner[0][0] == 0.5f + 0.25f);
    CHECK(sprite.corner[3][0] == 0.5f + 0.75f);
}

// Keeps sprite shapes and world offsets unchanged when their position anchor rotates.
void test_attached_sprites_do_not_rotate_with_the_anchor() {
    openrac::frontend::EffectParent parent{
        openrac::renderer::identity(), openrac::renderer::identity(), openrac::renderer::identity()
    };
    // A quarter-turn around Z would rotate a mesh point from +X to +Y.
    parent.drawn[0] = 0;
    parent.drawn[1] = 1;
    parent.drawn[4] = -1;
    parent.drawn[5] = 0;
    parent.drawn[12] = 2;
    openrac::renderer::EffectQuad sprite{};
    sprite.rigid = true;
    sprite.corner[0][0] = 1;
    sprite.corner[3][0] = 3;
    openrac::frontend::follow_effect_parent(sprite, parent);
    CHECK(sprite.corner[0][0] == 1 + 2);
    CHECK(sprite.corner[0][1] == 0);
    CHECK(sprite.corner[3][0] == 3 + 2);
    CHECK(sprite.corner[3][1] == 0);
}

// Keeps shaped callback geometry attached through the anchor's axes.
void test_attached_mesh_quads_follow_rotation() {
    openrac::frontend::EffectParent parent{
        openrac::renderer::identity(), openrac::renderer::identity(), openrac::renderer::identity()
    };
    parent.drawn[0] = 0;
    parent.drawn[1] = 1;
    parent.drawn[4] = -1;
    parent.drawn[5] = 0;
    openrac::renderer::EffectQuad quad{};
    quad.corner[0][0] = 1;
    openrac::frontend::follow_effect_parent(quad, parent);
    CHECK(quad.corner[0][0] == 0);
    CHECK(quad.corner[0][1] == 1);
}

// Pins the existing history blend without shrinking sprites that change angle between ticks.
void test_quad_history_preserves_rigid_sprite_shape() {
    const std::array<std::array<float, 4>, 4> previous{
        {{-1, -1, 0, 1}, {1, -1, 0, 1}, {-1, 1, 0, 1}, {1, 1, 0, 1}}
    };
    openrac::renderer::EffectQuad current{};
    current.rigid = true;
    current.corner[0][0] = 3;
    current.corner[0][1] = -1;
    current.corner[3][0] = 1;
    current.corner[3][1] = 1;

    // The current sprite's centre moves from X=0 to X=2 while its current angle remains unchanged.
    for (const float alpha : {0.0f, 0.25f, 0.5f, 1.0f}) {
        auto quad = current;
        openrac::frontend::blend_effect_quad(quad, previous, alpha);
        CHECK(quad.corner[0][0] == 1 + 2 * alpha);
        CHECK(quad.corner[3][0] == -1 + 2 * alpha);
        CHECK(quad.corner[0][1] == -1);
        CHECK(quad.corner[3][1] == 1);
    }
}

// Pins the existing nonrigid history blend of all homogeneous corner coordinates.
void test_quad_history_blends_shaped_effects() {
    const std::array<std::array<float, 4>, 4> previous{
        {{0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}}
    };
    openrac::renderer::EffectQuad quad{};
    quad.corner[0][0] = 2;
    quad.corner[0][1] = 1;
    quad.corner[0][3] = 2;
    quad.corner[3][0] = 3;
    quad.corner[3][1] = 2;
    quad.corner[3][3] = 2;
    openrac::frontend::blend_effect_quad(quad, previous, 0.25f);
    CHECK(quad.corner[0][0] == 2 * 0.25f);
    CHECK(quad.corner[0][1] == 0.25f);
    CHECK(quad.corner[0][3] == 1 + 0.25f);
    CHECK(quad.corner[3][0] == 1 + 2 * 0.25f);
}

// Pins the existing four-unit slot-reuse cutoff instead of blending unrelated particles.
void test_quad_history_rejects_reused_slots() {
    const std::array<std::array<float, 4>, 4> previous{};
    openrac::renderer::EffectQuad quad{};
    quad.corner[0][0] = 4;
    quad.corner[3][0] = 4;
    openrac::frontend::blend_effect_quad(quad, previous, 0.0f);
    CHECK(quad.corner[0][0] == 4);
    CHECK(quad.corner[3][0] == 4);
}

}  // namespace

// Runs the captured-anchor regression cases without game data or a graphics device.
int main() {
    test_meshless_anchor_aligns_body_and_outline();
    test_missing_anchors_have_no_transform();
    test_discontinuous_anchors_are_not_interpolated();
    test_newborn_sprites_follow_the_anchor_position();
    test_attached_sprites_do_not_rotate_with_the_anchor();
    test_attached_mesh_quads_follow_rotation();
    test_quad_history_preserves_rigid_sprite_shape();
    test_quad_history_blends_shaped_effects();
    test_quad_history_rejects_reused_slots();
    return openrac::test::result();
}
