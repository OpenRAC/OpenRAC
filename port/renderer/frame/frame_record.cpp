// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/frame/frame_record.h"

#include <algorithm>
#include <cmath>

namespace openrac::renderer {

bool FrameView::valid() const {
    return view < kFrameViews && std::isfinite(screen_width) && std::isfinite(screen_height)
           && screen_width >= 1.0f && screen_height >= 1.0f
           && std::all_of(
               rect.begin(), rect.end(), [](float edge) { return std::isfinite(edge); }
           )
           && rect[2] > rect[0] && rect[3] > rect[1];
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

void FrameRecording::clear() {
    serial = 0;
    commands.clear();
    ui_quads.clear();
    ui_items.clear();
    images.clear();
    views.clear();
}

std::uint32_t FrameRecorder::push(FrameCommand command) {
    // A game that records and is never captured (no window frame for a long while) keeps the last
    // ones only.
    if (m_open.commands.size() >= kMostCommands) {
        close();
    }
    command.anchor = m_anchor;
    command.screen_width = m_screen[0];
    command.screen_height = m_screen[1];

    // Each player's world layers stay with that view, not the frame-wide union.
    if (m_view_open && command.op == FrameOp::World && !m_open.views.empty()) {
        m_open.views.back().layers |= command.index;
        m_open.views.back().has_world = true;
    }

    m_open.commands.push_back(command);
    return static_cast<std::uint32_t>(m_open.commands.size() - 1);
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
    push(c);
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

void FrameRecorder::close() {
    m_anchor = 0;
    // A view still open when the frame closes ends with it.
    if (m_view_open) {
        view_end();
    }
    if (m_open.empty()) {
        m_open.clear();
        return;
    }
    // The oldest kept goes.
    auto oldest = std::min_element(
        m_kept.begin(), m_kept.end(), [](const FrameRecording& a, const FrameRecording& b) {
            return a.serial < b.serial;
        }
    );
    *oldest = std::move(m_open);
    oldest->serial = ++m_serial;
    m_open = {};
}

const FrameRecording* FrameRecorder::match(std::span<const std::uint32_t> tags) const {
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
    const FrameRecording* best = nullptr;
    for (const FrameRecording& r : m_kept) {
        if (r.serial != 0 && !r.empty() && !r.anchored()
            && (best == nullptr || r.serial > best->serial)) {
            best = &r;
        }
    }
    return best;
}

void FrameRecorder::reset() {
    m_open.clear();
    for (FrameRecording& r : m_kept) {
        r.clear();
    }
    m_anchor = 0;
    m_screen = {512.0f, 448.0f};
    m_view_open = false;
}

std::vector<std::size_t> place_commands(
    const FrameRecording& recording, std::span<const std::uint32_t> tags
) {
    std::vector<std::size_t> out;
    out.reserve(recording.commands.size());
    std::size_t at = 0;
    for (const FrameCommand& c : recording.commands) {
        if (c.anchor == 0) {
            out.push_back(at);
            continue;
        }
        std::size_t found = kDropped;
        for (std::size_t t = at; t < tags.size(); ++t) {
            if (tags[t] == c.anchor) {
                found = t;
                break;
            }
        }
        if (found != kDropped) {
            at = found;
        }
        out.push_back(found);
    }
    return out;
}

}  // namespace openrac::renderer
