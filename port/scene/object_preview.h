// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Owned developer object preview; independent of game GPU resources and the gameplay camera.
#pragma once

#include <memory>

#include "common/result.h"
#include "renderer/framebuffer.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "scene/object_picker.h"

namespace openrac::scene {
// One selected material run, with a private texture index and compact vertex range.
struct PreviewPart {
    scene::Material material;
    unsigned first = 0, count = 0;
};

// CPU-owned selection, including original skin weights for following subsequent displayed poses.
struct PreviewMesh {
    std::vector<scene::Vertex> source, vertices;
    std::vector<PreviewPart> parts;
    std::vector<renderer::Rgba8Image> images;
    std::vector<scene::ModelJoint> bones;
    std::vector<renderer::Mat4> bone_pose;
    scene::Instance instance;
    renderer::Vec3 pivot{};
    float radius = 1;  // Studio positions and ranges are in selected-object radii.
};

// Resolves stable moby identity, or the selected static instance; no live memory reads.
const scene::Instance* preview_instance(
    const scene::LevelData& level, const scene::PickHit& hit, std::span<const scene::Instance> live
);
// Extracts only the selected model (terrain: selected primitive), textures and displayed pose.
Result<PreviewMesh> preview_mesh(
    const scene::LevelData& level,
    const scene::PickHit& hit,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
);
// Reposes the private compact mesh; invalid palettes fail explicitly, preserving the previous pose.
Result<void> preview_pose(
    PreviewMesh& mesh, const scene::Instance& instance, std::span<const renderer::Mat4> palette
);

// Orbit camera around the selection, independent of gameplay and expressed in object radii.
struct PreviewOrbit {
    float yaw = -1.2f, pitch = .25f, distance = 2.5f, fly_speed = 2;
    renderer::Vec3 focus{};
    // Bounded mouse deltas; poles and zoom cannot make the view singular.
    void move(float x, float y, float wheel);
    // Pan in camera axes; fly look preserves the eye while orbit preserves the focus.
    void navigate(float x, float y, bool fly);
    void travel(const renderer::Vec3& movement);
    // Builds the same reversed-depth projection used by the game renderer.
    renderer::Camera camera(float aspect) const;
};

// Optional surface-lighting provider; the general viewer never includes enhanced renderer code.
class PreviewLighting {
public:
    virtual ~PreviewLighting() = default;
    // Supplies preview_illumination, leaving texture/alpha/material handling in the general viewer.
    virtual std::string source() const = 0;
    // Updates private lighting resources before this view is drawn.
    virtual void prepare(std::uint64_t frame) = 0;

    virtual bool wants_depth() const { return false; }

    virtual void cull(unsigned, const renderer::Camera&, int, int) {}

    virtual void bind(const renderer::Shader&, const renderer::Camera&, bool) const = 0;
    virtual void release() = 0;
};

// Selected object's local/bone frame into the normalized studio, also valid for frozen poses.
std::optional<renderer::Mat4> preview_attachment(const PreviewMesh& mesh, int joint);

// Created and released on the overlay thread after render-worker ownership has returned.
class ObjectPreview {
public:
    ~ObjectPreview();
    // Uploads an owned snapshot; failures release partial resources.
    Result<void> select(PreviewMesh mesh, std::unique_ptr<PreviewLighting> lighting = {});
    // Updates only vertex positions/normals; frozen previews simply do not call this.
    Result<void> pose(const scene::Instance&, std::span<const renderer::Mat4> palette);
    // Restores the source mesh and bone bind frames without altering the live game actor.
    Result<void> bind_pose();
    // Draws a private target using the production Forward+ light table, cones, falloff and GGX.
    Result<void> draw(int width, int height, const PreviewOrbit&);

    PreviewLighting* lighting() const { return m_lighting.get(); }

    // Releases before context teardown, window close or level replacement; safe when already empty.
    void release();

    // The private colour image for ImGui; zero means no selection has been uploaded.
    unsigned texture() const { return m_target.colour_texture(); }

    // Native test/debug readback; never changes game rendering.
    const renderer::FrameBuffer& target() const { return m_target; }

    // The current owned pose, also usable without GPU access in regression tests.
    const PreviewMesh& mesh() const { return m_mesh; }

private:
    // Creates vertex attributes and uploads the compact pose buffer.
    void upload_vertices();
    // Uploads only referenced material images, plus an opaque white fallback.
    void upload_images();
    // Draws cutout/opaque first, then source-over transparent material runs.
    void draw_parts(bool transparent);
    // Apply private target and camera state without borrowing world resources.
    void bind_target(const renderer::Camera&, int width, int height);
    // Opaque/partial-alpha passes share the optional private depth culler.
    void draw_surfaces(const renderer::Camera&, int width, int height);
    PreviewMesh m_mesh;
    renderer::FrameBuffer m_target;
    renderer::Shader m_shader;
    std::unique_ptr<PreviewLighting> m_lighting;
    std::vector<unsigned> m_images;
    unsigned m_vao = 0, m_vertices = 0;
    std::uint64_t m_frame = 0;
};
}  // namespace openrac::scene
