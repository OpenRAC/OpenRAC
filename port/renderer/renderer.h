// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The renderer's shape, after OpenGOAL's (OPENGOAL_NOTES.md, section 3): a
// frame is a list of per-subsystem renderers run in the order the game draws
// (RENDERER.md, section 1: sky, terrain, ties, shrubs, mobys, particles, then
// the 2D), each one drawing its own kind of thing on the GPU from what the
// game prepared for it. Nothing models the console's graphics hardware: a
// renderer either reads the packets the game built for its subsystem (the
// direct renderer, for the 2D path) or draws converted assets with the
// camera, visibility and lights the game computed (the world renderers, see
// subsystems.h).
//
// How the game hands a frame over is still open (DESIGN.md, section 4, item
// 3). FrameInput carries, per bucket, the bytes the game built for it at its
// sync point (VU1_sendChain) and the camera; it grows as the hand-off is
// decided.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "renderer/math.h"
#include "renderer/texture_pool.h"

namespace openrac::renderer {

// What the game draws, in DrawWorld's order.
enum class Bucket : std::uint8_t {
    Sky,
    Terrain,
    Ties,
    Shrubs,
    Mobys,
    Particles,
    Hud,  // the 2D path: HUD, text, fades, letterbox (GIF packets)
    Count,
};

inline constexpr std::size_t kBucketCount = static_cast<std::size_t>(Bucket::Count);

const char* bucket_name(Bucket bucket);

struct Camera {
    Mat4 view = identity();        // world (game axes) to view
    Mat4 projection = identity();  // view to clip, reversed depth (math.h)
    Vec3 position{};
};

struct FrameInput {
    // The packets the game built for each bucket this frame (empty when a
    // bucket has nothing, or its renderer draws from elsewhere).
    std::array<std::span<const std::uint8_t>, kBucketCount> packets{};
    Camera camera;
    std::array<float, 4> clear_colour{0.0f, 0.0f, 0.0f, 1.0f};
    std::uint64_t frame = 0;
    double seconds = 0.0;
};

// Where a frame is drawn: a framebuffer (0 is the window) and its size.
struct RenderTarget {
    unsigned framebuffer = 0;
    int width = 0;
    int height = 0;
};

struct FrameStats {
    int draw_calls = 0;
    std::uint64_t triangles = 0;
};

// What every renderer shares during a frame.
struct RenderState {
    TexturePool& textures;
    RenderTarget target;
    FrameStats stats;
};

// One subsystem's renderer (OpenGOAL's BucketRenderer).
class BucketRenderer {
public:
    BucketRenderer(std::string name, Bucket bucket) : m_name(std::move(name)), m_bucket(bucket) {}

    virtual ~BucketRenderer() = default;
    BucketRenderer(const BucketRenderer&) = delete;
    BucketRenderer& operator=(const BucketRenderer&) = delete;

    // Create GL objects (shaders, buffers); the context is current.
    virtual bool init(RenderState& state, std::string& error) {
        (void)state;
        (void)error;
        return true;
    }

    // Draw this subsystem's part of the frame.
    virtual void render(const FrameInput& input, RenderState& state) = 0;

    // Free GL objects; the context is still current.
    virtual void release() {}

    const std::string& name() const { return m_name; }

    Bucket bucket() const { return m_bucket; }

    bool enabled = true;

private:
    std::string m_name;
    Bucket m_bucket;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // After gl::load(): initialise every renderer added so far. Renderers
    // added later are initialised as they are added.
    bool init(std::string& error);

    // Appended in drawing order. Returns the renderer, or null if its init failed.
    BucketRenderer* add(std::unique_ptr<BucketRenderer> renderer, std::string& error);

    // The game's subsystems in DrawWorld's order: the placeholders of
    // subsystems.h for the world, and a DirectRenderer for the 2D.
    bool add_game_renderers(std::string& error);

    // Draw one frame into `target`: clear it (depth to 0, for reversed
    // depth), then run each enabled renderer in order.
    void render(const FrameInput& input, const RenderTarget& target);

    // Free every GL object (the context must still be current).
    void release();

    // Drop every subsystem renderer (released first) but keep the texture pool: the console's GS
    // memory outlives a change of level, and what the game uploaded stays there.
    void clear_renderers();

    TexturePool& textures() { return m_textures; }

    std::span<const std::unique_ptr<BucketRenderer>> renderers() const { return m_renderers; }

    const FrameStats& last_stats() const { return m_stats; }

private:
    TexturePool m_textures;
    std::vector<std::unique_ptr<BucketRenderer>> m_renderers;
    FrameStats m_stats;
    bool m_initialised = false;
};

}  // namespace openrac::renderer
