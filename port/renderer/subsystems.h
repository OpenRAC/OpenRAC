// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The world's renderers, one per subsystem the game draws (RENDERER.md,
// sections 2 and 6). They are placeholders: each says what it must draw and
// where that is documented, and draws nothing yet. A placeholder that is
// handed packets says so once in the log, so a game build shows which
// subsystems are still missing.
//
// They share one plan (RENDERER.md section 6, after OpenGOAL's TFRAG, TIE,
// Merc2 and Sprite3 renderers): geometry is converted when the level is
// extracted (the editor's readers: editor/terrain.py, ties.py, shrubs.py,
// moby_class.py, sky.py), uploaded when the level loads, and drawn with what
// the game computes each frame: the camera, the visibility bits of the
// occlusion grid, the level-of-detail weights, the lights, the joint
// matrices. No VU program is interpreted; each program's maths, once read,
// goes into a vertex shader. The level viewer (port/viewer) already draws
// the extracted geometry of terrain, ties, shrubs, moby bind poses and the
// sky the same way, without the game.

#pragma once

#include "renderer/renderer.h"

namespace openrac::renderer {

// A renderer for a subsystem that does not draw yet.
class PlaceholderRenderer : public BucketRenderer {
public:
    using BucketRenderer::BucketRenderer;

    void render(const FrameInput& input, RenderState& state) override;

private:
    bool m_reported = false;
};

// Terrain (tfrag): DrawTfrag func_002346C0, TfragProc func_002352C8 (asm),
// VU1 programs 55907 and 903379. Must draw the level's 0x40-byte terrain
// fragments with the level-of-detail morph SetTfragDists func_00234380
// computes (three distances, two morph weights for the refinement streams),
// culled by the occlusion mask, with baked vertex colours and the level's
// fog. Converted at extraction from editor/terrain.py (LOD 0 and 2 today).
class TfragRenderer : public PlaceholderRenderer {
public:
    TfragRenderer() : PlaceholderRenderer("tfrag", Bucket::Terrain) {}
};

// Ties, the instanced scenery: DrawTies_1/_2 func_00236BE0/00236CA8,
// TieProc func_00236F00 (asm), VU1 programs 13507 and 224979. Must draw each
// visible instance (0xE0-byte placements: matrix, draw distance, occlusion
// index, ambient colours at +0x50, directional lights at +0xD0) with its
// class's level of detail, lit by LightTies func_00238688; the second
// program adds an environment map. Classes from editor/ties.py.
class TieRenderer : public PlaceholderRenderer {
public:
    TieRenderer() : PlaceholderRenderer("tie", Bucket::Ties) {}
};

// Shrubs: DrawShrubs func_00229E50, ShrubProc func_00229F00 (asm), VU1
// programs 56467 and 912339. Must draw shrub instances with their colour and
// lights (LightShrubs func_0022B8F8), switching to the class's billboard
// (class +0x1C, not read yet) at distance. Classes from editor/shrubs.py.
class ShrubRenderer : public PlaceholderRenderer {
public:
    ShrubRenderer() : PlaceholderRenderer("shrub", Bucket::Shrubs) {}
};

// Mobys, the characters and objects: DrawMobys func_0020E2B0, MobyProc
// func_00212658 (asm), VU1 program 13859, animation through VU0. Must draw
// each moby's class mesh skinned on the GPU with the joint matrices the game
// builds on the scratchpad (0x70000000 + 0x40 * i), high or low detail by
// the class's distance table, with alpha test GEQUAL 0x60 and "keep colour
// on fail" (the double draw), metal and glow packets (not read yet). Meshes
// and skeletons from editor/moby_class.py and moby_anim.py.
class MobyRenderer : public PlaceholderRenderer {
public:
    MobyRenderer() : PlaceholderRenderer("moby", Bucket::Mobys) {}
};

// The sky: SkyDrawShell func_0022C9A8 (C), the textured and Gouraud shell
// drawers (not decompiled). Must draw the level's shells centred on the
// camera, in order, each blended over the last, with the sky's sprites (the
// particle program). Shells from editor/sky.py.
class SkyRenderer : public PlaceholderRenderer {
public:
    SkyRenderer() : PlaceholderRenderer("sky", Bucket::Sky) {}
};

// Particles and sprites: PartProc func_00218B10, UpdateParts
// func_00218A80 (asm), VU1 program 221571; sprites through
// DrawSpriteHelper_A func_001F7868 (VU1 57843). Must draw camera-facing quads
// from the particle records (built by level code, not decompiled yet), the
// way OpenGOAL's Sprite3 does: four vertices per sprite expanded in a vertex
// shader, batched by texture and draw mode.
class ParticleRenderer : public PlaceholderRenderer {
public:
    ParticleRenderer() : PlaceholderRenderer("particles", Bucket::Particles) {}
};

}  // namespace openrac::renderer
