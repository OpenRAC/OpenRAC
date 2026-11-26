// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native frame recorder's data (docs/port/FRAME_RECORDER.md): the ordered commands a game's
// host C appends through openrac/frame.h, the records they carry, and, while a game still sends
// residual 2D as packets in its own display list, where each command goes among them (anchors
// matched against the list's tag addresses: section 4). CPU only and knows no GS: the window
// splices it into its 2D stream and the 2D renderer draws it (renderer/direct).
//
// Only the game's thread records and matches (the window's tick capture runs on it); a matched
// recording is copied into the tick the window keeps, so render jobs read their own copy.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "renderer/direct/draw_state.h"
#include "renderer/frame/ui_records.h"
#include "renderer/math.h"

namespace openrac::renderer {

enum class FrameOp : std::uint8_t {
    World,       // the world (sky, terrain, ties, shrubs) is drawn here; `index` the layers
    WorldMobys,  // the world's moby pass starts here
    Mobys,       // a moby list with its own camera: `value` = count << 32 | list address
    UiQuad,      // ui_quads[index], in the 2D state in effect (transitional, until P4)
    UiItem,      // ui_items[index]
    EffectQuad,  // world effect quad `index` (its marker value) takes the tests in effect here
    Fill,        // a flat rectangle: rect (or whole), rgba, blend
    Clear,       // colour and/or depth of the target: rgba, clear
    Picture,     // picture `index` (a port id; kImagePicture: images[value]) over rect, rgba, blend
    Snapshot,    // the frame drawn so far kept as picture `index`
    TargetBegin,  // draws go into panel `index` (a port id >= kRecordedTargets), width x height
    TargetEnd,    // back to the screen
    Scissor,      // what follows is cut to rect (or the whole target)
    // Transitional placements, read where they land among the residual packets as the markers
    // the host C once wrote into the list were (rac1 and rac2 until R2/R3):
    WorldShrubs,  // the world's shrub pass starts here: effect strips before it come first
    EffectParent,  // strips follow attachment `index`'s parent (0 ends it), value bit 0: position only
    HudSide,       // what follows goes out to the HUD's side `index` (0 ends it); value bit 0
                   // stretches what crosses the middle, bit 1 parts it at value >> 2 instead
    Wide,          // `index` 1: what follows covers the whole target, as a fade does; 0 ends it
    PictureBand,  // `index` 1: a full-width scissor stays the picture's with "HUD: Edges"; 0 ends
    LastFrame,    // the frame the window last drew with its world in, over the whole target
    ViewBegin,    // the world that follows is split-screen view `index`'s, into rect
    ViewEnd,      // the open view ends (FrameRecording::views has its camera)
};

// Returns the command's contract name for recorder diagnostics, or "Unknown" for an invalid value.
const char* frame_op_name(FrameOp op);

// FrameCommand::clear bits.
inline constexpr std::uint8_t kClearColour = 1, kClearDepth = 2;

// A Picture command whose picture is the recording's images[value] (openrac_frame_picture_image).
inline constexpr std::uint32_t kImagePicture = 0xFFFFFFFFu;

// A command outside every local-player view; otherwise `view` indexes this recording's views.
inline constexpr std::uint32_t kNoFrameView = UINT32_MAX;

struct FrameCommand {
    FrameOp op = FrameOp::World;
    // The game's display-list write pointer when the command was recorded (openrac_frame_anchor),
    // 0 for none: where it goes among what the list still draws.
    std::uint32_t anchor = 0;
    std::uint32_t index = 0;
    std::uint64_t value = 0;
    // The view open when the command was recorded (its index in the recording's views), set by
    // FrameRecorder::push whether or not the view's ViewBegin anchor reaches the submitted list.
    std::uint32_t view = kNoFrameView;
    // Fill, Picture, Scissor: x0, y0, x1, y1 in the game's 2D screen pixels; `whole` for the whole
    // target (rect not read). `wide`: on a target wider than the picture, the whole target rather
    // than the picture's middle.
    std::array<float, 4> rect{};
    bool whole = true;
    bool wide = false;
    std::uint32_t rgba = 0x80808080u;  // 0xAABBGGRR, 0x80 = 1.0
    BlendMode blend = BlendMode::Opaque;
    std::uint8_t clear = 0;                               // kClear*
    std::uint16_t width = 0, height = 0;                  // TargetBegin
    float screen_width = 512.0f, screen_height = 448.0f;  // the game's 2D screen for rect
    // Fill: a depth of its own (openrac_frame_fill_depth). `depth_bits` 0 is the plain fill, which
    // neither tests nor writes depth; else `depth_value`, out of a `depth_bits`-bit buffer, is
    // compared by `depth` and stored when `depth_write`. `writes`: the colour channels written.
    DepthTest depth{};
    bool depth_write = false;
    std::uint8_t depth_bits = 0;
    std::uint32_t depth_value = 0;
    std::uint8_t writes = kWriteColour;  // kWrite* bits

    bool operator==(const FrameCommand&) const = default;
};

// A flat rectangle with a depth of its own (openrac_frame_fill_depth), in native values: `rect`
// x0, y0, x1, y1 of the game's 2D screen (null: the whole target), `wide` as FrameCommand's;
// `depth` out of a `depth_bits`-bit buffer (16, 24 or 32, bigger nearer), compared by `compare`
// and stored when `write_depth`; `writes` the colour channels (kWrite* bits). Empty for another
// bit count, a depth the buffer cannot hold, another comparison than Always, GreaterEqual or
// Greater, or channels outside kWriteColour: the caller draws its own way.
std::optional<FrameCommand> depth_fill_command(
    const float* rect,
    std::uint32_t rgba,
    BlendMode blend,
    bool wide,
    std::uint32_t depth,
    int depth_bits,
    DepthCompare compare,
    bool write_depth,
    std::uint8_t writes
);
// The number of local-player pictures the split-screen compositor retains.
inline constexpr std::uint32_t kFrameViews = 4;

// The camera a split-screen view was drawn with (the game's, read at its first World): the eye,
// its axes (forward, left, up, unit) and the tangents of half its fields of view.
struct FrameViewCamera {
    Vec3 position{};
    Vec3 forward{1, 0, 0};
    Vec3 left{0, 1, 0};
    Vec3 up{0, 0, 1};
    float tan_half_fov_x = 0.0f;
    float tan_half_fov_y = 0.0f;

    bool operator==(const FrameViewCamera&) const = default;
};

// A split-screen view of a recording (openrac_frame_view_begin / _end).
struct FrameView {
    std::uint32_t view = 0;
    // x0, y0, x1, y1 in the game's 2D screen pixels (screen_width x screen_height).
    std::array<float, 4> rect{};
    float screen_width = 512.0f, screen_height = 448.0f;
    FrameViewCamera camera;
    bool has_camera = false;
    // The layers drawn in this view (OPENRAC_DRAW_*, one bit a Bucket): its World calls' and those
    // the game said it drew while the view was open (openrac_game_draw), separate from other
    // players' layer masks.
    std::uint32_t layers = 0;
    // Layout setup without a World call must not replace a full-screen frame with cached views.
    bool has_world = false;

    // Returns whether the view id, screen and rectangle can be placed in a render target.
    bool valid() const;

    // Returns the rectangle as fractions of the screen; requires valid().
    std::array<float, 4> relative_rect() const;

    // Returns whether both views can belong to one retained layout: equal ids keep their rectangle,
    // and different ids occupy disjoint rectangles, independent of the screen's pixel dimensions.
    bool compatible_with(const FrameView& other) const;

    bool operator==(const FrameView&) const = default;
};

// One recording: the commands of one pass of the game's frame code, in call order, and their
// records.
struct FrameRecording {
    std::uint64_t serial = 0;  // 0: never closed
    // A producer-announced build identity, independent of coincident tag addresses; 0: legacy mode.
    std::uint32_t list_root = 0;
    std::uint64_t list_generation = 0;
    std::vector<FrameCommand> commands;
    std::vector<UiQuad> ui_quads;
    std::vector<UiItem> ui_items;
    std::vector<UiImage> images;
    // The split-screen views drawn in it, in the order they ended; none for a full-screen frame.
    std::vector<FrameView> views;

    bool empty() const { return commands.empty(); }

    bool anchored() const;
    void clear();
};

// Where a command of a recording goes: before byte `offset` of the window's 2D stream.
struct FrameSplice {
    std::size_t offset = 0;
    std::uint32_t command = 0;

    bool operator==(const FrameSplice&) const = default;
};

class FrameRecorder {
public:
    // The recording being made, and the anchor its next commands take.
    FrameRecording& open() { return m_open; }

    void set_anchor(std::uint32_t anchor) { m_anchor = anchor; }

    std::uint32_t anchor() const { return m_anchor; }

    // The game's 2D screen the next commands' coordinates are in (openrac_frame_screen); kept
    // across recordings.
    void set_screen(float width, float height);

    std::array<float, 2> screen() const { return m_screen; }

    // Split screen: view `view` starts (ending an open one); the open view's camera (the first
    // given is kept: the one its world was drawn with); layers the game draws in the open view (its
    // sky, ties, shrubs and mobys, which need no World call of their own); the open view ends.
    void view_begin(std::uint32_t view, const std::array<float, 4>& rect);
    void view_camera(const FrameViewCamera& camera);
    void view_layers(std::uint32_t layers);
    void view_end();

    bool view_open() const { return m_view_open; }

    // Appends `command` with the current anchor; returns its index.
    std::uint32_t push(FrameCommand command);

    // Starts a fresh build at `root`, retiring commands from its previous use even if this build
    // records nothing. Other buffers remain eligible for late/resent submission. The game calls
    // this only at its actual list reset/swap, before recording any commands of the new build.
    void begin_list(std::uint32_t root);

    // Keeps the open recording when nonempty and forgets the anchor. In producer-owned mode this
    // snapshots the current build without losing earlier commands when capture splits the build;
    // otherwise it starts another legacy recording. Empty owned builds still retire old ownership.
    void close();

    // The kept recording a display list draws with: the most recent one (non-empty) any of whose
    // anchors is one of `tags` (the list's DMA tag addresses, in walk order). Null for none.
    const FrameRecording* match(std::span<const std::uint32_t> tags) const;

    // Matches the build that owns `root`, then requires anchor membership as before. Once the
    // producer announces list builds, numerical tag coincidence alone cannot choose a recording.
    // Legacy producers that announce none retain the tag-only matching behavior.
    const FrameRecording* match(std::uint32_t root, std::span<const std::uint32_t> tags) const;

    // The most recent kept recording with no anchors (a frame with no display list at all), or null.
    const FrameRecording* latest_unanchored() const;

    // The recording closed last (latest_serial()), anchored or not and whichever a list matches, or
    // null. Its views are the split-screen views the game drew last: a game that sends each
    // player's view in a display list of its own closes one view a capture, while the list that
    // capture walks is the one sent before, built with an older recording.
    const FrameRecording* latest() const;

    // The serial of the recording closed last (0 before any).
    std::uint64_t latest_serial() const { return m_serial; }

    void reset();

    static constexpr std::size_t kKept = 4;
    static constexpr std::size_t kMostCommands = 1u << 20;

private:
    FrameRecording m_open;
    std::array<FrameRecording, kKept> m_kept{};
    std::uint64_t m_serial = 0;
    std::uint32_t m_anchor = 0;
    // Build generations are monotonic for this recorder; reset clears ownership but not this clock.
    std::uint64_t m_list_generation = 0;
    // True after a producer opts into list ownership; legacy matching cannot reappear on empty builds.
    bool m_owns_lists = false;
    // Only a push changes a build's captured payload; empty ticks must not evict a valid resend.
    bool m_open_dirty = false;
    std::array<float, 2> m_screen{512.0f, 448.0f};
    bool m_view_open = false;
};

// For each command of `recording`, the index of the tag of `tags` it goes before, or kDropped. A
// command goes before the first tag, from the one the command before it went to, whose address is
// its anchor. One whose anchor is not found there is dropped: the list never reached the place it
// was recorded at (a list the game sent again, cut short), as a marker written there would not have
// been in it. One with no anchor goes where the command before it went, including kDropped (the
// list's start for the first). Non-decreasing, kDropped aside.
inline constexpr std::size_t kDropped = static_cast<std::size_t>(-1);
std::vector<std::size_t> place_commands(
    const FrameRecording& recording, std::span<const std::uint32_t> tags
);

}  // namespace openrac::renderer
