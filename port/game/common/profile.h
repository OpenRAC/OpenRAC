// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the host program asks of the game it runs, in C++: a game version's profile. The host's
// shared code (the frontend, the debug overlay) knows no game (CODING_CONVENTIONS.md, section 15);
// where it needs a game's memory read (the camera, the live mobys, the display list shown), it
// asks the profile, and the game's folder answers with its own addresses and layouts.
//
// One profile per game version, which picks the version's tables as rac1_version.h does for C. A
// game's program links one definition of game_profile() (cmake/Games.cmake): its version's own
// (port/game/<id>/profile.cpp), else its game's (port/game/<game>/profile.cpp, rac1's for both of
// its discs), else game/common/profile_none.cpp, which reports no game state. The C side of what a
// game gives the host is openrac/game_host.h.

#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/math.h"
#include "scene/shrub_draw.h"

namespace openrac::host {

// What the game's shrub renderer draws of one shrub instance in a frame.
using ShrubDraw = scene::ShrubDraw;

// A moby as the renderers place it.
struct LiveMoby {
    std::uint32_t address = 0;  // The moby in game memory (its animation fields, its class).
    int class_id = 0;           // Its class, as the game numbers them.
    int model_class = -1;       // The class drawn for it, when not its own and the level has it.
    bool model_only = false;    // Drawn as model_class or not at all, never as its own class.
    std::uint16_t occlusion = 0x7f80;     // Its occlusion word, as the game keeps it.
    std::array<float, 4> world_sphere{};  // World centre and radius, game units; radius 0: none.
    renderer::Mat4 matrix = renderer::identity();  // Game axes, the moby's scale included.
};

// The game's state at one frame: the camera the world is drawn from, and the live mobys.
struct GameState {
    renderer::Vec3 camera_position{};  // The eye, in game units.
    std::optional<renderer::Vec3> camera_target;  // Smoothed follow focus, when known.
    std::uint64_t camera_target_id = 0;           // Camera record and saved-state identity.
    renderer::Vec3 forward{1, 0, 0};   // The unit axis the camera looks along.
    renderer::Vec3 left{0, 1, 0};      // The unit axis to its left.
    renderer::Vec3 up{0, 0, 1};        // The unit axis above it.
    float tan_half_fov_x = 0.63f;      // The tangent of half the horizontal field of view.
    float tan_half_fov_y = 0.48f;      // The tangent of half the vertical one.

    // The focal lengths in the view context, in the chip's pixels (+0xC0 and +0xD4, a 32nd of
    // them): 256 / 0.63 and 208 / 0.488 for the frame. 0 when unreadable.
    float focal_x = 0.0f;
    float focal_y = 0.0f;

    // The live mobys, in the order the game's table holds them.
    std::vector<LiveMoby> mobys;
    std::vector<renderer::Vec3> nanotech_orbs;  // The bobbing centres of R&C1's nanotech clusters.

    // What the game's moby renderer tests each moby's distance against, bit for bit as it loads
    // them: the third column of the view matrix (the depth of a point is its dot with it), the
    // global cap on a moby's draw distance and the near plane, in the sphere's 1/1024 units. Unset
    // for a game that does not give them.
    struct MobyView {
        std::array<float, 3> depth_axis{};
        std::int32_t draw_cap = 0;
        float near_plane = 0.0f;
        // Draw distance (settings.h): the reach times this; 0 none. 1, the game's, tests bit for
        // bit as MobyProc does.
        float scale = 1.0f;
    };

    std::optional<MobyView> moby_view;

    // The fog UpdateFog left in the view context (+0x218): FOGCOL (display bytes, 0..1), the
    // depths it starts and ends at (raw units, game units x 1024) and the GS fog value F at those
    // depths (255 = none). F is linear in camera depth, clamped between the two.
    std::array<float, 3> fog_colour{0, 0, 0};
    float fog_near = 0.0f;      // The depth the fog starts at.
    float fog_far = 1.0f;       // The depth it ends at.
    float fog_near_f = 255.0f;  // F at the near depth.
    float fog_far_f = 255.0f;   // F at the far depth.

    // The colour the game clears the frame to (SetBackgroundColor, the level settings' background
    // the gameplay loader leaves in the view context): what shows wherever no sky shell covers.
    // Display bytes, 0..1. Unset when the program keeps none: the level's own colour stands in.
    std::optional<std::array<float, 3>> background_colour;

    // Returns the colour to clear the frame to: the game's when it gave one, else `fallback`.
    std::array<float, 3> background_or(const std::array<float, 3>& fallback) const {
        return background_colour ? *background_colour : fallback;
    }

    // Returns the camera's view matrix, in the renderer's convention (looking down -Z, Y up).
    renderer::Mat4 view() const { return renderer::look_along(camera_position, forward, up); }

    // Returns a reversed-depth projection with the game's vertical field of view; the horizontal
    // one follows `aspect`, the target's width over its height.
    renderer::Mat4 projection(float aspect, float near_z, float far_z) const {
        return renderer::perspective_reversed(
            2.0f * std::atan(tan_half_fov_y), aspect, near_z, far_z
        );
    }
};

// What a profile's diagnostics get from the running program beside the game's memory.
struct DiagnosticsInput {
    // An executable address to the loaded program's copy of it.
    std::uint32_t (*relocate)(std::uint32_t address) = nullptr;
    // What the game's host C keeps for them (openrac_game_diagnostics, game_host.h), or null.
    void* game = nullptr;
};

// What one game version tells the host program about its memory.
//
// The base answers as a game nothing is known of: no game state (the camera as GameState starts
// it, no live moby), no display list known to be shown, no shadows, no hero, no extracted levels.
// A game's profile derives from it and overrides what its folder knows; a game with none uses the
// base itself (game/common/profile_none.cpp). Each profile is a constant of its game's folder,
// made once and never changed: the host keeps a reference to it for the program's life, and calls
// it from the window's thread only.
class GameProfile {
public:
    virtual ~GameProfile() = default;

    // Reads the camera and the live mobys from the game's active main memory, `ram`. `level` is
    // the level program loaded now (-1 for the boot program, the title and the menus): each has its
    // globals at other addresses. A table pointer outside memory leaves the moby list empty.
    virtual GameState read_state(std::span<const std::uint8_t> /*ram*/, int /*level*/) const {
        return {};
    }

    // Reads the moby at game address `moby` as the renderers place it; none when its position or
    // scale is unusable.
    virtual std::optional<LiveMoby> read_moby(
        std::span<const std::uint8_t> /*ram*/, std::uint32_t /*moby*/
    ) const {
        return std::nullopt;
    }

    // The vertex alpha's distance fade the game's moby renderer gives the moby whose record starts
    // at `moby`, seen from `state`'s camera, in the GS's units (0x80 none, 0 at its draw distance);
    // none when it leaves the moby undrawn by its distance.
    virtual std::optional<std::uint8_t> moby_distance_fade(
        const std::uint8_t* /*moby*/, const GameState& /*state*/
    ) const {
        return std::uint8_t{0x80};
    }

    // What the game's shrub renderer draws of each shrub instance this frame, in the game's
    // instance order; empty when it is not known. `level` as for read_state().
    virtual std::vector<ShrubDraw> shrub_draws(
        std::span<const std::uint8_t> /*ram*/, int /*level*/
    ) const {
        return {};
    }

    // Returns the game address of the display list of the frame being shown: the one of the
    // game's two list buffers it is not building; 0 when none is known. `level` as for
    // read_state(). Not asked of a game that sends_display_list().
    virtual std::uint32_t shown_chain(std::span<const std::uint8_t> /*ram*/, int /*level*/) const {
        return 0;
    }

    // Returns the GS block of the buffer the game draws its frame in; 0 for one right after the
    // display buffer, where the display's size puts it.
    virtual std::uint32_t draw_buffer_block() const { return 0; }

    // Returns where a moby keeps its animation fields (scene::set_moby_anim_fields).
    virtual std::uint32_t moby_anim_fields() const { return 0x50; }

    // Returns where a moby keeps its glow word, the RGB its glow vertices are drawn in while its
    // mode has bit 0x10 (MobyProc's glow list).
    virtual std::uint32_t moby_glow_word() const { return 0x90; }

    // Returns whether a moby's pose layers keep records (scene::set_moby_layer_records).
    virtual bool moby_layer_records() const { return false; }

    // Whether the frame shown is the display list the game sends each tick, rather than one of
    // two list buffers it alternates (shown_chain()).
    virtual bool sends_display_list() const { return false; }

    // Whether the 2D path of a frame is split for its mobys at the first moby the game drew from a
    // list of its own (a menu's or a shop's, openrac_game_mobys_drawn): the 2D before it goes under
    // them. Going Commando's Ship Shack draws its ship there, over its grid panel.
    virtual bool own_mobys_split_the_2d() const { return false; }

    // Whether a moby drawn with a camera of its own while the 2D path's frame is a buffer narrower
    // than the picture goes into that texture whatever its field of view (frontend/frame.cpp's
    // pose_mobys), rather than only when its view is wider than the frame's: Going Commando's
    // cockpit screen draws Fizzwidget into a 256 x 256 buffer with the world's own view. Its world
    // quads that sample such a buffer then draw the picture there (renderer::g_quads_sample_targets),
    // the screen's quad of block 0x3BE0.
    virtual bool own_mobys_into_narrow_buffers() const { return false; }

    // Whether an image the game sends outside its display list stays over what that tick's list,
    // built before it and replayed after it, puts at the same block: the game reloads a block
    // that the list was still paging into (Going Commando's flight, func_002E80B0).
    virtual bool images_outlast_list() const { return false; }

    // Whether a tick whose last picture stands (frontend/tick_picture.h: Repeat) still applies the
    // images its list sends to the chip: Going Commando's Galactic Map sends a planet's pictures
    // once (func_002D6F38) with its 512 x 512 level map, a list as long as a page menu's read-back,
    // on a tick that draws no world, so the tick was taken for a menu's gap and its images lost.
    virtual bool repeated_ticks_send_images() const { return false; }

    // Whether the moby program lights each moby by the point lights in reach of its sphere's centre
    // as a third directional light (Going Commando's MobyProc, 0x2A51C8 and 0x2A57F8), rather than
    // not at all.
    virtual bool mobys_take_point_lights() const { return false; }

    // Which variant of the level's swapped texture (scene::LevelData::variant_image) the game has
    // loaded: Going Commando's ship paint job (func_002926B8 keeps its number at 0x00152C30). -1
    // for none, which keeps the one bound.
    virtual int texture_variant(std::span<const std::uint8_t> ram) const {
        (void)ram;
        return -1;
    }

    // Whether a frame drawn between ticks moves each effect quad that carries an id (a particle's
    // slot) part way from its last tick's corners, with the mobys' fraction, rather than drawing
    // the latest tick's: Going Commando's Lancer halo led the gun by up to a tick at 120 fps.
    // Ratchet & Clank draws its particles the same way, but its frames between ticks stay as they
    // were until its render equivalence is run with the change.
    virtual bool interpolates_effect_quads() const { return false; }

    // Whether the terrain's vertex colours take the point lights in reach, as Going Commando's
    // PatchTfragVertexLighting (0x2F0468) adds them to the colours the level lit (mesh.vert),
    // rather than not at all.
    virtual bool terrain_takes_point_lights() const { return false; }

    // Returns the executable's addresses of the words that point at the ties' and the shrubs'
    // class-to-world matrices (64 bytes an instance, in the game's instance order; relocated to the
    // loaded program's copy when read), for a game that moves or removes them at run time: Going
    // Commando's mobys carry tie and shrub groups (the Maktar lamps' heads turn,
    // func_L00_00323958) and zero the matrices of the ones they remove. 0: drawn as loaded. The
    // third: the word that points at the ties' 32-byte records (bounding sphere at +0, the point
    // lights' list at +0x1A, +0x1E bit 2 hidden), for the ties a point light reaches and the ones
    // the game hides; the fourth the shrubs' 32-byte records (bounding-sphere centre at +0, +0x16
    // bit 0 hidden, the point lights' list at +0x1E); 0 for none.
    virtual std::array<std::uint32_t, 4> scenery_matrices() const { return {0, 0, 0, 0}; }

    // Whether the world's depth is precise (renderer/gl/precise_depth.h): reversed Z in a 32-bit float
    // buffer, clip-space z to the window as it is, where the context has glClipControl (OpenGL 4.5,
    // ARB_clip_control, the Vulkan backend; not macOS's OpenGL 4.1). Going Commando's Maktar dome
    // carries light strips (tie class 2713) within a few centimetres of the band under them (class
    // 2675): GL's depth parted them only within about 150 units of the window's near 0.05, and
    // farther away they flickered in and out of it.
    virtual bool precise_depth() const { return false; }

    // Where precise depth cannot be had: the near distance of the world's depth (game units), or 0
    // for the window's 0.05. GL's depth resolves about 1.2e-6 d^2 / (near / 0.05) units at a
    // distance d (2^-24 of its range, after GL's (z + w) / 2), so a larger near parts close surfaces
    // that much farther away. Nothing is clipped for it: the frame turns depth clamping on, so
    // what is nearer than this near is drawn at the nearest depth, in drawing order among itself,
    // instead of being cut away.
    virtual float depth_near() const { return 0.0f; }

    // Returns the executable's address of the four shadow directions the hero's shadow is cast
    // along (relocated to the loaded program's copy when read); 0 for a game without them.
    virtual std::uint32_t shadow_directions() const { return 0; }

    // Whether MobyProc keeps a moby's shadow slab across its up axis (Going Commando: +0x70 the
    // offset, +0x74 the half thickness; scene::moby_shadow_state_on_axis) instead of at two world
    // heights (Ratchet & Clank: +0x84, +0x88; scene::moby_shadow_state).
    virtual bool shadow_slab_on_up_axis() const { return false; }

    // Whether every moby MobyProc queues casts its shadow (Going Commando: Clank, platforms,
    // crates), not only the hero's class 0 (Ratchet & Clank's port).
    virtual bool every_moby_casts_shadow() const { return false; }

    // Returns the executable's address of the hero's state record (relocated when read); 0 for a
    // game without one.
    virtual std::uint32_t hero_state() const { return 0; }

    // Returns the offset in the hero's state record of its air-frames counter (a short: game
    // updates off the ground, 0 on landing), which the hero's shadow map tilts down by.
    virtual std::uint32_t hero_air_frames() const { return 0x30E; }

    // Whether the port has the game's levels extracted (editor/port.py), so that a level whose
    // scenery cannot be read is something missing, not a game with no scenery yet.
    virtual bool has_levels() const { return false; }

    // Returns whether this class is a gameplay subject, rather than scenery or an effect.
    virtual bool gameplay_essential(int /*class_id*/) const { return false; }

    // Whether a moby class keeps the game's own draw distance whatever Draw distance says: one the
    // game parks out of sight and shows only by its distance (Going Commando's Electrolyzer
    // boards).
    virtual bool keeps_game_draw_distance(int /*class_id*/) const { return false; }

    // The value of the game's mode word (openrac_game_mode) while a real-time cutscene plays in a
    // level, which enhanced motion blur takes the cutscene's tuning for; none for a game without
    // one known.
    virtual std::optional<std::uint32_t> cutscene_mode() const { return std::nullopt; }

    // The value of the game's mode word while the pause menu is open over a level (the Field of
    // View keeps the world under it as it was paused); none for a game without one known.
    virtual std::optional<std::uint32_t> pause_mode() const { return std::nullopt; }

    // Returns the executable's address of the particle updates, one per particle type byte
    // (relocated when read); 0 for a game without them.
    virtual std::uint32_t particle_updates() const { return 0; }

    // The game's dead zone on each stick axis, as its pad code reads the bytes: an axis counts
    // from {inner} away from 0x7F and is full {span} beyond (platform/pad.h's StickDeadZone);
    // {0, 0} for a game without a known one.
    virtual std::array<int, 2> stick_dead_zone() const { return {0, 0}; }

    // Developer tools: what of the game's state a diagnostics recording carries beside its
    // snapshots, as a JSON value ("" for nothing), read each tick from `ram` and the tick's `state`
    // while a recording runs. It may watch the next tick through `input.game`; diagnostics_off()
    // stops that.
    virtual std::string diagnostics(
        std::span<const std::uint8_t> /*ram*/,
        int /*level*/,
        const GameState& /*state*/,
        const DiagnosticsInput& /*input*/
    ) const {
        return {};
    }

    // Developer tools: the recording has ended; what diagnostics() watched is let go.
    virtual void diagnostics_off(const DiagnosticsInput& /*input*/) const {}

    // Projectile contact parameters for enhanced water, independent of render visibility.
    virtual std::optional<std::array<float, 2>> water_projectile(int /*class_id*/) const {
        return std::nullopt;
    }

    // Visual attachments to enhanced water; collision and gameplay poses stay untouched.
    virtual bool floats_on_water(int /*class_id*/) const { return false; }
};

// Returns the profile of the game version `game` (its port id, "rac1-pal"), which lives as long as
// the program. Defined once in each game's program, by the game's folder.
const GameProfile& game_profile(std::string_view game);

}  // namespace openrac::host
