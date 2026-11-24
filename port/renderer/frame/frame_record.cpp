// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/frame/frame_record.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace openrac::renderer {
namespace {

// Temporary submission state; committed only after every submitted command satisfies view ordering.
struct ViewSubmission {
    std::vector<FrameView> views;  // Value-owned cameras and payload indices for this tick.
    std::vector<bool> prepared;  // Commands executed on their view framebuffer before composition.
    std::vector<bool> colour;    // Native 2D preceded a post effect in this view.
    std::vector<bool> effects;   // A terminal post effect has begun in this view.
    std::uint32_t target = 0;    // Submitted explicit panel scope, independent of view ownership.
};

// Identifies the command whose ordering cannot be represented by an isolated world post pass.
Unexpected view_failure(std::uint32_t at, std::string_view reason) {
    return unexpected(
        Error{"frame recorder: " + std::string(reason) + " at command " + std::to_string(at)}
    );
}

// Schedules one terminal effect only when its submitted world and payload are available in isolation.
Result<void> schedule_view_effect(
    const FrameRecording& recording, std::uint32_t at, ViewSubmission& plan
) {
    const FrameCommand& command = recording.commands[at];
    const std::size_t owner = command.view;

    // A reachable effect does not prove that an earlier truncated world was submitted.
    if (!plan.views[owner].has_world) {
        return view_failure(at, "view effect has no submitted world");
    }

    // Native 2D still executes once after composition and cannot feed an earlier view post pass.
    if (plan.colour[owner]) {
        return view_failure(at, "view effect follows native 2D drawing");
    }

    // A missing payload must not become a no-op or an unrelated effect.
    if (command.index >= recording.screen_effects.size()) {
        return view_failure(at, "view effect payload is absent");
    }

    plan.views[owner].screen_effects.push_back(command.index);
    plan.prepared[at] = true;
    plan.effects[owner] = true;

    return {};
}

// Checks a submitted native command without changing the immutable recording or any GPU state.
Result<void> schedule_view_command(
    const FrameRecording& recording, std::uint32_t at, ViewSubmission& plan
) {
    const FrameCommand& command = recording.commands[at];

    // Explicit panel effects remain at their recorded position in the main native command stream.
    if (command.op == FrameOp::TargetBegin || command.op == FrameOp::TargetEnd) {
        plan.target = command.op == FrameOp::TargetBegin ? command.index : 0;
    }

    // World-in-panel rendering has no split compositor implementation; it cannot refresh the screen view.
    if (command.op == FrameOp::World && command.view != kNoFrameView && plan.target != 0) {
        return unexpected(Error{"frame recorder: split world inside an explicit panel target"});
    }

    // Unscoped commands and panel commands do not read or modify a local player's world picture.
    if (command.view == kNoFrameView || plan.target != 0) {
        return {};
    }

    // A missing owner is a broken recording, not permission to apply an effect to the whole frame.
    if (command.view >= plan.views.size()) {
        return view_failure(at, "invalid view owner");
    }

    const std::size_t owner = command.view;

    // World drawing after a post effect cannot be moved ahead of it without changing the picture.
    if (plan.effects[owner]
        && (command.op == FrameOp::World || command.op == FrameOp::WorldMobys
            || command.op == FrameOp::WorldShrubs || command.op == FrameOp::EffectQuad)) {
        return view_failure(at, "world drawing after view post effect");
    }

    // Only terminal world post passes can move into the owner's framebuffer without replaying its UI.
    if (command.op == FrameOp::ScreenEffect) {
        return schedule_view_effect(recording, at, plan);
    }

    // These operations remain in the main native 2D stream rather than the isolated world pass.
    if (command.op == FrameOp::UiItem || command.op == FrameOp::UiQuad
        || command.op == FrameOp::Fill || command.op == FrameOp::Clear
        || command.op == FrameOp::Picture || command.op == FrameOp::Mobys
        || command.op == FrameOp::Snapshot || command.op == FrameOp::LastFrame) {
        plan.colour[owner] = true;
    }

    return {};
}

// Validates splice order and collects only the world layers that the submitted chain reaches.
Result<void> collect_view_worlds(
    const FrameRecording& recording, std::span<const FrameSplice> submitted, ViewSubmission& plan
) {
    std::uint32_t previous = 0;
    bool first = true;

    // Splices are in call order, without duplicates, after unreachable anchors have been removed.
    for (const FrameSplice& splice : submitted) {
        // Invalid or repeated indices could read outside the recording or execute an effect twice.
        if (splice.command >= recording.commands.size() || (!first && splice.command <= previous)) {
            return unexpected(Error{"frame recorder: invalid submitted command order"});
        }

        previous = splice.command;
        first = false;
        const FrameCommand& command = recording.commands[splice.command];

        // The command's captured owner survives even when its transitional ViewBegin tag is absent.
        if (command.op == FrameOp::World && command.view != kNoFrameView) {
            // A submitted world needs a valid retained rectangle rather than an invented viewport.
            if (command.view >= plan.views.size() || !plan.views[command.view].valid()) {
                return unexpected(Error{"frame recorder: invalid submitted world view"});
            }

            plan.views[command.view].has_world = true;
            plan.views[command.view].layers |= command.index;
        }
    }

    return {};
}

}  // namespace

Result<void> FrameRecording::resolve_views(std::span<const FrameSplice> submitted) {
    // Single-view/unscoped recording semantics do not change.
    if (views.empty()) {
        return {};
    }

    ViewSubmission plan{
        views,
        std::vector<bool>(commands.size()),
        std::vector<bool>(views.size()),
        std::vector<bool>(views.size())
    };

    // Recorded intent is not proof that the submitted chain reached any of this player's world.
    for (FrameView& view : plan.views) {
        view.layers = 0;
        view.has_world = false;
        view.screen_effects.clear();
    }

    const Result<void> worlds = collect_view_worlds(*this, submitted, plan);

    // Malformed submission indices cannot be used to schedule payloads.
    if (!worlds) {
        return unexpected(worlds.error());
    }

    // Scheduling is atomic: an unsupported ordering never publishes partially consumed effects.
    for (const FrameSplice& splice : submitted) {
        const Result<void> result = schedule_view_command(*this, splice.command, plan);

        // A failure must not consume even the earlier valid effects in this submission.
        if (!result) {
            return unexpected(result.error());
        }
    }

    views = std::move(plan.views);

    // The command remains diagnostic evidence, but the compositor now owns its one execution.
    for (std::size_t n = 0; n < commands.size(); ++n) {
        commands[n].view_effect_prepared = plan.prepared[n];
    }

    view_error.clear();

    return {};
}

const char* frame_op_name(FrameOp op) {
    // Names follow the native command vocabulary, not the residual packet encoding.
    switch (op) {
        case FrameOp::World:
            return "World";
        case FrameOp::WorldMobys:
            return "WorldMobys";
        case FrameOp::Mobys:
            return "Mobys";
        case FrameOp::UiQuad:
            return "UiQuad";
        case FrameOp::UiItem:
            return "UiItem";
        case FrameOp::EffectQuad:
            return "EffectQuad";
        case FrameOp::Fill:
            return "Fill";
        case FrameOp::Clear:
            return "Clear";
        case FrameOp::Picture:
            return "Picture";
        case FrameOp::Snapshot:
            return "Snapshot";
        case FrameOp::TargetBegin:
            return "TargetBegin";
        case FrameOp::TargetEnd:
            return "TargetEnd";
        case FrameOp::Scissor:
            return "Scissor";
        case FrameOp::WorldShrubs:
            return "WorldShrubs";
        case FrameOp::EffectParent:
            return "EffectParent";
        case FrameOp::HudSide:
            return "HudSide";
        case FrameOp::Wide:
            return "Wide";
        case FrameOp::PictureBand:
            return "PictureBand";
        case FrameOp::LastFrame:
            return "LastFrame";
        case FrameOp::ViewBegin:
            return "ViewBegin";
        case FrameOp::ViewEnd:
            return "ViewEnd";
    }

    return "Unknown";
}

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
) {
    // The games' depth buffers are 16, 24 or 32 bits; a depth past the buffer's largest value
    // would be clamped where the producer asked for an exact one.
    constexpr int kDepth16 = 16, kDepth24 = 24, kDepth32 = 32;
    const bool known_bits =
        depth_bits == kDepth16 || depth_bits == kDepth24 || depth_bits == kDepth32;

    // Never is no rectangle at all, and a channel outside the colour's is no channel.
    const bool known_compare = compare == DepthCompare::Always
                               || compare == DepthCompare::GreaterEqual
                               || compare == DepthCompare::Greater;

    // Any of these leaves the rectangle to the caller's own drawing.
    if (!known_bits || static_cast<double>(depth) > depth_max(depth_bits) || !known_compare
        || (writes & ~kWriteColour) != 0) {
        return std::nullopt;
    }

    FrameCommand c;
    c.op = FrameOp::Fill;
    c.whole = rect == nullptr;

    // Only an explicit rectangle has coordinates; null is the whole target.
    if (rect != nullptr) {
        std::copy(rect, rect + c.rect.size(), c.rect.begin());
    }

    c.wide = wide;
    c.rgba = rgba;
    c.blend = blend;
    // A depth written without a comparison is still a depth pass: the back end tests to write.
    c.depth = {compare != DepthCompare::Always || write_depth, compare};
    c.depth_write = write_depth;
    c.depth_bits = static_cast<std::uint8_t>(depth_bits);
    c.depth_value = depth;
    c.writes = writes;
    return c;
}

bool FrameView::valid() const {
    return view < kFrameViews && std::isfinite(screen_width) && std::isfinite(screen_height)
           && screen_width >= 1.0f && screen_height >= 1.0f
           && std::all_of(
               rect.begin(), rect.end(), [](float edge) { return std::isfinite(edge); }
           )
           && rect[2] > rect[0] && rect[3] > rect[1];
}

Result<void> FrameRecording::validate_view_effect_context(FrameViewContext context) const {
    const bool effects = std::any_of(views.begin(), views.end(), [](const FrameView& view) {
        return !view.screen_effects.empty();
    });

    // Ordinary split-screen frames and unscoped/panel effects keep their existing renderer policy.
    if (!effects) {
        return {};
    }

    // A camera failure cannot silently discard an already scheduled effect.
    if (!context.has_camera) {
        return unexpected(Error{"frame recorder: view post effect requires a usable world camera"});
    }

    // Residual packets lack native view ownership; their colour order must not be guessed.
    if (context.residual_2d) {
        return unexpected(
            Error{"frame recorder: view post effect with residual 2D needs scoped ordering"}
        );
    }

    // Moving composite finishing before each view effect needs separate integration and pixel proof.
    if (context.composite_colour) {
        return unexpected(
            Error{"frame recorder: view post effect with composite colour finishing is unsupported"}
        );
    }

    return {};
}

std::array<float, 4> FrameView::relative_rect() const {
    return {
        rect[0] / screen_width,
        rect[1] / screen_height,
        rect[2] / screen_width,
        rect[3] / screen_height
    };
}

bool FrameView::compatible_with(const FrameView& other) const {
    // Malformed geometry cannot describe a retained picture's place.
    if (!valid() || !other.valid()) {
        return false;
    }

    const std::array<float, 4> first = relative_rect();
    const std::array<float, 4> second = other.relative_rect();

    // A changed rectangle replaces the layout, not only that player's old picture.
    if (view == other.view) {
        return first == second;
    }

    return first[2] <= second[0] || second[2] <= first[0] || first[3] <= second[1]
           || second[3] <= first[1];
}

bool FrameRecording::anchored() const {
    return std::any_of(commands.begin(), commands.end(), [](const FrameCommand& c) {
        return c.anchor != 0;
    });
}

bool FrameRecording::has_split_world() const {
    return std::any_of(commands.begin(), commands.end(), [this](const FrameCommand& command) {
        return command.op == FrameOp::World && command.view < views.size();
    });
}

void FrameRecording::clear() {
    serial = 0;
    list_root = 0;
    list_generation = 0;
    commands.clear();
    ui_quads.clear();
    ui_items.clear();
    images.clear();
    screen_effects.clear();
    views.clear();
    view_error.clear();
}

std::uint32_t FrameRecorder::push(FrameCommand command) {
    // A game that records and is never captured (no window frame for a long while) keeps the last
    // ones only.
    if (m_open.commands.size() >= kMostCommands) {
        const std::uint32_t root = m_open.list_root;
        const std::uint64_t generation = m_open.list_generation;
        const bool was_open = m_view_open;
        FrameView active;

        // A bounded recording rollover must not turn a view-owned effect into a global effect.
        if (was_open) {
            active = m_open.views.back();
            active.layers = 0;
            active.has_world = false;
            active.screen_effects.clear();
        }

        const std::uint32_t anchor = m_anchor;
        m_view_open = false;
        close();
        m_anchor = anchor;
        // The existing command bound starts a new payload; ownership still belongs to this build.
        m_open.clear();
        m_open.list_root = root;
        m_open.list_generation = generation;

        // Owner metadata crosses the boundary by value; payload storage still follows push().
        if (was_open) {
            m_open.views.push_back(std::move(active));
            m_view_open = true;
        }
    }
    command.anchor = m_anchor;
    command.screen_width = m_screen[0];
    command.screen_height = m_screen[1];
    command.view = m_view_open && !m_open.views.empty()
                       ? static_cast<std::uint32_t>(m_open.views.size() - 1)
                       : kNoFrameView;

    // View identity is captured on every command, not inferred from reachable begin/end markers.
    if (m_view_open) {
        command.view = static_cast<std::uint32_t>(m_open.views.size() - 1);
    }

    // Each player's world layers stay with that view, not the frame-wide union.
    if (m_view_open && command.op == FrameOp::World && !m_open.views.empty()) {
        m_open.views.back().layers |= command.index;
        m_open.views.back().has_world = true;
    }

    m_open.commands.push_back(command);
    m_open_dirty = true;
    return static_cast<std::uint32_t>(m_open.commands.size() - 1);
}

void FrameRecorder::begin_list(std::uint32_t root) {
    close();

    // Reusing a buffer invalidates its old command ownership, not just its old world anchors.
    for (FrameRecording& recording : m_kept) {
        if (recording.list_root == root) {
            recording.clear();
        }
    }

    m_owns_lists = true;
    m_open.clear();
    m_open.list_root = root;
    m_open.list_generation = ++m_list_generation;
    m_open_dirty = false;
}

void FrameRecorder::set_screen(float width, float height) {
    // A size no game draws in is none: the coordinates stay in the last one given.
    if (width >= 1.0f && height >= 1.0f && width <= 8192.0f && height <= 8192.0f) {
        m_screen = {width, height};
    }
}

void FrameRecorder::view_begin(std::uint32_t view, const std::array<float, 4>& rect) {
    FrameView v;
    v.view = view;
    v.rect = rect;
    v.screen_width = m_screen[0];
    v.screen_height = m_screen[1];

    // Invalid coordinates cannot reach the compositor's floating-point to pixel conversion.
    if (!v.valid()) {
        return;
    }

    // A new valid view closes the preceding player's world.
    if (m_view_open) {
        view_end();
    }

    FrameCommand c;
    c.op = FrameOp::ViewBegin;
    c.index = view;
    c.rect = rect;
    c.whole = false;
    const std::uint32_t at = push(c);
    m_open.commands[at].view = static_cast<std::uint32_t>(m_open.views.size());
    m_open.views.push_back(v);
    m_view_open = true;
}

void FrameRecorder::view_camera(const FrameViewCamera& camera) {
    if (m_view_open && !m_open.views.empty() && !m_open.views.back().has_camera) {
        m_open.views.back().camera = camera;
        m_open.views.back().has_camera = true;
    }
}

void FrameRecorder::view_layers(std::uint32_t layers) {
    // What the game draws while a view is open is that player's world, whichever pass names it.
    if (m_view_open && !m_open.views.empty()) {
        m_open.views.back().layers |= layers;
    }
}

void FrameRecorder::view_end() {
    if (!m_view_open) {
        return;
    }
    m_view_open = false;
    FrameCommand c;
    c.op = FrameOp::ViewEnd;
    c.index = m_open.views.empty() ? 0 : m_open.views.back().view;
    push(c);
}

void FrameRecorder::push_screen_effect(const ScreenEffect& effect) {
    FrameCommand command;
    command.op = FrameOp::ScreenEffect;
    const std::uint32_t at = push(command);

    // push() can close the full recording, so payload storage is selected after it returns.
    m_open.commands[at].index = static_cast<std::uint32_t>(m_open.screen_effects.size());
    m_open.screen_effects.push_back(effect);
}

void FrameRecorder::close() {
    m_anchor = 0;
    // A view still open when the frame closes ends with it.
    if (m_view_open && m_open.commands.size() < kMostCommands) {
        view_end();
    }
    m_view_open = false;
    // A residual-only owned build is meaningful: its predecessor was retired by begin_list.
    if (m_open.empty()) {
        // Legacy capture still releases payloads that have no command; owned builds keep their identity.
        if (m_open.list_generation == 0) {
            m_open.clear();
        }

        m_open_dirty = false;
        return;
    }

    // Repeated captures without new commands keep the same build and must not consume history slots.
    if (m_open.list_generation != 0 && !m_open_dirty) {
        return;
    }

    // The oldest kept goes.
    auto oldest = std::min_element(
        m_kept.begin(), m_kept.end(), [](const FrameRecording& a, const FrameRecording& b) {
            return a.serial < b.serial;
        }
    );

    // A second capture of one build replaces its snapshot, retaining its earlier commands/payloads.
    if (m_open.list_generation != 0) {
        auto same =
            std::find_if(m_kept.begin(), m_kept.end(), [&](const FrameRecording& recording) {
                return recording.list_generation == m_open.list_generation;
            });

        if (same != m_kept.end()) {
            oldest = same;
        }

        *oldest = m_open;
        oldest->serial = ++m_serial;
        m_open_dirty = false;
        return;
    }

    *oldest = std::move(m_open);
    oldest->serial = ++m_serial;
    m_open = {};
    m_open_dirty = false;
}

const FrameRecording* FrameRecorder::match(std::span<const std::uint32_t> tags) const {
    return match(0, tags);
}

const FrameRecording* FrameRecorder::match(
    std::uint32_t root, std::span<const std::uint32_t> tags
) const {
    // An owned recording must be requested with the submitted root, never an inferred anchor alone.
    if (m_owns_lists && root == 0) {
        return nullptr;
    }

    if (tags.empty()) {
        return nullptr;
    }
    std::vector<std::uint32_t> sorted(tags.begin(), tags.end());
    std::sort(sorted.begin(), sorted.end());
    const FrameRecording* best = nullptr;
    for (const FrameRecording& r : m_kept) {
        if (r.serial == 0 || r.empty() || (best != nullptr && r.serial < best->serial)) {
            continue;
        }

        // An empty new build cannot revive either an older owner or a legacy unowned recording.
        if (m_owns_lists && (r.list_generation == 0 || r.list_root != root)) {
            continue;
        }

        // Explicit unanchored commands may belong to a reached owned list, but not to another root.
        if (m_owns_lists && !r.anchored()) {
            best = &r;
            continue;
        }

        const bool found =
            std::any_of(r.commands.begin(), r.commands.end(), [&](const FrameCommand& c) {
                return c.anchor != 0 && std::binary_search(sorted.begin(), sorted.end(), c.anchor);
            });
        if (found) {
            best = &r;
        }
    }
    return best;
}

const FrameRecording* FrameRecorder::latest_unanchored() const {
    // Native commands of owned builds must not bypass list ownership through the standalone fallback.
    if (m_owns_lists) {
        return nullptr;
    }

    const FrameRecording* best = nullptr;
    for (const FrameRecording& r : m_kept) {
        if (r.serial != 0 && !r.empty() && !r.anchored()
            && (best == nullptr || r.serial > best->serial)) {
            best = &r;
        }
    }
    return best;
}

const FrameRecording* FrameRecorder::latest() const {
    const auto found = std::find_if(m_kept.begin(), m_kept.end(), [this](const FrameRecording& r) {
        return r.serial != 0 && r.serial == m_serial;
    });
    return found != m_kept.end() ? &*found : nullptr;
}

void FrameRecorder::reset() {
    m_open.clear();
    for (FrameRecording& r : m_kept) {
        r.clear();
    }
    m_anchor = 0;
    m_owns_lists = false;
    m_open_dirty = false;
    m_screen = {512.0f, 448.0f};
    m_view_open = false;
}

std::vector<std::size_t> place_commands(
    const FrameRecording& recording, std::span<const std::uint32_t> tags
) {
    std::vector<std::size_t> out;
    out.reserve(recording.commands.size());
    std::size_t at = 0;
    std::size_t previous = 0;

    // Search forward in walk order; paging can place lower addresses after higher ones.
    for (const FrameCommand& c : recording.commands) {
        // An unanchored command inherits even an unreachable predecessor's placement.
        if (c.anchor == 0) {
            out.push_back(previous);
            continue;
        }

        std::size_t found = kDropped;

        // A tag reached only before the previous successful placement cannot draw this command.
        for (std::size_t t = at; t < tags.size(); ++t) {
            if (tags[t] == c.anchor) {
                found = t;
                break;
            }
        }

        // A missing anchor must not prevent a later reachable anchor from resuming the recording.
        if (found != kDropped) {
            at = found;
        }

        previous = found;
        out.push_back(found);
    }
    return out;
}

}  // namespace openrac::renderer
